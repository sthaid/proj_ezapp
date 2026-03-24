#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

//
// defines
//

#define MAX_FILES 100

#define EVID_STOP       1    // stop 
#define EVID_DEV        2    // record or monitor device
#define EVID_MIC        3    // record or monitor microphone
#define EVID_MONITOR    4    // select monitor mode
#define EVID_REC        5    // select record mode
#define EVID_PAUSE      6    // pause play file
#define EVID_CONT       7    // continue play file
#define EVID_PLAY_FILE  100  // start play file, range 100-199

//
// variables
//

char *files[MAX_FILES];
int   max_files;

int y_title;
int y_controls;
int y_files_list;

sdlx_audio_state_t as;  // yyy move to common, and make a ptr

char files_dir[100];  // yyy move to common?
    
//
// prototypes
//

void register_events(void);
void get_list_of_files(void);

void remove_trailing_newline(char *s);
char *get_state_str(bool *recording);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    sdlx_event_t event;
    bool         end_program = false;
    char        *state_str;
    bool         recording;

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
    y_title = 950;
    y_controls = y_title + 1.5*sdlx_char_height_dflt;
    y_files_list = y_controls + 1.5*sdlx_char_height_dflt;

    // yyy
    color_organ_init();

    // runtime loop
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get audio state yyy should return a pointer, not the copy?
        // yyy call just once to get the ptr?
        // yyy or new api to get the fft info (a copy of it)
        sdlx_audio_get_state(&as);

        // display title line  yyy display in red when recording
        state_str = get_state_str(&recording);
        sdlx_render_printf_ex2(sdlx_win_width/2, y_title,
                               FONT_NORMAL, 
                               (recording ? COLOR_RED : COLOR_WHITE),
                               FLAG_X_CTR, WRAP_NONE,
                               "%s", state_str);

        // display color organ
        color_organ_display(&as);

        // register events
        register_events();

        // present the display
        sdlx_display_present();

        // wait for event, with 20 ms timeout;
        // if timedout then continue
        sdlx_get_event(20000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
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
            case EVID_DEV:
                // append         = false
                // start_paused   = true
                sdlx_audio_record_from_device(files_dir, "record_dev.mp3", false, true);
                break;

            // record or monitor microphone
            case EVID_MIC: {
                // auto_stop_secs = 0
                // append         = false
                // start_paused   = true
                sdlx_audio_record_from_mic(files_dir, "record_mic.mp3", 0, false, true);
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

void register_events(void)
{
    sdlx_loc_t *loc;

    // yyy comment
    if (as.state == AUDIO_STATE_IDLE) {
        loc = sdlx_render_printf(COL2X(0), y_controls, "%s", "dev");
        sdlx_register_event(loc, EVID_DEV);
        loc = sdlx_render_printf(sdlx_win_width-COL2X(3), y_controls, "%s", "mic");
        sdlx_register_event(loc, EVID_MIC);
    } else if (as.state == AUDIO_STATE_RECORD_FROM_MIC ||
               as.state == AUDIO_STATE_RECORD_FROM_DEVICE) {
        loc = sdlx_render_printf(COL2X(0), y_controls, "%s", "STOP");
        sdlx_register_event(loc, EVID_STOP);
        if (!as.paused) {
            loc = sdlx_render_printf(COL2X(10), y_controls, "%s", "MONITOR");
            sdlx_register_event(loc, EVID_MONITOR);
        } else {
            loc = sdlx_render_printf(COL2X(10), y_controls, "%s", "RECORD");
            sdlx_register_event(loc, EVID_REC);
        }
    } else if (as.state == AUDIO_STATE_PLAY_FILE) {
        loc = sdlx_render_printf(COL2X(0), y_controls, "%s", "STOP");
        sdlx_register_event(loc, EVID_STOP);
        if (!as.paused) {
            loc = sdlx_render_printf(COL2X(10), y_controls, "%s", "PAUSE");
            sdlx_register_event(loc, EVID_PAUSE);
        } else {
            loc = sdlx_render_printf(COL2X(10), y_controls, "%s", "CONT");
            sdlx_register_event(loc, EVID_CONT);
        }
    } else {
        printf("E %s: invalid audio state %d\n", progname, as.state);
    }

    // yyy comment  move to above
    if (as.state == AUDIO_STATE_IDLE) {
        int y = y_files_list;
        get_list_of_files();
        for (int i = 0; i < max_files; i++) {
            loc = sdlx_render_printf(0, y, "%s", files[i]);
            sdlx_register_event(loc, EVID_PLAY_FILE+i);
            y += 1.5*sdlx_char_height_dflt;
        }
    }

    // yyy comment
    color_organ_register_events();

    // register control event to end program
    sdlx_register_control_events(0, NULL,
                                 0, NULL,
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
}

void get_list_of_files(void)
{
    static long saved_mtime;

    // if files_dir has been modified then
    // update list of mp3 and wav files
    long mtime = util_file_mtime(files_dir, NULL);
    if (mtime != saved_mtime) {
        saved_mtime = mtime;
        printf("I %s: generate list of mp3/wav files\n", progname);

        // free existing list
        for (int i = 0; i < max_files; i++) {
            free(files[i]);
            files[i] = NULL;
        }
        max_files = 0;

        // make new list
        char s[200], cmd[300];
        FILE *fp;
        sprintf(cmd, "/bin/ls -1 %s/*.wav %s/*.mp3", files_dir, files_dir);
        fp = popen(cmd, "r");
        while (fgets(s, sizeof(s), fp) != NULL) {
            char *bn;
            remove_trailing_newline(s);
            bn = basename(s);
            printf("I %s: files[%d] = %s\n", progname, max_files, bn);
            files[max_files++] = strdup(bn);
        }
        pclose(fp);
    }
}

// -----------------  UTILS  -----------------------------------------

char *get_state_str(bool *recording)
{
    static char s[200];
    char buffer[100], *short_fn;

    *recording = false;

    switch (as.state) {
    case AUDIO_STATE_IDLE:
        return "Stopped";
    case AUDIO_STATE_PLAY_FILE:
        strcpy(buffer, as.pathname);
        short_fn = basename(buffer);
        if (!as.paused) {
            sprintf(s, "Playing %s", short_fn);
        } else {
            sprintf(s, "Paused %s", short_fn);
        }
        return s;
    case AUDIO_STATE_RECORD_FROM_MIC:
        if (!as.paused) {
            *recording = true;
            return "Recording MIC";
        } else {
            return "Monitoring MIC";
        }
        break;
    case AUDIO_STATE_RECORD_FROM_DEVICE:
        if (!as.paused) {
            *recording = true;
            return "Recording DEV";
        } else {
            return "Monitoring DEV";
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
