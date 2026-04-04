#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// xxx         
// - Settings
//   - print duration of each cycle  ???
//   - change values
// - negative rgb_k
// - circles mode multiplier constant too large?
// - add params for decay and exp smooth

//
// defines
//

// bands
#define LOW_BAND    0   // red
#define MID_BAND    1   // green
#define HIGH_BAND   2   // blue

// xxxxxxx
#define LOW_BAND_START   60
#define LOW_BAND_END     150
#define MID_BAND_START   200
#define MID_BAND_END     600
#define HIGH_BAND_START  800
#define HIGH_BAND_END    2200

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
#define MAX_COLOR_ORGAN      3
#define COLOR_ORGAN_BARS     0
#define COLOR_ORGAN_CIRCLES  1
#define COLOR_ORGAN_FFT      2

// misc
#define DISP_BAND_GAIN_DURATION 50
#define COH  COLOR_ORGAN_H  // abbreviation

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
float snap_filter_k;  // yyy rename back to _decay

// color organ band freq ranges
int band_start_freq[3] = {LOW_BAND_START, MID_BAND_START, HIGH_BAND_START};
int band_end_freq[3]   = {LOW_BAND_END, MID_BAND_END, HIGH_BAND_END};

// names
char *color_organ_name[MAX_COLOR_ORGAN] = {"BARS", "CIRC", "FFT"};
char *filter_name[MAX_FILTER] = {"NONE", "EXP", "SNAP"};

// circles used in COLOR_ORGAN_CIRCLES mode
int circle_radius;
sdlx_texture_t *red_circle_texture;
sdlx_texture_t *green_circle_texture;
sdlx_texture_t *blue_circle_texture;

// texture used to display color organ in either vertical or horizontal orientation
sdlx_texture_t *color_organ_texture;

// used to display band scaling values for a short time interval
int disp_band_gain;

//
// prototypes
//

void settings_init(void);

// -----------------  INIT & CLEANUP  --------------------------------

sdlx_texture_t *create_circle_texture(sdlx_color_t color);

void color_organ_init(void)
{
    // initialize settings from params file
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

sdlx_texture_t *create_circle_texture(sdlx_color_t color)
{
    sdlx_texture_t *t;

    t = sdlx_create_texture(200,200);
    sdlx_set_render_target(t);
    sdlx_render_fill_circle(100, 100, 100, color);
    sdlx_set_render_target(NULL);

    return t;
}

// -----------------  COLOR ORGAN DISPLAY  ---------------------------

void filter(float *val, float new_val);
void display_band(int which_band, float band_volume);

void color_organ_display(void)
{
    int    num_downsample = 4;
    int    num_samples = nearbyint(FRAMES_PER_SEC / num_downsample * 0.050);  // equals 600
    float  delta_f = ((double)FRAMES_PER_SEC/num_downsample) / num_samples;   // equals 20
    float  samples[600], fft[301];

    static float filtered_vol[3];

    // render to color_organ_texture
    sdlx_set_render_target(color_organ_texture);
    sdlx_clear_texture(color_organ_texture, COLOR_BLACK);

    // get audio samples and perform fft
    sdlx_get_audio_samples(num_samples, num_downsample, GET_SAMPLES_MONO, samples);
    util_fft_real_to_real(num_samples, samples, fft, true);

    // loop over the 3 bands
    for (int band = 0; band < 3; band++) {
        int   first_bin, last_bin;
        float raw_new_vol;

        // determine raw_new_vol for this band
        first_bin = nearbyint(band_start_freq[band] / delta_f);
        last_bin = nearbyint(band_end_freq[band] / delta_f);
        raw_new_vol = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// yyy picoc isue requires &fft[first_bin]

        // apply scale factor
        raw_new_vol *= band_gain[band];
        if (raw_new_vol > 1) raw_new_vol = 1;

        // apply filter
        filter(&filtered_vol[band], raw_new_vol);

        // display the band
        display_band(band, filtered_vol[band]);
    }

    // restore render target to the display
    sdlx_set_render_target(NULL);

    // copy the color_organ_texture to the display, based on the caller provided display orientation
    if (orientation == VERTICAL) {
        sdlx_render_texture(color_organ_texture, 0, 0);
    } else {
        // yyy comment
        float scale = 1000.0 / COH;
        int new_w = nearbyint(1000 * scale);
        int new_h = nearbyint(COH * scale);
        sdlx_render_texture_ex3(color_organ_texture, 
                                0, 0.5*sdlx_win_height - new_w/2, new_w, new_h,
                                90, new_h/2, new_h/2);
    }
}

void filter(float *val, float new_val)
{
    if (which_filter == FILTER_NONE) {
        *val = new_val;
    } else if (which_filter == FILTER_EXP_SMOOTH) {
        *val = *val + exp_filter_k * (new_val - *val);
    } else if (which_filter == FILTER_SNAP) {
        if (new_val > *val) {
            *val = new_val;
        } else {
            *val -= snap_filter_k;
            if (*val < new_val) *val = new_val;
        }
    } else {
        *val = 0;
    }
}

void display_band(int which_band, float band_volume)
{
    if (which_color_organ == COLOR_ORGAN_BARS) {
        int          x = which_band * 333;
        int          h = COH*band_volume;
        sdlx_color_t color;

        color = (which_band == LOW_BAND ? COLOR_RED : (which_band == MID_BAND ? COLOR_GREEN : COLOR_BLUE));
        sdlx_render_fill_rect(x, COH-h, 333, h, color);
    } else {
        sdlx_texture_t *t;
        int             x_ctr, y_ctr;
        float           k = 2.0;  // xxx param ?  OR eliminate somehow

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

        band_volume *= k;
        sdlx_color_mod_texture(t, band_volume, band_volume, band_volume);
        sdlx_render_texture_ex1(t, x_ctr-circle_radius, y_ctr-circle_radius, 2*circle_radius, 2*circle_radius);
    }
}

// -----------------  COLOR ORGAN EVENT HANDLING  --------------------

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);

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
        if (disp_band_gain == 0) {
            disp_band_gain = DISP_BAND_GAIN_DURATION;
            break;
        }
        band_gain[LOW_BAND] += (ev->event_id == EVID_RED_INCREASE ? 5 : -5);
        disp_band_gain = DISP_BAND_GAIN_DURATION;
        util_set_numeric_param(data_dir, "low_band_gain", band_gain[LOW_BAND]);
        break;
    case EVID_GREEN_INCREASE:
    case EVID_GREEN_DECREASE:
        if (disp_band_gain == 0) {
            disp_band_gain = DISP_BAND_GAIN_DURATION;
            break;
        }
        band_gain[MID_BAND] += (ev->event_id == EVID_GREEN_INCREASE ? 5 : -5);
        disp_band_gain = DISP_BAND_GAIN_DURATION;
        util_set_numeric_param(data_dir, "mid_band_gain", band_gain[MID_BAND]);
        break;
    case EVID_BLUE_INCREASE:
    case EVID_BLUE_DECREASE:
        if (disp_band_gain == 0) {
            disp_band_gain = DISP_BAND_GAIN_DURATION;
            break;
        }
        band_gain[HIGH_BAND] += (ev->event_id == EVID_BLUE_INCREASE ? 5 : -5);
        disp_band_gain = DISP_BAND_GAIN_DURATION;
        util_set_numeric_param(data_dir, "high_band_gain", band_gain[HIGH_BAND]);
        break;
    }
}

