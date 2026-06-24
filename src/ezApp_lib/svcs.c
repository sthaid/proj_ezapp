#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>
#include <private.h>

// xxx
// - number of services
// - scroll services menu

// xxx
// - test a delay in stopping, does it stay in delpending
//   . can it get out of delpend

//
// defines
//

#define SERVICE_STATE_STOPPED           0     // white
#define SERVICE_STATE_RUNNING           1     // green
#define SERVICE_STATE_STOPPING          2     // yellow
#define SERVICE_STATE_STOPPED_BY_ERROR  3     // red

#define SERVICE_STATE_STR(state) \
    ((state) == SERVICE_STATE_STOPPED           ? "STOPPED"          : \
     (state) == SERVICE_STATE_RUNNING           ? "RUNNING"          : \
     (state) == SERVICE_STATE_STOPPING          ? "STOPPING"         : \
     (state) == SERVICE_STATE_STOPPED_BY_ERROR  ? "STOPPED_BY_ERROR" : \
                                                  "????")

#define SERVICE_STATE_TO_COLOR(state) \
    ((state) == SERVICE_STATE_STOPPED           ? COLOR_WHITE  : \
     (state) == SERVICE_STATE_RUNNING           ? COLOR_GREEN  : \
     (state) == SERVICE_STATE_STOPPING          ? COLOR_YELLOW : \
                                                  COLOR_RED)

#define SERVICE_IS_STOPPED(state)  ((state) == SERVICE_STATE_STOPPED || \
                                    (state) == SERVICE_STATE_STOPPED_BY_ERROR)

#define MAX_SVCS 20
#define MAX_SVC_NAME 30
#define MAX_SVC_REQ_QUEUE 5

#define MS  1000L
#define SEC 1000000L

//
// typedefs
//

typedef struct {
    char            name[MAX_SVC_NAME];
    int             state;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    svc_req_t      *req[MAX_SVC_REQ_QUEUE];
} svc_t;

//
// variables
//

static svc_t  svcs[MAX_SVCS];
static int    max_svcs;

// set by eztest when testing a svc
int svc_eztest_mode;

//
// prototypes
//

static void update_list_of_svcs(void);
static void process_svc_start_req(int id);
static void process_svc_stop_req(int id);
static void run_svc(char *svc_name);
static int svc_thread(void *cx);

static int svc_name_to_id(char *svc_name);
static void remove_trailing_newline(char *s);

// -----------------  SVCS ROUTINES USED BY MAIN.C  ---------------

void svcs_start(void)
{
    char dir[100];
    bool stopped;
    int  id;
    char tmp_name[MAX_SVC_NAME];

    INFO("start services\n");

    update_list_of_svcs();

    // start all svcs that are not tagged with 'stopped' file 
    // and are in SERVICE_STATE_STOPPED
    // xxx check for name is deleted
    for (id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];

        strcpy(tmp_name, x->name); //xxx comment
        snprintf(dir, sizeof(dir), "svcs/%s", tmp_name);
        stopped = util_file_exists(dir, "stopped");

        if (!stopped && SERVICE_IS_STOPPED(x->state)) {
            memset(x->req, 0, sizeof(x->req));
            x->state = SERVICE_STATE_RUNNING;
            run_svc(x->name);
        }
    }
}

void svcs_stop(void)
{
    int id, duration_ms = 0;
    bool all_stopped;

    INFO("stop services\n");

    for (id = 0; id < max_svcs; id++) {
        // xxx all loops like this, skip 'deleted'
        svc_t *x = &svcs[id];
        if (x->state == SERVICE_STATE_RUNNING) {
            x->state = SERVICE_STATE_STOPPING;
            svc_make_req(svcs[id].name, SVC_REQ_ID_STOP, NULL, 0, 5);
        }
    }

    while (true) {
        all_stopped = true;
        for (id = 0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];
            if (!SERVICE_IS_STOPPED(x->state)) {
                all_stopped = false;
                break;
            }
        }

        if (all_stopped) {
            INFO("all services are stopped\n");
            break;
        }

        if (duration_ms > 30000) {
            ERROR("the following services have failed to stop ...\n");
            for (id = 0; id < max_svcs; id++) {
                svc_t *x = &svcs[id];
                if (!SERVICE_IS_STOPPED(x->state)) {
                    ERROR("- %-12s %s\n", x->name, SERVICE_STATE_STR(x->state));
                }
            }
            break;
        }

        usleep(100*MS);
        duration_ms += 100;
    }
}

