#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// xxx inprog
// - ezput the entire dir and subdir
// - need to make the files subdir if not already there
// - freq sweep
// - scroll files
// - negative rgb_k
// - param values      7 15 25   ?
// - use light blue

// xxx DONE
// - change SLCT to the name
// - color organ with circles instead
// - enable / disable box
// - rename this file
// - show / hide - event to display params

// xxx todo
// - call to get_samples should return fps and possibly num_channels, and 
//   caller should deal with the fps
// - circles mode multiplier constant too large?
// - horizontal orientation,  display left & right channels;  no params selections
// - add params for decay and exp smooth
// - ezput should copy subdirs too
// - make a freq seep test file
// - ctrls to rename and delete files
// - auto adjust params

//
// defines
//

#define RED    0
#define GREEN  1
#define BLUE   2

#define CIRCLE_RADIUS  134   // 500 / (2 + sqrt(3))

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
//int   rgb_k_default[MAX_FILTER] = {15, 25, 50};
int   rgb_k_default[MAX_FILTER] = {15, 30, 150};

int   disp_k;

sdlx_texture_t *red_circle_texture;
sdlx_texture_t *green_circle_texture;
sdlx_texture_t *blue_circle_texture;

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
}

void color_organ_cleanup(void)
{
    sdlx_destroy_texture(red_circle_texture);
    sdlx_destroy_texture(green_circle_texture);
    sdlx_destroy_texture(blue_circle_texture);
}

// -----------------  COLOR ORGAN DISPLAY  ---------------------------

#define DISP_K_DURATION 30

void display_band(int which, float band_volume);

