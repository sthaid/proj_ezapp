#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>
#include <private.h>

//
// defines
//

#define SERVICE_STATE_DELETED           0     // blue  (should not be seen)
#define SERVICE_STATE_STOPPED           1     // white
#define SERVICE_STATE_RUNNING           2     // green
#define SERVICE_STATE_STOPPING          3     // yellow
#define SERVICE_STATE_DELETING          4     // yellow
#define SERVICE_STATE_STOPPED_BY_ERROR  5     // red

#define SERVICE_STATE_STR(state) \
    ((state) == SERVICE_STATE_DELETED           ? "DELETED"          : \
     (state) == SERVICE_STATE_STOPPED           ? "STOPPED"          : \
     (state) == SERVICE_STATE_RUNNING           ? "RUNNING"          : \
     (state) == SERVICE_STATE_STOPPING          ? "STOPPING"         : \
     (state) == SERVICE_STATE_DELETING          ? "DELETING"         : \
     (state) == SERVICE_STATE_STOPPED_BY_ERROR  ? "STOPPED_BY_ERROR" : \
                                                  "????")

#define SERVICE_STATE_TO_COLOR(state) \
    ((state) == SERVICE_STATE_DELETED           ? COLOR_BLUE   : \
     (state) == SERVICE_STATE_STOPPED           ? COLOR_WHITE  : \
     (state) == SERVICE_STATE_RUNNING           ? COLOR_GREEN  : \
     (state) == SERVICE_STATE_STOPPING          ? COLOR_YELLOW : \
     (state) == SERVICE_STATE_DELETING          ? COLOR_YELLOW : \
                                                  COLOR_RED)

#define SERVICE_IS_STOPPED(state)  ((state) == SERVICE_STATE_STOPPED || \
                                    (state) == SERVICE_STATE_STOPPED_BY_ERROR)

#define MAX_SVCS 20
#define MAX_SVC_NAME 30
#define MAX_SVC_REQ_QUEUE 5

#define MS  1000
#define SEC 1000000

#define SVC_STOP_TIMEOUT_SECS  5
#define SVC_STOP_TIMEOUT_MS    (SVC_STOP_TIMEOUT_SECS * MS)

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

static void update_svcs_tbl(void);
static void process_svc_start_req(int id);
static void process_svc_stop_req(int id);
static void run_svc(char *svc_name);
static int svc_thread(void *cx);

static int svc_name_to_id(char *svc_name);
static void remove_trailing_newline(char *s);
static int num_svcs(void);

// -----------------  SVCS ROUTINES USED BY MAIN.C  ---------------

// start all services, except if tagged stopped;
// a svc is tagged stopped if it has a 'stopped' file in its dir
void svcs_start(void)
{
    char dir[100];
    bool stopped;
    int  id;
    char tmp_name[MAX_SVC_NAME];

    INFO("start services\n");

    // find all svc names, these are the svcs/* directories, and populates
    // the svcs tbl based on the names found
    update_svcs_tbl();

    // start all svcs that are not tagged with 'stopped' file 
    // and are in SERVICE_STATE_STOPPED
    for (id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];

        // determine if svc is marked 'stopped'
        // note: use of tmp_name is to workaround compiler warning about sprintf truncation
        strcpy(tmp_name, x->name);
        snprintf(dir, sizeof(dir), "svcs/%s", tmp_name);
        stopped = util_file_exists(dir, "stopped");

        // if svc is not marked 'stopped', and its state is SERVICE_STATE_STOPPED, then run the svc
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
    bool stop_requested[MAX_SVCS];

    INFO("stop services\n");

    // request running svcs to stop
    memset(stop_requested, 0, sizeof(stop_requested));
    for (id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];

        if (x->state == SERVICE_STATE_RUNNING) {
            x->state = SERVICE_STATE_STOPPING;
            svc_make_req(svcs[id].name, SVC_REQ_ID_STOP, NULL, 0, SVC_STOP_TIMEOUT_SECS);
            stop_requested[id] = true;
        }
    }

    // wait, with timeout, for all services to be stopped
    while (true) {
        // determine if all svcs for which stop was just requested, are stopped
        all_stopped = true;
        for (id = 0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];
            if (stop_requested[id] && !SERVICE_IS_STOPPED(x->state)) {
                all_stopped = false;
                break;
            }
        }

        // if all stopped then this routine has completed
        if (all_stopped) {
            INFO("all stop requested services are stopped\n");
            break;
        }

        // if all services have not stopped within timeout then
        // print which services did not stop, and return
        if (duration_ms > SVC_STOP_TIMEOUT_MS) {
            ERROR("the following services have failed to stop ...\n");
            for (id = 0; id < max_svcs; id++) {
                svc_t *x = &svcs[id];
                if (stop_requested[id] && !SERVICE_IS_STOPPED(x->state)) {
                    ERROR("- %-12s %s\n", x->name, SERVICE_STATE_STR(x->state));
                }
            }
            break;
        }

        // short sleep
        usleep(100*MS);
        duration_ms += 100;
    }
}

