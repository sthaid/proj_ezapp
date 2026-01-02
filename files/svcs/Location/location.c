#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Location/location.h"
#include "svcs/Location/common.h"

// defines
#define INTERVAL 60  // xxx was 3600

// variables
loc_hist_t *loc_hist;
bool        param_enabled = false;
bool        end_program = false;
bool        test_loc_hist = false;

// prototypes
void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name);
char *most_recent_loc_hist_name(void);
void create_loc_data_str(time_t t, double latitude, double longitude, double elevation,
                         char *name, double miles, bool extra, char *data_str);
void add_simulated_entries_to_loc_hist(void);
double rand_double(void);
void clear_loc_history(void);
void process_req(svc_req_t *req);

// -----------------  MAIN  -----------------------------------------

int main(int argc, char **argv)
{
    char          name[MAX_NAME];
    double        latitude, longitude, miles;
    long          abstime;
    svc_req_t    *req;
    int           rc;
    int           created;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // read location data
    rc = read_loc_data();
    if (rc != 0) {
        printf("ERROR: %s failed to read location data\n", progname);
        return 1;
    }

    // map the loc_hist file
    // - create_if_needed = true
    // - read_only = false
    // - created (return flag) = NULL
    loc_hist = util_map_file(data_dir, LOC_HIST_FILENAME, sizeof(loc_hist_t), true, false, &created);
    if (loc_hist == NULL) {
        printf("ERROR: %s failed to map %s\n", progname, LOC_HIST_FILENAME);
        return 1;
    }

    // when test_mode is enabled and the loc_hist file was just created,
    // add simulated entries to the loc_hist file
    if (test_loc_hist && created) {
        add_simulated_entries_to_loc_hist();
    }

    // read parameters
    param_enabled = util_get_numeric_param(data_dir, "enabled", 1);
    printf("INFO: hisotry collection is %s\n", param_enabled ? "enabled" : "disabled");

    // set absolute time at which svc_wait_for_req will timeout;
    // this time is rounded down to the prior hour so that the first
    // call to svc_wait_for_req will timeout immedeately
    abstime = time(NULL) / INTERVAL * INTERVAL;

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, abstime);

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            printf("ERROR %s: svc_wait_for_req returned unexpected error %d\n", progname, rc);
            sleep(1);
            continue;
        }

        // if svc_wait_for_req timedout
        // - find location in database that is closest to current lat/long;
        // - if name is different than most recent entry in loc_file
        //    then add new entry to loc file, 
        // - increment abstime
        // endif
        if (rc == SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            if (param_enabled) {
                // find location in database that is closest to current lat/long;
                util_get_location(&latitude, &longitude, NULL);
                find_closest_loc_data(latitude, longitude, name, &miles);

                // if name is different than most recent entry in loc_file
                // then add new entry to loc file, 
                // xxx temp, always add
                //if (strcmp(name, "Not Found") != 0 &&
                //    strcmp(most_recent_loc_hist_name(), name) != 0)
                {
                    add_entry_to_loc_hist(time(NULL), latitude, longitude, name);
                }
            }

            // update abstime to next hour
            abstime += INTERVAL;
            continue;
        }

        // if req was recvd then process the req
        if (req != NULL) {
            process_req(req);
        }
    }

    // cleanup and end program
    free_loc_data();
    util_unmap_file(loc_hist, sizeof(loc_hist_t));
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  LOC_HIST SUPPORT  -----------------------------

void add_entry_to_loc_hist(time_t t, double latitude, double longitude, char *name)
{
    double miles = 0;       // not included in loc hist file
    double elevation = 0;   // not included in loc hist file

    // if buffer is full then discard the first half (oldest data)
    if (loc_hist->count == MAX_LOC_HIST) {
        memmove(&loc_hist->loc[0], 
                &loc_hist->loc[MAX_LOC_HIST-MAX_LOC_HIST/2], 
                (MAX_LOC_HIST/2)*sizeof(loc_hist->loc[0]));

        memset(&loc_hist->loc[MAX_LOC_HIST/2],
               0,
               (MAX_LOC_HIST-MAX_LOC_HIST/2)*sizeof(loc_hist->loc[0]));

        loc_hist->count = MAX_LOC_HIST/2;
    }

    // add entry
    create_loc_data_str(t, latitude, longitude, elevation, name, miles,
                        false, loc_hist->loc[loc_hist->count].data_str);
    loc_hist->count++;

    // sync memory mapped buffer to storage
    util_sync_file(loc_hist, sizeof(loc_hist_t));
}

char *most_recent_loc_hist_name(void)
{
    static char name[MAX_NAME];
    char *ptr, *data_str;

    if (loc_hist->count == 0) {
        return "";
    }

    data_str = loc_hist->loc[loc_hist->count-1].data_str;

    ptr = strchr(data_str, '\n');
    if (ptr == NULL) {
        printf("ERROR %s: newline char not found in data_str '%s'\n", progname, data_str);
        return "";
    }

    memcpy(name, data_str, ptr-data_str);
    name[ptr-data_str] = '\0';

    printf("INFO %s: most recent name = '%s'\n", progname, name);
    return name;
}

