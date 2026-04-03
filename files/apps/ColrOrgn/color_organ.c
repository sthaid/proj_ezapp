#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// xxx at end of playing the state does nt xfer to stopped

// yyy new
// - use the fps info
// - Settings
//   - print duration of each cycle  ???
// - option to delete and rename files

// yyy next
// - ctrls to rename and delete files
// - negative rgb_k
// - print the duration of each cycle, and set event wait time to allow for the duration  INPROG

// yyy todo
// - call to get_samples should return fps and possibly num_channels, and 
//   caller should deal with the fps
// - circles mode multiplier constant too large?
// - add params for decay and exp smooth

//
// defines
//

// bands
#define LOW_BAND    0   // red
#define MID_BAND    1   // green
#define HIGH_BAND   2   // blue

// events
#define EVID_FILTER_SLCT      1001
#define EVID_COLOR_ORGAN_SLCT 1002
#define EVID_RED_INCREASE     1010
#define EVID_RED_DECREASE     1011
#define EVID_GREEN_INCREASE   1012
#define EVID_GREEN_DECREASE   1013
#define EVID_BLUE_INCREASE    1014
#define EVID_BLUE_DECREASE    1015

// filters used to reduce color organ jitter
#define MAX_FILTER         3
#define FILTER_NONE        0
#define FILTER_EXP_SMOOTH  1
#define FILTER_SNAP        2

// color organ display formats
#define MAX_COLOR_ORGAN      2
#define COLOR_ORGAN_BARS     0
#define COLOR_ORGAN_CIRCLES  1

// misc
#define DISP_BAND_SCALE_DURATION 30
#define COH                      COLOR_ORGAN_H  // abbreviation

// default param values
#define DFLT_COLOR_ORGAN       COLOR_ORGAN_BARS
#define DFLT_FILTER            FILTER_EXP_SMOOTH
#define DFLT_LOW_BAND_GAIN     10
#define DFLT_MID_BAND_GAIN     15
#define DFLT_HIGH_BAND_GAIN    25
#define DFLT_EXP_FILTER_K      0.6
#define DFLT_SNAP_FILTER_K     0.08

//
// variables
//

// params
int   which_color_organ;
int   which_filter;
int   band_gain[3];
float exp_filter_k;
float snap_filter_k;  // xxx rename back to _decay

// names
char *color_organ_name[MAX_COLOR_ORGAN] = {"BARS", "CIRC"};
char *filter_name[MAX_FILTER] = {"NONE", "EXP", "SNAP"};

// circles used in COLOR_ORGAN_CIRCLES mode
int circle_radius;
sdlx_texture_t *red_circle_texture;
sdlx_texture_t *green_circle_texture;
sdlx_texture_t *blue_circle_texture;

// texture used to display color organ in either vertical or horizontal orientation
sdlx_texture_t *color_organ_texture;

// used to display band scaling values for a short time interval
int disp_band_scale;

//
// prototypes
//

// settings
void settings_init(void);
void settings_reset_to_dflt(void);
void settings(void);

// utils
sdlx_texture_t *create_circle_texture(sdlx_color_t color);
void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);

// -----------------  INIT & CLEANUP  --------------------------------

void color_organ_init(void)
{
    settings_init();

    // create textures used by COLOR_ORGAN_CIRCLES
    red_circle_texture   = create_circle_texture(COLOR_RED);
    green_circle_texture = create_circle_texture(COLOR_GREEN);
    blue_circle_texture  = create_circle_texture(COLOR_BLUE);
    circle_radius = COH / (2 + sqrt(3));

    // create texture used to display the color organ either vertical or horizontal
    color_organ_texture = sdlx_create_texture(1000, COH);
}

void color_organ_cleanup(void)
{
    // destroy textures
    sdlx_destroy_texture(red_circle_texture);
    sdlx_destroy_texture(green_circle_texture);
    sdlx_destroy_texture(blue_circle_texture);
    sdlx_destroy_texture(color_organ_texture);
}

// -----------------  COLOR ORGAN DISPLAY  ---------------------------

void display_band(int which_band, float band_volume);

