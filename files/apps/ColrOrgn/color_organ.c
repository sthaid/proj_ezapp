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

void color_organ_display(sdlx_audio_state_t *as);
void color_organ_init(void);
void color_organ_cleanup(void);
void color_organ_register_events(void);
void color_organ_process_event(sdlx_event_t *ev);

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

    // xxx
    color_organ_init();

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

    // xxx comment
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

    // xxx comment
    color_organ_register_events();

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
// xxx new file

#define EVID_CO_RED_INCREASE    1000
#define EVID_CO_RED_DECREASE    1001
#define EVID_CO_GREEN_INCREASE  1002
#define EVID_CO_GREEN_DECREASE  1003
#define EVID_CO_BLUE_INCREASE   1004
#define EVID_CO_BLUE_DECREASE   1005
#define EVID_CO_RESET           1006
#define EVID_CO_PARAMS          1007

#define X_RED_CIRCLE    250
#define Y_RED_CIRCLE    683
#define X_GREEN_CIRCLE  750
#define Y_GREEN_CIRCLE  683
#define X_BLUE_CIRCLE   500
#define Y_BLUE_CIRCLE   250

#define DECAY  0.03

#define DEFAULT_K_RED    30
#define DEFAULT_K_GREEN  50
#define DEFAULT_K_BLUE   70
#define DELTA_K          5
#define MIN_K            0
#define MAX_K            100
#define DISP_K_DURATION  100   // cycles

int k_red   = DEFAULT_K_RED;
int k_green = DEFAULT_K_GREEN;
int k_blue  = DEFAULT_K_BLUE;
int disp_k  = 0;

// -------------------------------------------------------------------

void color_organ_init(void)
{
    // init color organ red,green,blue circle textures
    red_circle_texture   = create_circle_texture(COLOR_RED);
    green_circle_texture = create_circle_texture(COLOR_GREEN);
    blue_circle_texture  = create_circle_texture(COLOR_BLUE);

    k_red = util_get_numeric_param(data_dir, "k_red", DEFAULT_K_RED);
    k_green = util_get_numeric_param(data_dir, "k_green", DEFAULT_K_GREEN);
    k_blue = util_get_numeric_param(data_dir, "k_blue", DEFAULT_K_BLUE);
}

void color_organ_cleanup(void)
{
    // destroy color organ textures
    sdlx_destroy_texture(red_circle_texture);
    sdlx_destroy_texture(green_circle_texture);
    sdlx_destroy_texture(blue_circle_texture);
}

// -------------------------------------------------------------------

void proc(sdlx_texture_t *t, int x_ctr, int y_ctr, double newval, double *work, int k);

void color_organ_display(sdlx_audio_state_t *as)
{
    static double work_red, work_green, work_blue;
    static int    audio_state_last = AUDIO_STATE_IDLE;
    static bool   audio_paused_last = false;
    static bool   first_call = true;

    if (as->state != AUDIO_STATE_IDLE) {
        proc(red_circle_texture, X_RED_CIRCLE, Y_RED_CIRCLE, as->color_organ.low_band, &work_red, k_red);
        proc(green_circle_texture, X_GREEN_CIRCLE, Y_GREEN_CIRCLE, as->color_organ.mid_band, &work_green, k_green);
        proc(blue_circle_texture, X_BLUE_CIRCLE, Y_BLUE_CIRCLE, as->color_organ.high_band, &work_blue, k_blue);
    }

    if ((as->state != AUDIO_STATE_IDLE) && 
        (!as->paused) &&
        (as->state != audio_state_last || as->paused != audio_paused_last))
    {
        disp_k = DISP_K_DURATION;
    }
    if (first_call) {
        disp_k = DISP_K_DURATION;
    }
    audio_state_last = as->state;
    audio_paused_last = as->paused;
    first_call = false;
    
    if (disp_k > 0) {
        sdlx_render_printf_ex2(X_RED_CIRCLE, Y_RED_CIRCLE,
                               FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                               "%d", k_red);
        sdlx_render_printf_ex2(X_GREEN_CIRCLE, Y_GREEN_CIRCLE,
                               FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                               "%d", k_green);
        sdlx_render_printf_ex2(X_BLUE_CIRCLE, Y_BLUE_CIRCLE,
                               FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                               "%d", k_blue);
        disp_k = disp_k - 1;
    }
}

