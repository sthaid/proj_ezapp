#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <logging.h>
#include <svcs.h>
#include <main.h>

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

#define MAX_SVCS 10
#define MAX_SVC_REQ_QUEUE 5

#define MS  1000L
#define SEC 1000000L

//
// typedefs
//

typedef struct {
    char            name[30];
    char            autostart;   // y or n
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

//
// prototypes
//

static void process_svc_start_req(int id);
static void process_svc_stop_req(int id);
static void process_svc_stopped_callback(int id, int rc);
static void run_svc(int id);
static int svc_name_to_id(char *svc_name);

// -----------------  SVCS ROUTINES USED BY MAIN.C  ---------------

void svcs_init(void)
{
    static bool first_call = true;
    FILE *fp;
    char str[100];
    int cnt, line=0;

    // on first call init the pthread mutex and condition
    if (first_call) {
        for (int i = 0; i < MAX_SVCS; i++) {
            pthread_mutex_init(&svcs[i].mutex, NULL);
            pthread_cond_init(&svcs[i].cond, NULL);
        }
        first_call = false;
    }

    // stop all svcs
    // xxx what if this doesnt stop them all?
    svcs_stop_all();

    // get svc names and autostart indicator from the svcs file
    max_svcs = 0;
    fp = fopen("svcs/svcs", "r");
    if (fp == NULL) {
        ERROR("failed to open svcs/svcs, %s\n", strerror(errno));
        return;
    }
    while (fgets(str, sizeof(str), fp) != NULL) {
        svc_t *x = &svcs[max_svcs];
        line++;

        // ignore lines that begin with '#', space, or newline
        if (str[0] == '\n' || str[0] == ' ' || str[0] == '#') {
            continue;
        }

        // line format: <name> <y/n>
        cnt = sscanf(str, "%s %c", x->name, &x->autostart);
        if (cnt != 2) {
            ERROR("invalid line %d in svcs file\n", line);
            fclose(fp);
            max_svcs = 0;
            return;
        }

        // increment max_svcs
        max_svcs++;

        // if svcs table is full print warning and break
        if (max_svcs == MAX_SVCS) {
            WARN("svcs tbl is full\n");
            break;
        }
    }
    fclose(fp);

    // print the list of services and autostart indicator that was
    // just obtained by the preceeding code
    INFO("Services ...\n");
    for (int id = 0; id < max_svcs; id++) {
        INFO("%20s  %c\n", svcs[id].name, svcs[id].autostart);
    }

    // start all autostart svcs
    for (int id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];
        if (SERVICE_IS_STOPPED(x->state) && x->autostart == 'y') {
            memset(x->req, 0, sizeof(x->req));
            x->state = SERVICE_STATE_RUNNING;
            run_svc(id);
        }
    }
}

void svcs_display(int bg_color)
{
    sdlx_event_t event;
    int         id;
    bool        done = false;
    sdlx_loc_t  *loc;
    double      row;

    #define EVID_SVC_START    100
    #define EVID_SVC_STOP     200

    // handle the setting display
    while (true) {
        // init display and display title line
        sdlx_display_init(bg_color);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, bg_color);
        sdlx_render_text_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Services");

        // display name and controls for each service
        row = 2;
        for (id = 0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];

            sdlx_print_init_color(SERVICE_STATE_TO_COLOR(x->state), bg_color);
            sdlx_render_printf(0, ROW2Y(row), "%-s", x->name);

            sdlx_print_init_color(COLOR_LIGHT_BLUE, bg_color);
            if (SERVICE_IS_STOPPED(x->state)) {
                loc = sdlx_render_printf(COL2X(10), ROW2Y(row), "start");
                sdlx_register_event(loc, EVID_SVC_START+id);
            } else if (x->state == SERVICE_STATE_RUNNING) {
                loc = sdlx_render_printf(COL2X(10), ROW2Y(row), "stop");
                sdlx_register_event(loc, EVID_SVC_STOP+id);
            }

            row += 1.5;
        }

        // display the control event 'X' to exit this
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, bg_color, 0, 0, EVID_QUIT);  // xxx arg for COLOR_WHITE?

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
            process_svc_start_req(id);
            break;
        case EVID_SVC_STOP ... EVID_SVC_STOP + MAX_SVCS - 1:
            id = event.event_id - EVID_SVC_STOP;
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

void svcs_stop_all(void)
{
    int id, duration_ms = 0;
    bool all_stopped;

    INFO("stopping all services\n");

    for (id = 0; id < max_svcs; id++) {
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

// -----------------  SVCS ROUTINES AVAIL IN PICOC  ---------------

//
// routines called by apps
//

int svc_make_req(char *svc_name, int req_id, char *req_data, int req_data_len, int timeout_secs)
{
    #define TIMEOUT_USEC (20L * SEC)  // xxx needs arg

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

//
// routines called by svcs
//

int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs)
{
    struct timespec ts = {timeout_abstime_secs, 0 };
    int             ret;
    int             id;

    INFO("svc_name=%s timeout_abstime_secs=%ld time_until_timeout=%ld\n", 
         svc_name, timeout_abstime_secs, timeout_abstime_secs-time(NULL));
 
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

// -----------------  HANDLERS  -----------------------------------

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
    run_svc(id);
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

static void process_svc_stopped_callback(int id, int rc)
{
    svc_t *x = &svcs[id];

    INFO("called for id=%d name=%s rc=%d\n", id, x->name, rc);

    x->state = (rc == 0 ? SERVICE_STATE_STOPPED : SERVICE_STATE_STOPPED_BY_ERROR);
}   

// -----------------  RUN A SVC  ------------------------------------

static int svc_thread(void *cx);

static void run_svc(int id)
{
    sdlx_create_detached_thread(svc_thread, (void*)(long)id);
}

static int svc_thread(void *cx)
{
    int id = (int)(long)cx;
    svc_t *x = &svcs[id];
    int rc;

    rc = run(x->name, true);  // is_svc = true

    process_svc_stopped_callback(id, rc);

    return 0;
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
