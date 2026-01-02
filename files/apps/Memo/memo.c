#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <sdlx.h>
#include <utils.h>

// xxx todo
// - adjust record volume
// - make pixel_t
// - record auto stop prblem
// - settings:
//   - setting for record volume scaling
//   - setting for minimum volume
// - record speed ?

// - xxx maybe not

//
// defines
//

#define EVID_NEW      1
#define EVID_STOP     2
#define EVID_GOTO_TOP 3
#define EVID_PLAY     100
#define EVID_APPEND   200
#define EVID_DELETE   300

#define MAX_FILENAME 100

//
// variables
//

char *progname;
char *data_dir;

char *filename[MAX_FILENAME];
char *friendlyname[MAX_FILENAME];
int   max_filename;

//
// prototypes
//

void get_list_of_files(void);
void cleanup_filename_allocations(void);
void remove_trailing_newline(char *s);
void substring(char *s, int start, int len, char *substring);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int                rc;
    sdlx_event_t       event;
    bool               end_program = false;
    int                y_display_begin;
    int                y_display_end;
    double             y_top;
    int                y;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // xxx picoc test
    rc = COLOR_RED;
    if (rc == COLOR_RED) {
        printf("INFO %s: picoc test okay\n", progname);
    } else {
        printf("ERROR %s: picoc test failed, rc=0x%x\n", progname, rc);
    }

    // init sdl video and audio subsystems
    rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_AUDIO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // init variables which define the vertical region of the display
    // being used for the filename list
    y_display_begin = 100;
    y_display_end   = sdlx_win_height - 500;
    y_top           = y_display_begin;

    // init char size, in numchars across the screen
    sdlx_print_init_numchars(23);

    // runtime loop
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get list of audio files
        get_list_of_files();

        // get current audio state
        sdlx_audio_state_t audio_state;
        sdlx_audio_state(&audio_state);

        // display the audio filename, followed by events to append, or delete
        for (int idx = 0; idx < max_filename; idx++) {
            sdlx_loc_t  *loc;
            unsigned int color;
            int          file_duration_secs;

            // if y display location is above the top of the display region,
            // or below the bottom of the display region, then either continue or break
            y = y_top + ROW2Y(2*idx);
            if (y < y_display_begin-30) continue;
            if (y > y_display_end) break;

            // determine the color in which the filename[idx] will be displayed
            if (strstr(audio_state.filename, filename[idx]) != NULL) {
                color = (audio_state.state == AUDIO_STATE_PLAY_FILE     ? COLOR_GREEN :   // xxx picoc problem
                        (audio_state.state == AUDIO_STATE_RECORD        ? COLOR_RED :
                        (audio_state.state == AUDIO_STATE_RECORD_APPEND ? COLOR_RED :
                                                                          COLOR_LIGHT_BLUE)));
            } else {
                color = COLOR_LIGHT_BLUE;
            }

            // display the friendly filename and duration in the color determined above
            sdlx_print_init_color(color, COLOR_BLACK);
            file_duration_secs = sdlx_audio_file_duration(data_dir, filename[idx]);
            loc = sdlx_render_printf(0, y, "%s-%02d", friendlyname[idx], file_duration_secs);
            if (color == COLOR_LIGHT_BLUE || color == COLOR_GREEN) {
                sdlx_register_event(loc, EVID_PLAY+idx);
            }

            // if color is light blue then
            //   register append and delete events
            // else
            //   register stop event, to stop the inprogress playback or record
            // endif
            if (color == COLOR_LIGHT_BLUE) {
                sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                loc = sdlx_render_printf(COL2X(16.5), y, "%s", "+");
                sdlx_register_event(loc, EVID_APPEND+idx);
                loc = sdlx_render_printf(COL2X(21), y, "%s", "X");
                sdlx_register_event(loc, EVID_DELETE+idx);
            } else {
                sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                loc = sdlx_render_printf(COL2X(15.5), y, "%s", "STOP");
                sdlx_register_event(loc, EVID_STOP);
            }
        }

        // display bar which indicates:
        // - playback: the amount of the file that has played
        // - record: the record volume
        y = sdlx_win_height-300;
        int bar_height = 75;
        if (audio_state.state == AUDIO_STATE_PLAY_FILE) {
            int bar_value_w = sdlx_win_width * audio_state.processed_ms / audio_state.total_ms;
            sdlx_render_fill_rect(0, y, bar_value_w, bar_height, COLOR_GREEN);
            sdlx_render_rect(0, y, sdlx_win_width, bar_height, 2, COLOR_WHITE);
        } else if (audio_state.state == AUDIO_STATE_RECORD || audio_state.state == AUDIO_STATE_RECORD_APPEND) {
            int bar_value_w =  sdlx_win_width * audio_state.volume / 100;
            sdlx_render_printf(sdlx_win_width-COL2X(2), y, "%2d", audio_state.volume);
            sdlx_render_fill_rect(0, y, bar_value_w, bar_height, COLOR_RED);
            sdlx_render_rect(0, y, sdlx_win_width, bar_height, 2, COLOR_WHITE);
        }

        // register motion and control events
        sdlx_register_event(NULL, EVID_MOTION);
        sdlx_register_control_events("+", "TOP", "X", COLOR_WHITE, COLOR_BLACK, EVID_NEW, EVID_GOTO_TOP, EVID_QUIT);

        // present the display
        sdlx_display_present();

        // wait for event, with 100ms timeout
        sdlx_get_event(100000, &event);

        // process events
        if (event.event_id == -1) {
            // no event received, must be timeout
        } else if (event.event_id == EVID_MOTION) {
            y_top += event.u.motion.yrel;
            if (y_top >= y_display_begin) {
                y_top = y_display_begin;
            }
        } else if (event.event_id == EVID_GOTO_TOP) {
            y_top = y_display_begin;
        } else if (event.event_id == EVID_NEW) {
            time_t t = time(NULL);
            struct tm tm;
            char new_filename[100];

            localtime_r(&t, &tm);
            sprintf(new_filename, "%04d%02d%02d%02d%02d%02d.raw",
                    tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            //printf("INFO %s: EVID_NEW recording to '%s'\n", progname, new_filename);
            sdlx_audio_record(data_dir, new_filename, 30, 2, false);
        } else if (event.event_id == EVID_STOP) {
            //printf("INFO %s: EVID_STOP\n", progname);
            sdlx_audio_ctl(AUDIO_REQ_STOP);
        } else if (event.event_id >= EVID_PLAY && event.event_id < EVID_PLAY + max_filename) {
            int idx = event.event_id - EVID_PLAY;
            //printf("INFO %s: EVID_PLAY %d\n", progname, idx);
            sdlx_audio_play(data_dir, filename[idx]);
        } else if (event.event_id >= EVID_APPEND && event.event_id < EVID_APPEND + max_filename) {
            int idx = event.event_id - EVID_APPEND;
            //printf("INFO %s: EVID_APPEND %d\n", progname, idx);
            sdlx_audio_record(data_dir, filename[idx], 30, 2, true);
        } else if (event.event_id >= EVID_DELETE && event.event_id < EVID_DELETE + max_filename) {
            int idx = event.event_id - EVID_DELETE;
            //printf("INFO %s: EVID_DELETE %d\n", progname, idx);
            util_delete_file(data_dir, filename[idx]);
        } else if (event.event_id == EVID_QUIT) {
            end_program = true;
        }
    }

    // cleanup and end program
    cleanup_filename_allocations();
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_AUDIO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

