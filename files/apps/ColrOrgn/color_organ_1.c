#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

// xxx todo
// - get rid of boxes, make it a setup option
// - color organ with circles instead
// - use light blue
// - horizontal orientation,  display left & right channels;  no params selections
// - add params for decay and exp smooth

//
// defines
//

#define EVID_RESET           1000
#define EVID_RGB_FILTER      1001

#define EVID_RED_INCREASE    1010
#define EVID_RED_DECREASE    1011
#define EVID_GREEN_INCREASE  1012
#define EVID_GREEN_DECREASE  1013
#define EVID_BLUE_INCREASE   1014
#define EVID_BLUE_DECREASE   1015

#define RED    0
#define GREEN  1
#define BLUE   2

#define RGB_FILTER_NONE        0
#define RGB_FILTER_EXP_SMOOTH  1
#define RGB_FILTER_SNAP        2
#define RGB_FILTER_DEFAULT     RGB_FILTER_EXP_SMOOTH
#define MAX_RGB_FILTER         3

//
// variables
//

int   rgb_filter = RGB_FILTER_DEFAULT;
int   rgb_k[MAX_RGB_FILTER] = {20, 40, 100};
int   rgb_k_default[MAX_RGB_FILTER] = {20, 40, 100};
char *rgb_filter_name[MAX_RGB_FILTER] = {"NONE", "EXP", "SNAP"};

int   disp_k;

//
// prototypes
//

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);

// -----------------  INIT & CLEANUP  --------------------------------

void color_organ_init(void)
{
    rgb_k[RED]   = util_get_numeric_param(data_dir, "rgb_k_red", rgb_k_default[RED]);
    rgb_k[GREEN] = util_get_numeric_param(data_dir, "rgb_k_green", rgb_k_default[GREEN]);
    rgb_k[BLUE]  = util_get_numeric_param(data_dir, "rgb_k_blue", rgb_k_default[BLUE]);

    rgb_filter   = util_get_numeric_param(data_dir, "rgb_filter", RGB_FILTER_DEFAULT);

}

void color_organ_cleanup(void)
{
}

// -----------------  COLOR ORGAN DISPLAY  ---------------------------

#define LOW_BAND_START   60
#define LOW_BAND_END     150
#define MID_BAND_START   200
#define MID_BAND_END     600
#define HIGH_BAND_START  800
#define HIGH_BAND_END    2200

#define DECAY  0.0010  // xxx param

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

    if (disp_k > 0) {
        disp_k--;
        sdlx_render_printf_ex2(333/2, 250, 
                               FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                               "%d", rgb_k[RED]); 
        sdlx_render_printf_ex2(333/2+333, 250, 
                               FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                               "%d", rgb_k[GREEN]); 
        sdlx_render_printf_ex2(333/2+666, 250, 
                               FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                               "%d", rgb_k[BLUE]); 
    }
}

void display_band(int which, float band_volume)
{
    static int  h_smoothed[3];
    int         h_unfiltered, x;
    sdlx_color_t color;

    x = which * 333;
    color = (which == RED ? COLOR_RED : (which == GREEN ? COLOR_GREEN : COLOR_BLUE));

    h_unfiltered = band_volume * rgb_k[which] * 500;
    if (h_unfiltered > 500) h_unfiltered = 500;

    if (rgb_filter == RGB_FILTER_NONE) {
        h_smoothed[which] = h_unfiltered;
    } else if (rgb_filter == RGB_FILTER_EXP_SMOOTH) {
        h_smoothed[which] = h_smoothed[which] + 0.6 * (h_unfiltered - h_smoothed[which]);
    } else if (rgb_filter == RGB_FILTER_SNAP) {
        if (h_unfiltered > h_smoothed[which]) {
            h_smoothed[which] = h_unfiltered;
        } else {
            h_smoothed[which] -= 40;
            if (h_smoothed[which] < h_unfiltered) h_smoothed[which] = h_unfiltered;
        }
    } else {
        h_smoothed[which] = 0;
    }

    sdlx_render_fill_rect(x, 500-h_smoothed[which], 333, h_smoothed[which], color);
}

// -----------------  COLOR ORGAN EVENT HANDLING  --------------------

void color_organ_register_events(void)
{
    sdlx_loc_t *locp;
    sdlx_loc_t loc;

    locp = sdlx_render_printf(0, 500, "%s", rgb_filter_name[rgb_filter]);
    sdlx_register_event(locp, EVID_RGB_FILTER);

    locp = sdlx_render_printf(sdlx_win_width-5*sdlx_char_width_dflt, 500, "%s", "RESET");
    sdlx_register_event(locp, EVID_RESET);

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
}

void color_organ_process_event(sdlx_event_t *ev)
{
    switch (ev->event_id) {
    case EVID_RGB_FILTER:
        rgb_filter = (rgb_filter + 1) % MAX_RGB_FILTER;
        util_set_numeric_param(data_dir, "rgb_filter", rgb_filter);
        break;
    case EVID_RED_INCREASE:
    case EVID_RED_DECREASE:
        rgb_k[RED] += (ev->event_id == EVID_RED_INCREASE ? 1 : -1);
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "rgb_k_red", rgb_k[RED]);
        break;
    case EVID_GREEN_INCREASE:
    case EVID_GREEN_DECREASE:
        rgb_k[GREEN] += (ev->event_id == EVID_GREEN_INCREASE ? 1 : -1);
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "rgb_k_green", rgb_k[GREEN]);
        break;
    case EVID_BLUE_INCREASE:
    case EVID_BLUE_DECREASE:
        rgb_k[BLUE] += (ev->event_id == EVID_BLUE_INCREASE ? 1 : -1);
        disp_k = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "rgb_k_blue", rgb_k[BLUE]);
        break;
    case EVID_RESET: // xxx more resets
        memcpy(rgb_k, rgb_k_default, sizeof(rgb_k));
        util_set_numeric_param(data_dir, "rgb_k_red",   rgb_k[RED]);
        util_set_numeric_param(data_dir, "rgb_k_green", rgb_k[GREEN]);
        util_set_numeric_param(data_dir, "rgb_k_blue",  rgb_k[BLUE]);

        rgb_filter = RGB_FILTER_DEFAULT;
        util_set_numeric_param(data_dir, "rgb_filter",  rgb_filter);

        disp_k  = DISP_K_DURATION;
        break;
    }
}

// -----------------  UTILS  -----------------------------------------

// xxx delete ?
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
