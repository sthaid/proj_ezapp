#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

// xxx
// - handle other orientations

// defines
#define RAD_TO_DEG (180 / M_PI)
#define DEG_TO_RAD (M_PI / 180)

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
void horizontal(double ax, double ay, double az);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int          rc;
    double       ax_raw, ay_raw, az_raw;
    double       ax=INVALID_NUMBER, ay=INVALID_NUMBER, az=INVALID_NUMBER;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO|SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // create textures
    green_circle      = sdlx_create_filled_circle_texture(50, COLOR_GREEN);
    blue_circle       = sdlx_create_filled_circle_texture(50, COLOR_BLUE);
    red_circle        = sdlx_create_filled_circle_texture(50, COLOR_RED);
    gray_circle       = sdlx_create_filled_circle_texture(500, COLOR_GRAY);
    light_gray_circle = sdlx_create_filled_circle_texture(500, COLOR_LIGHT_GRAY);

    // use default font size and color
    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

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

        // determine orientation
        // xxx todo

        // if orientation is horizontal ..
        horizontal(ax, ay, az);
    }

    // free allocations
    sdlx_destroy_texture(green_circle);
    sdlx_destroy_texture(blue_circle);
    sdlx_destroy_texture(red_circle);
    sdlx_destroy_texture(gray_circle);
    sdlx_destroy_texture(light_gray_circle);

    // quit sdl subsystems and end program
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_SENSOR);
    printf("INFO %s: terminating\n", progname);
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
    sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);
    sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height/2, "No Accelerometer");
    sdlx_display_present();

    sdlx_get_event(100000, &event);  // 100 ms timeout
    switch (event.event_id) {
    case EVID_QUIT:
        end_program = true;
        break;
    }
}

// -----------------  HORIZONTAL  ---------------------------

#define EVID_INCR_MAX_BULLS_EYE 1
#define EVID_DECR_MAX_BULLS_EYE 2

#define MAX_BULLS_EYE_DEFAULT 5

void horizontal(double ax, double ay, double az)
{
    int                 width, xctr, yctr, deg;
    sdlx_texture_t     *t;
    double              tilt_dir, tilt_amount;
    sdlx_event_t        event;
    sdlx_print_state_t  print_state;

    static int          max_bulls_eye = -1;

    // if max_bulls_eye param has not been read, then do so
    if (max_bulls_eye == -1) {
        max_bulls_eye = util_get_numeric_param(data_dir, "max_bulls_eye", MAX_BULLS_EYE_DEFAULT);
    }

    // init the backbuffer
    sdlx_display_init(COLOR_BLACK);

    // register control event to
    // - end program
    // - adjust max_bulls_eye
    sdlx_register_control_events(
            "-", "+", "X", COLOR_WHITE, COLOR_BLACK, 
            EVID_DECR_MAX_BULLS_EYE, EVID_INCR_MAX_BULLS_EYE, EVID_QUIT);

    // init center location of the bulls-eye
    xctr = sdlx_win_width/2;
    yctr = sdlx_win_height/2;

    // draw bulls-eye
    t = gray_circle;
    for (deg = max_bulls_eye; deg >= 1; deg--) {
        width = nearbyint((double)sdlx_win_width / max_bulls_eye * deg);
        t = (t == gray_circle ? light_gray_circle : gray_circle);
        sdlx_render_texture(xctr-width/2, yctr-width/2, width, width, t);
    }

    // calculate tilt amount and direction
    tilt_dir    = atan2(ax, ay) * RAD_TO_DEG;
    tilt_amount = atan( sqrt(ax*ax + ay*ay) / az ) * RAD_TO_DEG;
    
    // display ...
    // - tilt_amount
    sdlx_print_save(&print_state);
    sdlx_print_init(LARGE_FONT, COLOR_WHITE, COLOR_BLACK);  // xxx add new sdlx routine to just choose the font size
    sdlx_render_printf_xyctr(
            xctr,
            yctr - sdlx_win_width/2 - ROW2Y(1.5),
            "%0.2f",
            tilt_amount);
    sdlx_print_restore(&print_state);
    // - max_bulls_eye (degrees)
    sdlx_render_printf_xyctr(sdlx_win_width/2, sdlx_win_height-ROW2Y(3), "max %d deg", max_bulls_eye);

    // limit tilt amount to the max that can be displayed on the bulls-eye
    if (tilt_amount > max_bulls_eye) {
        tilt_amount = max_bulls_eye;
    }

    // display small circle on the bulls-eye pattern, 
    // at location indicating the tilt direction and amount
    double dx, dy;
    int x, y, small_circle_diameter;

    dx = tilt_amount * sin(tilt_dir*DEG_TO_RAD) * ((double)(sdlx_win_width/2) / max_bulls_eye);
    dy = tilt_amount * cos(tilt_dir*DEG_TO_RAD) * ((double)(sdlx_win_width/2) / max_bulls_eye);
    x = nearbyint(xctr  + dx);
    y = nearbyint(yctr - dy);

    small_circle_diameter = 50;

    t = ((fabs(tilt_amount) < 0.2)            ? green_circle :
         ((fabs(tilt_amount) < max_bulls_eye) ? blue_circle :
                                                red_circle));

    sdlx_render_texture(x-small_circle_diameter/2, 
                        y-small_circle_diameter/2, 
                        small_circle_diameter, 
                        small_circle_diameter, 
                        t);

    // display dot at center of bulls_eye
    sdlx_render_point(xctr, yctr, COLOR_BLACK, 9);

    // present the display
    sdlx_display_present();

    // wait for event, with 100ms timeout
    sdlx_get_event(100000, &event);

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
