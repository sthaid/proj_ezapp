#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>
#include <logging.h>
#include <picoc_ezapp.h>

#ifdef ANDROID
#include <SDL3/SDL.h>
#endif

#include "version.h"

// xxx
// - update comments throughout

//
// defines
//

#ifdef ANDROID
#define MAIN SDL_main
#else
#define MAIN main
#endif

#define DEFAULT_DEVEL_PORT     9000   // IANA registered port range 1024 - 49151
#define DEFAULT_DEVEL_PASSWORD "secret"

#define LAST_PAGE ((max_apps - 1) / 18)

#define BG_COLOR COLOR_PURPLE

#define EVID_PAGE_DECREMENT  900
#define EVID_PAGE_INCREMENT  901
#define EVID_MINIMIZE        902

#define MS  1000
#define SEC 1000000

#define CREATE_FILES_INIT                1
#define CREATE_FILES_RESET_APPS_AND_SVCS 2

//#define PRINT_TYPE_SIZES

//
// typedefs
//

typedef struct {
    bool   devel_mode;
    int    devel_port;
    char   devel_password[50];
    bool   foreground_enabled;
    double record_gain;
    double record_silence;
    bool   event_box_enable;
} params_t;

//
// variables
//

static char       *storage_path;
static params_t    params;
static pthread_t   server_tid;

//
// prototypes
//

static void processing(void);
static int devel_mode_server_thread(void *cx);

// -----------------  MAIN  ------------------------------------------

static int init(void);
static void cleanup(void);
static void sigusr2_hndlr(int signum);
static void print_type_sizes(void) __attribute__ ((unused));
static void create_files(int action);
static int run(char *name, bool is_svc);

int MAIN(int argc, char **argv)
{
    int rc;

    rc = init();
    if (rc != 0) {
        return 1;
    }

    processing();

    cleanup();

    return 0;
}

static int init(void)
{
    int rc;

    // get storage_path, and
    // set current working directory to storage_path
    storage_path = sdlx_get_storage_path();
    chdir(storage_path);

    // init logging
    rc = log_init();
    if (rc != 0) {
        return -1;
    }

    // print startup message
    INFO("========== STARTING: %s %s  ==========\n", VERSION, BUILD_DATE);
    INFO("storage_path = %s\n", storage_path);

#ifdef PRINT_TYPE_SIZES
    // print type sizes
    print_type_sizes();
#endif

    // get permissions when running on Android;
    // this is noop when running on Linux
    // xxx test without these granted
    #define GET_PERMISSION(str) \
        do { \
            if (sdlx_get_permission("android.permission." str) != 0) { \
                ERROR("failed to get permission %s\n", str); \
            } \
        } while (0)
    GET_PERMISSION("POST_NOTIFICATIONS");
    GET_PERMISSION("ACCESS_COARSE_LOCATION");
    GET_PERMISSION("ACCESS_FINE_LOCATION");
    GET_PERMISSION("ACTIVITY_RECOGNITION");
    GET_PERMISSION("RECORD_AUDIO");

    // init android utils, which provide support for:
    // - text to speech
    // - location
    // - flashlight
    // this is noop when running on Linux
    util_android_utils_init();

    // get params, if they don't exist, set to default value
    params.devel_mode = util_get_numeric_param(".", "devel_mode", false);
    params.devel_port = util_get_numeric_param(".", "devel_port", DEFAULT_DEVEL_PORT);
    strcpy(params.devel_password, util_get_str_param(".", "devel_password", DEFAULT_DEVEL_PASSWORD));
    params.foreground_enabled = util_get_numeric_param(".", "foreground_enabled", 1);
    params.record_gain = util_get_numeric_param(".", "record_gain", DEFAULT_RECORD_GAIN);
    params.record_silence = util_get_numeric_param(".", "record_silence", DEFAULT_RECORD_SILENCE);
    params.event_box_enable = util_get_numeric_param(".", "event_box_enable", false);

    // provide params to other modules, when needed
    sdlx_event_box_ctrl(params.event_box_enable);
    sdlx_audio_params_t ap = { params.record_gain, params.record_silence };
    sdlx_audio_set_params(&ap);

    // extract files.tar, if needed
    create_files(CREATE_FILES_INIT);

    // allocate SIGUSR2, this signal is sent to the devel_mode_server_thread
    // when developer mode is disabled or developer mode port is changed
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigusr2_hndlr;
    sigaction(SIGUSR2, &action, NULL);

    // create devel mode server thread
    sdlx_create_detached_thread(devel_mode_server_thread, NULL);

    // init sdl xxx move
    // xxx need to init sensors and possibly audio, after permissions
    sdlx_init(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);
    INFO("sdlx_win_width,height = %d %d  sdlx_char_width,height=%d %d\n",
         sdlx_win_width, sdlx_win_height, sdlx_char_width_dflt, sdlx_char_height_dflt);

    // start/stop foreground mode based on the foreground_enabled param
    if (params.foreground_enabled) {
        util_start_foreground();
    } else {
        util_stop_foreground();
    }

    // init services, this will xxx
    svcs_init(run);

    // success
    return 0;
}

