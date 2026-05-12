#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#include <sdlx.h>
#include <svcs.h>

// program args
char *progname;
char *data_dir;

// flag set when SVC_REQ_ID_STOP received
bool end_program = false;

// prototypes
void process_req(svc_req_t *req);
void periodic_processing(void);

// -----------------  TEMPLATE SERVICE  --------------------------------------

int main(int argc, char **argv)
{
    svc_req_t *req;
    int rc;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, time(NULL)+5);

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            printf("E %s: svc_wait_for_req returned unexpected error %d\n", progname, rc);
            sleep(1);
            continue;
        }

        // if scv_wait_for_req timedout then do periodic svc processing
        if (rc == SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            periodic_processing();
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
#if 1
    // print interval since last call
    static time_t t_last_call;
    if (t_last_call != 0) {
        printf("I %s: periodic interval = %ld secs\n", progname, t_now-t_last_call);
    }
    t_last_call = t_now;
#endif
}
