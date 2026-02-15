#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>

#include <sdlx.h>
#include <utils.h>

// defines
#define RAD_TO_DEG (180 / M_PI)
#define DEG_TO_RAD (M_PI / 180)
#define TWO_PI     (2 * M_PI)

#define SMALL_CIRCLE_RADIUS 25
#define LARGE_CIRCLE_RADIUS 500

#define VERT_TEXTURE_WH 1000

#define TILT_CLOSE_TO_ZERO 0.2

// variables
char *progname;
char *data_dir;

bool end_program = false;

sdlx_texture_t *green_circle;
sdlx_texture_t *blue_circle;
sdlx_texture_t *red_circle;
sdlx_texture_t *gray_circle;
sdlx_texture_t *light_gray_circle;
sdlx_texture_t *vert;
    
// prototypes
void smooth(double newval, double *smoothed);
void no_accelerometer(void);
sdlx_texture_t *create_filled_circle_texture(int radius, sdlx_color_t color);
void display_tilt_horizontal(double ax, double ay, double az);
void display_tilt_vertical(double ax, double ay, double az, double roll, double pitch);

// -----------------  MAIN  ------------------------------------------
    
int main(int argc, char **argv)
{
    int    rc;
    double ax_raw, ay_raw, az_raw;
    double ax=INVALID_NUMBER, ay=INVALID_NUMBER, az=INVALID_NUMBER;
    double roll_raw, pitch_raw;
    double roll=INVALID_NUMBER, pitch=INVALID_NUMBER;

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

    // create textures for horizontal display
    green_circle      = create_filled_circle_texture(SMALL_CIRCLE_RADIUS, COLOR_GREEN);
    blue_circle       = create_filled_circle_texture(SMALL_CIRCLE_RADIUS, COLOR_BLUE);
    red_circle        = create_filled_circle_texture(SMALL_CIRCLE_RADIUS, COLOR_RED);
    gray_circle       = create_filled_circle_texture(LARGE_CIRCLE_RADIUS, COLOR_GRAY);
    light_gray_circle = create_filled_circle_texture(LARGE_CIRCLE_RADIUS, COLOR_LIGHT_GRAY);

    // create texture for vertical display
    vert = sdlx_create_texture(VERT_TEXTURE_WH, VERT_TEXTURE_WH);

    // set default font size and color
    sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);

#ifndef ANDROID
    // for testing on Linux
    while (!end_program) {
        display_tilt_vertical(3,9,0);
    }
    return 1;
#endif

    // runtime loop
    while (!end_program) {
        // obtain smoothed accelerometer values
        rc = sdlx_sensor_read_accelerometer(&ax_raw, &ay_raw, &az_raw);
        if (rc != 0) {
            no_accelerometer();
            continue;
        }
        smooth(ax_raw, &ax);
        smooth(ay_raw, &ay);
        smooth(az_raw, &az);

        // obtain smoothed roll/pitch values, units=degrees
        sdlx_sensor_read_roll_pitch(&roll_raw, &pitch_raw);
        smooth(roll_raw, &roll);
        smooth(pitch_raw, &pitch);

        // display the tilt, using the accelerometer values
        if (az > 7) {
            display_tilt_horizontal(ax, ay, az);
        } else {
            display_tilt_vertical(ax, ay, az, roll, pitch);
        }
    }

    // free allocations
    sdlx_destroy_texture(green_circle);
    sdlx_destroy_texture(blue_circle);
    sdlx_destroy_texture(red_circle);
    sdlx_destroy_texture(gray_circle);
    sdlx_destroy_texture(light_gray_circle);
    sdlx_destroy_texture(vert);

    // quit sdl subsystems and end program
    sdlx_quit(SUBSYS_VIDEO|SUBSYS_SENSOR);
    printf("I %s: terminating\n", progname);
    return 0;
}

#define SMOOTH_K 0.985
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

// -----------------  DISPLAY TILT - HORIZONTAL ORIENTATION  -------------------

#define EVID_INCR_MAX_BULLS_EYE 1
#define EVID_DECR_MAX_BULLS_EYE 2