void color_organ_display(sdlx_audio_state_t *as)
{
    if (as->state != AUDIO_STATE_IDLE) {
        int    num_downsample = 4;
        int    num_samples = nearbyint(FRAMES_PER_SEC / num_downsample * 0.050);  // equals 600
        float  delta_f = ((double)FRAMES_PER_SEC/num_downsample) / num_samples;
        float  samples[600], fft[301];
        int    first_bin, last_bin;
        float low_band=0, mid_band=0, high_band=0;

        sdlx_get_audio_samples(num_samples, num_downsample, GET_SAMPLES_MONO, samples);
        util_fft_real_to_real(num_samples, samples, fft, true);

        first_bin = nearbyint(LOW_BAND_START/delta_f);
        last_bin = nearbyint(LOW_BAND_END/delta_f);
        low_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]
        display_band(RED, low_band);

        first_bin = nearbyint(MID_BAND_START/delta_f);
        last_bin = nearbyint(MID_BAND_END/delta_f);
        mid_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]
        display_band(GREEN, mid_band);

        first_bin = nearbyint(HIGH_BAND_START/delta_f);
        last_bin = nearbyint(HIGH_BAND_END/delta_f);
        high_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]
        display_band(BLUE, high_band);
    }

    if (show_params || disp_k > 0) {
        int x_ctr, y_ctr, param_value;

        disp_k--;

        x_ctr = (which_color_organ == COLOR_ORGAN_BARS ? 333/2 : 500-CIRCLE_RADIUS);
        y_ctr = (which_color_organ == COLOR_ORGAN_BARS ? 250   : 500-CIRCLE_RADIUS);
        param_value = rgb_k[RED];
        sdlx_render_printf_ex2(x_ctr, y_ctr, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", param_value); 

        x_ctr = (which_color_organ == COLOR_ORGAN_BARS ? 333/2+333 : 500+CIRCLE_RADIUS);
        y_ctr = (which_color_organ == COLOR_ORGAN_BARS ? 250       : 500-CIRCLE_RADIUS);
        param_value = rgb_k[GREEN];
        sdlx_render_printf_ex2(x_ctr, y_ctr, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", param_value); 

        x_ctr = (which_color_organ == COLOR_ORGAN_BARS ? 333/2+666 : 500);
        y_ctr = (which_color_organ == COLOR_ORGAN_BARS ? 250       : CIRCLE_RADIUS);
        param_value = rgb_k[BLUE];
        sdlx_render_printf_ex2(x_ctr, y_ctr, FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE, "%d", param_value); 
    }
}

void display_band(int which, float band_volume)
{
    static float smoothed[3];
    float        scaled;

    scaled = band_volume * rgb_k[which];
    if (scaled > 1) scaled = 1;

    if (which_filter == FILTER_NONE) {
        smoothed[which] = scaled;
    } else if (which_filter == FILTER_EXP_SMOOTH) {
        smoothed[which] = smoothed[which] + 0.6 * (scaled - smoothed[which]);
    } else if (which_filter == FILTER_SNAP) {
        if (scaled > smoothed[which]) {
            smoothed[which] = scaled;
        } else {
            smoothed[which] -= 0.08;
            if (smoothed[which] < scaled) smoothed[which] = scaled;
        }
    } else {
        smoothed[which] = 0;
    }

    if (which_color_organ == COLOR_ORGAN_BARS) {
        int          x = which * 333;
        int          h = 500*smoothed[which];
        sdlx_color_t color;

        color = (which == RED ? COLOR_RED : (which == GREEN ? COLOR_GREEN : COLOR_BLUE));
        sdlx_render_fill_rect(x, 500-h, 333, h, color);
    } else {
        sdlx_texture_t *t;
        int             x_ctr, y_ctr;
        float           intensity;
        float           k = 2.0;

        if (which == RED) {
            intensity = smoothed[RED] * k;
            t = red_circle_texture;
            x_ctr = 500 - CIRCLE_RADIUS;
            y_ctr = 500 - CIRCLE_RADIUS;
        } else if (which == GREEN) {
            intensity = smoothed[GREEN] * k;
            t = green_circle_texture;
            x_ctr = 500 + CIRCLE_RADIUS;
            y_ctr = 500 - CIRCLE_RADIUS;
        } else {
            intensity = smoothed[BLUE] * k;
            t = blue_circle_texture;
            x_ctr = 500;
            y_ctr = CIRCLE_RADIUS;
        }
        sdlx_color_mod_texture(t, intensity, intensity, intensity);
        sdlx_render_texture_ex1(t, x_ctr-CIRCLE_RADIUS, y_ctr-CIRCLE_RADIUS, 2*CIRCLE_RADIUS, 2*CIRCLE_RADIUS);
    }
}

// -----------------  COLOR ORGAN EVENT REGISTRATION  ----------------

void color_organ_register_events(void)
{
    sdlx_loc_t *locp;
    sdlx_loc_t loc;
    char      *clr_orgn_name;

    locp = sdlx_render_printf(0, 500, "%s", filter_name[which_filter]);
    sdlx_register_event(locp, EVID_FILTER_SLCT);

    clr_orgn_name = color_organ_name[which_color_organ];
    locp = sdlx_render_printf(sdlx_win_width-strlen(clr_orgn_name)*sdlx_char_width_dflt, 500, "%s", clr_orgn_name);
    sdlx_register_event(locp, EVID_COLOR_ORGAN_SLCT);

    if (which_color_organ == COLOR_ORGAN_BARS) {
        init_loc(&loc, 0, 0, 333, 250);
        sdlx_register_event(&loc, EVID_RED_INCREASE);
        init_loc(&loc, 0, 250, 333, 250);
        sdlx_register_event(&loc, EVID_RED_DECREASE);

        init_loc(&loc, 333, 0, 333, 250);
        sdlx_register_event(&loc, EVID_GREEN_INCREASE);
        init_loc(&loc, 333, 250, 333, 250);
        sdlx_register_event(&loc, EVID_GREEN_DECREASE);

        init_loc(&loc, 666, 0, 333, 250);
        sdlx_register_event(&loc, EVID_BLUE_INCREASE);
        init_loc(&loc, 666, 250, 333, 250);
        sdlx_register_event(&loc, EVID_BLUE_DECREASE);
    } else {
        int x_ctr, y_ctr, r;
        r=134;
        x_ctr = 500 - r;
        y_ctr = 500 - r;
        init_loc(&loc, x_ctr-r/2, y_ctr-r, r, r);
        sdlx_register_event(&loc, EVID_RED_INCREASE);
        init_loc(&loc, x_ctr-r/2, y_ctr, r, r);
        sdlx_register_event(&loc, EVID_RED_DECREASE);

        x_ctr = 500 + r;
        y_ctr = 500 - r;
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
