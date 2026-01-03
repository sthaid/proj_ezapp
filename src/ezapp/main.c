#include <std_hdrs.h>

#include <sdlx.h>
#include <utils.h>
#include <svcs.h>
#include <logging.h>
#include <main.h>

#ifdef ANDROID
#include <SDL3/SDL.h>
#endif
#include <picoc/picoc_ezapp.h>

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

#define DEFAULT_DEVEL_PORT 9000   // IANA registered port range 1024 - 49151
#define DEFAULT_DEVEL_PASSWORD "secret"

#define LAST_PAGE ((max_apps - 1) / 18)

#define BG_COLOR (!params.devel_mode ? COLOR_TEAL : COLOR_VIOLET)

#define SMALLEST_FONT 40
#define SMALL_FONT    30
#define DEFAULT_FONT  20
#define LARGE_FONT    10

#define EVID_PAGE_DECREMENT  900
#define EVID_PAGE_INCREMENT  901
#define EVID_MINIMIZE        902

#define MS  1000
#define SEC 1000000

#define CREATE_FILES_INIT  1
#define CREATE_FILES_RESET 2

//
// typedefs
//

typedef struct {
    bool   devel_mode;
    int    devel_port;
    char   devel_password[50];
    bool   foreground_enabled;
    double record_scale;
    double record_silence;
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

// -----------------  XXXXXXXXXXXXXX  --------------------------------

#if 0
#include "../lame/lame.h"
lame_global_flags *gfp;

int lame_test(void)
{
    int rc;
    short *pcm;
    int file_bytes;

    //lame_set_errorf(gfp, my_debugf);
    //lame_set_debugf(gfp, my_debugf);
    //lame_set_msgf(gfp, my_debugf);

    gfp = lame_init();
    printf("lame_init ret %p\n", gfp);
    if (gfp == NULL) {
        printf("ERROR lame_init failed\n");
        return 1;
    }

#define STEREO           0
#define JOINT_STEREO     1
#define DUAL_CHANNEL     2
#define MONO             3

    lame_set_num_channels(gfp,1);
    //lame_set_in_samplerate(gfp,48000);
    lame_set_in_samplerate(gfp,44100);
    lame_set_brate(gfp,64);
    lame_set_mode(gfp,MONO); 
    lame_set_quality(gfp,2);   /* 2=high  5 = medium  7=low */

    printf("NUM CHANNELS %d\n", lame_get_num_channels(gfp));

    rc = lame_init_params(gfp);
    printf("lame_init_params ret %d\n", rc);
    if (rc < 0) {
        printf("ERROR lame_init_params failed\n");
        return 1;
    }


    pcm = util_read_file(".", "captured_audio.pcm", &file_bytes);
    if (pcm == NULL) {
        printf("ERROR failed to read test file\n");
        return 1;
    }
    printf("file_bytes = %d\n", file_bytes);
    
    void *mp3buf = malloc(10000000);
    void *mp3buf_orig = mp3buf;
    int num_samples = file_bytes / 2;
    memset(mp3buf, 0, 10000000);

#if 0
    rc = lame_encode_buffer_interleaved(gfp, pcm, num_samples, mp3buf, 0);
#else
    int remaining = num_samples;
    while (remaining) {
        int xfer = remaining > 44100 ? 44100 : remaining;
        printf("xfer = %d\n", xfer);
        rc = lame_encode_buffer(gfp, pcm, NULL, xfer, mp3buf, 0);
        printf("lame_encode_buffer rc = %d\n", rc);
        if (rc < 0) {
            printf("ERROR\n");
            return 1;
        }
        mp3buf += rc;
        remaining -= xfer;
        pcm += xfer;
    }
#endif

    rc = lame_encode_flush(gfp, mp3buf, 0);
    printf("lame_encode_flush rc = %d\n", rc);
    if (rc < 0) {
        printf("ERROR\n");
        return 1;
    }
    mp3buf += rc;

    util_write_file(".", "out.mp3", mp3buf_orig, mp3buf-mp3buf_orig);

    return 0;
}
#endif


// -----------------  MAIN  ------------------------------------------

static int init(void);
static void cleanup(void);
static void sigusr2_hndlr(int signum);
static void print_type_sizes(void);
#ifdef ANDROID  // xxx get rid of some ifdefs
static void create_files(int action);
#endif

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

    // get params, if they don't exist, set to default value
    params.devel_mode = util_get_numeric_param(".", "devel_mode", 0);
    params.devel_port = util_get_numeric_param(".", "devel_port", DEFAULT_DEVEL_PORT);
    strcpy(params.devel_password, util_get_str_param(".", "devel_password", DEFAULT_DEVEL_PASSWORD));
    params.foreground_enabled = util_get_numeric_param(".", "foreground_enabled", 0);

    // xxx numeric keypad decimal point
    // xxx keyboard can be dismaissed and then stuck
    // xxx audio record scaling, and params
    params.record_scale = util_get_numeric_param(".", "record_scale", DEFAULT_RECORD_SCALE);
    params.record_silence = util_get_numeric_param(".", "record_silence", DEFAULT_RECORD_SILENCE);
    sdlx_audio_params_t ap = { params.record_scale, params.record_silence };
    sdlx_audio_set_params(&ap);

#ifdef ANDROID
    // copy asset files to the working directory
    sdlx_copy_asset_file("files.tar", ".");

    // if apps dir doesn't exist then 
    // extract all from files.tar
    // xxx true is temporary
    if (true || !util_file_exists(".", "apps")) { 
        create_files(CREATE_FILES_INIT);
    }
#endif

    // allocate SIGUSR2, this signal is sent to the devel_mode_server_thread
    // when developer mode is disabled or developer mode port is changed
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = sigusr2_hndlr;
    sigaction(SIGUSR2, &action, NULL);

    // create devel mode server thread
    sdlx_create_detached_thread(devel_mode_server_thread, NULL);

    // init sdl xxx move
    sdlx_init(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);
    INFO("sdlx_win_width,height = %d %d  sdlx_char_width,height=%d %d\n",
         sdlx_win_width, sdlx_win_height, sdlx_char_width, sdlx_char_height);

#ifdef ANDROID
    // get permissions when running on Android
    if (sdlx_get_permission("android.permission.POST_NOTIFICATIONS") != 0) {
        ERROR("failed to get permission POST_NOTIFICATION\n");
    }
    if (sdlx_get_permission("android.permission.ACCESS_COARSE_LOCATION") != 0) {
        ERROR("failed to get permission ACCESS_COARSE_LOCATION\n");
    }
//  xxx needed?
//  if (sdlx_get_permission("android.permission.ACCESS_FINE_LOCATION") != 0) {
//      ERROR("failed to get permission ACCESS_FINE_LOCATION\n");
//  }
    if (sdlx_get_permission("android.permission.ACTIVITY_RECOGNITION") != 0) {
        ERROR("failed to get permission ACTIVITY_RECOGNITION\n");
    }
    if (sdlx_get_permission("android.permission.RECORD_AUDIO") != 0) {
        ERROR("failed to get permission RECORD_AUDIO\n");
    }
#endif

    // init services, this will xxx
    svcs_init();

    // print type sizes
    print_type_sizes();

    // start/stop foreground mode based on the foreground_enabled param
    if (params.foreground_enabled) {
        util_start_foreground();
    } else {
        util_stop_foreground();
    }

    // success
    return 0;
}

