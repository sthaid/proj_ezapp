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
// - tune the duration dynamically

//
// defines
//

// bands
#define LOW_BAND    0   // red
#define MID_BAND    1   // green
#define HIGH_BAND   2   // blue

// events
#define EVID_FILTER_SLCT           1001
#define EVID_COLOR_ORGAN_SLCT      1002
#define EVID_LOW_BAND_INCREASE     1010
#define EVID_LOW_BAND_DECREASE     1011
#define EVID_MID_BAND_INCREASE     1012
#define EVID_MID_BAND_DECREASE     1013
#define EVID_HIGH_BAND_INCREASE    1014
#define EVID_HIGH_BAND_DECREASE    1015
#define EVID_FFT_INCREASE          1016
#define EVID_FFT_DECREASE          1017

// filters used to reduce visual jitter
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
#define DISP_SCALE_FACTOR_DURATION 50
#define COH  COLOR_ORGAN_H  // abbreviation

// default param values
#define DFLT_COLOR_ORGAN       COLOR_ORGAN_BARS
#define DFLT_FILTER            FILTER_EXP_SMOOTH
#define DFLT_LOW_BAND_SCALE    10
#define DFLT_MID_BAND_SCALE    15
#define DFLT_HIGH_BAND_SCALE   25
#define DFLT_FFT_SCALE         10
#define DFLT_EXP_FLTR_K      0.6
#define DFLT_SNAP_FLTR_K     0.08

//
// variables
//

// params
int   which_color_organ;
int   which_filter;
int   band_scale[3];
int   fft_scale;
float exp_fltr_k;
float snap_fltr_decay;

// color organ band freq ranges
int band_start_freq[3] = {  60, 200,  800 };
int band_end_freq[3]   = { 150, 600, 2200 };

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
int disp_scale_factor;

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

#define MAX_SAMPLES 600
#define MAX_FFT     301

void color_organ_display_fft(float *fft);
void filter(float *val, float new_val);
void display_band(int which_band, float band_volume);

void color_organ_display(void)
{
    int    num_downsample = 4;
    int    num_samples = nearbyint(FRAMES_PER_SEC / num_downsample * 0.050);  // equals 600
    float  delta_f = ((double)FRAMES_PER_SEC/num_downsample) / num_samples;   // equals 20
    float  samples[MAX_SAMPLES], fft[MAX_FFT];

    static float filtered_vol[3];

    // sanity check
    if (num_samples != MAX_SAMPLES) {
        printf("E %s: BUG num_samples=%d should be %d\n", progname, num_samples, MAX_SAMPLES);
        return;
    }

    // render to color_organ_texture
    sdlx_set_render_target(color_organ_texture);
    sdlx_clear_texture(color_organ_texture, COLOR_BLACK);

    // get audio samples and perform fft
    sdlx_get_audio_samples(num_samples, num_downsample, GET_SAMPLES_MONO, samples);
    util_fft_real_to_real(num_samples, samples, fft, true);

    // handle COLOR_ORGAN_BARS and COLOR_ORGAN_CIRCLES
    if (which_color_organ == COLOR_ORGAN_BARS || which_color_organ == COLOR_ORGAN_CIRCLES) {
        // loop over the 3 bands
        for (int band = 0; band < 3; band++) {
            int   first_bin, last_bin;
            float raw_new_vol;

            // determine raw_new_vol for this band
            first_bin = nearbyint(band_start_freq[band] / delta_f);
            last_bin = nearbyint(band_end_freq[band] / delta_f);
            raw_new_vol = util_rms_float(&fft[first_bin], last_bin-first_bin+1);// xxx picoc isue requires &fft[first_bin]

            // apply scale factor
            raw_new_vol *= band_scale[band];
            if (raw_new_vol > 1) raw_new_vol = 1;

            // apply filter
            filter(&filtered_vol[band], raw_new_vol);

            // display the band
            display_band(band, filtered_vol[band]);
        }
    } else {  // handle COLOR_ORGAN_FFT
        color_organ_display_fft(fft);
    }

    // restore render target to the display
    sdlx_set_render_target(NULL);

    // copy the color_organ_texture to the display, based on the caller provided display orientation
    if (orientation == VERTICAL) {
        sdlx_render_texture(color_organ_texture, 0, 0);
    } else {
        // xxx comment
        float scale = 1000.0 / COH;
        int new_w = nearbyint(1000 * scale);
        int new_h = nearbyint(COH * scale);
        sdlx_render_texture_ex3(color_organ_texture, 
                                0, 0.5*sdlx_win_height - new_w/2, new_w, new_h,
                                90, new_h/2, new_h/2);
    }
}

