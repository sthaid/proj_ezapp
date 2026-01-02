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

// -----------------  TEMPLATE SERVICE  --------------------------------------

int main(int argc, char **argv)
{
    svc_req_t *req;
    long abstime;
    int rc;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // set absolute time at which svc_wait_for_req will timeout;
    // this time is rounded down to the prior minute so that the first
    // call to svc_wait_for_req will timeout immedeately
    abstime = time(NULL) / 60 * 60;

    // service runtime loop
    while (!end_program) {
        // wait for req or timeout
        rc = svc_wait_for_req(progname, &req, abstime); //xxx use abstime

        // if an unexpected error is returned, then delay and try again
        if (rc != 0 && rc != SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            printf("ERROR %s: svc_wait_for_req returned unexpected error %d\n", progname, rc);
            sleep(1);
            continue;
        }

        // if scv_wait_for_req timedout then do periodic svc processing
        if (rc == SVC_REQ_WAIT_ERROR_TIMEDOUT) {
            printf("INFO %s: do some processing\n", progname);
            abstime += 60;
            continue;
        }

        // if req was recvd then process the req
        if (req != NULL) {
            printf("INFO %s: req=%p req->req_id=%d\n", progname, req, req->req_id);
            process_req(req);
        }
    }
            
    // print terminating msg
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void process_req(svc_req_t *req)
{
    printf("INFO %s: got req_id %d\n", progname, req->req_id);

    // process the request
    switch (req->req_id) {
    case SVC_REQ_ID_STOP:
        svc_req_completed(req, SVC_REQ_OK);
        end_program = true;
        break;
    default:
        printf("ERROR %s: req %d is invalid\n", progname, req->req_id);
        svc_req_completed(req, SVC_REQ_ERROR_INVALID_REQ);
        break;
    }
}

