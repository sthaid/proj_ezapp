#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

#include "svcs/Steps/steps.h"

// defines
#define CREATE_IF_NEEDED true
#define READ_WRITE       false

// program args
char          *progname;
char          *data_dir;

// variables
bool          end_program = false;
steps_file_t *steps_file;
FILE         *fp;  // xxx temp

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
    time_t     t_now, t_last=time(NULL), timeout=0;

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
        // wait for req or timeout
        timeout = (timeout == 0 ? time(NULL) : time(NULL)+5);
        rc = svc_wait_for_req(progname, &req, time(NULL)+5);

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            printf("E %s: svc_wait_for_req rc=%d\n", progname, rc);
            sleep(1);
            continue;
        }

        // if scv_wait_for_req timedout then do periodic svc processing
        if (rc == SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            // debug print actual duration of the timeout
            t_now = time(NULL);
            printf("I %s: delta_t=%ld calling period_processing\n", progname, t_now-t_last);
            if (fp) {
                fprintf(fp, "I %s: delta_t=%ld calling period_processing\n", progname, t_now-t_last);
                fflush(fp);
            }
            t_last = t_now;

            // perform periodic processing
            periodic_processing();
            continue;
        }

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
    int tries = 0;
    int created_flag;

    printf("I %s: sizeof(steps_file_t) = 0x%zx\n", progname, sizeof(steps_file_t));

try_again:
    // map the steps data file (steps.dat)
    steps_file = util_map_file(data_dir, STEPS_FILENAME, sizeof(steps_file_t),
                               CREATE_IF_NEEDED, READ_WRITE, &created_flag);
    if (steps_file == NULL) {
        printf("E %s: failed to map %s\n", progname, STEPS_FILENAME);
        return -1;
    }

    // if the steps.dat file was created then 
    //   init the file version field
    // else if file magic is invalid then 
    //   delete the file and call util_map_file again
    // endif
    if (created_flag) {
        steps_file->version = VERSION;
        util_sync_file(&steps_file->version, sizeof(steps_file->version));
    } else if (steps_file->version != VERSION) {
        printf("I %s: steps_file version=0x%lx expected=0x%lx, recreating steps_file\n",
               progname, steps_file->version, VERSION);
        util_delete_file(data_dir, STEPS_FILENAME);
        if (++tries == 1) {
            goto try_again;
        } else {
            return -1;
        }
    }

    // xxx temp
    fp = fopen("svcs/Steps/debug.log", "a");

    // success
    return 0;
}

void cleanup(void)
{
    if (steps_file) {
        util_unmap_file(steps_file, sizeof(steps_file_t));
        steps_file = NULL;
    }

    if (fp) {
        fclose(fp);
        fp = NULL;
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
    unsigned long step_count_sensor;
    int           steps, rc, year, month, day, hour;
    time_t        t_now;
    struct tm     tm;

    static int           t_last_sync;
    static bool          sync_needed;
    static unsigned long last_step_count_sensor;

    // get current time
    t_now = time(NULL);

    // sync the steps_file, if needed
    if (sync_needed && (t_now - t_last_sync > 60)) {
        printf("I %s: calling util_sync_file\n", progname);
        util_sync_file(steps_file, sizeof(steps_file_t));
        t_last_sync = t_now;
        sync_needed = false;
    }

    // read step counter sensor
    rc = sdlx_sensor_read_step_counter(&step_count_sensor);
    if (rc != 0) {
        printf("E %s: failed to read step counter sensor\n", progname);
        return;
    }

    // if don't yet have the last_step_count_sensor value then set it and return
    if (last_step_count_sensor == 0) {
        last_step_count_sensor = step_count_sensor;
        printf("I %s: init last_step_count_sensor %ld\n", progname, last_step_count_sensor);
        return;
    }

    // determine number of steps since previous read of the sensor
    steps = (step_count_sensor - last_step_count_sensor);
    last_step_count_sensor = step_count_sensor;

    // if no new steps then return
    if (steps == 0) {
        return;
    }

    // accumulate steps to memory mapped steps_file
    localtime_r(&t_now, &tm);
    year  = tm.tm_year + 1900 - YEAR0;
    month = tm.tm_mon;       // 0 - 11
    day   = tm.tm_mday - 1;  // 0 - 30
    hour  = tm.tm_hour;      // 0 - 23 
    steps_file->year[year]                   += steps;
    steps_file->month[year][month]           += steps;
    steps_file->day[year][month][day]        += steps;
    steps_file->hour[year][month][day][hour] += steps;

    // set sync_needed flag
    sync_needed = true;

    // debug print, and also print to logfile
    printf("I %s: ymdh=%d %d %d %d  steps=%d  sensor=%ld\n", 
           progname, year, month, day, hour, steps, step_count_sensor);
    if (fp) {
        fprintf(fp, "I %s: ymdh=%d %d %d %d  steps=%d  sensor=%ld\n", 
                progname, year, month, day, hour, steps, step_count_sensor);
        fflush(fp);
    }
}