// xxx is this ever called
// xxx are other cleanup routines called?
// xxx free svc_call allocations ?
static void cleanup(void)
{
    INFO("TERMINATING\n");

    INFO("stopping services\n");
    svcs_stop_all();

    INFO("destroying android utils\n");
    util_android_utils_destroy();

    INFO("quitting SDL subsystems\n");
    sdlx_quit(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);

    INFO("cleanup completed\n");
}

static void create_files(int action)
{
#ifndef ANDROID
    INFO("this routine is only executed when on Android\n");
    return;
#endif

    if (action == CREATE_FILES_INIT) {
        bool extract_needed = false;
        int  rc;
        do {
            if (!util_file_exists(".", "apps")) { 
                INFO("extracting because no apps dir\n");
                extract_needed = true;
                break;
            }

            if (!util_file_exists(".", "files.tar")) { 
                INFO("extracting because no files.tar\n");
                extract_needed = true;
                break;
            }

            sdlx_copy_asset_file("files.tar.sig", ".");
            rc = system("cmp --quiet files.tar.sig files.tar.sig.save");
            rc = WEXITSTATUS(rc);
            util_delete_file(".", "files.tar.sig");
            if (rc != 0) {
                INFO("extracting because files.tar.sig different\n");
                extract_needed = true;
                break;
            }
        } while (0);

        if (extract_needed) {
            sdlx_copy_asset_file("files.tar", ".");
            sdlx_copy_asset_file("files.tar.sig", ".");
            system("tar -xvf files.tar");
            system("mv files.tar.sig files.tar.sig.save");  // xxx add util for this
        } else {
            INFO("not extracting\n");
        }
            
    } else if (action ==  CREATE_FILES_RESET_APPS_AND_SVCS) {
        svcs_stop_all();
        system("rm -rf apps svcs");
        system("tar -xvf files.tar apps svcs");
        svcs_init(run);
    } else {
        ERROR("invalid arg, action %d\n", action);
    }
}

static void sigusr2_hndlr(int signum)
{
    // nothing needed here
}

static void print_type_sizes(void)
{
    INFO("type sizes ...\n");
    INFO("  sizoef(char)   = %zd", sizeof(char));
    INFO("  sizoef(short)  = %zd", sizeof(short));
    INFO("  sizoef(int)    = %zd", sizeof(int));
    INFO("  sizoef(long)   = %zd", sizeof(long));
    INFO("  sizoef(size_t) = %zd", sizeof(size_t));
    INFO("  sizoef(off_t)  = %zd", sizeof(off_t));
    INFO("  sizoef(time_t) = %zd", sizeof(time_t));
    INFO("  sizoef(clock_t)= %zd", sizeof(clock_t));
    INFO("  sizoef(float)  = %zd", sizeof(float));
    INFO("  sizoef(double) = %zd", sizeof(double));
    INFO("  sizeof(1)      = %zd", sizeof(1));
    INFO("  sizeof(1L)     = %zd", sizeof(1L));
}

// -----------------  PROCESSING  ------------------------------------

#define MAX_APPS 100

static char *apps[MAX_APPS];
static int   max_apps;
static int   page;

static void display_menu(void);
static void get_list_of_apps(void);
static void settings(void);

static void processing(void)
{
    sdlx_event_t event;

    // sdlx_show_toast("STARTING");

    while (true) {
        // clear the display, and set the font to default
        sdlx_display_init(BG_COLOR, PORTRAIT);
        sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

        // display menu, and register for events
        display_menu();

        // register for screen bottom control events
        if (LAST_PAGE > 0) {
            sdlx_register_control_events(EVID_PAGE_DECREMENT, "<",
                                         EVID_PAGE_INCREMENT, ">",
                                         EVID_MINIMIZE, "X",
                                         COLOR_WHITE, BG_COLOR);
        } else {
            sdlx_register_control_events(0, NULL, 
                                         0, NULL, 
                                         EVID_MINIMIZE, "X",
                                         COLOR_WHITE, BG_COLOR);
        }

        // update the display
        sdlx_display_present();

        // wait for an event, 1 sec timeout
        sdlx_get_event(1*SEC, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process the event
        INFO("proc event_id %d\n", event.event_id);
        if (event.event_id == EVID_QUIT) {
            INFO("got EVID_QUIT\n");
            break;
        } else if (event.event_id == EVID_MINIMIZE) {
            INFO("got EVID_MINIMIZE\n");
            sdlx_minimize_window();
        } else if (event.event_id == EVID_PAGE_DECREMENT) {
            if (--page < 0) {
                page = LAST_PAGE;
            }
        } else if (event.event_id == EVID_PAGE_INCREMENT) {
            if (++page > LAST_PAGE) {
                page = 0;
            }
        } else if (event.event_id >= 0 && event.event_id <= max_apps-1) {
            int id = event.event_id;
            sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);
            if (apps[id] == NULL) {
                ERROR("apps[%d] is NULL\n", id);
            } else if (strcmp(apps[id], "Settings") == 0) {
                settings();
            } else {
                run(apps[id], false);
            }
        }
    }
}