// xxx is this ever called
static void cleanup(void)
{
    INFO("TERMINATING\n");

    svcs_stop_all();

    // xxx free svc_call allocations ?

    sdlx_quit(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);
}

#ifdef ANDROID
static void create_files(int action)
{
    switch (action) {
    case CREATE_FILES_INIT:
        system("tar -xvf files.tar");
        break;
    case CREATE_FILES_RESET:
        svcs_stop_all();
        system("rm -rf apps svcs");
        system("tar -xvf files.tar apps svcs");
        svcs_init();
        break;
    }
}
#endif

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

    //lame_test();

    while (true) {
        // clear the display, and set the font to default
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);

        // display menu, and register for events
        display_menu();

        // register for screen bottom control events
        if (LAST_PAGE > 0) {
            sdlx_register_control_events("<", ">", "X", COLOR_WHITE, BG_COLOR,
                                        EVID_PAGE_DECREMENT, EVID_PAGE_INCREMENT, EVID_MINIMIZE);
        } else {
            sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, BG_COLOR, 0, 0, EVID_MINIMIZE);
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
            break;
        } else if (event.event_id == EVID_MINIMIZE) {
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
            if (apps[id] == NULL) {
                ERROR("apps[%d] is NULL\n", id);
            } else if (strcmp(apps[id], "Settings") == 0) {
                sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);
                settings();
            } else {
                sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);
                run(apps[id], false);
            }
        }
    }
}

int run(char *name, bool is_svc)
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