void get_list_of_files(void)
{
    FILE *fp;
    int   i;
    char  cmd[100], s[100];
    long  mtime;

    static long mtime_last;

    // return if no change
    mtime = util_file_mtime(data_dir, NULL);
    if (mtime == mtime_last) {
        return;
    }
    mtime_last = mtime;

    // debug print
    printf("INFO %s: updating filenames\n", progname);

    // cleanup current filename string allocations
    cleanup_filename_allocations();

    // run 'ls -lr' to get reverse sorted list of filenames,
    // starting with the most recent
    sprintf(cmd, "cd %s; /bin/ls -1r *.raw", data_dir);
    fp = popen(cmd, "r");
    while (fgets(s, sizeof(s), fp)) {
        remove_trailing_newline(s);
        filename[max_filename++] = strdup(s);
    }
    pclose(fp);

    // create friendly filenames, for example:
    // - filename:    20251219071933.raw
    // - friendlyname: Dec19-07:19
    for (i = 0; i < max_filename; i++) {
        char month[8], day[8], hour[8], minute[8], month_abbrev[16];
        char friendly[50];
        struct tm tm;

        substring(filename[i], 4, 2, month);
        substring(filename[i], 6, 2, day);
        substring(filename[i], 8, 2, hour);
        substring(filename[i], 10, 2, minute);
        memset(&tm, 0, sizeof(tm));
        tm.tm_mon = atoi(month)-1;
        strftime(month_abbrev, sizeof(month_abbrev), "%b", &tm);
        sprintf(friendly, "%s%s-%s%s", month_abbrev, day, hour, minute);
        friendlyname[i] = strdup(friendly);
    }

    // print new list of filenames and friendlynames
    for (i = 0; i < max_filename; i++) {
        printf("INFO %s:   %d '%s' '%s'\n", progname, i, filename[i], friendlyname[i]);
    }
}

void cleanup_filename_allocations(void)
{
    int i;

    for (i = 0; i < max_filename; i++) {
        free(filename[i]);
        filename[i] = NULL;
        free(friendlyname[i]);
        friendlyname[i] = NULL;
    }
    max_filename = 0;
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);   

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';   
    }
}

void substring(char *s, int start, int len, char *substring)
{
    memcpy(substring, s+start, len);
    substring[len] = '\0';
}