void svcs_display(int bg_color)
{
    sdlx_event_t event;
    int         id, k;
    bool        done = false;
    sdlx_loc_t  *loc;
    char        dir[100];
    int         y_top, y_bottom, y2;
    double      y;

    #define EVID_SVC_START    100
    #define EVID_SVC_STOP     200

    sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

    // init variables which define the vertical region of the display
    // being used for the filename list
    y_top    = ROW2Y(3);
    y_bottom = sdlx_win_height;
    y        = y_top;

    // handle the setting display
    while (true) {
        // find all svc names, these are the svcs/* directories, and populates
        // the svcs tbl based on the names found
        update_svcs_tbl();

        // init display and display title line
        sdlx_display_init(bg_color, PORTRAIT);
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), 
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, 
                               "Services");

        // display name and controls for each service
        for (k=0, id=0; id < max_svcs; id++) {
            svc_t *x = &svcs[id];

            // if svc has been deleted then continue so that it is not displayed
            if (x->state == SERVICE_STATE_DELETED) {
                continue;
            }

            // disable scroling if there are 12 or fewer services
            if (num_svcs() <= 12) {
                y = y_top;
            }

            // if y display location is above the top of the display region,
            // or below the bottom of the display region, then either continue or break
            y2 = y + 2 * k++ * sdlx_char_height_dflt;
            if (y2 < y_top - sdlx_char_height_dflt/2) continue;
            if (y2 > y_bottom) break;

            // display svc name
            sdlx_render_printf_ex1(0, y2, FONT_NORMAL, SERVICE_STATE_TO_COLOR(x->state), "%-s", x->name);

            // display state info, or control for this svc
            if (x->state == SERVICE_STATE_DELETING) {
                sdlx_render_printf_ex1(COL2X(10), y2, FONT_NORMAL, COLOR_WHITE, "deleting");
            } else if (x->state == SERVICE_STATE_STOPPING) {
                sdlx_render_printf_ex1(COL2X(10), y2, FONT_NORMAL, COLOR_WHITE, "stopping");
            } else if (SERVICE_IS_STOPPED(x->state)) {
                loc = sdlx_render_printf_ex1(COL2X(10), y2, FONT_NORMAL, COLOR_LIGHT_BLUE, "start");
                sdlx_register_event(loc, EVID_SVC_START+id);
            } else if (x->state == SERVICE_STATE_RUNNING) {
                loc = sdlx_render_printf_ex1(COL2X(10), y2, FONT_NORMAL, COLOR_LIGHT_BLUE, "stop");
                sdlx_register_event(loc, EVID_SVC_STOP+id);
            }
        }

        // register motion and control events
        sdlx_register_event(NULL, EVID_MOTION);
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
        case EVID_MOTION:
            y += event.u.motion.yrel;
            if (y >= y_top) {
                y = y_top;
            }
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