static int run(char *name, bool is_svc)
{
    char           dir_path[100];
    int            rc;
    DIR           *dir;
    struct dirent *dirent;
    char          *p;
    char           picoc_args[1000];

    // xxx comment
    if (!is_svc) {
        sprintf(dir_path, "apps/%s", name);
    } else {
        sprintf(dir_path, "svcs/%s", name);
    }

    // construct list of *.c files in the dir
    picoc_args[0] = '\0';
    dir = opendir(dir_path);
    if (dir == NULL) {
        ERROR("%s: failed to opendir %s, %s\n", name, dir_path, strerror(errno));
        return 99;
    }
    p = picoc_args;
    while ((dirent = readdir(dir)) != NULL) {
        char *fn = dirent->d_name;
        int len = strlen(fn);
        if (len > 2 && strcmp(fn+len-2, ".c") == 0) {
            p += sprintf(p, "%s/%s ", dir_path, fn);
        }
    }
    closedir(dir);

    // error if no source code found in dir_path
    if (picoc_args[0] == '\0') {
        ERROR("%s: no source code in %s\n", name, dir_path);
        return 99;
    }

    // xxx comment
    p += sprintf(p, " - %s %s", name, dir_path);

    // run the app using the picoc c language interpreter
    INFO("%s: starting, args = %s\n", name, picoc_args);
    rc = picoc_ezapp(picoc_args);
    INFO("%s: completed, rc = %d\n", name, rc);

    // return completion status
    return rc;
}

// -----------------  DISPLAY MENU  -------------------------------

static sdlx_texture_t *create_filled_circle_texture(int radius, sdlx_color_t color);

static void display_menu(void)
{
    static sdlx_texture_t *circle;
    int first, last;

    #define RADIUS 100

    // allocate circle texture, which is used when displaying menu items
    if (circle == NULL) {
        circle = create_filled_circle_texture(RADIUS, COLOR_BLUE);  // xxx free?
    }

    // get the list of apps: 
    // - this initializes the apps[] array of  app names
    // - the dir names must be the same as the app names
    // - the apps array is indexed by the location on the display, for
    //   example idx=0 is top left, and idx=17 is bottom right
    get_list_of_apps();

    first = page * 18;
    last  = first + 17;

#if 0 //xxx del
    if (LAST_PAGE > 0) { // xxx test multiple pages
        sdlx_render_printf_ex1(sdlx_win_width/2, sdlx_char_height_dflt/2, 
                               FONT_SMALL, COLOR_WHITE, 
                               "Page %d", page);
    }
#endif

    for (int i = first; i <= last; i++) {
        char     *name = apps[i];
        char      s1[10], s2[10];
        int       len, l1, l2, lmax, x, y;
        double    chw, chh, numchars;
        sdlx_loc_t loc;

        if (name == NULL) {
            continue;
        }

        len  = strlen(name);
        if (len > 8) len = 8;

        if (len <= 4) {
            l1 = len;
            l2 = 0;
            strcpy(s1, name);
            s2[0] = '\0';
            lmax = l1;
        } else {
            l1 = len / 2;
            l2 = len - l1;
            strncpy(s1, name, l1);
            strncpy(s2, name+l1, l2);
            s1[l1] = '\0';
            s2[l2] = '\0';
            lmax = l2;
        }

        if (s2[0] == '\0') {
            double k = (len == 1 ? 1 : 1.5);
            chw = (k * RADIUS) / lmax;
            numchars = sdlx_win_width / chw;
        } else {
            double k = ((len == 5 || len == 6) ? 1.35 : 1.5);
            chw = (k * RADIUS) / lmax;
            numchars = sdlx_win_width / chw;
        }
        chh = chw / 0.6;

        // determine dispaly location of the center of the menu item
        x = (sdlx_win_width/3/2) + (i%3) * (sdlx_win_width/3);
        y = ((sdlx_win_height-150)/6/2) + ((i-first)/3) * ((sdlx_win_height-150)/6);

        // display the menu item
        // - first render the circle
        // - then render the app name text within the circle
        sdlx_render_texture(circle, x-RADIUS, y-RADIUS);
        if (s2[0] == '\0') {
            sdlx_render_printf_ex2(x, y, 
                                   numchars, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                                   "%s", s1);
        } else {
            sdlx_render_printf_ex2(x, nearbyint(y-0.5*chh), 
                                   numchars, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                                   "%s", s1);
            sdlx_render_printf_ex2(x, nearbyint(y+0.5*chh), 
                                   numchars, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                                   "%s", s2);
        }

        // register event
        loc.x = x - RADIUS;
        loc.y = y - RADIUS;
        loc.w = 2 * RADIUS;
        loc.h = 2 * RADIUS;
        sdlx_register_event(&loc, i);
    }
}