static void display_menu(void)
{
    static sdlx_texture_t *circle;
    int first, last, w, h;
    sdlx_print_state_t print_state;

    #define RADIUS 100

    // allocate circle texture, which is used when displaying menu items
    if (circle == NULL) {
        circle = sdlx_create_filled_circle_texture(RADIUS, COLOR_BLUE);
    }

    // get the list of apps: 
    // - this initializes the apps[] array of  app names
    // - the dir names must be the same as the app names
    // - the apps array is indexed by the location on the display, for
    //   example idx=0 is top left, and idx=17 is bottom right
    get_list_of_apps();

    first = page * 18;
    last  = first + 17;

    if (LAST_PAGE > 0) {
        sdlx_print_save(&print_state);
        sdlx_print_init(SMALL_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Page %d", page);
        sdlx_print_restore(&print_state);
    }

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
        sdlx_query_texture(circle, &w, &h);
        sdlx_render_texture(x-RADIUS, y-RADIUS, w, h, circle);
        sdlx_print_init(numchars, COLOR_WHITE, COLOR_BLUE);
        if (s2[0] == '\0') {
            sdlx_render_text_xyctr(x, y, s1);
        } else {
            sdlx_render_text_xyctr(x, nearbyint(y-0.5*chh), s1);
            sdlx_render_text_xyctr(x, nearbyint(y+0.5*chh), s2);
        }

        // register event
        loc.x = x - RADIUS;
        loc.y = y - RADIUS;
        loc.w = 2 * RADIUS;
        loc.h = 2 * RADIUS;
        sdlx_register_event(&loc, i);
    }
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

// xxx y scrolling

static void copyright(void);
static double get_number(char *prompt, double min, double max); // xxx use in other places

static void settings(void)
{
    // record_test_state values
    #define IDLE      0
    #define RECORDING 1
    #define PLAYBACK  2

    #define RECORD_TEST_FILENAME "record_test.raw"

    #define EVID_COPYRIGHT            1001
    #define EVID_DEVEL_MODE           1002
    #define EVID_DEVEL_PORT           1003
    #define EVID_DEVEL_PASSWORD       1004
    #define EVID_SERVICES             1005
    #define EVID_RECORD_SCALE         1006
    #define EVID_RECORD_SILENCE       1007
    #define EVID_RECORD_TEST          1008
#ifdef ANDROID
    #define EVID_RESET_APPS_AND_SVCS  1020
    #define EVID_FOREGROUND           1021
#endif

    #define GET_Y ({ y = y_next; \
                     y_next += 2*sdlx_char_height; \
                     y >= y_display_begin - 30 && y <= y_display_end - sdlx_char_height; })

    bool                done = false;
    sdlx_event_t        event;
    sdlx_loc_t         *loc;
    char               *msg = NULL;
    long                msg_time = 0;
    char               *ipaddr;
    int                 record_test_state = IDLE;
    sdlx_audio_params_t ap;
    sdlx_audio_state_t  as;
    int                 y_display_begin;
    int                 y_display_end;
    double              y_top, y_next, y;

    // get this device ipaddr
    ipaddr = util_get_ipaddr();

    // init variables which define the vertical region of the display
    // being used for the filename list
    y_display_begin = ROW2Y(4.5);
    y_display_end   = sdlx_win_height - 500;
    y_top           = y_display_begin;

    // handle the setting display
    while (true) {
        // init display and display title line
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_render_text_xyctr(sdlx_win_width/2, sdlx_char_height/2, "Settings");

        // display version
        sdlx_render_printf(0, ROW2Y(1), "Version = %s", VERSION);
        sdlx_render_printf(0, ROW2Y(2), "%s", BUILD_DATE);

        // when in developer mode display ipaddr
        if (params.devel_mode) {
            sdlx_render_printf(0, ROW2Y(3), "%s:%d", ipaddr, params.devel_port);
        }

        // init print color to COLOR_LIGHT_BLUE for the following,
        // because these all are selectable
        sdlx_print_init_color(COLOR_LIGHT_BLUE, BG_COLOR);

        // init y_next; used by GET_Y to obtain the y location;
        // GET_Y will return true if the y value is in the display range
        y_next = y_top;

        // display Copyright
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Copyright");
            sdlx_register_event(loc, EVID_COPYRIGHT);
        }

        // display Devel_Mode
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Devel_Mode = %s", params.devel_mode ? "ON" : "OFF");
            sdlx_register_event(loc, EVID_DEVEL_MODE);
        }

        // display Devel_Port
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Devel_Port = %d", params.devel_port);
            sdlx_register_event(loc, EVID_DEVEL_PORT);
        }

        // display Devel_Password
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Devel_Password");
            sdlx_register_event(loc, EVID_DEVEL_PASSWORD);
        }

        // display Services
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Services");
            sdlx_register_event(loc, EVID_SERVICES);
        }

        // display Record_Scale xxx change to Gain
        if (GET_Y) {
            sdlx_audio_get_params(&ap);
            loc = sdlx_render_printf(0, y, "Record_Scale = %0.1f", ap.record_scale);
            sdlx_register_event(loc, EVID_RECORD_SCALE);
        }

        // display Record_Silence
        if (GET_Y) {
            sdlx_audio_get_params(&ap);
            loc = sdlx_render_printf(0, y, "Record_Silence = %0.0f", ap.record_silence);
            sdlx_register_event(loc, EVID_RECORD_SILENCE);
        }

        // display Record_Test
        if (GET_Y) {
            sdlx_audio_state(&as);
            if (record_test_state == IDLE) {
                loc = sdlx_render_printf(0, y, "Record_Test");
                sdlx_register_event(loc, EVID_RECORD_TEST);
            } else if (record_test_state == RECORDING) {
                int bar_value_w =  sdlx_win_width * as.volume / 100;
                int bar_height = sdlx_char_height;
                sdlx_render_printf(sdlx_win_width-COL2X(2), y, "%2d", as.volume);
                sdlx_render_fill_rect(0, y, bar_value_w, bar_height, COLOR_RED);
                sdlx_render_rect(0, y, sdlx_win_width, bar_height, 2, COLOR_WHITE);
            } else if (record_test_state == PLAYBACK) {
                int bar_value_w = (as.total_ms ? (sdlx_win_width * as.processed_ms / as.total_ms) : 0);
                int bar_height = sdlx_char_height;
                sdlx_render_fill_rect(0, y, bar_value_w, bar_height, COLOR_GREEN);
                sdlx_render_rect(0, y, sdlx_win_width, bar_height, 2, COLOR_WHITE);
            }
        }