#define MAX_BULLS_EYE_DEFAULT 5

void display_tilt_horizontal(double ax, double ay, double az)
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
                           "%0.1f", tilt_amount);
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

    t = ((fabs(tilt_amount) < TILT_CLOSE_TO_ZERO) ? green_circle :
        ((fabs(tilt_amount) < max_bulls_eye)      ? blue_circle :
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

// -----------------  DISPLAY TILT - VERTICAL ORIENTATION  -------------------

#define EVID_MINUS 11
#define EVID_PLUS  12

#define Y_OFFSET             30
#define CHORD_LEN            850
#define MAX_ARC              100
#define ARC_SPAN_DEG_DEFAULT 20

void display_tilt_vertical(double ax, double ay, double az, double roll, double pitch)
{
    int             rotate_deg;
    double          angle_deg, angle_rad;
    sdlx_event_t    event;
    sdlx_point_t    points[MAX_ARC];
    int             i, j, x, y, x_offset;
    int             tick_deg, tick_delta_deg;
    sdlx_texture_t *vert_loc;

    // statics
    static double       arc_span_deg, arc_span_rad, arc_radius, arc_radius_squared;
    static double       arc_initialized;
    static sdlx_point_t arc[MAX_ARC];
    static int          max_arc;

    // if arc_span_deg param has not been read, then do so
    if (arc_span_deg == 0) {
        arc_span_deg = util_get_numeric_param(data_dir, "arc_span_deg", ARC_SPAN_DEG_DEFAULT);
        arc_span_rad = arc_span_deg * DEG_TO_RAD;
    }

    // init the backbuffer
    sdlx_display_init(COLOR_BLACK);

    // set render target to the rendering texture, and
    // clear the rendering texture
    sdlx_set_render_target(vert);
    sdlx_clear_texture(vert, COLOR_BLACK);

    // determine the rotation needed for the device orientation
    if (fabs(ay) > fabs(ax)) {
        rotate_deg = (ay > 0 ? 0 : 180);
    } else {
        rotate_deg = (ax > 0 ? 90 : 270);
    }

    // determine the tilt angle;
    // the roll/pitch values give good results even when the device 
    // orientation deviates from vertical
    switch (rotate_deg) {
    case 0:   angle_deg = -roll;  break;
    case 90:  angle_deg = -pitch; break;
    case 180: angle_deg =  roll;  break;
    case 270: angle_deg =  pitch; break;
    }
    angle_rad = angle_deg * DEG_TO_RAD;

    // initialize arc points for the current selected arc_span
    x_offset = (sdlx_win_width - CHORD_LEN) / 2;
    if (arc_initialized != arc_span_rad) {
        printf("I %s: initializing arc points for span %0.0f\n", progname, arc_span_deg);
        arc_radius = (CHORD_LEN/2) / sin(arc_span_rad/2);
        arc_radius_squared = arc_radius * arc_radius;
        max_arc = 0;
        for (x = -CHORD_LEN/2; x <= CHORD_LEN/2; x+= 10) {
            y = sqrt(arc_radius_squared - x*x);
            arc[max_arc].x = x + CHORD_LEN/2 + x_offset;
            arc[max_arc].y = arc_radius - y + Y_OFFSET;
            max_arc++;
            if (max_arc == MAX_ARC) {
                printf("E %s: too many arc points\n", progname);
                break;
            }
        }
        arc_initialized = arc_span_rad;
    }

    // draw arc, made up of 5 arc lines for a better display appearance
    for (i = -2; i <= 2; i++) {
        for (j = 0; j < max_arc; j++) {
            points[j].x = arc[j].x;
            points[j].y = arc[j].y + i;
        }
        sdlx_render_lines(points, max_arc, COLOR_WHITE);
    }

    // draw small cirle on the arc, at the vertical location;
    // use green circle when within 0.2 degrees of arc center
    vert_loc = (fabs(angle_deg) < 0.2) ? green_circle : blue_circle;
    x = arc_radius * sin(angle_rad) + CHORD_LEN / 2 + x_offset;
    y = arc_radius - arc_radius * cos(angle_rad) + Y_OFFSET;
    sdlx_render_texture(vert_loc, x-SMALL_CIRCLE_RADIUS, y-SMALL_CIRCLE_RADIUS);

    // print tilt angle at both ends of the arc, and at arc center
    x = arc_radius * sin(-arc_span_rad/2) + CHORD_LEN / 2 + x_offset;
    y = arc_radius - arc_radius * cos(-arc_span_rad/2) + sdlx_char_height(FONT_SMALL);
    sdlx_render_printf_ex2(x, y, FONT_SMALL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "%g", -arc_span_deg/2);

    x = arc_radius * sin(arc_span_rad/2) + CHORD_LEN / 2 + x_offset;
    y = arc_radius - arc_radius * cos(arc_span_rad/2) + sdlx_char_height(FONT_SMALL);
    sdlx_render_printf_ex2(x, y, FONT_SMALL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "%g", arc_span_deg/2);

    x = arc_radius * sin(0) + CHORD_LEN / 2 + x_offset;
    y = arc_radius - arc_radius * cos(0) + sdlx_char_height(FONT_SMALL);
    sdlx_render_printf_ex2(x, y, FONT_SMALL, COLOR_WHITE, FLAG_X_CTR, WRAP_NONE, "0");

    // add interval marks on the arc
    tick_delta_deg = (arc_span_deg <= 5  ? 1 :
                     (arc_span_deg <= 20 ? 5   
                                         : 10));
    for (tick_deg = 0; tick_deg <= arc_span_deg/2; tick_deg += tick_delta_deg) {
        double tick_rad = tick_deg * DEG_TO_RAD;

        x = arc_radius * sin(tick_rad) + CHORD_LEN / 2 + x_offset;
        y = arc_radius - arc_radius * cos(tick_rad);
        sdlx_render_point(x, y+Y_OFFSET, COLOR_PINK, MAX_POINT_SIZE);

        x = arc_radius * sin(-tick_rad) + CHORD_LEN / 2 + x_offset;
        y = arc_radius - arc_radius * cos(tick_rad);
        sdlx_render_point(x, y+Y_OFFSET, COLOR_PINK, MAX_POINT_SIZE);
    }

    // print the tilt angle at the center of the rendering texture
    sdlx_render_printf_ex2(VERT_TEXTURE_WH/2, VERT_TEXTURE_WH/2, FONT_LARGE, COLOR_WHITE,
                           FLAG_XY_CTR, WRAP_NONE, "%0.1f", angle_deg);

    // set render target back to the display
    sdlx_set_render_target(NULL);

    // render the rendering texture to the display, centered and rotated
    x = 0;
    y = (sdlx_win_height - CONTROL_EVENTS_DISPLAY_HEIGHT - VERT_TEXTURE_WH) / 2;
    sdlx_render_texture_ex2(vert, x, y, VERT_TEXTURE_WH, VERT_TEXTURE_WH, rotate_deg);

    // register control event to adjust the arc span and end-program
    sdlx_register_control_events(EVID_MINUS, "-",
                                 EVID_PLUS, "+",
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);

    // present the display
    sdlx_display_present();

    // wait for event, with 10 ms timeout
    sdlx_get_event(10000, &event);

    // process events
    switch (event.event_id) {
    case EVID_MINUS:
        arc_span_deg -= 5;
        if (arc_span_deg < 5) arc_span_deg = 5;
        arc_span_rad = arc_span_deg * DEG_TO_RAD;
        util_set_numeric_param(data_dir, "arc_span_deg", arc_span_deg);
        break;
    case EVID_PLUS:
        arc_span_deg += 5;
        if (arc_span_deg > 90) arc_span_deg = 90;
        arc_span_rad = arc_span_deg * DEG_TO_RAD;
        util_set_numeric_param(data_dir, "arc_span_deg", arc_span_deg);
        break;
    case EVID_QUIT:
        end_program = true;
        break;
    }
}
