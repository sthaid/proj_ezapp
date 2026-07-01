#ifndef __SVCS_H__
#define __SVCS_H__

#ifdef __cplusplus
extern "C" {
#endif

// common values for req_id
#define SVC_REQ_ID_STOP 1

// sizeof of req->data
#define MAX_SVC_REQ_DATA 100

// svc request struct
typedef struct {
    int  req_id;
    int  comp_status;
    char data[MAX_SVC_REQ_DATA];
} svc_req_t;

int svc_make_req(char *svc_name, svc_req_t *req);

int svc_wait_for_req(char *svc_name, svc_req_t *req, int timeout_secs);
void svc_req_completed(char *svc_name, svc_req_t *req, int comp_status);


// xxx update this ...
// Routine called by miniApps, to make a request of a service:
// - svc_name:     name of the miniSvc
// - req_id:       identifies the request, the req_id values should be defined in a
//                 miniSvc header file
// - req_data:     in/out buffer, containing request and response details
// - req_data_len: length of the req_data buffer, not to exceed MAX_SVC_REQ_DATA
// - timeout_secs: svc_make_req will return status SVC_REQ_ERROR_NOT_COMPLETED
//                 if timeout occurred

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

#ifdef __cplusplus
}
#endif

#endif
