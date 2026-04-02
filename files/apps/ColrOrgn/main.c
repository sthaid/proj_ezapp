#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// yyy todo
// - add events to rename and delete files
// - use provided frames_per_sec
// - review how FRAMES_PER_SEC is used

//
// defines
//

#define EVID_STOP                1     // stop 
#define EVID_MON_REC             2     // monitor or record from device
#define EVID_MONITOR             3     // select monitor mode
#define EVID_RECORD              4     // select record mode
#define EVID_PAUSE               5     // pause play file
#define EVID_CONT                6     // continue play file
#define EVID_HORIZONTAL_OVERRIDE 7     // force horizontal orientation, when testing on linux
#define EVID_PLAY_FILE           100   // start play file, range 100-199
#define EVID_DELETE_FILE         200   // delete file, range 200-299

#define STATE_STOPPED               0
#define STATE_PLAYING_FILE          1
#define STATE_PLAYING_FILE_PAUSED   2
#define STATE_MONITORING_DEV        3
#define STATE_RECORDING_DEV         4

#define MAX_FILES 100

#define LINE_SPACING 1.85

#define NO_APPEND    false
#define START_PAUSED true

//
// variables
//

char *files[MAX_FILES];
char *files_noext[MAX_FILES];
int   max_files;

int   y_state;
int   y_controls;
int   y_files_list;
int   y_files_list_top;
int   y_files_list_bottom;

int   state = STATE_STOPPED;
char  playing_file[100];
bool  end_program;
bool  horizontal_override;

#ifdef ANDROID // yyy fix picoc to use ifdef in code
bool android = true;
#else
bool android = false;
#endif

//
// prototypes
//

// event handling
void process_event(sdlx_event_t *ev);
void register_events(int orientation);

// utils
void get_list_of_files(void);
void remove_trailing_newline(char *s);
char *get_state_str(void);
int get_device_orientation(void);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    sdlx_event_t event;
    long         time_start=util_microsec_timer(), time_now, duration;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init files_directory string
    sprintf(files_dir, "%s/files", data_dir);

    // init y locations
    y_state             = 0;
    y_controls          = y_state + LINE_SPACING*sdlx_char_height_dflt;
    y_files_list        = y_controls + LINE_SPACING*sdlx_char_height_dflt;
    y_files_list_top    = y_files_list;
    y_files_list_bottom = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT - COLOR_ORGAN_H;

    // initialize color organ
    color_organ_init();

    // runtime loop
    while (!end_program) {
        // get device orientation
        orientation = get_device_orientation();

        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // display state line
        print(0, y_state, (state==STATE_RECORDING_DEV ? COLOR_RED : COLOR_WHITE), get_state_str());

        // display color organ
        color_organ_display(state == STATE_STOPPED);

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
        process_event(&event);
    }

    // cleanup
    sdlx_audio_stop();
    color_organ_cleanup();
    state = STATE_STOPPED;
    playing_file[0] = '\0';

    // terminate
    printf("I %s: terminating\n", progname);
    return 0;
}

// -----------------  EVENT HANDLING  --------------------------------

