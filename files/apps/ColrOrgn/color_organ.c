#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

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

char *progname;
char *data_dir;

char *files[MAX_FILES];
int   max_files;

sdlx_texture_t *red_circle_texture;
sdlx_texture_t *green_circle_texture;
sdlx_texture_t *blue_circle_texture;

int y_title;
int y_controls;
int y_files_list;

sdlx_audio_state_t as;
    
//
// prototypes
//

void register_events(void);
void get_file_list(void);
void display_color_organ(void);
void remove_trailing_newline(char *s);
char *state_str(void);
sdlx_texture_t *create_circle_texture(sdlx_color_t color);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    sdlx_event_t event;
    bool         end_program = false;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init y locations
    y_title = 950;
    y_controls = y_title + 1.5*sdlx_char_height_dflt;
    y_files_list = y_controls + 1.5*sdlx_char_height_dflt;

    // init color organ red,green,blue circle textures
    red_circle_texture   = create_circle_texture(COLOR_RED);
    green_circle_texture = create_circle_texture(COLOR_GREEN);
    blue_circle_texture  = create_circle_texture(COLOR_BLUE);

    // runtime loop
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // get audio state xxx should return a pointer, not the copy?
        sdlx_audio_get_state(&as);

        // display title line  xxx display in red when recording
        sdlx_render_printf_ex2(sdlx_win_width/2, y_title,
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE,
                               "%s", state_str());

        // display color organ
        if (as.state != AUDIO_STATE_IDLE) {
            display_color_organ();
        }

        // display controls and register events
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
            sdlx_audio_play_file(data_dir, files[idx]);
        } else {
            switch (event.event_id) {
            // stop audio
            case EVID_STOP:
                sdlx_audio_stop();
                break;

            // record or monitor device
            case EVID_DEV:
                sdlx_audio_record_from_device(data_dir, "dev.mp3");
                break;

            // record or monitor microphone
            case EVID_MIC: {
                int  max_secs       = 0;  // no limit
                int  auto_stop_secs = 0;  // no limit
                bool append         = false;
                sdlx_audio_record_from_mic(data_dir, "mic.mp3", max_secs, auto_stop_secs, append);
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
            }
        }
    }

    // cleanup
    sdlx_audio_stop();
    sdlx_destroy_texture(red_circle_texture);
    sdlx_destroy_texture(green_circle_texture);
    sdlx_destroy_texture(blue_circle_texture);

    // terminate
    printf("I %s: terminating\n", progname);
    return 0;
}

// -----------------  EVENT REGISTRATION  ----------------------------

