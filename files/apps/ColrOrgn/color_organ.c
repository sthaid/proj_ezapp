#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// xxx inprog
// - horizontal orientation,  display left & right channels;  no params selections
// - dont do stereo OR dont do circles
// - add ctrl for testing hori/vert

// xxx next
// - ctrls to rename and delete files
// - negative rgb_k
// - print the duration of each cycle, and set event wait time to allow for the duration  INPROG

// xxx todo
// - call to get_samples should return fps and possibly num_channels, and 
//   caller should deal with the fps
// - circles mode multiplier constant too large?
// - add params for decay and exp smooth
// - auto adjust params

//
// defines
//

#define RED    0
#define GREEN  1
#define BLUE   2

#define COH  COLOR_ORGAN_H  // abbreviation

#define EVID_FILTER_SLCT      1001
#define EVID_COLOR_ORGAN_SLCT 1002
#define EVID_RED_INCREASE     1010
#define EVID_RED_DECREASE     1011
#define EVID_GREEN_INCREASE   1012
#define EVID_GREEN_DECREASE   1013
#define EVID_BLUE_INCREASE    1014
#define EVID_BLUE_DECREASE    1015

#define FILTER_NONE        0
#define FILTER_EXP_SMOOTH  1
#define FILTER_SNAP        2
#define FILTER_DEFAULT     FILTER_EXP_SMOOTH
#define MAX_FILTER         3

#define COLOR_ORGAN_BARS     0
#define COLOR_ORGAN_CIRCLES  1
#define COLOR_ORGAN_DEFAULT  COLOR_ORGAN_BARS
#define MAX_COLOR_ORGAN      2

//
// variables
//

int   which_color_organ = COLOR_ORGAN_DEFAULT;
char *color_organ_name[MAX_COLOR_ORGAN] = {"BARS", "CIRCLES"};

int   which_filter = FILTER_DEFAULT;
char *filter_name[MAX_FILTER] = {"NONE", "EXP", "SNAP"};

int   rgb_k[MAX_FILTER];
int   rgb_k_default[MAX_FILTER] = {10, 15, 25};

int   disp_k;

sdlx_texture_t *red_circle_texture;
sdlx_texture_t *green_circle_texture;
sdlx_texture_t *blue_circle_texture;

int circle_radius;

sdlx_texture_t *color_organ_texture;

//
// prototypes
//

sdlx_texture_t *create_circle_texture(sdlx_color_t color);
void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);

// -----------------  INIT & CLEANUP  --------------------------------

void color_organ_init(void)
{
    rgb_k[RED]        = util_get_numeric_param(data_dir, "rgb_k_red",   rgb_k_default[RED]);
    rgb_k[GREEN]      = util_get_numeric_param(data_dir, "rgb_k_green", rgb_k_default[GREEN]);
    rgb_k[BLUE]       = util_get_numeric_param(data_dir, "rgb_k_blue",  rgb_k_default[BLUE]);
    which_filter      = util_get_numeric_param(data_dir, "filter",      FILTER_DEFAULT);
    which_color_organ = util_get_numeric_param(data_dir, "color_organ", COLOR_ORGAN_DEFAULT);

    red_circle_texture   = create_circle_texture(COLOR_RED);
    green_circle_texture = create_circle_texture(COLOR_GREEN);
    blue_circle_texture  = create_circle_texture(COLOR_BLUE);
    circle_radius = COH / (2 + sqrt(3));

    color_organ_texture = sdlx_create_texture(1000, COH);

    if (!util_file_exists(files_dir, "test_all.mp3")) {
        printf("I %s: creating test files\n", progname);
        sdlx_create_test_file(files_dir, "test_all.mp3", TEST_FILE_FREQ_SWEEP, 
                              LOW_BAND_START, HIGH_BAND_END, 10);
        sdlx_create_test_file(files_dir, "test_low.mp3", TEST_FILE_FREQ_SWEEP,
                              LOW_BAND_START, LOW_BAND_END, 10);
        sdlx_create_test_file(files_dir, "test_mid.mp3", TEST_FILE_FREQ_SWEEP,
                              MID_BAND_START, MID_BAND_END, 10);
        sdlx_create_test_file(files_dir, "test_high.mp3", TEST_FILE_FREQ_SWEEP,
                              HIGH_BAND_START, HIGH_BAND_END, 10);
    }
}

void color_organ_cleanup(void)
{
    sdlx_destroy_texture(red_circle_texture);
    sdlx_destroy_texture(green_circle_texture);
    sdlx_destroy_texture(blue_circle_texture);
    sdlx_destroy_texture(color_organ_texture);
}

// -----------------  COLOR ORGAN DISPLAY  ---------------------------

#define DISP_K_DURATION 30

void display_band(int which_band, float band_volume);