void process_event(sdlx_event_t *ev)
{
    if (ev->event_id >= EVID_PLAY_FILE && ev->event_id < EVID_PLAY_FILE+MAX_FILES) {
        // play the selected file
        int idx = ev->event_id - EVID_PLAY_FILE;
        sdlx_audio_play_file(files_dir, files[idx]);
        state = STATE_PLAYING_FILE;
        strcpy(playing_file, files[idx]);
    } else if (ev->event_id >= EVID_DELETE_FILE && ev->event_id < EVID_DELETE_FILE+MAX_FILES) {
        // delete the selected file
        int idx = ev->event_id - EVID_DELETE_FILE;
        util_delete_file(files_dir, files[idx]);
    } else {
        switch (ev->event_id) {
        // stop audio
        case EVID_STOP:
            sdlx_audio_stop();
            state = STATE_STOPPED;
            playing_file[0] = '\0';

            // if recorded file exists then rename it
            if (util_file_exists(files_dir, ".record.mp3")) {
                char *input = sdlx_get_input_str("RecordedFileName", NULL, false, COLOR_BLACK);
                char  new_name[100];
                if (input[0] != '\0') {
                    sprintf(new_name, "%s.mp3", input);
                } else {
                    strcpy(new_name, "New.mp3");
                }
                util_rename_file(files_dir, ".record.mp3", files_dir, new_name);
            }
            break;

        // monitor or record device, applies when in STATE_ATOPPED
        case EVID_MON_REC:  //yyy is just monitor ?
            sdlx_audio_record_from_device(files_dir, ".record.mp3", NO_APPEND, START_PAUSED);
            state = STATE_MONITORING_DEV;
            playing_file[0] = '\0';
            break;

        // these apply when monitoring or recording the android device
        case EVID_MONITOR:
            sdlx_audio_pause();
            state = STATE_MONITORING_DEV;
            playing_file[0] = '\0';
            break;
        case EVID_RECORD:
            sdlx_audio_resume();
            state = STATE_RECORDING_DEV;
            playing_file[0] = '\0';
            break;

        // this apply when playing a file
        case EVID_PAUSE:
            sdlx_audio_pause();
            state = STATE_PLAYING_FILE_PAUSED;
            break;
        case EVID_CONT:
            sdlx_audio_resume();
            state = STATE_PLAYING_FILE;
            break;

        // end program
        case EVID_QUIT:
            end_program = true;
            break;

        // scroll file list
        case EVID_MOTION:
            if (orientation == VERTICAL) {
                y_files_list += ev->u.motion.yrel;
            } else {
                y_files_list -= ev->u.motion.xrel;
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
            color_organ_process_event(ev);
            break;
        }
    }
}

void register_events(int orientation)
{
    sdlx_loc_t *loc;

    // yyy comment
    if (state != STATE_STOPPED) {
        reg_event(0, y_controls, COLOR_LIGHT_BLUE, "STOP", EVID_STOP);
    }

    // yyy
    if (state == STATE_STOPPED) {
        reg_event(0, y_controls, COLOR_LIGHT_BLUE, "MON/REC", EVID_MON_REC);
    } else if (state == STATE_MONITORING_DEV) {
        reg_event(5*sdlx_char_width_dflt, y_controls, COLOR_LIGHT_BLUE, "REC", EVID_RECORD);
    } else if (state == STATE_RECORDING_DEV) {
        reg_event(5*sdlx_char_width_dflt, y_controls, COLOR_LIGHT_BLUE, "MON", EVID_MONITOR);
    } else if (state == STATE_PLAYING_FILE) {
        reg_event(5*sdlx_char_width_dflt, y_controls, COLOR_LIGHT_BLUE, "PAUSE", EVID_PAUSE);
    } else if (state == STATE_PLAYING_FILE_PAUSED) {
        reg_event(5*sdlx_char_width_dflt, y_controls, COLOR_LIGHT_BLUE, "CONT", EVID_CONT);
    } else {
        printf("E %s: invalid state %d\n", progname, state);
    }

    // get list of mp3 files, and register events to play, rename, or delete each file
    get_list_of_files();
    for (int i = 0; i < max_files; i++) {
        sdlx_color_t color;
        int          y;

        // handle scrolling of the files list
        y = y_files_list + i * (LINE_SPACING*sdlx_char_height_dflt);
        if (y+30 < y_files_list_top) continue;
        if (y+sdlx_char_height_dflt > y_files_list_bottom) break;

        // register event to play the file
        if (state == STATE_PLAYING_FILE && strcmp(playing_file, files[i]) == 0) {
            color = COLOR_GREEN;
        } else {
            color = COLOR_LIGHT_BLUE;
        }
        reg_event(0, y, color, files_noext[i], EVID_PLAY_FILE+i);

        // register event to delete the file;
        // - don't allow delete of file if it is being played
        // - supported only in vertical orientation
        if (color == COLOR_LIGHT_BLUE && orientation == VERTICAL) {
            reg_event(sdlx_win_width-4*sdlx_char_width_dflt, y, COLOR_LIGHT_BLUE, " DEL", EVID_DELETE_FILE+i);
        }

        // advance to next file
        y += LINE_SPACING*sdlx_char_height_dflt;
    }

    // register motion event, this is used to scroll the file list
    sdlx_register_event(NULL, EVID_MOTION);

    // if not running on android then provide control to simulate horizontal orientation
    if (!android) {
        int x = sdlx_win_width - 2*sdlx_char_width_dflt;
        int y = sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT - 1.50*sdlx_char_height_dflt;
        loc = sdlx_render_printf_ex1(x, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", "H");
        sdlx_register_event(loc, EVID_HORIZONTAL_OVERRIDE);
    }

    // register color organ events
    color_organ_register_events(y_controls);

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

        // create new list of files
        sprintf(cmd, "/bin/ls -1 %s/*.mp3", files_dir);
        fp = popen(cmd, "r");
        while (fgets(s, sizeof(s), fp) != NULL) {
            remove_trailing_newline(s);

            bn = basename(s);
            files[max_files] = strdup(bn);

            if ((p = strstr(bn, ".mp3")) != NULL) *p = '\0';
            files_noext[max_files] = strdup(bn);

            printf("I %s: files[%d] = %s\n", progname, max_files, files[max_files]);
            max_files++;
        }
        pclose(fp);
    }
}

char *get_state_str(void)
{
    switch (state) {
    case STATE_STOPPED:
        return "Stopped";
    case STATE_PLAYING_FILE:
        return "Playing";
    case STATE_PLAYING_FILE_PAUSED:
        return "Paused";
    case STATE_MONITORING_DEV:
        return "MonDev";
    case STATE_RECORDING_DEV:
        return "RecDev";
    default:
        return "???";
    }
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }
}

int get_device_orientation(void)
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

void reg_event(int x, int y, sdlx_color_t color, char *name, int event_id)
{
    sdlx_loc_t *loc;

    if (orientation == VERTICAL) {
        loc = sdlx_render_printf_ex2(x, y+COLOR_ORGAN_H,
                                     FONT_NORMAL, color, FLAG_NONE, WRAP_NONE,
                                     "%s", name);
    } else {
        int x1 = sdlx_win_width - sdlx_char_height_dflt - y;
        int y1 = x;
        loc = sdlx_render_printf_ex2(x1, y1,
                                     FONT_NORMAL, color, FLAG_ROT_90, WRAP_NONE,
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
