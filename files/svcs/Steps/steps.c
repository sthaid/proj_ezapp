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

// program args
char *progname;
char *data_dir;

// flag set when SVC_REQ_ID_STOP received
bool end_program = false;

// prototypes
void process_req(svc_req_t *req);
int initialize(void);
void cleanup(void);
void periodic_processing(void);

// -----------------  STEPS SERVICE  -----------------------------------------

int main(int argc, char **argv)
{
    svc_req_t *req;
    long abstime;
    int rc;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

#if 0
    // unit test code
    int cnt=0;
    initialize();
    while (true) {
        periodic_processing();
        sleep(1);
        if (++cnt >= 5) break;
    }
    cleanup();
    return 0;
#endif

    // initialize
    rc = initialize();
    if (rc != 0) {
        printf("E %s: failed to initialize\n", progname);
        return 1;
    }

    // set absolute time at which svc_wait_for_req will timeout;
    // this time is rounded down to the prior minute so that the first
    // call to svc_wait_for_req will timeout immedeately
    // xxx why abstime?
    abstime = time(NULL) / 60 * 60;

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, abstime);

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            sleep(1);
            continue;
        }

        // if scv_wait_for_req timedout then do periodic svc processing
        // xxx maybe use relative time
        if (rc == SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            // xxx adjust interval
            // - when app is running and not dozing or minimized then use 1 second
            // - else use 1 minute
            abstime += 1; // xxx why so iregular;  set to wake up xxx from now
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

// -----------------  INITIALIZE  ----------------------------------

// defines
#define CREATE_IF_NEEDED true
#define READ_WRITE       false

// variables
steps_file_t  *steps_file;
unsigned long  last_step_count_sensor;

// prototypes

int initialize(void)
{
    int rc;
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

    // init the last_step_count_sensor variable
    rc = sdlx_sensor_read_step_counter(&last_step_count_sensor);
    if (rc != 0) {
        printf("E %s: failed to read step counter sensor\n", progname);
        return -1;
    }

    // success
    return 0;
}

void cleanup(void)
{
    if (steps_file) {
        util_unmap_file(steps_file, sizeof(steps_file_t));
        steps_file = NULL;
    }
}

// -----------------  PERIODIC PROCESSING  -------------------------

void periodic_processing(void)
{
    unsigned long step_count_sensor;
    int           steps, rc, year, month, day, hour;
    time_t        t;
    struct tm     tm;

    static int count;

    // read step counter sensor
    rc = sdlx_sensor_read_step_counter(&step_count_sensor);
    if (rc != 0) {
        printf("E %s: failed to read step counter sensor\n", progname);
        return;
    }

    // determine number of steps since last read of the sensor
    steps = (step_count_sensor - last_step_count_sensor);
    last_step_count_sensor = step_count_sensor;
    //printf("I %s: step_count_sensor = %ld  steps = %d\n", progname, step_count_sensor, steps);

    // if no new steps then return
    if (steps == 0) {
        return;
    }

    // accumulate steps
    t = time(NULL);
    localtime_r(&t, &tm);
    year  = tm.tm_year + 1900 - YEAR0;
    month = tm.tm_mon;       // 0 - 11
    day   = tm.tm_mday - 1;  // 0 - 30
    hour  = tm.tm_hour;      // 0 - 23 
    steps_file->year[year]                   += steps;
    steps_file->month[year][month]           += steps;
    steps_file->day[year][month][day]        += steps;
    steps_file->hour[year][month][day][hour] += steps;

    // xxx sync file periodically
    // xxx use time based, hourly
    if (++count == 10) {
        count = 0;
        printf("I %s: calling util_sync_file\n", progname);
        util_sync_file(steps_file, sizeof(steps_file_t));
    }
}
