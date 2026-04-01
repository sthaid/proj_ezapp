#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// xxx todo
// - add events to rename and delete files
// - use provided frames_per_sec
// - review how FRAMES_PER_SEC is used

//
// defines
//

#define MAX_FILES 100

#define EVID_STOP       1          // stop 
#define EVID_DEV        2          // record or monitor device
#define EVID_MIC        3          // record or monitor microphone
#define EVID_MONITOR    4          // select monitor mode
#define EVID_REC        5          // select record mode
#define EVID_PAUSE      6          // pause play file
#define EVID_CONT       7          // continue play file
#define EVID_HORIZONTAL_OVERRIDE 8 // force horizontal orientation, when testing on linux
#define EVID_PLAY_FILE  100        // start play file, range 100-199

#define LINE_SPACING 1.85

//
// variables
//

char *files[MAX_FILES];
char *files_noext[MAX_FILES];
int   max_files;

int   y_state;
int   y_main_controls;
int   y_color_organ_controls;
int   y_files_list;
int   y_files_list_top;
int   y_files_list_bottom;

sdlx_audio_state_t as;

bool horizontal_override;

#ifdef ANDROID // yyy fix picoc to use ifdef in code
bool android = true;
#else
bool android = false;
#endif

//
// prototypes
//

void register_events(int orientation);