void color_organ_display(bool idle)
{
    // yyy FRAMES_PER_SEC?
    // yyy probably dont return fps when getting samples

    int    num_downsample = 4;
    int    num_samples = nearbyint(FRAMES_PER_SEC / num_downsample * 0.050);  // equals 600
    float  delta_f = ((double)FRAMES_PER_SEC/num_downsample) / num_samples;   // equals 20
    float  samples[600], fft[301];
    int    first_bin, last_bin;
    float  low_band, mid_band, high_band;

    // render to color_organ_texture
    sdlx_set_render_target(color_organ_texture);
    sdlx_clear_texture(color_organ_texture, COLOR_BLACK);

    // if not idle then
    //   get band volumes from fft of audio samples
    // else
    //   set band volumes to zero
    // endif
    if (!idle) {
        // get audio samples and perform fft
        sdlx_get_audio_samples(num_samples, num_downsample, GET_SAMPLES_MONO, samples);
        util_fft_real_to_real(num_samples, samples, fft, true);

        // calculate the band volume for each of the 3 bands
        first_bin = nearbyint(LOW_BAND_START/delta_f);
        last_bin = nearbyint(LOW_BAND_END/delta_f);
        low_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// yyy picoc isue requires &fft[first_bin]

        first_bin = nearbyint(MID_BAND_START/delta_f);
        last_bin = nearbyint(MID_BAND_END/delta_f);
        mid_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);

        first_bin = nearbyint(HIGH_BAND_START/delta_f);
        last_bin = nearbyint(HIGH_BAND_END/delta_f);
        high_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);
    } else {
        low_band = 0;
        mid_band = 0;
        high_band = 0;
    }

    // render the band volume for the 3 bands
    display_band(LOW_BAND, low_band);
    display_band(MID_BAND, mid_band);
    display_band(HIGH_BAND, high_band);

    // restore render target to the display
    sdlx_set_render_target(NULL);

    // copy the color_organ_texture to the display, based on the caller provided display orientation
    if (orientation == VERTICAL) {
        sdlx_render_texture(color_organ_texture, 0, 0);
    } else {
        // yyy set scale incorporating the win height
        float scale = 1000.0 / COH;
        int new_w = nearbyint(1000 * scale);
        int new_h = nearbyint(COH * scale);
        // yyy comment this
        // yyy was 0.55
        sdlx_render_texture_ex3(color_organ_texture, 
                                0, 0.5*sdlx_win_height - new_w/2, new_w, new_h,
                                90, new_h/2, new_h/2);
    }

    // decrement disp_band_scale, this will stop the display of the band scaling constants
    // after a short interval
    if (disp_band_scale > 0) disp_band_scale--;
}