#ifdef ANDROID
        // display Reset_Apps_And_svcs
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Reset_Apps_And_Svcs");
            sdlx_register_event(loc, EVID_RESET_APPS_AND_SVCS);
        }

        // display Foreground
        if (GET_Y) {
            loc = sdlx_render_printf(0, y, "Foreground = %s", params.foreground_enabled ? "ENABLED" : "DISABLED");
            sdlx_register_event(loc, EVID_FOREGROUND);
        }
#endif

        // change print color back to white
        sdlx_print_init_color(COLOR_WHITE, BG_COLOR);

        // Record_Test processing
        sdlx_audio_state(&as);
        if (record_test_state == RECORDING && as.state == AUDIO_STATE_IDLE) {
            sdlx_audio_play(".", RECORD_TEST_FILENAME);
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
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, BG_COLOR, 0, 0, EVID_QUIT);

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
        case EVID_RECORD_SCALE: {
            double number = get_number("Record_Scale", 1, 100);  // xxx was limitted to 10
            if (number != INVALID_NUMBER) {
                params.record_scale = number;
                util_set_numeric_param(".", "record_scale", number);
                sdlx_audio_get_params(&ap);
                ap.record_scale = number;
                sdlx_audio_set_params(&ap);
            }
            break; }
        case EVID_RECORD_SILENCE: {
            double number = get_number("Record_Silence", 0, 20);
            if (number != INVALID_NUMBER) {
                params.record_silence = number;
                util_set_numeric_param(".", "record_silence", number);
                sdlx_audio_get_params(&ap);
                ap.record_silence = number;
                sdlx_audio_set_params(&ap);
            }
            break; }
        case EVID_RECORD_TEST: {
            sdlx_audio_record(".", RECORD_TEST_FILENAME, 5, 2, false);
            record_test_state = RECORDING;
            break; }
#ifdef ANDROID
        case EVID_RESET_APPS_AND_SVCS: {
            char *str; 
            str = sdlx_get_input_str("Reset y/n", "", false, BG_COLOR);
            if (strcasecmp(str, "y") != 0) {
                break;
            }
            // xxx this did not work, maybe issue with log_fifo?
            create_files(CREATE_FILES_RESET);
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
#endif
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }
            break;
        case EVID_QUIT:
            if (record_test_state != IDLE) {
                sdlx_audio_ctl(AUDIO_REQ_STOP);
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
    char       *str;
    int         y_display_begin, y_display_end, y_top;
    int         len;
    sdlx_event_t event;
    bool        done = false;

    // read the copyright file
    str = util_read_file(".", "copyright", &len);
    if (str == NULL) {
        ERROR("failed to read copyright file\n");
        return;
    }

    // init vars
    y_display_begin = 100;
    y_display_end = sdlx_win_height - 200;
    y_top = y_display_begin;

    // display copyright, support motion (for scrolling)
    while (true) {
        // display copyright and register for motion (scrolling) & exit events
        sdlx_display_init(BG_COLOR);
        sdlx_print_init(SMALLEST_FONT, COLOR_WHITE, BG_COLOR);
        sdlx_register_event(NULL, EVID_MOTION);
        char *lines[1] = { str };
        sdlx_render_multiline_text(y_top, y_display_begin, y_display_end, lines, 1);
        sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, BG_COLOR, 0, 0, EVID_QUIT);
        sdlx_display_present();

        sdlx_get_event(-1, &event);
        switch (event.event_id) {
        case EVID_MOTION:
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
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
    free(str);
}

// ----------------- DEVEL MODE SERVER  ----------------

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
        if (!feof(fp)) {
            printf("ERROR: get failed - error=%d\n", ferror(fp));
        }
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