static void update_svcs_tbl(void)
{
    FILE *fp;
    char s[100];
    char new[MAX_SVCS][MAX_SVC_NAME];
    int  max_new = 0;
    long new_mtime;

    static bool first_call = true;
    static long mtime;

    // if the svcs dir has not been modifies since last call then return
    new_mtime = util_file_mtime("svcs", NULL);
    if (new_mtime == mtime) {
        return;
    }
    mtime = new_mtime;

    // get list of svcs/* directories, store in 'new' tbl
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

        // if this svc is already deleted or deleting then continue
        if (x->state == SERVICE_STATE_DELETED || x->state == SERVICE_STATE_DELETING) {
            continue;
        }

        // determine if svc name is contained in the new names list
        bool found_in_new_names = false;
        for (int i = 0; i < max_new; i++) {
            if (strcmp(x->name, new[i]) == 0) {
                found_in_new_names = true;
                break;
            }
        }

        // if svc name is not contained in the new names list then delete the svc
        if (!found_in_new_names) {
            INFO("deleting svc %s\n", x->name);

            // if svc is running then send it stop request
            if (x->state == SERVICE_STATE_RUNNING) {
                x->state = SERVICE_STATE_STOPPING;
                svc_make_req(x->name, SVC_REQ_ID_STOP, NULL, 0, SVC_STOP_TIMEOUT_SECS);
            }

            // wait for service to be in stopped state
            int ms = 0;
            while (ms < SVC_STOP_TIMEOUT_MS && !SERVICE_IS_STOPPED(x->state)) {
                usleep(100*MS);
                ms += 100;
            }

            // the svc should be stopped now
            //
            // if it is stopped then
            //   remove the svc
            // else
            //   set svc state to SERVICE_STATE_DELETING
            // endif
            if (SERVICE_IS_STOPPED(x->state)) {
                x->name[0] = '\0';
                x->state = SERVICE_STATE_DELETED;
                pthread_mutex_destroy(&x->mutex);
                pthread_cond_destroy(&x->cond);
                memset(x->req, 0, sizeof(x->req));
            } else {
                x->state = SERVICE_STATE_DELETING;
            }
        }
    }

    // for all new svc names that don't currently exist in svcs tbl add entry to svcs tbl
    for (int i = 0; i < max_new; i++) {
        int id;

        // if new name is already in svcs tbl then continue
        if (svc_name_to_id(new[i]) != -1) {
            continue;
        }

        // find first free entry in svcs tbl;
        // if not found then add new entry to the end of svcs tbl
        for (id = 0; id < max_svcs; id++) {
            if (svcs[id].state == SERVICE_STATE_DELETED) {
                break;
            }
        }
        if (id == max_svcs) {
            if (max_svcs == MAX_SVCS) {
                ERROR("svcs tbl is full\n");
                break;
            }
            max_svcs++;
        }

        // init the new svcs tbl entry for the new svc
        svc_t *x = svcs + id;
        strcpy(x->name, new[i]);
        x->state = SERVICE_STATE_STOPPED;
        pthread_mutex_init(&x->mutex, NULL);
        pthread_cond_init(&x->cond, NULL);
        memset(x->req, 0, sizeof(x->req));

        // except on first call, the new svc is tagged 'stopped'
        if (!first_call) {
            char dir[100], tmp_name[MAX_SVC_NAME];
            strcpy(tmp_name, x->name);
            sprintf(dir, "svcs/%s", tmp_name);
            util_write_file(dir, "stopped", NULL, 0);
        }
    }

    // debug print new svc_tbl
    INFO("Services List ...\n");
    for (int id = 0; id < max_svcs; id++) {
        svc_t *x = &svcs[id];
        if (x->state != SERVICE_STATE_DELETED) {
            INFO("%15s %s\n", x->name, SERVICE_STATE_STR(x->state));
        }
    }

    // clear first_call flag
    first_call = false;
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
    svc_make_req(x->name, SVC_REQ_ID_STOP, NULL, 0, SVC_STOP_TIMEOUT_SECS);
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
    bool       okay;

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

    // verify svc state is okay to make a request to it
    x = &svcs[svc_id];
    okay = ((x->state == SERVICE_STATE_RUNNING) ||
            (req_id == SVC_REQ_ID_STOP && x->state == SERVICE_STATE_STOPPING));
    if (!okay) {
        ERROR("service %s state %s invalid to make req\n",
              x->name, SERVICE_STATE_STR(x->state));
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

// returns the number of services that are not in state SERVICE_STATE_DELETED
static int num_svcs(void)
{
    int num = 0;

    for (int id = 0; id < max_svcs; id++) {
        svc_t *x = svcs+id;
        if (x->state != SERVICE_STATE_DELETED) {
            num++;
        }
    }

    return num;
}