void register_events(void)
{
    sdlx_loc_t *loc;

    // xxx comment
    if (as.state == AUDIO_STATE_IDLE) {
        loc = sdlx_render_printf(COL2X(0), y_controls, "%s", "DEVICE");
        sdlx_register_event(loc, EVID_DEV);
        loc = sdlx_render_printf(COL2X(9), y_controls, "%s", "MICROPHONE");
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

    // xxx comment
    if (as.state == AUDIO_STATE_IDLE) {
        int y = y_files_list;
        get_file_list();
        for (int i = 0; i < max_files; i++) {
            loc = sdlx_render_printf(0, y, "%s", files[i]);
            sdlx_register_event(loc, EVID_PLAY_FILE+i);
            y += 1.5*sdlx_char_height_dflt;
        }
    }

    // register control event to end program
    sdlx_register_control_events(0, NULL,
                                 0, NULL,
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
}

void get_file_list(void)
{
    static long saved_mtime;

    // if data_dir has been modified then
    // update list of mp3 and wav files
    long mtime = util_file_mtime(data_dir, NULL);
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
        char s[200], cmd[200];
        FILE *fp;
        sprintf(cmd, "/bin/ls -1 %s/*.wav %s/*.mp3", data_dir, data_dir);
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

// -----------------  DISPLAY COLOR ORGAN  ---------------------------

#define X_RED_CTR    250
#define Y_RED_CTR    683
#define X_GREEN_CTR  750
#define Y_GREEN_CTR  683
#define X_BLUE_CTR   500
#define Y_BLUE_CTR   250


#define DECAY_RATE 0.01

#define SFL 3.5
#define SFM 3.5
#define SFH 5.0

// xxx improve this
void display_color_organ(void)
{
    int x, y, wh=500;
    
    //printf("%s  %f %f %f\n", 
        //as.pathname,
        //as.color_organ.low_band,
        //as.color_organ.mid_band,
        //as.color_organ.high_band);

    //static double scaled_low_band;
    //scale(as.color_organ.low_band, &scaled_low_band);
    //printf("current/target = %f %f\n", as.color_organ.low_band, scaled_low_band);

    double scaled_low_band;
    static double current_low_band;
    if (as.color_organ.low_band > current_low_band) {
        current_low_band = as.color_organ.low_band;
    } else {
        current_low_band -= DECAY_RATE;
        if (current_low_band < 0) current_low_band = 0;
    }
    scaled_low_band = current_low_band * SFL;
    if (scaled_low_band > 1) scaled_low_band = 1;

    double scaled_mid_band;
    static double current_mid_band;
    if (as.color_organ.mid_band > current_mid_band) {
        current_mid_band = as.color_organ.mid_band;
    } else {
        current_mid_band -= DECAY_RATE;
        if (current_mid_band < 0) current_mid_band = 0;
    }
    scaled_mid_band = current_mid_band * SFM;
    if (scaled_mid_band > 1) scaled_mid_band = 1;

    // ------------------
    double scaled_high_band;
    static double current_high_band;
    if (as.color_organ.high_band > current_high_band) {
        current_high_band = as.color_organ.high_band;
    } else {
        current_high_band -= DECAY_RATE;
        if (current_high_band < 0) current_high_band = 0;
    }
    //scaled_high_band = 0.72 * log(current_high_band) + 2.16;
    scaled_high_band = 0.333 * log(current_high_band) + 1.30;
    if (scaled_high_band > 1) scaled_high_band = 1;
    if (scaled_high_band < 0) scaled_high_band = 0;
    printf("%f\n", scaled_high_band);

    x = X_RED_CTR - wh/2;
    y = Y_RED_CTR - wh/2;
    sdlx_color_mod_texture(red_circle_texture, scaled_low_band, 0, 0);
    sdlx_render_texture_ex1(red_circle_texture, x, y, wh, wh);

    x = X_GREEN_CTR - wh/2;
    y = Y_GREEN_CTR - wh/2;
    sdlx_color_mod_texture(green_circle_texture, 0, scaled_mid_band, 0);
    sdlx_render_texture_ex1(green_circle_texture, x, y, wh, wh);

    x = X_BLUE_CTR - wh/2;
    y = Y_BLUE_CTR - wh/2;
    sdlx_color_mod_texture(blue_circle_texture, 0, 0, scaled_high_band);
    sdlx_render_texture_ex1(blue_circle_texture, x, y, wh, wh);
#if 0
    wh = as.color_organ.low_band * 500;
    //wh = 500;  xxx del
    x = X_RED_CTR - wh/2;
    y = Y_RED_CTR - wh/2;
    if (wh > 500) wh = 500;
    sdlx_render_texture_ex1(red_circle_texture, x, y, wh, wh);

    wh = as.color_organ.mid_band * 500;
    //wh = 500;  xxx del
    x = X_GREEN_CTR - wh/2;
    y = Y_GREEN_CTR - wh/2;
    if (wh > 500) wh = 500;
    sdlx_render_texture_ex1(green_circle_texture, x, y, wh, wh);

    wh = as.color_organ.high_band * 500;
    //wh = 500;  xxx del
    x = X_BLUE_CTR - wh/2;
    y = Y_BLUE_CTR - wh/2;
    if (wh > 500) wh = 500;
    sdlx_render_texture_ex1(blue_circle_texture, x, y, wh, wh);
#endif
}

// -----------------  UTILS  -----------------------------------------

char *state_str(void)
{
    static char s[200];
    char buffer[100], *short_fn, *p;

    switch (as.state) {
    case AUDIO_STATE_IDLE:
        return "Stopped";
    case AUDIO_STATE_PLAY_FILE:
        strcpy(buffer, as.pathname);
        short_fn = basename(buffer);
        if ((p = strstr(short_fn, ".mp3")) != NULL) *p = '\0';
        if ((p = strstr(short_fn, ".wav")) != NULL) *p = '\0';
        if (!as.paused) {
            sprintf(s, "Playing %s", short_fn);
        } else {
            sprintf(s, "Paused %s", short_fn);
        }
        return s;
    case AUDIO_STATE_RECORD_FROM_MIC:
        if (!as.paused) {
            return "Recording MIC";
        } else {
            return "Monitoring MIC";
        }
        break;
    case AUDIO_STATE_RECORD_FROM_DEVICE:
        if (!as.paused) {
            return "Recording DEV";
        } else {
            return "Monitoring DEV";
        }
        break;
    default:
        return "INVALID STATE";
    }
}

sdlx_texture_t *create_circle_texture(sdlx_color_t color)
{
    sdlx_texture_t *t;

    t = sdlx_create_texture(200,200);
    sdlx_set_render_target(t);
    sdlx_render_fill_circle(100, 100, 100, color);
    sdlx_set_render_target(NULL);

    return t;
}

void remove_trailing_newline(char *s)
{
    int len = strlen(s);

    if (len > 0 && s[len-1] == '\n') {
        s[len-1] = '\0';
    }
}