static sdlx_texture_t *create_filled_circle_texture(int radius, sdlx_color_t color)
{
    int w, h;
    sdlx_texture_t *t;

    w = h = 2 * radius;

    t = sdlx_create_texture(w, h);
    sdlx_set_render_target(t);
    sdlx_render_fill_circle(w/2, h/2, radius, color);
    sdlx_set_render_target(NULL);

    return t;
}

static void get_list_of_apps(void)
{
    const char *layout_file_path = "apps/layout";
    struct stat statbuf;
    int         rc, i, cnt, secs, n, line_num;
    FILE       *fp;
    char        str[200], s[3][100];

    static long layout_file_mtime;
    static bool first_call = true;

    // if layout file doesn't exist then
    // return without changing the list of apps,
    // because the file may have just been temporarily deleted
    rc = stat(layout_file_path, &statbuf);
    if (rc != 0) {
        return;
    }

    // if layout file has not changed then 
    // return without updating the list of apps
    if (statbuf.st_mtime == layout_file_mtime) {
        return;
    }
    layout_file_mtime = statbuf.st_mtime;

    // obtain the list of apps from the layout file ...

    // the layut file may currently being updated;
    // wait for the layout file to not have any processes having it open
    if (!first_call) {
        secs = 0;
        while (true) {
            str[0] = '\0';
            fp = popen("lsof apps/layout | wc -l", "r");
            fgets(str, sizeof(str), fp);
            pclose(fp);
            cnt = sscanf(str, "%d", &n);
            if (cnt != 1) {
                ERROR("invalid output from wc, '%s'\n", str);
                break;
            }
            if (n == 0) {
                INFO("layout file not open by any processes, secs=%d\n", secs);
                break;
            }
            if (secs >= 3) {
                ERROR("timedout waiting for layout file\n");
                break;
            }
            sleep(1);
            secs++;
        }
    }
    first_call = false;

    // free the current apps names
    for (i = 0; i < max_apps; i++) {
        free(apps[i]);
        apps[i] = NULL;
    }
    max_apps = 0;

    // read the app names, which must be the same as their dir names, from the layout file
    line_num = 0;
    fp = fopen(layout_file_path, "r");
    while (fgets(str, sizeof(str), fp)) {
        line_num++;

        // xxx cleanup input str by removing terminating newline 
        // and removing leading spaces

        // ignore lines that begin with '#', space, or newline
        if (str[0] == '\n' || str[0] == ' ' || str[0] == '#') {
            continue;
        }

        // read 3 app names from each line of the layout file
        cnt = sscanf(str, "%s %s %s", s[0], s[1], s[2]);
        if (cnt != 3) {
            ERROR("invalid line, line_num = %d\n", line_num);
            break;
        }

        // store the app names, just read, to the apps[] array;
        // ignoring app names that are "-"
        for (i = 0; i < 3; i++) {
            if (strcmp(s[i], "-") != 0) {
                apps[max_apps] = strdup(s[i]);
            }
            max_apps++;
        }
    }
    fclose(fp);

    // debug print the list of apps names
    INFO("max_apps = %d\n", max_apps);
    for (i = 0; i < max_apps; i++) {
        if (apps[i] != NULL) {
            INFO("apps[%d] = %s\n", i, apps[i]);
        }
    }
}

// -----------------  SETTINGS  -----------------------------------

static void copyright(void);
static double get_number(char *prompt, double min, double max) __attribute__ ((unused)); // xxx use in other places