void svcs_display(int bg_color)
{
    sdlx_event_t event;
    int         id;
    bool        done = false;
    sdlx_loc_t  *loc;
    double      row;
    char        dir[100];

    #define EVID_SVC_START    100
    #define EVID_SVC_STOP     200

    sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

    // handle the setting display
    while (true) {
        // xxx comment
        update_list_of_svcs();

        // init display and display title line
        sdlx_display_init(bg_color, PORTRAIT);
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), 
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                               "Services");

        // display name and controls for each service
        row = 3;
        for (id = 0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];

            sdlx_render_printf_ex1(0, ROW2Y(row), FONT_NORMAL, SERVICE_STATE_TO_COLOR(x->state), "%-s", x->name);

            if (SERVICE_IS_STOPPED(x->state)) {
                loc = sdlx_render_printf_ex1(COL2X(10), ROW2Y(row), FONT_NORMAL, COLOR_LIGHT_BLUE, "start");
                sdlx_register_event(loc, EVID_SVC_START+id);
            } else if (x->state == SERVICE_STATE_RUNNING) {
                loc = sdlx_render_printf_ex1(COL2X(10), ROW2Y(row), FONT_NORMAL, COLOR_LIGHT_BLUE, "stop");
                sdlx_register_event(loc, EVID_SVC_STOP+id);
            }

            row += 2;
        }

        // display the control event 'X' to exit this
        sdlx_register_control_events(0, NULL, 
                                     0, NULL, 
                                     EVID_QUIT, "X");

        // present the display
        sdlx_display_present();

        // wait for an event, with 100 ms timeout;
        // if no event received then re-display
        sdlx_get_event(100*MS, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        switch (event.event_id) {
        case EVID_SVC_START ... EVID_SVC_START + MAX_SVCS - 1:
            id = event.event_id - EVID_SVC_START;
            sprintf(dir, "svcs/%s", svcs[id].name);
            util_delete_file(dir, "stopped");
            process_svc_start_req(id);
            break;
        case EVID_SVC_STOP ... EVID_SVC_STOP + MAX_SVCS - 1:
            id = event.event_id - EVID_SVC_STOP;
            sprintf(dir, "svcs/%s", svcs[id].name);
            util_write_file(dir, "stopped", NULL, 0);
            process_svc_stop_req(id);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }
}

// xxx comments needed
static void update_list_of_svcs(void)
{
    FILE *fp;
    char s[100];
    char new[MAX_SVCS][MAX_SVC_NAME]; // xxx
    int  max_new = 0;
    long new_mtime;

    static long mtime;

    new_mtime = util_file_mtime("svcs", NULL);
    if (new_mtime == mtime) {
        return;
    }
    mtime = new_mtime;

    fp = popen("cd svcs; find * -maxdepth 0 -type d", "r");
    while (fgets(s, sizeof(s), fp)) {
        remove_trailing_newline(s);
        strcpy(new[max_new++], s);
        if (max_new == MAX_SVCS) {
            break;
        }
    }
    pclose(fp);

    // for all existing svc names that are not in the new svc names, delete the svc
    for (int id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];
        bool found_in_new_names = false;

        for (int i = 0; i < max_new; i++) {
            if (strcmp(x->name, new[i]) == 0) {
                found_in_new_names = true;
                break;
            }
        }

        if (!found_in_new_names) {
            INFO("deleting svc %s\n", x->name);

            // if svc is running then send it stop request
            if (x->state == SERVICE_STATE_RUNNING) {
                x->state = SERVICE_STATE_STOPPING;
                svc_make_req(svcs[id].name, SVC_REQ_ID_STOP, NULL, 0, 5);
            }

            // wait for service to be in stopped state
            strcpy(x->name, "delpend");
            int ms = 0;
            while (ms < 5000 && !SERVICE_IS_STOPPED(x->state)) {
                usleep(100000);
                ms += 100;
            }

            // xxx comment
            if (SERVICE_IS_STOPPED(x->state)) {
                strcpy(x->name, "deleted");
                x->state = SERVICE_STATE_STOPPED;
                pthread_mutex_destroy(&x->mutex);
                pthread_cond_destroy(&x->cond);
                memset(x->req, 0, sizeof(x->req));
            }
        }
    }

    // for all new svc names that don't exist, add entry to svcs tbl
    for (int i = 0; i < max_new; i++) {
        int id = svc_name_to_id(new[i]);
        if (id == -1) {
            // service name new[i] is not in svcs tbl; so add it
            int j = svc_name_to_id("deleted");
            svc_t *x;

            if (j >= 0) {
                x = &svcs[j];
            } else if (max_svcs < MAX_SVCS) {
                x = &svcs[max_svcs];
                max_svcs++;
            } else {
                ERROR("svcs tbl is full\n");
                break;
            }
            strcpy(x->name, new[i]);
            x->state = SERVICE_STATE_STOPPED;
            pthread_mutex_init(&x->mutex, NULL);
            pthread_cond_init(&x->cond, NULL);
            memset(x->req, 0, sizeof(x->req));
        }
    }

    INFO("Services List ...\n");
    for (int i = 0; i < max_svcs; i++) {
        INFO("%20s\n", svcs[i].name);
    }
}