void display_band(int which_band, float band_volume)
{
    static float smoothed_save[3];
    float        scaled, smoothed;

    scaled = band_volume * band_gain[which_band];
    if (scaled > 1) scaled = 1;

    smoothed = smoothed_save[which_band];
    if (which_filter == FILTER_NONE) {
        smoothed = scaled;
    } else if (which_filter == FILTER_EXP_SMOOTH) {
        smoothed = smoothed + exp_filter_k * (scaled - smoothed);
    } else if (which_filter == FILTER_SNAP) {
        if (scaled > smoothed) {
            smoothed = scaled;
        } else {
            smoothed -= snap_filter_k;
            if (smoothed < scaled) smoothed = scaled;
        }
    } else {
        smoothed = 0;
    }
    smoothed_save[which_band] = smoothed;

    if (which_color_organ == COLOR_ORGAN_BARS) {
        int          x = which_band * 333;
        int          h = COH*smoothed;
        sdlx_color_t color;

        color = (which_band == LOW_BAND ? COLOR_RED : (which_band == MID_BAND ? COLOR_GREEN : COLOR_BLUE));
        sdlx_render_fill_rect(x, COH-h, 333, h, color);

        if (show_params || disp_band_scale > 0) {
            sdlx_render_printf_ex2(x+333/2, COH/2, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", band_gain[which_band]); 
        }
    } else {
        sdlx_texture_t *t;
        int             x_ctr, y_ctr;
        float           intensity;
        float           k = 2.0;

        intensity = smoothed * k;
        if (which_band == LOW_BAND) {
            t = red_circle_texture;
            x_ctr = 500 - circle_radius;
            y_ctr = COH - circle_radius;
        } else if (which_band == MID_BAND) {
            t = green_circle_texture;
            x_ctr = 500 + circle_radius;
            y_ctr = COH - circle_radius;
        } else {
            t = blue_circle_texture;
            x_ctr = 500;
            y_ctr = circle_radius;
        }
        sdlx_color_mod_texture(t, intensity, intensity, intensity);
        sdlx_render_texture_ex1(t, x_ctr-circle_radius, y_ctr-circle_radius, 2*circle_radius, 2*circle_radius);

        if (show_params || disp_band_scale > 0) {
            sdlx_render_printf_ex2(x_ctr, y_ctr, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", band_gain[which_band]);
        }
    }
}

// -----------------  COLOR ORGAN EVENT REGISTRATION  ----------------

void color_organ_register_events(int y_controls)
{
    sdlx_loc_t loc;

    if (orientation == VERTICAL) {
        reg_event(COL2X(11), y_controls, COLOR_LIGHT_BLUE, filter_name[which_filter], EVID_FILTER_SLCT);
        reg_event(COL2X(16), y_controls, COLOR_LIGHT_BLUE,
                color_organ_name[which_color_organ], EVID_COLOR_ORGAN_SLCT);
    }

    if (orientation == HORIZONTAL) return; //xxx

    if (which_color_organ == COLOR_ORGAN_BARS) {
        init_loc(&loc, 0, 0, 333, COH/2);
        sdlx_register_event(&loc, EVID_RED_INCREASE);
        init_loc(&loc, 0, COH/2, 333, COH/2);
        sdlx_register_event(&loc, EVID_RED_DECREASE);

        init_loc(&loc, 333, 0, 333, COH/2);
        sdlx_register_event(&loc, EVID_GREEN_INCREASE);
        init_loc(&loc, 333, COH/2, 333, COH/2);
        sdlx_register_event(&loc, EVID_GREEN_DECREASE);

        init_loc(&loc, 666, 0, 333, COH/2);
        sdlx_register_event(&loc, EVID_BLUE_INCREASE);
        init_loc(&loc, 666, COH/2, 333, COH/2);
        sdlx_register_event(&loc, EVID_BLUE_DECREASE);
    } else {
        int x_ctr, y_ctr;
        int r = circle_radius;

        x_ctr = 500 - r;
        y_ctr = COH - r;
        init_loc(&loc, x_ctr-r/2, y_ctr-r, r, r);
        sdlx_register_event(&loc, EVID_RED_INCREASE);
        init_loc(&loc, x_ctr-r/2, y_ctr, r, r);
        sdlx_register_event(&loc, EVID_RED_DECREASE);

        x_ctr = 500 + r;
        y_ctr = COH - r;
        init_loc(&loc, x_ctr-r/2, y_ctr-r, r, r);
        sdlx_register_event(&loc, EVID_GREEN_INCREASE);
        init_loc(&loc, x_ctr-r/2, y_ctr, r, r);
        sdlx_register_event(&loc, EVID_GREEN_DECREASE);

        x_ctr = 500;
        y_ctr = r;
        init_loc(&loc, x_ctr-r/2, y_ctr-r, r, r);
        sdlx_register_event(&loc, EVID_BLUE_INCREASE);
        init_loc(&loc, x_ctr-r/2, y_ctr, r, r);
        sdlx_register_event(&loc, EVID_BLUE_DECREASE);
    }
}

// -----------------  COLOR ORGAN EVENT HANDLER  ---------------------

void color_organ_process_event(sdlx_event_t *ev)
{
    switch (ev->event_id) {
    case EVID_FILTER_SLCT:
        which_filter = (which_filter + 1) % MAX_FILTER;
        util_set_numeric_param(data_dir, "filter", which_filter);
        break;
    case EVID_COLOR_ORGAN_SLCT:
        which_color_organ = (which_color_organ + 1) % MAX_COLOR_ORGAN;
        util_set_numeric_param(data_dir, "color_organ", which_color_organ);
        break;
    case EVID_RED_INCREASE: // yyy name
    case EVID_RED_DECREASE:
        band_gain[LOW_BAND] += (ev->event_id == EVID_RED_INCREASE ? 5 : -5);
        disp_band_scale = DISP_BAND_SCALE_DURATION;
        util_set_numeric_param(data_dir, "low_band_gain", band_gain[LOW_BAND]);
        break;
    case EVID_GREEN_INCREASE:
    case EVID_GREEN_DECREASE:
        band_gain[MID_BAND] += (ev->event_id == EVID_GREEN_INCREASE ? 5 : -5);
        disp_band_scale = DISP_BAND_SCALE_DURATION;
        util_set_numeric_param(data_dir, "mid_band_gain", band_gain[MID_BAND]);
        break;
    case EVID_BLUE_INCREASE:
    case EVID_BLUE_DECREASE:
        band_gain[HIGH_BAND] += (ev->event_id == EVID_BLUE_INCREASE ? 5 : -5);
        disp_band_scale = DISP_BAND_SCALE_DURATION;
        util_set_numeric_param(data_dir, "high_band_gain", band_gain[HIGH_BAND]);
        break;
    case EVID_SHOW_PARAMS:
        show_params = !show_params;
        break;
    case EVID_SETTINGS:
        settings();
        break;
    }
}

// -----------------  COLOR ORGAN SETTINGS  --------------------------

void settings_init(void)
{
    band_gain[LOW_BAND]  = util_get_numeric_param(data_dir, "low_band_gain",     DFLT_LOW_BAND_GAIN);
    band_gain[MID_BAND]  = util_get_numeric_param(data_dir, "mid_band_gain",     DFLT_MID_BAND_GAIN);
    band_gain[HIGH_BAND] = util_get_numeric_param(data_dir, "high_band_gain",    DFLT_HIGH_BAND_GAIN);
    which_color_organ    = util_get_numeric_param(data_dir, "which_color_organ", DFLT_COLOR_ORGAN);
    which_filter         = util_get_numeric_param(data_dir, "which_filter",      DFLT_FILTER);
    exp_filter_k         = util_get_numeric_param(data_dir, "exp_filter_k",      DFLT_EXP_FILTER_K);
    snap_filter_k        = util_get_numeric_param(data_dir, "snap_filter_k",     DFLT_SNAP_FILTER_K);
}

void settings_reset_to_dflt(void)
{
    util_set_numeric_param(data_dir, "low_band_gain",     DFLT_LOW_BAND_GAIN);
    util_set_numeric_param(data_dir, "mid_band_gain",     DFLT_MID_BAND_GAIN);
    util_set_numeric_param(data_dir, "high_band_gain",    DFLT_HIGH_BAND_GAIN);
    util_set_numeric_param(data_dir, "which_color_organ", DFLT_COLOR_ORGAN);
    util_set_numeric_param(data_dir, "which_filter",      DFLT_FILTER);
    util_set_numeric_param(data_dir, "exp_filter_k",      DFLT_EXP_FILTER_K);
    util_set_numeric_param(data_dir, "snap_filter_k",     DFLT_SNAP_FILTER_K);

    settings_init();
}

#define EVID_SETTINGS_RESET 2001
#define EVID_SETTINGS_CREATE_TEST_FILES 2002

void settings(void)
{
    bool         done = false;
    sdlx_event_t event;
    int          y;
    sdlx_loc_t  *loc;

    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // display settings
        y = 0;
        sdlx_render_printf(0, y, "%-14s=%d",    "low_band_gain",   band_gain[LOW_BAND]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-14s=%d",    "mid_band_gain",   band_gain[MID_BAND]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-14s=%d",    "high_band_gain",  band_gain[HIGH_BAND]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-14s=%s",    "color_organ",     color_organ_name[which_color_organ]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-14s=%s",    "filter",          filter_name[which_filter]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-14s=%0.3f", "exp_fltr_k",      exp_filter_k);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-14s=%0.3f", "snap_fltr_k",     snap_filter_k);
        y += 2*sdlx_char_height_dflt;

        // register events
        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "RESET");
        sdlx_register_event(loc, EVID_SETTINGS_RESET);
        y += 2*sdlx_char_height_dflt;

        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "CREATE_TEST_FILES");
        sdlx_register_event(loc, EVID_SETTINGS_CREATE_TEST_FILES);
        y += 2*sdlx_char_height_dflt;

        // register events
        sdlx_register_control_events(0, NULL,
                                     0, NULL,
                                     EVID_QUIT, "X",
                                     COLOR_WHITE, COLOR_BLACK);

        // present the display
        sdlx_display_present();

        // wait for event, with infinite timeout
        sdlx_get_event(-1, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process event
        switch (event.event_id) {
        case EVID_SETTINGS_RESET:
            printf("I %s: resetting settings to default\n", progname);
            settings_reset_to_dflt();
            break;
        case EVID_SETTINGS_CREATE_TEST_FILES:
            printf("I %s: creating test files\n", progname);
            sdlx_create_test_file(files_dir, "test_all.mp3", TEST_FILE_FREQ_SWEEP, 
                                  LOW_BAND_START, HIGH_BAND_END, 10);
            sdlx_create_test_file(files_dir, "test_low.mp3", TEST_FILE_FREQ_SWEEP,
                                  LOW_BAND_START, LOW_BAND_END, 10);
            sdlx_create_test_file(files_dir, "test_mid.mp3", TEST_FILE_FREQ_SWEEP,
                                  MID_BAND_START, MID_BAND_END, 10);
            sdlx_create_test_file(files_dir, "test_high.mp3", TEST_FILE_FREQ_SWEEP,
                                  HIGH_BAND_START, HIGH_BAND_END, 10);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }
    }
}

// -----------------  UTILS  -----------------------------------------

sdlx_texture_t *create_circle_texture(sdlx_color_t color)
{
    sdlx_texture_t *t;

    t = sdlx_create_texture(200,200);
    sdlx_set_render_target(t);
    sdlx_render_fill_circle(100, 100, 100, color);
    sdlx_set_render_target(NULL);

    return t;
}

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h)
{
    loc->x = x;
    loc->y = y;
    loc->w = w;
    loc->h = h;
}