void color_organ_display(int orientation, bool idle)
{
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
        low_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]

        first_bin = nearbyint(MID_BAND_START/delta_f);
        last_bin = nearbyint(MID_BAND_END/delta_f);
        mid_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]

        first_bin = nearbyint(HIGH_BAND_START/delta_f);
        last_bin = nearbyint(HIGH_BAND_END/delta_f);
        high_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]
    } else {
        low_band = 0;
        mid_band = 0;
        high_band = 0;
    }

    // render the band volume for the 3 bands
    display_band(RED, low_band);
    display_band(GREEN, mid_band);
    display_band(BLUE, high_band);

    // restore render target to the display
    sdlx_set_render_target(NULL);

    // copy the color_organ_texture to the display, based on the caller provided display orientation
    if (orientation == VERTICAL) {
        sdlx_render_texture(color_organ_texture, 0, 0);
    } else {
        sdlx_render_texture_ex3(color_organ_texture, 
                                0, sdlx_win_height/2 - 1000/2, 1000, COH,
                                90, COH/2, COH/2);
    }

    // decrement disp_k, this will stop the display of the band scaling constants
    // after a short interval
    if (disp_k > 0) disp_k--;
}

void display_band(int which_band, float band_volume)
{
    static float smoothed_save[3];
    float        scaled, smoothed;

    scaled = band_volume * rgb_k[which_band];
    if (scaled > 1) scaled = 1;

    smoothed = smoothed_save[which_band];
    if (which_filter == FILTER_NONE) {
        smoothed = scaled;
    } else if (which_filter == FILTER_EXP_SMOOTH) {
        smoothed = smoothed + 0.6 * (scaled - smoothed);
    } else if (which_filter == FILTER_SNAP) {
        if (scaled > smoothed) {
            smoothed = scaled;
        } else {
            smoothed -= 0.08;
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

        color = (which_band == RED ? COLOR_RED : (which_band == GREEN ? COLOR_GREEN : COLOR_BLUE));
        sdlx_render_fill_rect(x, COH-h, 333, h, color);

        if (show_params || disp_k > 0) {
            sdlx_render_printf_ex2(x+333/2, COH/2, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", rgb_k[which_band]); 
        }
    } else {
        sdlx_texture_t *t;
        int             x_ctr, y_ctr;
        float           intensity;
        float           k = 2.0;

        intensity = smoothed * k;
        if (which_band == RED) {
            t = red_circle_texture;
            x_ctr = 500 - circle_radius;
            y_ctr = COH - circle_radius;
        } else if (which_band == GREEN) {
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

        if (show_params || disp_k > 0) {
            sdlx_render_printf_ex2(x_ctr, y_ctr, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", rgb_k[which_band]); 
        }
    }
}

// -----------------  COLOR ORGAN EVENT REGISTRATION  ----------------

void color_organ_register_events(int orientation)
{
    sdlx_loc_t *locp;
    sdlx_loc_t loc;
    char      *clr_orgn_name;

    // xxx todo
    if (orientation == HORIZONTAL) {
        return;
    }

    locp = sdlx_render_printf_ex1(0, COH, FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", filter_name[which_filter]);
    sdlx_register_event(locp, EVID_FILTER_SLCT);

    clr_orgn_name = color_organ_name[which_color_organ];
    locp = sdlx_render_printf_ex1(sdlx_win_width-strlen(clr_orgn_name)*sdlx_char_width_dflt, COH, 
                                  FONT_NORMAL, COLOR_LIGHT_BLUE, "%s", clr_orgn_name);
    sdlx_register_event(locp, EVID_COLOR_ORGAN_SLCT);

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
    case EVID_RED_INCREASE:
    case EVID_RED_DECREASE:
        rgb_k[RED] += (ev->event_id == EVID_RED_INCREASE ? 5 : -5);
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "rgb_k_red", rgb_k[RED]);
        break;
    case EVID_GREEN_INCREASE:
    case EVID_GREEN_DECREASE:
        rgb_k[GREEN] += (ev->event_id == EVID_GREEN_INCREASE ? 5 : -5);
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "rgb_k_green", rgb_k[GREEN]);
        break;
    case EVID_BLUE_INCREASE:
    case EVID_BLUE_DECREASE:
        rgb_k[BLUE] += (ev->event_id == EVID_BLUE_INCREASE ? 5 : -5);
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "rgb_k_blue", rgb_k[BLUE]);
        break;
    case EVID_RESET: // xxx more resets for the color organ params
        memcpy(rgb_k, rgb_k_default, sizeof(rgb_k));
        util_set_numeric_param(data_dir, "rgb_k_red",   rgb_k[RED]);
        util_set_numeric_param(data_dir, "rgb_k_green", rgb_k[GREEN]);
        util_set_numeric_param(data_dir, "rgb_k_blue",  rgb_k[BLUE]);

        which_filter = FILTER_DEFAULT;
        util_set_numeric_param(data_dir, "filter", which_filter);

        which_color_organ = COLOR_ORGAN_DEFAULT;
        util_set_numeric_param(data_dir, "color_organ", which_color_organ);

        disp_k  = DISP_K_DURATION;
        break;
    case EVID_SHOW_PARAMS:
        show_params = !show_params;
        break;
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
