#include <std_hdrs.h>

#include <svcs.h>
#include <logging.h>

//
// routines called by apps
//

int svc_make_req(char *svc_name, int req_id, char *req_data, int req_data_len, int timeout_secs)
{
    ERROR("STUB svc_make_req: svc_name=%s req_id=%d\n", svc_name, req_id);
    sleep(1);
    return SVC_REQ_ERROR_NOT_COMPLETED;
}

//
// routines called by svcs
//

int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs)
{
    ERROR("STUB svc_wait_for_req: svc_name=%s\n", svc_name);
    sleep(1);
    *req = NULL;
    return SVC_REQ_WAIT_ERROR_TIMEDOUT;
}

void svc_req_completed(svc_req_t *req, int status)
{
    ERROR("STUB svc_req_completed: req_id=%d\n", req->req_id);
}