static void settings(void)
{
    // record_test_state values
    #define IDLE      0
    #define RECORDING 1
    #define PLAYBACK  2

    #define RECORD_TEST_FILENAME "record_test.mp3"

    #define EVID_COPYRIGHT            1001
    #define EVID_DEVEL_MODE           1002
    #define EVID_DEVEL_PORT           1003
    #define EVID_DEVEL_PASSWORD       1004
    #define EVID_SERVICES             1005
    #define EVID_RECORD_GAIN          1006
    #define EVID_RECORD_SILENCE       1007
    #define EVID_RECORD_TEST          1008
    #define EVID_RESET_APPS_AND_SVCS  1020
    #define EVID_FOREGROUND           1021
    #define EVID_EVENT_BOX_ENABLE     1022

    #define GET_Y2 ({ y2 += 2*sdlx_char_height_dflt; \
                      y2 >= y_top - 1.5 * sdlx_char_height_dflt && y2 <= y_bottom; })

    bool                done = false;
    sdlx_event_t        event;
    sdlx_loc_t         *loc;
    char               *msg = NULL;
    long                msg_time = 0;
    char               *ipaddr;
    int                 record_test_state = IDLE;
    sdlx_audio_params_t ap;
    sdlx_audio_state_t  as;
    int                 y_top;
    int                 y_bottom;
    double              y;
    int                 y2;

    // get this device ipaddr
    ipaddr = util_get_ipaddr();

    // init variables which define the vertical region of the display
    // being used for the filename list
    y_top    = ROW2Y(4.5);
    y_bottom = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT;
    y        = y_top;

    // handle the setting display
    while (true) {
        // init display and font size/color
        sdlx_display_init(BG_COLOR, PORTRAIT);

        // display title line, version, and ipaddr
        sdlx_render_fill_rect(0, 0, sdlx_win_width, 4*sdlx_char_height_dflt, BG_COLOR);
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(0),
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE,
                               "%s", "Settings");
        sdlx_render_printf(0, ROW2Y(1), "Version = %s", VERSION);
        sdlx_render_printf(0, ROW2Y(2), "%s", BUILD_DATE);
        sdlx_render_printf(0, ROW2Y(3), "%s:%d", ipaddr, params.devel_port);

        // display Copyright
        sdlx_print_set_default(FONT_NORMAL, COLOR_LIGHT_BLUE);
        y2 = nearbyint(y - 2*sdlx_char_height_dflt);
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Copyright");
            sdlx_register_event(loc, EVID_COPYRIGHT);
        }

        // display Devel_Mode
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Devel_Mode = %s", params.devel_mode ? "ON" : "OFF");
            sdlx_register_event(loc, EVID_DEVEL_MODE);
        }

        // display Devel_Port
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Devel_Port = %d", params.devel_port);
            sdlx_register_event(loc, EVID_DEVEL_PORT);
        }

        // display Devel_Password
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Devel_Password");
            sdlx_register_event(loc, EVID_DEVEL_PASSWORD);
        }

        // display Services
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Services");
            sdlx_register_event(loc, EVID_SERVICES);
        }

        // display Record_Gain
        if (GET_Y2) {
            sdlx_audio_get_params(&ap);
            loc = sdlx_render_printf(0, y2, "Rec_Gain = %0.2f", ap.record_gain);
            sdlx_register_event(loc, EVID_RECORD_GAIN);
        }

        // display Record_Silence
        if (GET_Y2) {
            sdlx_audio_get_params(&ap);
            loc = sdlx_render_printf(0, y2, "Rec_Silence = %0.2f", ap.record_silence);
            sdlx_register_event(loc, EVID_RECORD_SILENCE);
        }

        // display Record_Test
        if (GET_Y2) {
            sdlx_audio_get_state(&as);
            if (record_test_state == IDLE) {
                loc = sdlx_render_printf(0, y2, "Rec_Test");
                sdlx_register_event(loc, EVID_RECORD_TEST);
            } else if (record_test_state == RECORDING) {
                int bar_value_w =  sdlx_win_width * as.volume;
                int bar_height = sdlx_char_height_dflt;
                sdlx_render_printf(sdlx_win_width-COL2X(4), y2, "%4.2f", as.volume);
                sdlx_render_fill_rect(0, y2, bar_value_w, bar_height, COLOR_RED);
                sdlx_render_rect(0, y2, sdlx_win_width, bar_height, 2, COLOR_WHITE);
            } else if (record_test_state == PLAYBACK) {
                int bar_value_w = (as.play_total_secs ? (sdlx_win_width * as.play_current_secs / as.play_total_secs) : 0);
                int bar_height = sdlx_char_height_dflt;
                sdlx_render_fill_rect(0, y2, bar_value_w, bar_height, COLOR_GREEN);
                sdlx_render_rect(0, y2, sdlx_win_width, bar_height, 2, COLOR_WHITE);
            }
        }

        // display Reset_Apps_And_svcs
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Reset_Apps_And_Svcs");
            sdlx_register_event(loc, EVID_RESET_APPS_AND_SVCS);
        }

        // display Foreground
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Foreground = %s", params.foreground_enabled ? "ENABLED" : "DISABLED");
            sdlx_register_event(loc, EVID_FOREGROUND);
        }

        // display Event_Box
        if (GET_Y2) {
            loc = sdlx_render_printf(0, y2, "Event_Box = %s", params.event_box_enable ? "ENABLED" : "DISABLED");
            sdlx_register_event(loc, EVID_EVENT_BOX_ENABLE);
        }

        // change print color back to white
        sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

        // Record_Test processing
        sdlx_audio_get_state(&as);
        if (record_test_state == RECORDING && (as.record_secs > 6 || as.state == AUDIO_STATE_IDLE)) {
            sdlx_audio_play_file(".", RECORD_TEST_FILENAME);
            record_test_state = PLAYBACK;
        } else if (record_test_state == PLAYBACK && as.state == AUDIO_STATE_IDLE) {
            util_delete_file(".", RECORD_TEST_FILENAME);
            record_test_state = IDLE;
        }

        // if a message is requested for display then do so;
        // otherwise, when in developer mode, display ipaddr:port
        if (msg && (util_microsec_timer() - msg_time) < 3000000) {
            sdlx_render_printf(0, sdlx_win_height-300, "%s", msg);
        }

        // register motion and control events
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_register_control_events(0, NULL, 0, NULL, EVID_QUIT, "X", COLOR_WHITE, BG_COLOR);

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
        case EVID_COPYRIGHT:
            copyright();
            break;
        // xxx add case for credits
        case EVID_DEVEL_MODE:
            params.devel_mode = (params.devel_mode ? 0 : 1);
            util_set_numeric_param(".", "devel_mode", params.devel_mode);
            if (!params.devel_mode) {
                INFO("sending SIGUSR2 to devel_mode_server_thread\n");
                pthread_kill(server_tid, SIGUSR2);
            }
            break;
        case EVID_DEVEL_PORT: {
            char *str; 
            int cnt, port;
            str = sdlx_get_input_str("Port", "1024 - 49151", true, BG_COLOR);
            cnt = sscanf(str, "%d", &port);
            if (cnt == 1 && (port >= 1024 && port <= 49151)) {
                params.devel_port = port;
                util_set_numeric_param(".", "devel_port", port);
                if (params.devel_mode) {
                    INFO("sending SIGUSR2 to devel_mode_server_thread\n");
                    pthread_kill(server_tid, SIGUSR2);
                }
            }
            break; }
        case EVID_DEVEL_PASSWORD: {
            char *str; 
            str = sdlx_get_input_str("Password", "Min Length 4", false, BG_COLOR);
            if (strlen(str) >= 4) {
                strcpy(params.devel_password, str);
                util_set_str_param(".", "devel_password", str);
                msg = "Password changed";
                msg_time = util_microsec_timer();
            } else {
                msg = "Password too short";
                msg_time = util_microsec_timer();
            }
            break; }
        case EVID_SERVICES:
            svcs_display(BG_COLOR);
            break;
        case EVID_RECORD_GAIN: {
            double number = get_number("Rec_Gain", 1, 20);
            if (number != INVALID_NUMBER) {
                params.record_gain = number;
                util_set_numeric_param(".", "record_gain", number);
                sdlx_audio_get_params(&ap);
                ap.record_gain = number;
                sdlx_audio_set_params(&ap);
            }
            break; }
        case EVID_RECORD_SILENCE: {
            double number = get_number("Rec_Silence", 0, 20);
            if (number != INVALID_NUMBER) {
                params.record_silence = number;
                util_set_numeric_param(".", "record_silence", number);
                sdlx_audio_get_params(&ap);
                ap.record_silence = number;
                sdlx_audio_set_params(&ap);
            }
            break; }
        case EVID_RECORD_TEST: {
            // auto_stop_secs = 3
            // append         = false
            // start_paused   = false
            sdlx_audio_record_from_mic(".", RECORD_TEST_FILENAME, 3, false, false);
            record_test_state = RECORDING;
            break; }
        case EVID_RESET_APPS_AND_SVCS: {
            char *str; 
            str = sdlx_get_input_str("Reset y/n", "", false, BG_COLOR);
            if (strcasecmp(str, "y") != 0) {
                break;
            }
            create_files(CREATE_FILES_RESET_APPS_AND_SVCS);
            msg = "Apps/Svcs are reset";
            msg_time = util_microsec_timer();
            break; }
        case EVID_FOREGROUND: {
            params.foreground_enabled = (params.foreground_enabled ? false : true);
            util_set_numeric_param(".", "foreground_enabled", params.foreground_enabled);
            if (params.foreground_enabled) {
                util_start_foreground();
            } else {
                util_stop_foreground();
            }
            break; }
        case EVID_EVENT_BOX_ENABLE: {
            params.event_box_enable = (params.event_box_enable ? false : true);
            util_set_numeric_param(".", "event_box_enable", params.event_box_enable);
            sdlx_event_box_ctrl(params.event_box_enable);
            break; }
        case EVID_MOTION:
            y += event.u.motion.yrel;
            if (y >= y_top) {
                y = y_top;
            }
            break;
        case EVID_QUIT:
            if (record_test_state != IDLE) {
                sdlx_audio_stop();
                record_test_state = IDLE;
            }
            done = true;
            break;
        }

        if (done) {
            break;
        }
    }
}

