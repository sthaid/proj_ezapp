#ifndef __SVCS_H__
#define __SVCS_H__

// xxx comments needed

void svcs_init(void);
void svcs_stop_all(void);
void svcs_acquire_mutex(void);
void svcs_release_mutex(void);
void svcs_start_all_autostart_services(void);
void svcs_display(int bg_color);

// ----------------------------

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

// sizeof of req->data
#define MAX_SVC_REQ_DATA 200

typedef struct {
    int  req_id;
    bool completed;
    int  status;
    char data[MAX_SVC_REQ_DATA];
} svc_req_t;

// routines called by apps
int svc_make_req(char *svc_name, int req_id, char *req_data, int req_data_len, int timeout_secs);

// routines called by svcs
int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs);
void svc_req_completed(svc_req_t *req, int comp_status);

#endif
