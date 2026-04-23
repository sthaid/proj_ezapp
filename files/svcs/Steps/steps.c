#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <sdlx.h>
#include <svcs.h>

// program args
char *progname;
char *data_dir;

// flag set when SVC_REQ_ID_STOP received
bool end_program = false;

// prototypes
void init();
void process_req(svc_req_t *req);
void periodic_svc_processing(void);

// -----------------  TEMPLATE SERVICE  --------------------------------------

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

    init();

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
            periodic_svc_processing(); //xxx temp
            //xxx printf("E %s: svc_wait_for_req returned unexpected error %d\n", progname, rc);
            sleep(1);
            continue;
        }

        // if scv_wait_for_req timedout then do periodic svc processing
        if (rc == SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            abstime += 60; // xxx why so iregular;  set to wake up 1 minute from now
            periodic_svc_processing();

            exit(1); //xxx

            continue;
        }

        // if req was recvd then process the req
        if (req != NULL) {
            printf("I %s: req=%p req->req_id=%d\n", progname, req, req->req_id);
            process_req(req);
        }
    }
            
    // print terminating msg
    printf("I %s: terminating\n", progname);
    return 0;
}

// -----------------------------------------------------------------

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

// -----------------------------------------------------------------

time_t TIME_0;

#define MAX_YEAR  20
#define MAX_MONTH (MAX_YEAR * 12)
#define MAX_DAY   (MAX_YEAR * 365)
#define MAX_HOUR  (MAX_YEAR * 8760)

#define YEAR_T0  2026

typedef struct {
    int year[MAX_YEAR];
    int month[MAX_MONTH];
    int day[MAX_DAY];
    int hour[MAX_HOUR];
} step_data_t;

step_data_t step_data;
unsigned long last_step_count_sensor;

void periodic_svc_processing(void)
{
    unsigned long step_count_sensor;
    int steps;

    printf("I %s: sizeof(step_data_t) = %zd\n", progname, sizeof(step_data_t));

    sdlx_sensor_read_step_counter(&step_count_sensor);
    steps = (step_count_sensor - last_step_count_sensor);
    last_step_count_sensor = step_count_sensor;

    printf("I %s: step_count = %ld  steps = %d\n", progname, step_count_sensor, steps);

    int hour_idx, day_idx, month_idx, year_idx;

    time_t t_now = time(NULL);
    struct tm tm_now;

    localtime_r(&t_now, &tm_now);
    printf("xxxx month %d  year %d\n", tm_now.tm_mon, tm_now.tm_year);

    hour_idx  = (t_now - TIME_0) / 3600;
    day_idx   = (t_now - TIME_0) / 86400;
    year_idx  = (tm_now.tm_year + 1900) - YEAR_T0;
    month_idx = tm_now.tm_mon + 12 * (year_idx);

    printf("I %s: hour_idx=%d day_idx=%d month_idx=%d year_idx=%d\n",
           progname, hour_idx, day_idx, month_idx, year_idx);


    step_data.year[year_idx]  += steps;
    step_data.month[month_idx] += steps;
    step_data.day[day_idx]    += steps;
    step_data.hour[hour_idx]  += steps;

    // xxx msync periodically
    // xxx does util_map create msync?
}

void init(void)
{
    struct tm tm;
    time_t tnow;

    sdlx_sensor_read_step_counter(&last_step_count_sensor);

    // Specify the local time
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = 2026 - 1900; // Year is 2026
    tm.tm_mon  = 0;           // Month is January (0-indexed)
    tm.tm_mday = 1;           // Day is 1st
    tm.tm_hour = 0;           // Hour
    tm.tm_min  = 0;           // Minute
    tm.tm_sec  = 0;           // Second
    tm.tm_isdst = -1;         // Let system determine DST

    TIME_0 = mktime(&tm);
    tnow = time(NULL);
    printf("i %s: tnow   = %s", progname, ctime(&tnow));
    printf("i %s: TIME_0 = %s", progname, ctime(&TIME_0));
}