static void process_svc_start_req(int id)
{
    svc_t *x = &svcs[id];

    INFO("called for id=%d name=%s\n", id, x->name);

    if (!SERVICE_IS_STOPPED(x->state)) {
        ERROR("id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }
    
    memset(x->req, 0, sizeof(x->req));
    x->state = SERVICE_STATE_RUNNING;
    run_svc(x->name);
}

static void process_svc_stop_req(int id)
{
    svc_t *x = &svcs[id];

    INFO("called for id=%d name=%s\n", id, x->name);

    if (x->state != SERVICE_STATE_RUNNING) {
        ERROR("not running: id=%d name=%s state=%s\n", id, x->name, SERVICE_STATE_STR(x->state));
        return;
    }

    x->state = SERVICE_STATE_STOPPING;
    svc_make_req(x->name, SVC_REQ_ID_STOP, NULL, 0, 5);
}

static void run_svc(char *svc_name_arg)
{
    char thread_name[100];
    char *svc_name = strdup(svc_name_arg);

    sprintf(thread_name, "svc_%s", svc_name);
    sdlx_create_detached_thread(svc_thread, thread_name, svc_name);
}

static int svc_thread(void *cx)
{
    #define IS_SVC true

    char *svc_name = cx;
    int   rc, id;

    rc = run(svc_name, IS_SVC);

    id = svc_name_to_id(svc_name);
    if (id != -1) {
        svcs[id].state = (rc == 0 ? SERVICE_STATE_STOPPED : SERVICE_STATE_STOPPED_BY_ERROR);
    }

    free(svc_name);
    return 0;
}

// -----------------  SVCS ROUTINES AVAIL IN PICOC  ---------------

// ---- routines called by apps ----

int svc_make_req(char *svc_name, int req_id, char *req_data, int req_data_len, int timeout_secs)
{
    int        svc_id, i, req_status;
    svc_t     *x;
    svc_req_t *req;
    long       start_us;

    INFO("svc_name = %s req_id = %d\n", svc_name, req_id);

    // check that req_data_len is valid
    if (req_data_len > MAX_SVC_REQ_DATA) {
        ERROR("req_data_len %d is too large, max allowed = %d\n",
              req_data_len, MAX_SVC_REQ_DATA);
        return SVC_REQ_ERROR_DATA_LEN;
    }

    // get svc id for the svc_name
    svc_id = svc_name_to_id(svc_name);
    if (svc_id == -1) {
        ERROR("service %s not found\n", svc_name);
        return SVC_REQ_ERROR_SVC_NOT_FOUND;
    }

    // check that the svc is active
    x = &svcs[svc_id];
    if (req_id == SVC_REQ_ID_STOP && x->state == SERVICE_STATE_STOPPING) {
        // okay
    } else if (x->state != SERVICE_STATE_RUNNING) {
        ERROR("service %s not running\n", svc_name);
        return SVC_REQ_ERROR_SVC_NOT_RUNNING;
    }

    // acquire mutex
    pthread_mutex_lock(&x->mutex);

    // find avail entry in the svc req queue;
    // if no entry found then return error
    for (i = 0; i < MAX_SVC_REQ_QUEUE; i++) {
        if (x->req[i] == NULL) {
            break;
        }
    }
    if (i == MAX_SVC_REQ_QUEUE) {
        ERROR("service %s req queue is full\n", svc_name);
        pthread_mutex_unlock(&x->mutex);
        return SVC_REQ_ERROR_QUEUE_FULL;
    }
        
    // allocate zeroed req, and init the req
    req = calloc(1, sizeof(svc_req_t));
    req->req_id = req_id;
    req->status = SVC_REQ_ERROR_NOT_COMPLETED;
    if (req_data) {
        memcpy(req->data, req_data, req_data_len);
    }

    // queue the req,
    req->status = SVC_REQ_ERROR_NOT_COMPLETED;
    x->req[i] = req;

    // wake the svc to process the req:
    // - signal the condition
    // - release mutex
    pthread_cond_signal(&x->cond);
    pthread_mutex_unlock(&x->mutex);

    // poll for req to have completed (either ok or with an error)
    start_us = util_microsec_timer();
    while (true) {
        if (req->completed) {
            break;
        }
        if (util_microsec_timer() - start_us > (timeout_secs * SEC)) {
            break;
        }
        usleep(100*MS);
    }
    INFO("duration = %ld secs\n", (util_microsec_timer() - start_us) / SEC);

    // prepare to return req_status and req_data
    req_status = req->status;
    __sync_synchronize();
    if (req_data) {
        memcpy(req_data, req->data, req_data_len);
    }

    // free req
    free(req);

    // return req_status
    return req_status;
}

// ---- routines called by svcs ----

int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs)
{
    struct timespec ts = {timeout_abstime_secs, 0 };
    int             ret;
    int             id;

    //INFO("svc_name=%s timeout_abstime_secs=%ld time_until_timeout=%ld\n", 
    //     svc_name, timeout_abstime_secs, timeout_abstime_secs-time(NULL));
 
    // this flag is set when eztest is run on a svc;
    // improves eztest ability to test a svc
    if (svc_eztest_mode) {
        sleep(timeout_abstime_secs - time(NULL));
        return SVC_REQ_WAIT_ERROR_TIMEDOUT;
    }

try_again:

    // get svc id for the svc_name
    id = svc_name_to_id(svc_name);
    if (id == -1) {
        ERROR("svc_name %s not found\n", svc_name);
        *req = NULL;
        return SVC_REQ_WAIT_ERROR_SVC_NOT_FOUND;
    }
    svc_t *x = &svcs[id];

    // acquire mutex
    pthread_mutex_lock(&x->mutex);

    // wait, with timeout, for a request to be available 
    while (x->req[0] == NULL) {
        ret = pthread_cond_timedwait(&x->cond, &x->mutex, &ts);
        if (ret == ETIMEDOUT) {
            *req = NULL;
            pthread_mutex_unlock(&x->mutex);
            return SVC_REQ_WAIT_ERROR_TIMEDOUT;
        } else if (ret != 0) {
            // sleep and try again, 
            // perhaps the error is a glitch that will clear itself
            ERROR("pthread_cond_timedwait ret=%d, retry in 10 secs\n", ret);
            pthread_mutex_unlock(&x->mutex);
            sleep(1);
            goto try_again;
        }
    }

    // return req;
    // remove req from queue
    *req = x->req[0];
    memmove(&x->req[0], &x->req[1], (MAX_SVC_REQ_QUEUE-1)*sizeof(void*));
    x->req[MAX_SVC_REQ_QUEUE-1] = NULL;

    // release mutex
    pthread_mutex_unlock(&x->mutex);

    // success, req is being returned
    return SVC_REQ_WAIT_OK;
}

void svc_req_completed(svc_req_t *req, int status)
{
    req->status = status;
    __sync_synchronize();
    req->completed = true;
}

// -----------------  UTILS  ----------------------------------------

// return -1 if svc_name not foumd, else return the id for svc_name
static int svc_name_to_id(char *svc_name)
{
    int id;

    for (id = 0; id < max_svcs; id++) {
        if (strcmp(svcs[id].name, svc_name) == 0) {
            return id;
        }
    }

    return -1;
}

static void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }
}

