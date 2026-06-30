#ifndef __SVCS_H__
#define __SVCS_H__

#ifdef __cplusplus
extern "C" {
#endif

// common values for req_id
#define SVC_REQ_ID_STOP 1

// values returned by svc_make_req
#define SVC_REQ_OK                     0
#define SVC_REQ_ERROR_NOT_COMPLETED    1
#define SVC_REQ_ERROR_DATA_LEN         2
#define SVC_REQ_ERROR_SVC_NOT_FOUND    3
#define SVC_REQ_ERROR_SVC_NOT_RUNNING  4
#define SVC_REQ_ERROR_QUEUE_FULL       5
#define SVC_REQ_ERROR_INVALID_REQ      6
#define SVC_REQ_ERROR                  7

// values returned by svc_wait_for_req
#define SVC_REQ_WAIT_OK                    0
#define SVC_REQ_WAIT_ERROR_SVC_NOT_FOUND   1
#define SVC_REQ_WAIT_ERROR_TIMEDOUT        2

// size of req->data
#define MAX_SVC_REQ_DATA 200

// xxx instead preallocate?
// xxx maybe not in this file?  and not in picoc
typedef struct {
    int  req_id;
    bool completed;
    int  status;
    char data[MAX_SVC_REQ_DATA];
} svc_req_t;

// Routine called by miniApps, to make a request of a service:
// - svc_name:     name of the miniSvc
// - req_id:       identifies the request, the req_id values should be defined in a
//                 miniSvc header file
// - req_data:     in/out buffer, containing request and response details
// - req_data_len: length of the req_data buffer, not to exceed MAX_SVC_REQ_DATA
// - timeout_secs: svc_make_req will return status SVC_REQ_ERROR_NOT_COMPLETED
//                 if timeout occurred
int svc_make_req(char *svc_name, int req_id, char *req_data, int req_data_len, int timeout_secs);

// miniSvc processing summary:
// - call svc_wait_for_req: to wait, with timeout, for a request
// - if timeout occurred then the miniSvc may perform periodic processing
// - if a request was recevied, the miniSvc must process the request, and 
//   acknowledge completion by calling svc_req_complete
//
// MiniSvcs must support receiving the SVC_REQ_ID_STOP request.
// When received the service must cleanup and terminate.
// Prior to terminating the miniSvc must acknowledge by calling 
// svc_req_complete(req, SVC_REQ_OK),
int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs);
void svc_req_completed(svc_req_t *req, int comp_status);

#ifdef __cplusplus
}
#endif

#endif
