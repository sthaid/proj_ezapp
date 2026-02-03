#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define RAD_TO_DEG (180 / M_PI)
#define DEG_TO_RAD (M_PI / 180)

#define SMALL_CIRCLE_RADIUS 25
#define LARGE_CIRCLE_RADIUS 500

// variables
char *progname;
char *data_dir;

bool end_program = false;

sdlx_texture_t *green_circle;
sdlx_texture_t *blue_circle;
sdlx_texture_t *red_circle;
sdlx_texture_t *gray_circle;
sdlx_texture_t *light_gray_circle;
    
// prototypes
void smooth(double newval, double *smoothed);
void no_accelerometer(void);
sdlx_texture_t *create_filled_circle_texture(int radius, sdlx_color_t color);
void display_tilt(double ax, double ay, double az);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int    rc;
    double ax_raw, ay_raw, az_raw;
    double ax=INVALID_NUMBER, ay=INVALID_NUMBER, az=INVALID_NUMBER;

    // set line buffering
    setlinebuf(stdout);

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_SENSOR);
    if (rc != 0) {
        printf("E %s: sdlx_init failed\n", progname);
        return 1;
    }

    // create textures
    green_circle      = create_filled_circle_texture(SMALL_CIRCLE_RADIUS, COLOR_GREEN);
    blue_circle       = create_filled_circle_texture(SMALL_CIRCLE_RADIUS, COLOR_BLUE);
    red_circle        = create_filled_circle_texture(SMALL_CIRCLE_RADIUS, COLOR_RED);
    gray_circle       = create_filled_circle_texture(LARGE_CIRCLE_RADIUS, COLOR_GRAY);
    light_gray_circle = create_filled_circle_texture(LARGE_CIRCLE_RADIUS, COLOR_LIGHT_GRAY);

    // use normal font size and color white
    sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

    // runtime loop
    while (!end_program) {
        // read accelerometer values
        rc = sdlx_sensor_read_accelerometer(&ax_raw, &ay_raw, &az_raw);
        if (rc != 0) {
            no_accelerometer();
            continue;
        }

        // smooth accelerometer values
        smooth(ax_raw, &ax);
        smooth(ay_raw, &ay);
        smooth(az_raw, &az);

        // display the tilt, using the accelerometer values
        display_tilt(ax, ay, az);
    }

    // free allocations
    sdlx_destroy_texture(green_circle);
    sdlx_destroy_texture(blue_circle);
    sdlx_destroy_texture(red_circle);
    sdlx_destroy_texture(gray_circle);
    sdlx_destroy_texture(light_gray_circle);

    // quit sdl subsystems and end program
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_SENSOR);
    printf("I %s: terminating\n", progname);
    return 0;
}

#define SMOOTH_K 0.9
void smooth(double newval, double *smoothed)
{
    if (*smoothed == INVALID_NUMBER) {
        *smoothed = newval;
        return;
    }

    *smoothed = SMOOTH_K * *smoothed + (1.0 - SMOOTH_K) * newval;
}