static double get_number(char *prompt1, double min, double max)
{
    char  *input_str; 
    double number;
    char   prompt2[100];

    sprintf(prompt2, "%0.1f - %0.1f", min, max);

    input_str = sdlx_get_input_str(prompt1, prompt2, true, BG_COLOR);
    if (sscanf(input_str, "%lf", &number) != 1) {
        return INVALID_NUMBER;
    }
    if (min < max) {
        if (number < min) number = min;
        if (number > max) number = max;
    }
    return number;
}

static void copyright(void)
{
    double      y;
    int         y_top, y_bottom;
    int         len;
    sdlx_event_t event;
    bool        done = false;
    char       *lines[1];

    // read the copyright file
    lines[0] = util_read_file(".", "copyright", &len);
    if (lines[0] == NULL) {
        ERROR("failed to read copyright file\n");
        return;
    }

    // use tiny font
    sdlx_print_set_default(FONT_TINY, COLOR_WHITE);

    // init vars
    y_top    = 0;
    y_bottom = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT;
    y        = y_top;

    // display copyright
    while (true) {
        // display copyright and register for motion (scrolling) & exit events
        sdlx_display_init(BG_COLOR, PORTRAIT);
        sdlx_render_multiline_text(0, y, y_top, y_bottom, FONT_TINY, lines, NULL, 1);
        sdlx_register_control_events(0, NULL, 0, NULL, EVID_QUIT, "X", COLOR_WHITE, BG_COLOR);
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_display_present();

        sdlx_get_event(-1, &event);
        switch (event.event_id) {
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

    // free allocated copyrght buffer
    free(lines[0]);
}

// ----------------- DEVELOPER MODE SERVER  ----------------

#define MAX_PID_TBL 20

static int process_req_thread(void *cx);

static int devel_mode_server_thread(void *cx)
{
    struct sockaddr_in server_address;
    int                listen_sockfd, ret;

    // save server thread id in global, so that signals can be sent to this thread
    server_tid = pthread_self();

again:
    // wait for developer mode to be enabled
    // xxx should this terminate when program closes
    INFO("waiting for devel_mode enabled\n");
    sleep(1);
    while (params.devel_mode == false) {
        sleep(1);
    }
    INFO("server starting, listening on port %d\n", params.devel_port);

    // create listen socket
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd == -1) {
        ERROR("socket, %s\n", strerror(errno));
        return 0;
    }

    // set socket options
    int reuseaddr = 1;
    ret = setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(reuseaddr));
    if (ret == -1) {
        ERROR("setsockopt SO_REUSEADDR, %s\n", strerror(errno));
        return 0;
    }

    // bind socket to any ip addr, for specified port
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = htons(params.devel_port);
    ret = bind(listen_sockfd,
               (struct sockaddr *)&server_address,
               sizeof(server_address));
    if (ret == -1) {
        ERROR("bind, %s\n", strerror(errno));
        close(listen_sockfd);
        sleep(1);
        goto again;
    }

    // listen 
    ret = listen(listen_sockfd, 5);
    if (ret == -1) {
        ERROR("listen, %s\n", strerror(errno));
        return 0;
    }

    // accept and process connections
    INFO("accepting connections\n");
    while (1) {
        int                sockfd;
        struct sockaddr_in peer_addr;
        socklen_t          peer_addr_len;

        // accept connection
        peer_addr_len = sizeof(peer_addr);
        sockfd = accept(listen_sockfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (sockfd == -1) {
            ERROR("accept, %s\n", strerror(errno));
            break;
        }

        // create thread to process the client request
        sdlx_create_detached_thread(process_req_thread, (void*)(long)sockfd);
    }

    // close listen socket
    close(listen_sockfd);

    // goto top to reinit devel_mode_server_thread
    goto again;

    // not reached
    INFO("DEVEL_MODE_SERVER_THREAD TERMINATING\n");
    return 0;
}

int put_fmt(FILE *fp, char *fmt, ...)
{
    va_list ap;
    int rc;

    va_start(ap, fmt);

    rc = vfprintf(fp, fmt, ap);
    if (rc < 0) {
        return -1;
    }

    rc = fflush(fp);
    if (rc == EOF) {
        return -1;
    }

    va_end(ap);

    return 0;
}

char *get_str(FILE *fp, char *s, int s_len)
{
    char *p;
    int len;

    s[0] = '\0';

    p = fgets(s, s_len, fp);
    if (p == NULL) {
        //if (!feof(fp)) {
        //    printf("ERROR: get failed - error=%d\n", ferror(fp));
        //}
        return NULL;
    }

    len = strlen(s);
    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }

    return s;
}