void get_list_of_files(void);
void remove_trailing_newline(char *s);
char *get_state_str(bool *recording);
int get_orientation(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    sdlx_event_t event;
    bool         end_program = false;
    char        *state_str;
    bool         recording;
    long         time_start=util_microsec_timer(), time_now, duration;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init files_dir
    sprintf(files_dir, "%s/files", data_dir);

    // init y locations
    y_state                = 0;
    y_main_controls        = y_state + sdlx_char_height_dflt;
    y_color_organ_controls = y_main_controls + LINE_SPACING*sdlx_char_height_dflt;
    y_files_list           = y_color_organ_controls + LINE_SPACING*sdlx_char_height_dflt;
    y_files_list_top       = y_files_list;
    y_files_list_bottom    = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT - COLOR_ORGAN_H;

    // initialize color organ
    color_organ_init();

    // runtime loop
    while (!end_program) {
        // get device orientation
        orientation = get_orientation();

        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get audio state 
        sdlx_audio_get_state(&as);

        // display state line
        state_str = get_state_str(&recording);
        print(0, y_state, (recording ? COLOR_RED : COLOR_WHITE), state_str);

        // display color organ
        color_organ_display(as.state == AUDIO_STATE_IDLE);

        // register events
        register_events(orientation);

        // present the display
        sdlx_display_present();

#if 0
        // yyy comment
        time_now = util_microsec_timer();
        duration = time_now - time_start;
        time_start = time_now;
        printf("I %s: dur %ld ms\n", progname, duration/1000);
#endif

        // wait for event, with 50 ms timeout;
        // if timedout then continue   yyy comment on 45 vs 50
        sdlx_get_event(45000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        // xxx make a routine
        if (event.event_id >= EVID_PLAY_FILE && event.event_id < EVID_PLAY_FILE+MAX_FILES) {
            // play the selected file
            int idx = event.event_id - EVID_PLAY_FILE;
            sdlx_audio_play_file(files_dir, files[idx]);
        } else {
            switch (event.event_id) {
            // stop audio
            case EVID_STOP:
                sdlx_audio_stop();
                break;

            // record or monitor device
            // yyy better way to get the name at top than using a1
            case EVID_DEV:
                // append         = false
                // start_paused   = true
                sdlx_audio_record_from_device(files_dir, "record.mp3", false, true);
                break;

            // record or monitor microphone
            // yyy better way to get the name at top than using a1
            case EVID_MIC: {
                // auto_stop_secs = 0
                // append         = false
                // start_paused   = true
                sdlx_audio_record_from_mic(files_dir, "record.mp3", 0, false, true);
                break; }

            // these apply when recording from device or microphone
            case EVID_MONITOR:
                sdlx_audio_pause();
                break;
            case EVID_REC:
                sdlx_audio_resume();
                break;

            // this apply when playing a file
            case EVID_PAUSE:
                sdlx_audio_pause();
                break;
            case EVID_CONT:
                sdlx_audio_resume();
                break;

            // end program
            case EVID_QUIT:
                end_program = true;
                break;

            // scroll file list
            case EVID_MOTION:
                if (orientation == VERTICAL) {
                    y_files_list += event.u.motion.yrel;
                } else {
                    y_files_list -= event.u.motion.xrel;
                }
                if (y_files_list >= y_files_list_top) {
                    y_files_list = y_files_list_top;
                }
                break;

            // This event is only registered when this program is being tested on linux;
            // and will toggle override of the orientation so that horizontal orientation
            // can be tested on linux. Linux does not have the acceleration sensor that is
            // available on android to detect the orientation.
            case EVID_HORIZONTAL_OVERRIDE:
                horizontal_override = !horizontal_override;
                break;

            // adjust color organ
            default:
                color_organ_process_event(&event);
                break;
            }
        }
    }

    // cleanup
    sdlx_audio_stop();
    color_organ_cleanup();

    // terminate
    printf("I %s: terminating\n", progname);
    return 0;
}

// -----------------  EVENT REGISTRATION  ----------------------------

void register_events(int orientation)
{
    sdlx_loc_t *loc;

    // register events:
    // - EVID_DEV:     start monitor/record of device audio
    // - EVID_MIC:     start monitor/record of microphone
    // - EVID_STOP:    stop playback or recording
    // - EVID_MONITOR: monitor microphone or device audio
    // - EVID_REC:     record microphone, or device audio
    // - EVID_PAUSE:   pause playback/record
    // - EVID_CONT:    continue
    if (as.state == AUDIO_STATE_IDLE) {
        reg_event(0, y_main_controls, "DEV", EVID_DEV);
        reg_event(5*sdlx_char_width_dflt, y_main_controls, "MIC", EVID_MIC);
    } else if (as.state == AUDIO_STATE_RECORD_FROM_MIC || as.state == AUDIO_STATE_RECORD_FROM_DEVICE) {
        reg_event(0, y_main_controls, "STOP", EVID_STOP);
        if (!as.paused) {
            reg_event(5*sdlx_char_width_dflt, y_main_controls, "MON", EVID_MONITOR);
        } else {
            reg_event(5*sdlx_char_width_dflt, y_main_controls, "REC", EVID_REC);
        }
    } else if (as.state == AUDIO_STATE_PLAY_FILE) {
        reg_event(0, y_main_controls, "STOP", EVID_STOP);
        if (!as.paused) {
            reg_event(5*sdlx_char_width_dflt, y_main_controls, "PAUSE", EVID_PAUSE);
        } else {
            reg_event(5*sdlx_char_width_dflt, y_main_controls, "CONT", EVID_CONT);
        }
    } else {
        printf("E %s: invalid audio state %d\n", progname, as.state);
    }

    // get list of mp3 files, and register events to play, rename, or delete each file
    get_list_of_files();
    for (int i = 0; i < max_files; i++) {
        int y = y_files_list + i * (LINE_SPACING*sdlx_char_height_dflt);
        if (y+30 < y_files_list_top) continue;
        if (y+sdlx_char_height_dflt > y_files_list_bottom) break;

        reg_event(0, y, files_noext[i], EVID_PLAY_FILE+i);

        y += LINE_SPACING*sdlx_char_height_dflt;
    }

    // register motion event, which is used to scroll the file list
    sdlx_register_event(NULL, EVID_MOTION);

    // if not running on android then provide control to simulate horizontal orientation
    if (!android) {
        int x = sdlx_win_width - 2*sdlx_char_width_dflt;
        int y = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT - 1.50*sdlx_char_height_dflt;
        loc = sdlx_render_printf_ex1(x, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", "H");
        sdlx_register_event(loc, EVID_HORIZONTAL_OVERRIDE);
    }

    // register color organ events
    color_organ_register_events(y_color_organ_controls);

    // register control events
    sdlx_register_control_events(EVID_SETTINGS, "STG",
                                 EVID_SHOW_PARAMS, (!show_params ? "SHOW" : "HIDE"),
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
}

// -----------------  UTILS  -----------------------------------------

void get_list_of_files(void)
{
    static long saved_mtime;

    char s[200], cmd[300], *bn, *p;
    FILE *fp;
    long  mtime;

    // if files_dir has been modified then
    // update list of mp3 and wav files
    mtime = util_file_mtime(files_dir, NULL);
    if (mtime != saved_mtime) {
        saved_mtime = mtime;
        printf("I %s: generate list of mp3/wav files\n", progname);

        // free existing list
        for (int i = 0; i < max_files; i++) {
            free(files[i]);
            files[i] = NULL;
            free(files_noext[i]);
            files_noext[i] = NULL;
        }
        max_files = 0;

        // make new file list ...

        // if record.mp3 exists then add it to the begining 
        if (util_file_exists(files_dir, "record.mp3")) {
            files[0] = strdup("record.mp3");
            files_noext[0] = strdup("record");
            max_files++;
        }

        // xxx comment ...
        sprintf(cmd, "/bin/ls -1 %s/*.mp3", files_dir);
        fp = popen(cmd, "r");
        while (fgets(s, sizeof(s), fp) != NULL) {
            remove_trailing_newline(s);

            bn = basename(s);

            // if bn is record.mp3 then this name has already been added,
            // so continue
            if (strcmp(bn, "record.mp3") == 0) {
                continue;
            }

            files[max_files] = strdup(bn);

            if ((p = strstr(bn, ".mp3")) != NULL) *p = '\0';
            files_noext[max_files] = strdup(bn);

            printf("I %s: files[%d] = %s\n", progname, max_files, files[max_files]);
            max_files++;
        }
        pclose(fp);
    }
}

char *get_state_str(bool *recording)
{
    static char s[200];
    char buffer[100], *short_fn, *p;

    *recording = false;

    switch (as.state) {
    case AUDIO_STATE_IDLE:
        return "Stopped";
    case AUDIO_STATE_PLAY_FILE:
        strcpy(buffer, as.pathname);
        short_fn = basename(buffer);
        if ((p = strstr(short_fn, ".mp3")) != NULL) *p = '\0';
        if (!as.paused) {
            sprintf(s, "%s", short_fn);
        } else {
            sprintf(s, "%s", short_fn);
        }
        return s;
    case AUDIO_STATE_RECORD_FROM_MIC:
        if (!as.paused) {
            *recording = true;
            return "REC MIC";
        } else {
            return "MON MIC";
        }
        break;
    case AUDIO_STATE_RECORD_FROM_DEVICE:
        if (!as.paused) {
            *recording = true;
            return "REC DEV";
        } else {
            return "MON DEV";
        }
        break;
    default:
        return "INVALID STATE";
    }
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }
}

int get_orientation(void)
{
    double ax, ay, az;
    int rc;
    static int orientation = VERTICAL;
    static bool printed;

    if (horizontal_override) {
        return HORIZONTAL;
    }

    rc = sdlx_sensor_read_accelerometer(&ax, &ay, &az);
    if (rc != 0) {
        if (!printed) {
            printf("E %s: failed to read accelerometer\n", progname);
            printed = true;
        }
        return orientation;
    }

    if (ay > 7 && orientation != VERTICAL) {
        printf("I %s: orientation is now VERTICAL\n", progname);
        orientation = VERTICAL;
    }

    if (ax > 7 && orientation != HORIZONTAL) {
        printf("I %s: orientation is now HORIZONTAL\n", progname);
        orientation = HORIZONTAL;
    }

    return orientation;
}

void reg_event(int x, int y, char *name, int event_id)
{
    sdlx_loc_t *loc;

    if (orientation == VERTICAL) {
        loc = sdlx_render_printf_ex2(x, y+COLOR_ORGAN_H,
                                     FONT_NORMAL, COLOR_LIGHT_BLUE, FLAG_NONE, WRAP_NONE,
                                     "%s", name);
    } else {
        int x1 = sdlx_win_width - sdlx_char_height_dflt - y;
        int y1 = x;
        loc = sdlx_render_printf_ex2(x1, y1,
                                     FONT_NORMAL, COLOR_LIGHT_BLUE, FLAG_ROT_90, WRAP_NONE,
                                     "%s", name);
    }
    sdlx_register_event(loc, event_id);
}

void print(int x, int y, sdlx_color_t color, char *str)
{
    if (orientation == VERTICAL) {
        sdlx_render_printf_ex2(x, y+COLOR_ORGAN_H,
                               FONT_NORMAL, color, FLAG_NONE, WRAP_NONE,
                               "%s", str);
    } else {
        int x1 = sdlx_win_width - sdlx_char_height_dflt - y;
        int y1 = x;
        sdlx_render_printf_ex2(x1, y1,
                               FONT_NORMAL, color, FLAG_ROT_90, WRAP_NONE,
                               "%s", str);
    }
}
