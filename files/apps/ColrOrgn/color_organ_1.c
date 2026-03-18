#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/ColrOrgn/common.h"

//
// defines
//

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

//
// variables
//

int k_red   = DEFAULT_K_RED;
int k_green = DEFAULT_K_GREEN;
int k_blue  = DEFAULT_K_BLUE;
int disp_k  = 0;

sdlx_texture_t *red_circle_texture;
sdlx_texture_t *green_circle_texture;
sdlx_texture_t *blue_circle_texture;

//
// prototypes
//

void proc(sdlx_texture_t *t, int x_ctr, int y_ctr, double newval, double *work, int k);  // xxx name0

void init_loc(sdlx_loc_t *loc, int x, int y, int w, int h);
sdlx_texture_t *create_circle_texture(sdlx_color_t color);

// -----------------  INIT & CLEANUP  --------------------------------

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

// -----------------  COLOR ORGAN DISPLAY  ---------------------------

// xxx make 'as' a glbl ptr
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

// -----------------  COLOR ORGAN EVENT HANDLING  --------------------

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
