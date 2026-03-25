#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

//
// defines
//

#define EVID_RGB_FILTER  1100

#define EVID_RED_INCREASE    1000
#define EVID_RED_DECREASE    1001
#define EVID_GREEN_INCREASE  1002
#define EVID_GREEN_DECREASE  1003
#define EVID_BLUE_INCREASE   1004
#define EVID_BLUE_DECREASE   1005
#define EVID_RESET     1006
//#define EVID_PARAMS          1007

//#define X_RED_CIRCLE    250
//#define Y_RED_CIRCLE    683
//#define X_GREEN_CIRCLE  750
//#define Y_GREEN_CIRCLE  683
//#define X_BLUE_CIRCLE   500
//#define Y_BLUE_CIRCLE   250


//#define DEFAULT_K_RED    20 
//#define DEFAULT_K_GREEN  100
//#define DEFAULT_K_BLUE   100
//#define DELTA_K          5
//#define MIN_K            0
//#define MAX_K            100
//#define DISP_K_DURATION  100   // cycles

//
// variables
//

//int k_red   = DEFAULT_K_RED;
//int k_green = DEFAULT_K_GREEN;
//int k_blue  = DEFAULT_K_BLUE;
//int disp_k  = 0;

//
// prototypes
//

void proc(sdlx_texture_t *t, int x_ctr, int y_ctr, double newval, double *work, int k);  // xxx name0

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);

// -----------------  INIT & CLEANUP  --------------------------------

void color_organ_init(void)
{
    //k_red = util_get_numeric_param(data_dir, "k_red", DEFAULT_K_RED);
    //k_green = util_get_numeric_param(data_dir, "k_green", DEFAULT_K_GREEN);
    //k_blue = util_get_numeric_param(data_dir, "k_blue", DEFAULT_K_BLUE);
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

// xxx make 'as' a glbl ptr
#define DECAY  0.0010

#define RED    0
#define GREEN  1
#define BLUE   2

#define MAX_RGB_FILTER 3

#define DISP_K_DURATION 30

int disp_k;
int rgb_filter = 1;
int rgb_k[3] = {20, 40, 100};  // xxx init from params
int rgb_k_default[3] = {20, 40, 100};
char *rgb_filter_name[3] = {"NONE", "EXP", "SNAP"};

void display(int which, float band_volume);

void color_organ_display(sdlx_audio_state_t *as)
{
    //static double work_red, work_green, work_blue;
    //static int    audio_state_last = AUDIO_STATE_IDLE;
    //static bool   audio_paused_last = false;
    //static bool   first_call = true;


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
        display(RED, low_band);

        first_bin = nearbyint(MID_BAND_START/delta_f);
        last_bin = nearbyint(MID_BAND_END/delta_f);
        mid_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]
        display(GREEN, mid_band);

        first_bin = nearbyint(HIGH_BAND_START/delta_f);
        last_bin = nearbyint(HIGH_BAND_END/delta_f);
        high_band = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]
        display(BLUE, high_band);

        //printf("%f %f %f\n", low_band, mid_band, high_band);
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



    if (as->state != AUDIO_STATE_IDLE) {
        //static float filtered;
        //float actual = low_band;
        
#if 0
        double *work, filtered, actual;
        work = &work_red;
        actual = low_band;
        filtered = *work;
        if (actual > filtered) {
            filtered = actual;
        } else {
            filtered -= 0.001;   
            if (filtered < actual) filtered = actual;
        }
        *work = filtered;
#endif
#if 0
        int filtering = 2;
        int h_unfiltered;
        static int h_smoothed;

        h_unfiltered = low_band * k_red * 500;
        if (h_unfiltered > 500) h_unfiltered = 500;

        if (filtering == 0) {
            h_smoothed = h_unfiltered;
        } else if (filtering == 1) {
            h_smoothed = h_smoothed + 0.6 * (h_unfiltered - h_smoothed);
        } else if (filtering == 2) {
            if (h_unfiltered > h_smoothed) {
                h_smoothed = h_unfiltered;
            } else {
                h_smoothed -= 40;
                if (h_smoothed < h_unfiltered) h_smoothed = h_unfiltered;
            }
        } else {
            h_smoothed = 0;
        }

        sdlx_render_fill_rect(0, 500-h_smoothed, 333, h_smoothed, COLOR_RED);
#endif

        //proc(NULL, X_RED_CIRCLE, Y_RED_CIRCLE, low_band, &work_red, k_red);
        //proc(NULL, X_GREEN_CIRCLE, Y_GREEN_CIRCLE, mid_band, &work_green, k_green);
        //proc(NULL, X_BLUE_CIRCLE, Y_BLUE_CIRCLE, high_band, &work_blue, k_blue);
        //printf("--------------\n");
    }