void proc(sdlx_texture_t *t, int x_ctr, int y_ctr, double newval, double *work, int k)
{
    double current, intensity;

    current = *work;
    if (newval > current) {
        current = newval;
    } else {
        current -= DECAY;
        if (current < 0) current = 0;
    }
    *work = current;

    intensity = (k / 10.0) * current;
    if (intensity > 1) intensity = 1;
    if (intensity < 0) intensity = 0;

    sdlx_color_mod_texture(t, intensity, intensity, intensity);
    sdlx_render_texture_ex1(t, x_ctr-250, y_ctr-250, 500, 500);
}

// -------------------------------------------------------------------

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);

void color_organ_register_events(void)
{
    sdlx_loc_t loc;
    sdlx_loc_t *locp;

    init_loc(&loc, X_RED_CIRCLE-125, Y_RED_CIRCLE-250, 250, 250);
    sdlx_register_event(&loc, EVID_CO_RED_INCREASE);
    init_loc(&loc, X_RED_CIRCLE-125, Y_RED_CIRCLE, 250, 250);
    sdlx_register_event(&loc, EVID_CO_RED_DECREASE);

    init_loc(&loc, X_GREEN_CIRCLE-125, Y_GREEN_CIRCLE-250, 250, 250);
    sdlx_register_event(&loc, EVID_CO_GREEN_INCREASE);
    init_loc(&loc, X_GREEN_CIRCLE-125, Y_GREEN_CIRCLE, 250, 250);
    sdlx_register_event(&loc, EVID_CO_GREEN_DECREASE);

    init_loc(&loc, X_BLUE_CIRCLE-125, Y_BLUE_CIRCLE-250, 250, 250);
    sdlx_register_event(&loc, EVID_CO_BLUE_INCREASE);
    init_loc(&loc, X_BLUE_CIRCLE-125, Y_BLUE_CIRCLE, 250, 250);
    sdlx_register_event(&loc, EVID_CO_BLUE_DECREASE);

    locp = sdlx_render_printf(sdlx_win_width-COL2X(3), ROW2Y(0.5), "%s", "RST");
    sdlx_register_event(locp, EVID_CO_RESET);

    locp = sdlx_render_printf(sdlx_win_width-COL2X(3), ROW2Y(2.5), "%s", "PRM");
    sdlx_register_event(locp, EVID_CO_PARAMS);
}

void color_organ_process_event(sdlx_event_t *ev)
{
    // xxx only if active ?

    switch (ev->event_id) {
    case EVID_CO_RED_INCREASE:
    case EVID_CO_RED_DECREASE:
        k_red += (ev->event_id == EVID_CO_RED_INCREASE ? DELTA_K : - DELTA_K);
        if (k_red > MAX_K) k_red = MAX_K;
        if (k_red < MIN_K) k_red = MIN_K;
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "k_red", k_red);
        break;
    case EVID_CO_GREEN_INCREASE:
    case EVID_CO_GREEN_DECREASE:
        k_green += (ev->event_id == EVID_CO_GREEN_INCREASE ? DELTA_K : - DELTA_K);
        if (k_green > MAX_K) k_green = MAX_K;
        if (k_green < MIN_K) k_green = MIN_K;
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "k_green", k_green);
        break;
    case EVID_CO_BLUE_INCREASE:
    case EVID_CO_BLUE_DECREASE:
        k_blue += (ev->event_id == EVID_CO_BLUE_INCREASE ? DELTA_K : - DELTA_K);
        if (k_blue > MAX_K) k_blue = MAX_K;
        if (k_blue < MIN_K) k_blue = MIN_K;
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "k_blue", k_blue);
        break;
    case EVID_CO_RESET:
        k_red   = DEFAULT_K_RED;
        k_green = DEFAULT_K_GREEN;
        k_blue  = DEFAULT_K_BLUE;
        disp_k  = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "k_red", k_red);
        util_set_numeric_param(data_dir, "k_green", k_green);
        util_set_numeric_param(data_dir, "k_blue", k_blue);
        break;
    case EVID_CO_PARAMS:
        disp_k  = DISP_K_DURATION;
        break;
    }
}

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h)
{
    loc->x = x;
    loc->y = y;
    loc->w = w;
    loc->h = h;
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