void color_organ_display_fft(float *fft)
{
    int          x, y, w, h, wavelength;
    sdlx_color_t color;
    float        raw_scaled;
    static float filtered[MAX_FFT];

    // visible light range is 380 (violet) to 750 (red) nm;
    // wavelength variable range is from 699 (red) down to 400 (violet)

    // loop over the fft output array
    // xxx comments
    for (int i = 1; i < MAX_FFT; i++) {
        raw_scaled = fft[i] * fft_scale; 
        if (raw_scaled > 1) raw_scaled = 1;

        filter(&filtered[i], raw_scaled);

        x = 3*(i-1) + 50;
        y = COH - COH * filtered[i];
        w = 3;
        h = COH * filtered[i];

        wavelength = 700 - i;
        color = sdlx_wavelength_to_color(wavelength);
        sdlx_render_fill_rect(x, y, w, h, color);
    }
}

void filter(float *val, float new_val)
{
    if (which_filter == FILTER_NONE) {
        *val = new_val;
    } else if (which_filter == FILTER_EXP_SMOOTH) {
        *val = *val + exp_fltr_k * (new_val - *val);
    } else if (which_filter == FILTER_SNAP) {
        if (new_val > *val) {
            *val = new_val;
        } else {
            *val -= snap_fltr_decay;
            if (*val < new_val) *val = new_val;
        }
    } else {
        *val = 0;
    }
}

// this routine is called only for COLOR_ORGAN_BARS and COLOR_ORGAN_CIRCLES
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

        // Note that the value of 'k' was determined empirically 
        // to give good visual result of the circle intensities.
        float k = 2.0;

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
    case EVID_LOW_BAND_INCREASE:
    case EVID_LOW_BAND_DECREASE:
        if (disp_scale_factor == 0) {
            disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
            break;
        }
        band_scale[LOW_BAND] += (ev->event_id == EVID_LOW_BAND_INCREASE ? 5 : -5);
        if (band_scale[LOW_BAND] < 5) band_scale[LOW_BAND] = 5;
        disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
        util_set_numeric_param(data_dir, "low_band_scale", band_scale[LOW_BAND]);
        break;
    case EVID_MID_BAND_INCREASE:
    case EVID_MID_BAND_DECREASE:
        if (disp_scale_factor == 0) {
            disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
            break;
        }
        band_scale[MID_BAND] += (ev->event_id == EVID_MID_BAND_INCREASE ? 5 : -5);
        if (band_scale[MID_BAND] < 5) band_scale[MID_BAND] = 5;
        disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
        util_set_numeric_param(data_dir, "mid_band_scale", band_scale[MID_BAND]);
        break;
    case EVID_HIGH_BAND_INCREASE:
    case EVID_HIGH_BAND_DECREASE:
        if (disp_scale_factor == 0) {
            disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
            break;
        }
        band_scale[HIGH_BAND] += (ev->event_id == EVID_HIGH_BAND_INCREASE ? 5 : -5);
        if (band_scale[HIGH_BAND] < 5) band_scale[HIGH_BAND] = 5;
        disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
        util_set_numeric_param(data_dir, "high_band_scale", band_scale[HIGH_BAND]);
        break;
    case EVID_FFT_INCREASE:
    case EVID_FFT_DECREASE:
        if (disp_scale_factor == 0) {
            disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
            break;
        }
        fft_scale += (ev->event_id == EVID_FFT_INCREASE ? 5 : -5);
        if (fft_scale < 5) fft_scale = 5;
        disp_scale_factor = DISP_SCALE_FACTOR_DURATION;
        util_set_numeric_param(data_dir, "fft_scale", fft_scale);
        break;
    }
}