#define METERS_TO_FEET 3.28084

void create_loc_data_str(time_t t, double latitude, double longitude, double elevation, 
                         char *name, double miles, bool extra, char *data_str)
{
    struct tm *tm;
    char time_str[50];
    char *p;

    // example:
    //   Bolton
    //   Dec 5 23:00 EST
    //   -42.1234 -130.1234
    // extra line:
    //   el 300 ft, 1.1 mi       elevation, and distance to published lat/long

    // create time string
    tm = localtime(&t);
    //xxx strftime(time_str, sizeof(time_str), "%b %d %H:%M %Z", tm);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M:%S %Z", tm);

    // sprint location info to str
    p = data_str;
    if (latitude != INVALID_NUMBER && longitude != INVALID_NUMBER) {
        p += sprintf(p, "%s\n%s\n%0.4f %0.4f\n", name, time_str, latitude, longitude);
    } else {
        p += sprintf(p, "%s\n%s\nLocation Unavailable\n", name, time_str);
    }
    if (extra) {
        p += sprintf(p, "el %0.0f ft, %0.1f mi\n", elevation * METERS_TO_FEET, miles);
    }
    p += sprintf(p, "\n");
}

void add_simulated_entries_to_loc_hist(void)
{
    double latitude, longitude, miles;
    char name[MAX_NAME];
    time_t t;

    t = time(NULL) - 30 * 86400;
    t = t / 3600 * 3600;

    for (int i = 0; i < 20; i++) {
        // get random location in Massachusett
        latitude  = 41.23 + (42.88 - 41.23) * rand_double();
        longitude = -(69.93 + (73.50 - 69.93) * rand_double());

        // find closest location from loc_data
        find_closest_loc_data(latitude, longitude, name, &miles);

        // add to loc_hist file
        add_entry_to_loc_hist(t, latitude, longitude, name);

        // advance time one hour
        t += 3600;
    }
}

double rand_double(void)
{
    double rand;

    rand = (double)random() / 0x7fffffff;
    printf("%f\n", rand);
    return rand;
}

void clear_loc_history(void)
{
    loc_hist->count++;
    memset(loc_hist, 0, sizeof(loc_hist_t));
    util_sync_file(loc_hist, sizeof(loc_hist_t));
}

// -----------------  PROCESS REQ SUPPORT  --------------------------

void process_req(svc_req_t *req)
{
    switch (req->req_id) {
    case SVC_REQ_ID_STOP:
        svc_req_completed(req, SVC_REQ_OK);
        end_program = true;
        break;
    case SVC_LOCATION_REQ_GET_LOC_INFO: {
        double latitude, longitude, elevation, miles;
        char name[MAX_NAME];

        util_get_location(&latitude, &longitude, &elevation);
        find_closest_loc_data(latitude, longitude, name, &miles);
        create_loc_data_str(time(NULL), latitude, longitude, elevation,
                            name, miles, true, req->data);
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    case SVC_LOCATION_REQ_ADD_COUNTRY_INFO: {
        char *country_code = req->data;

        if (strlen(country_code) != 2) {
            printf("ERROR %s: invalid country code '%s', len must be 2\n", progname, country_code);
            svc_req_completed(req, SVC_REQ_ERROR);
            break;
        }

        for (int i = 0; i < strlen(country_code); i++) {
            country_code[i] = tolower(country_code[i]);
        }
            
        download_country_loc_data(country_code);
        read_loc_data();
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    case SVC_LOCATION_REQ_DEL_COUNTRY_INFO: {
        char filename[220];

        sprintf(filename, "%s.loc", req->data);
        util_delete_file(data_dir, filename);
        read_loc_data();
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    case SVC_LOCATION_REQ_LIST_COUNTRY_INFO: {
        FILE *fp;
        char *p, *p2, s[100], cmd[100];

        sprintf(cmd, "cd %s; ls -1 *.loc", data_dir);
        fp = popen(cmd, "r");
        if (fp == NULL) {
            strcpy(req->data, "No Country Info");
            svc_req_completed(req, SVC_REQ_OK);
            break;
        }
        p = req->data;
        while (fgets(s, sizeof(s), fp) != NULL) {
            p2 = strstr(s, ".loc");
            if (p2) {
                *p2 = '\0';
                p += sprintf(p, "%s\n", s);
            }
        }
        pclose(fp);
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    case SVC_LOCATION_REQ_CLEAR_HISTORY: {
        clear_loc_history();
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    case SVC_LOCATION_REQ_QUERY_ENABLED: {
        req->data[0] = param_enabled;
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    case SVC_LOCATION_REQ_SET_ENABLED: {
        param_enabled = req->data[0];
        util_set_numeric_param("data_dir", "enabled", param_enabled);
        printf("INFO: hisotry collection is now %s\n",
               param_enabled ? "enabled" : "disabled");
        svc_req_completed(req, SVC_REQ_OK);
        break; }
    default:
        printf("ERROR %s: req %d is invalid\n", progname, req->req_id);
        svc_req_completed(req, SVC_REQ_ERROR_INVALID_REQ);
        break;
    }
}