// xxx make a routine for this
}

//xxxxxxxxxxxxx

void display(int which, float band_volume)
{
    static int  h_smoothed[3];
    int         h_unfiltered, x;
    sdlx_color_t color;

    x = which * 333;
    color = (which == RED ? COLOR_RED : (which == GREEN ? COLOR_GREEN : COLOR_BLUE));

    h_unfiltered = band_volume * rgb_k[which] * 500;
    if (h_unfiltered > 500) h_unfiltered = 500;

    if (rgb_filter == 0) {
        h_smoothed[which] = h_unfiltered;
    } else if (rgb_filter == 1) {
        h_smoothed[which] = h_smoothed[which] + 0.6 * (h_unfiltered - h_smoothed[which]);
    } else if (rgb_filter == 2) {
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

    // xxx use LIGHT_BLUE

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

    //sdlx_loc_t loc;

//    init_loc(&loc, X_RED_CIRCLE-125, Y_RED_CIRCLE-250, 250, 250);
//    sdlx_register_event(&loc, EVID_RED_INCREASE);
//    init_loc(&loc, X_RED_CIRCLE-125, Y_RED_CIRCLE, 250, 250);
//    sdlx_register_event(&loc, EVID_RED_DECREASE);
//
//    init_loc(&loc, X_GREEN_CIRCLE-125, Y_GREEN_CIRCLE-250, 250, 250);
//    sdlx_register_event(&loc, EVID_GREEN_INCREASE);
//    init_loc(&loc, X_GREEN_CIRCLE-125, Y_GREEN_CIRCLE, 250, 250);
//    sdlx_register_event(&loc, EVID_GREEN_DECREASE);
//
//    init_loc(&loc, X_BLUE_CIRCLE-125, Y_BLUE_CIRCLE-250, 250, 250);
//    sdlx_register_event(&loc, EVID_BLUE_INCREASE);
//    init_loc(&loc, X_BLUE_CIRCLE-125, Y_BLUE_CIRCLE, 250, 250);
//    sdlx_register_event(&loc, EVID_BLUE_DECREASE);

    //locp = sdlx_render_printf(sdlx_win_width-COL2X(3), ROW2Y(0.5), "%s", "RST");
    //sdlx_register_event(locp, EVID_RESET);

    //locp = sdlx_render_printf(sdlx_win_width-COL2X(3), ROW2Y(2.5), "%s", "PRM");
    //sdlx_register_event(locp, EVID_PARAMS);
}

void color_organ_process_event(sdlx_event_t *ev)
{
    // xxx only if active ?

    switch (ev->event_id) {
    case EVID_RGB_FILTER:
        rgb_filter = (rgb_filter + 1) % MAX_RGB_FILTER;
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
    case EVID_RESET:
        // xxx more resets
        memcpy(rgb_k, rgb_k_default, sizeof(rgb_k));
        util_set_numeric_param(data_dir, "rgb_k_red",   rgb_k[RED]);
        util_set_numeric_param(data_dir, "rgb_k_green", rgb_k[GREEN]);
        util_set_numeric_param(data_dir, "rgb_k_blue",  rgb_k[BLUE]);
        disp_k  = DISP_K_DURATION;
        break;
    }
#if 0
    switch (ev->event_id) {
    case EVID_RESET:
        k_red   = DEFAULT_K_RED;
        k_green = DEFAULT_K_GREEN;
        k_blue  = DEFAULT_K_BLUE;
        disp_k  = DISP_K_DURATION;
        util_set_numeric_param(data_dir, "k_red", k_red);
        util_set_numeric_param(data_dir, "k_green", k_green);
        util_set_numeric_param(data_dir, "k_blue", k_blue);
        break;
    case EVID_PARAMS:
        disp_k  = DISP_K_DURATION;
        break;
    }
#endif
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