void color_organ_register_events(int y_controls_2)
{
    sdlx_loc_t loc;
    int flags = FLAG_XY_CTR | (orientation == HORIZONTAL ? FLAG_ROT_90 : 0);

    // xxx comment
    if (orientation == VERTICAL || show_horizontal) {
        reg_event(COL2X(0), y_controls_2, COLOR_LIGHT_BLUE,
                  color_organ_name[which_color_organ], EVID_COLOR_ORGAN_SLCT);
        reg_event(COL2X(5), y_controls_2, COLOR_LIGHT_BLUE, filter_name[which_filter], EVID_FILTER_SLCT);
    }

    // xxx comment
    if (which_color_organ == COLOR_ORGAN_BARS) {
        for (int band = 0; band < 3; band++) {
            init_loc(&loc, 333*band, 0, 333, COH/2);
            sdlx_register_event(&loc, band == 0 ? EVID_LOW_BAND_INCREASE : (band == 1 ? EVID_MID_BAND_INCREASE : EVID_HIGH_BAND_INCREASE));
            init_loc(&loc, 333*band, COH/2, 333, COH/2);
            sdlx_register_event(&loc, band == 0 ? EVID_LOW_BAND_DECREASE : (band == 1 ? EVID_MID_BAND_DECREASE : EVID_HIGH_BAND_DECREASE));
            if (disp_scale_factor) {
                init_loc(&loc, 333*band+166, COH/2, 0, 0);
                sdlx_render_printf_ex2(loc.x, loc.y,
                                       FONT_NORMAL, COLOR_WHITE, flags, WRAP_NONE,
                                       "%d", band_scale[band]);
            }
        }
    } else if (which_color_organ == COLOR_ORGAN_CIRCLES) {
        int r = circle_radius;
        int x_ctr[3] = {500-r, 500+r, 500};
        int y_ctr[3] = {COH-r, COH-r, r};

        for (int band = 0; band < 3; band++) {
            init_loc(&loc, x_ctr[band]-r/2, y_ctr[band]-r, r, r);
            sdlx_register_event(&loc, band == 0 ? EVID_LOW_BAND_INCREASE : (band == 1 ? EVID_MID_BAND_INCREASE : EVID_HIGH_BAND_INCREASE));
            init_loc(&loc, x_ctr[band]-r/2, y_ctr[band], r, r);
            sdlx_register_event(&loc, band == 0 ? EVID_LOW_BAND_DECREASE : (band == 1 ? EVID_MID_BAND_DECREASE : EVID_HIGH_BAND_DECREASE));
            if (disp_scale_factor) {
                init_loc(&loc, x_ctr[band], y_ctr[band], 0, 0);
                sdlx_render_printf_ex2(loc.x, loc.y,
                                       FONT_NORMAL, COLOR_WHITE, flags, WRAP_NONE,
                                       "%d", band_scale[band]);
            }
        }
    } else {  // which_color_organ == COLOR_ORGAN_FFT
        init_loc(&loc, 0, 0, 1000, COH/2);
        sdlx_register_event(&loc, EVID_FFT_INCREASE);
        init_loc(&loc, 0, COH/2, 1000, COH/2);
        sdlx_register_event(&loc, EVID_FFT_DECREASE);

        if (disp_scale_factor) {
            init_loc(&loc, 500, COH/2, 0, 0);
            sdlx_render_printf_ex2(loc.x, loc.y,
                                   FONT_NORMAL, COLOR_WHITE, flags, WRAP_NONE,
                                   "%d", fft_scale);
        }
    }

    // stop displaying the scale values after a short time interval
    if (disp_scale_factor) {
        disp_scale_factor--;
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
    band_scale[LOW_BAND]  = util_get_numeric_param(data_dir, "low_band_scale",    DFLT_LOW_BAND_SCALE);
    band_scale[MID_BAND]  = util_get_numeric_param(data_dir, "mid_band_scale",    DFLT_MID_BAND_SCALE);
    band_scale[HIGH_BAND] = util_get_numeric_param(data_dir, "high_band_scale",   DFLT_HIGH_BAND_SCALE);
    fft_scale             = util_get_numeric_param(data_dir, "fft_scale",         DFLT_FFT_SCALE);
    which_color_organ     = util_get_numeric_param(data_dir, "color_organ",       DFLT_COLOR_ORGAN);
    which_filter          = util_get_numeric_param(data_dir, "filter",            DFLT_FILTER);
    exp_fltr_k            = util_get_numeric_param(data_dir, "exp_fltr_k",        DFLT_EXP_FLTR_K);
    snap_fltr_decay       = util_get_numeric_param(data_dir, "snap_fltr_decay",   DFLT_SNAP_FLTR_K);
}

void settings_reset_to_dflt(void)
{
    util_set_numeric_param(data_dir, "low_band_scale",    DFLT_LOW_BAND_SCALE);
    util_set_numeric_param(data_dir, "mid_band_scale",    DFLT_MID_BAND_SCALE);
    util_set_numeric_param(data_dir, "high_band_scale",   DFLT_HIGH_BAND_SCALE);
    util_set_numeric_param(data_dir, "fft_scale",         DFLT_FFT_SCALE);
    util_set_numeric_param(data_dir, "color_organ",       DFLT_COLOR_ORGAN);
    util_set_numeric_param(data_dir, "filter",            DFLT_FILTER);
    util_set_numeric_param(data_dir, "exp_fltr_k",        DFLT_EXP_FLTR_K);
    util_set_numeric_param(data_dir, "snap_fltr_decay",   DFLT_SNAP_FLTR_K);

    settings_init();
}

#define EVID_SETTINGS_RESET             2001
#define EVID_SETTINGS_CREATE_TEST_FILES 2002
#define EVID_SETTINGS_EXP_FLTR_K        2003
#define EVID_SETTINGS_SNAP_FLTR_DECAY   2004

void color_organ_settings(void)
{
    bool         done = false;
    sdlx_event_t event;
    int          y;
    sdlx_loc_t  *loc;
    float        val;
    char        *str;

    while (!done) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK);

        // display settings
        sdlx_print_set_default(24, COLOR_WHITE);
        y = 0;
        sdlx_render_printf(0, y, "%-15s = %d",    "low_band_scale",   band_scale[LOW_BAND]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %d",    "mid_band_scale",   band_scale[MID_BAND]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %d",    "high_band_scale",  band_scale[HIGH_BAND]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %d",    "fft_scale",        fft_scale);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %s",    "color_organ",      color_organ_name[which_color_organ]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %s",    "filter",           filter_name[which_filter]);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %0.3f", "exp_fltr_k",       exp_fltr_k);
        y += sdlx_char_height_dflt;
        sdlx_render_printf(0, y, "%-15s = %0.3f", "snap_fltr_decay",  snap_fltr_decay);
        y += 2*sdlx_char_height_dflt;
        sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

        // register events
        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "RESET");
        sdlx_register_event(loc, EVID_SETTINGS_RESET);
        y += 2*sdlx_char_height_dflt;

        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "CREATE_TEST_FILES");
        sdlx_register_event(loc, EVID_SETTINGS_CREATE_TEST_FILES);
        y += 2*sdlx_char_height_dflt;

        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "EXP_FLTR_K");
        sdlx_register_event(loc, EVID_SETTINGS_EXP_FLTR_K);
        y += 2*sdlx_char_height_dflt;

        loc = sdlx_render_printf_ex1(0, y, FONT_NORMAL, COLOR_LIGHT_BLUE, "SNAP_FLTR_DECAY");
        sdlx_register_event(loc, EVID_SETTINGS_SNAP_FLTR_DECAY);
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

            // create test files for the color organ bands
            sdlx_create_test_file(files_dir, "test_low.mp3",
                                  band_start_freq[LOW_BAND], band_end_freq[LOW_BAND], 10);
            sdlx_create_test_file(files_dir, "test_mid.mp3",
                                  band_start_freq[MID_BAND], band_end_freq[MID_BAND], 10);
            sdlx_create_test_file(files_dir, "test_high.mp3",
                                  band_start_freq[HIGH_BAND], band_end_freq[HIGH_BAND], 10);
            sdlx_create_test_file(files_dir, "test_bands.mp3",
                                  band_start_freq[LOW_BAND], band_end_freq[HIGH_BAND], 10);

            // create test file for the fft span
            sdlx_create_test_file(files_dir, "test_span.mp3", 100, 6000, 10);
            break;
        case EVID_SETTINGS_EXP_FLTR_K:
            str = sdlx_get_input_str("exp_fltr_k", "", true, COLOR_BLACK);
            if (sscanf(str, "%f", &val) != 1) break;
            exp_fltr_k = val;
            util_set_numeric_param(data_dir, "exp_fltr_k", exp_fltr_k);
            break;
        case EVID_SETTINGS_SNAP_FLTR_DECAY:
            str = sdlx_get_input_str("snap_fltr_decay", "", true, COLOR_BLACK);
            if (sscanf(str, "%f", &val) != 1) break;
            snap_fltr_decay = val;
            util_set_numeric_param(data_dir, "snap_fltr_decay", snap_fltr_decay);
            break;
        case EVID_QUIT:
            done = true;
            break;
        }
    }
}