void no_accelerometer(void)
{
    sdlx_event_t event;

    sdlx_display_init(COLOR_BLACK);
    sdlx_register_control_events(0, NULL,
                                 0, NULL,
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
    sdlx_render_printf_ex2(sdlx_win_width/2, sdlx_win_height/2, 
                           FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, WRAP_NONE,
                           "%s", "No Accelerometer");
    sdlx_display_present();

    sdlx_get_event(-1, &event);  // infinite timeout
    switch (event.event_id) {
    case EVID_QUIT:
        end_program = true;
        break;
    }
}

sdlx_texture_t *create_filled_circle_texture(int radius, sdlx_color_t color)
{
    int w, h;
    sdlx_texture_t *t;

    w = h = 2 * radius;

    t = sdlx_create_texture(w, h);
    sdlx_set_render_target(t);
    sdlx_render_fill_circle(w/2, h/2, radius, color);
    sdlx_set_render_target(NULL);

    return t;
}

// -----------------  DISPLAY TILT  -------------------------

#define EVID_INCR_MAX_BULLS_EYE 1
#define EVID_DECR_MAX_BULLS_EYE 2

#define MAX_BULLS_EYE_DEFAULT 5

void display_tilt(double ax, double ay, double az)
{
    int                 diameter, xctr, yctr, deg;
    sdlx_texture_t     *t;
    double              tilt_dir, tilt_amount;
    sdlx_event_t        event;

    static int          max_bulls_eye = -1;

    // if max_bulls_eye param has not been read, then do so
    if (max_bulls_eye == -1) {
        max_bulls_eye = util_get_numeric_param(data_dir, "max_bulls_eye", MAX_BULLS_EYE_DEFAULT);
    }

    // init the backbuffer
    sdlx_display_init(COLOR_BLACK);

    // init center location of the bulls-eye
    xctr = sdlx_win_width/2;
    yctr = sdlx_win_height/2;

    // draw bulls-eye
    t = gray_circle;
    for (deg = max_bulls_eye; deg >= 1; deg--) {
        diameter = nearbyint((double)sdlx_win_width / max_bulls_eye * deg);
        t = (t == gray_circle ? light_gray_circle : gray_circle);
        sdlx_render_texture_ex1(t, xctr-diameter/2, yctr-diameter/2, diameter, diameter);
    }

    // calculate tilt amount and direction
    tilt_dir    = atan2(ax, ay) * RAD_TO_DEG;
    tilt_amount = atan( sqrt(ax*ax + ay*ay) / az ) * RAD_TO_DEG;
    //printf("I %s: tilt dir = %0.1f amount = %0.1f\n", progname, tilt_dir, tilt_amount);
    
    // display ...
    // - tilt_amount
    diameter = sdlx_win_width;
    sdlx_render_printf_ex2(xctr, yctr - diameter/2 - 1.5 * sdlx_char_height(FONT_LARGE),
                           FONT_LARGE, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE,
                           "%0.2f", tilt_amount);
    // - max_bulls_eye (degrees)
    sdlx_render_printf_ex2(sdlx_win_width/2, sdlx_win_height-CONTROL_EVENTS_DISPLAY_HEIGHT-2*sdlx_char_height(FONT_NORMAL),
                           FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE,
                           "max %d deg", max_bulls_eye);

    // limit tilt amount to the max that can be displayed on the bulls-eye
    if (tilt_amount > max_bulls_eye) {
        tilt_amount = max_bulls_eye;
    }

    // display small circle on the bulls-eye pattern, 
    // at location indicating the tilt direction and amount
    double dx, dy;
    int x, y;

    dx = tilt_amount * sin(tilt_dir*DEG_TO_RAD) * ((double)(sdlx_win_width/2) / max_bulls_eye);
    dy = tilt_amount * cos(tilt_dir*DEG_TO_RAD) * ((double)(sdlx_win_width/2) / max_bulls_eye);
    x = nearbyint(xctr  + dx);
    y = nearbyint(yctr - dy);

    t = ((fabs(tilt_amount) < 0.2)            ? green_circle :
         ((fabs(tilt_amount) < max_bulls_eye) ? blue_circle :
                                                red_circle));

    sdlx_render_texture(t, x-SMALL_CIRCLE_RADIUS, y-SMALL_CIRCLE_RADIUS);

    // display dot at center of bulls_eye
    sdlx_render_point(xctr, yctr, COLOR_BLACK, 9);

    // register control event to
    // - end program
    // - adjust max_bulls_eye
    sdlx_register_control_events(EVID_DECR_MAX_BULLS_EYE, "-",
                                 EVID_INCR_MAX_BULLS_EYE, "+",
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);

    // present the display
    sdlx_display_present();

    // wait for event, with 10 ms timeout
    // xxx trying 10 ms, was 100 ms
    sdlx_get_event(10000, &event);

    // process events
    switch (event.event_id) {
    case EVID_INCR_MAX_BULLS_EYE:
        if (max_bulls_eye < 20) {
            max_bulls_eye++;
            util_set_numeric_param(data_dir, "max_bulls_eye", max_bulls_eye);
        }
        break;
    case EVID_DECR_MAX_BULLS_EYE:
        if (max_bulls_eye > 1) {
            max_bulls_eye--;
            util_set_numeric_param(data_dir, "max_bulls_eye", max_bulls_eye);
        }
        break;
    case EVID_QUIT:
        end_program = true;
        break;
    }
}
