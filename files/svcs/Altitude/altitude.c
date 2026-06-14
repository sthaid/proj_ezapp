#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Altitude/altitude.h"

// defines
#define CREATE_IF_NEEDED true
#define READ_WRITE       false

// program args
char *progname;
char *data_dir;

// variables
bool             end_program = false;
altitude_file_t *altitude_file;

// prototypes
void process_req(svc_req_t *req);
void periodic_processing(void);

// -----------------  MAIN  ----------------------------------------

int initialize(void);
void cleanup(void);

int main(int argc, char **argv)
{
    svc_req_t *req;
    int        rc;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // initialize
    rc = initialize();
    if (rc != 0) {
        printf("E %s: failed to initialize\n", progname);
        cleanup();
        return 1;
    }

    // service runtime loop
    while (!end_program) {
        // wait for req or 10 sec timeout
        rc = svc_wait_for_req(progname, &req, time(NULL)+10);

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            printf("E %s: svc_wait_for_req rc=%d\n", progname, rc);
            sleep(1);
            continue;
        }

        // do periodic svc processing
        periodic_processing();

        // if req was recvd then process the req
        if (req != NULL) {
            printf("I %s: req=%p req->req_id=%d\n", progname, req, req->req_id);
            process_req(req);
        }
    }

    // cleanup
    cleanup();
            
    // print terminating msg
    printf("I %s: terminating\n", progname);
    return 0;
}

int initialize(void)
{
    int created_flag;

    printf("I %s: sizeof(altitude_file_t) = 0x%zx %0.3f MB\n",
           progname, sizeof(altitude_file_t), (double)sizeof(altitude_file_t)/0x100000);

    // map the altitude data file;
    // create the file if needed because either it doesn't exist or has incorrect size
    altitude_file = util_map_file(data_dir, ALTITUDE_FILENAME, sizeof(altitude_file_t),
                                  CREATE_IF_NEEDED, READ_WRITE, &created_flag);
    if (altitude_file == NULL) {
        printf("E %s: failed to map %s\n", progname, ALTITUDE_FILENAME);
        return -1;
    }

    // if file was created or has incorrect version then init the file
    if (created_flag || altitude_file->version != ALTITUDE_FILE_VERSION) {
        printf("I %s: initializing altitude_file\n", progname);
        altitude_file->version = ALTITUDE_FILE_VERSION;
        int *tmp = (int*)&altitude_file->altitude_ft[0][0][0][0];
        for (int i = 0; i < sizeof(altitude_file->altitude_ft)/sizeof(int); i++) {
            tmp[i] = NO_ALTITUDE_DATA;
        }
        util_sync_file(altitude_file, sizeof(altitude_file_t));
    }

    // success
    return 0;
}

void cleanup(void)
{
    if (altitude_file) {
        util_unmap_file(altitude_file, sizeof(altitude_file_t));
        altitude_file = NULL;
    }
}

// -----------------  PROCESS REQ  ---------------------------------

void process_req(svc_req_t *req)
{
    printf("I %s: got req_id %d\n", progname, req->req_id);

    // process the request
    switch (req->req_id) {
    case SVC_REQ_ID_STOP:
        svc_req_completed(req, SVC_REQ_OK);
        end_program = true;
        break;
    default:
        printf("E %s: req %d is invalid\n", progname, req->req_id);
        svc_req_completed(req, SVC_REQ_ERROR_INVALID_REQ);
        break;
    }
}

// -----------------  PERIODIC PROCESSING  -------------------------

void periodic_processing(void)
{
    double        altitude_ft;
    bool          alt_is_wgs84;
    int           year, month, day, hour;
    time_t        t_now;
    struct tm     tm;

    static bool   wgs84_warning_printed;

    // get current time
    t_now = time(NULL);

#if 0
    // print interval since last call
    static time_t t_last_call;
    if (t_last_call != 0) {
        printf("I %s: periodic interval = %ld secs\n", progname, t_now-t_last_call);
    }
    t_last_call = t_now;
#endif

    // get the current altitude;
    util_get_location(NULL, NULL, &altitude_ft, &alt_is_wgs84);

    // if no data then return
    if (altitude_ft == INVALID_NUMBER) {
        printf("E %s: failed to get altitude_ft\n", progname);
        return;
    }

    // if altitude is wgs84 then print warning once
    if (alt_is_wgs84 && !wgs84_warning_printed) {
        printf("E %s: WARNING altitude is WGS84\n", progname);
        wgs84_warning_printed = true;
    }

    // accumulate altitude to memory mapped altitude_file
    localtime_r(&t_now, &tm);
    year  = tm.tm_year + 1900 - YEAR0;
    month = tm.tm_mon;       // 0 - 11
    day   = tm.tm_mday - 1;  // 0 - 30
    hour  = tm.tm_hour;      // 0 - 23 
    if (altitude_ft > altitude_file->altitude_ft[year][month][day][hour]) {
        altitude_file->altitude_ft[year][month][day][hour] = altitude_ft;
        util_sync_file(&altitude_file->altitude_ft[year][month][day][hour], sizeof(int));
    }

    // debug print
    printf("I %s: ymdh=%d %d %d %d  altitude_ft=%0.0f\n", 
           progname, year, month, day, hour, altitude_ft);
}