void color_organ_register_events(int y_controls_2)
{
    sdlx_loc_t loc;
    int flags = FLAG_XY_CTR | (orientation == HORIZONTAL ? FLAG_ROT_90 : 0);

    reg_event(COL2X(0), y_controls_2, COLOR_LIGHT_BLUE,
            color_organ_name[which_color_organ], EVID_COLOR_ORGAN_SLCT);
    reg_event(COL2X(5), y_controls_2, COLOR_LIGHT_BLUE, filter_name[which_filter], EVID_FILTER_SLCT);

    if (which_color_organ == COLOR_ORGAN_BARS) {
        for (int band = 0; band < 3; band++) {
            init_loc(&loc, 333*band, 0, 333, COH/2);
            sdlx_register_event(&loc, band == 0 ? EVID_RED_INCREASE : (band == 1 ? EVID_GREEN_INCREASE : EVID_BLUE_INCREASE));
            init_loc(&loc, 333*band, COH/2, 333, COH/2);
            sdlx_register_event(&loc, band == 0 ? EVID_RED_DECREASE : (band == 1 ? EVID_GREEN_DECREASE : EVID_BLUE_DECREASE));
            if (disp_band_gain) {
                init_loc(&loc, 333*band+166, COH/2, 0, 0);
                sdlx_render_printf_ex2(loc.x, loc.y,
                                       FONT_NORMAL, COLOR_WHITE, flags, WRAP_NONE,
                                       "%d", band_gain[band]);
            }
        }
    } else if (which_color_organ == COLOR_ORGAN_CIRCLES) {
        int r = circle_radius;
        int x_ctr[3] = {500-r, 500+r, 500};
        int y_ctr[3] = {COH-r, COH-r, r};

        for (int band = 0; band < 3; band++) {
            init_loc(&loc, x_ctr[band]-r/2, y_ctr[band]-r, r, r);
            sdlx_register_event(&loc, band == 0 ? EVID_RED_INCREASE : (band == 1 ? EVID_GREEN_INCREASE : EVID_BLUE_INCREASE));
            init_loc(&loc, x_ctr[band]-r/2, y_ctr[band], r, r);
            sdlx_register_event(&loc, band == 0 ? EVID_RED_DECREASE : (band == 1 ? EVID_GREEN_DECREASE : EVID_BLUE_DECREASE));
            if (disp_band_gain) {
                init_loc(&loc, x_ctr[band], y_ctr[band], 0, 0);
                sdlx_render_printf_ex2(loc.x, loc.y,
                                       FONT_NORMAL, COLOR_WHITE, flags, WRAP_NONE,
                                       "%d", band_gain[band]);
            }
        }
    } else {
        // xxx fft 
    }

    if (disp_band_gain) {
        disp_band_gain--;
    }
}

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h)
{
    if (orientation == VERTICAL) {
        loc->x = x;
        loc->y = y;
        loc->w = w;
        loc->h = h;
    } else {
        float scale = 1000.0 / COH;
        int   y_top = sdlx_win_height/2 - (1000*scale)/2;
        loc->h = w * scale;
        loc->w = h * scale;
        loc->y = x * scale + y_top;
        loc->x = sdlx_win_width - y * scale - loc->w;
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

void color_organ_settings(void)
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