static int process_req_thread(void *cx)
{
    int   sockfd = (int)(long)cx;
    FILE *sockfp;
    char  password[200];

    // get fp for socket
    sockfp = fdopen(sockfd, "w+");
    
    // read password from socket
    password[0] = '\0';
    get_str(sockfp, password, sizeof(password));

    // validate password
    if (strcmp(password, params.devel_password) == 0) {  // xxx get from param
        put_fmt(sockfp, "%s\n", "password okay");
    } else {
        put_fmt(sockfp, "%s\n", "password invalid");
        goto disconnect;
    }

    // put storage_path to socket
    put_fmt(sockfp, "%s\n", storage_path);

    // process cmds
    while (true) {
        char str[1000];
        int  status=99;

        // read from socket
        // - if socket has been closed on other end then goto disconnect
        // - else verify 'run' is received
        if (get_str(sockfp, str, sizeof(str)) == NULL) {
            goto disconnect;
        }
        if (strcmp(str, "run") != 0) {
            ERROR("'run' expected\n");
            goto disconnect;
        }

        // read cmdline from socket
        get_str(sockfp, str, sizeof(str));

        // the following cmdline are handled by this code;
        // - put        : create/update file on android device, arg=file_path
        // - get        : get file contents, arg=file_path
        // otherwise the cmdline is passed to popen for the 
        // android shell to process
        //
        // status return is either a negative errno, or a positive exit code
        if (strncmp(str, "put ", 4) == 0) {
            char *data, *p;
            int   data_len, rc, cnt;
            DIR  *dir;
            char  dest_path[200], src_filename[200];

            // extract dest_path and src_filename from str
            cnt = sscanf(str, "put %s %s", dest_path, src_filename);
            if (cnt != 2) {
                ERROR("dest_path and src_filename both required\n");
                goto disconnect;
            }

            // if dest_path is a directory then append src_filename
            dir = opendir(dest_path);
            if (dir != NULL) {
                int len = strlen(dest_path);
                if (len > 0 && dest_path[len-1] != '/') {
                    strcat(dest_path, "/");
                }
                strcat(dest_path, src_filename);
                closedir(dir);
            }

            // read data_len from sockfp
            p = get_str(sockfp, str, sizeof(str));
            if (p == NULL || sscanf(str, "data_len %d", &data_len) != 1) {
                ERROR("failed to get data_len\n");
                goto disconnect;
            }

            // read data from socket
            data = calloc(data_len, 1);             // nmemb=data_len, size=1
            status = fread(data, 1, data_len, sockfp);  // size=1, nmemb=data_len
            if (status != data_len) {
                ERROR("failed to read data from socket\n");
                free(data);
                goto disconnect;
            }

            // write data to android file
            rc = util_write_file(dest_path, NULL, data, data_len);
            status = (rc == 0 ? 0 : errno != 0 ? -errno : -EINVAL);

            // free allocated data
            free(data);
        } else if (strncmp(str, "get ", 4) == 0) {
            char  src_path[200];
            char *data;
            int   data_len, rc;

            // save src_path
            strcpy(src_path, str+4);

            // read android file
            data = util_read_file(src_path, NULL, &data_len);
            if (data == NULL) {
                // failed to read file
                status = (errno != 0 ? -errno : -EINVAL);
                put_fmt(sockfp, "data_len %d\n", 0);
            } else {
                // write data_len to socket
                put_fmt(sockfp, "data_len %d\n", data_len);

                // write data to socket
                rc = fwrite(data, 1, data_len, sockfp);  // size=1, nmemb=data_len
                if (rc != data_len) {
                    ERROR("failed to write data to socket\n");
                    free(data);
                    goto disconnect;
                }

                // free data, and set success status
                free(data);
                status = 0;
            }
        } else {
            FILE *fp;
            int rc;

            fp = popen(str, "r");
            if (fp == NULL) {
                status = (errno != 0 ? -errno : -EINVAL);
                ERROR("popen '%s' failed, %s\n", str, strerror(errno));
            } else {
                while (get_str(fp, str, sizeof(str)) != NULL) {
                    put_fmt(sockfp, "%s\n", str);
                }
                rc = pclose(fp);
                status = WEXITSTATUS(rc);
                INFO("pclose_rc=%d, WEXITSTATUS=%d\n", rc, status);
            }
        }

        // all cmds termintate with CMD_COMPLETE <status>
        put_fmt(sockfp, "CMD_COMPLETE %d\n", status);
    }

disconnect:
    fclose(sockfp);
    return 0;
}
