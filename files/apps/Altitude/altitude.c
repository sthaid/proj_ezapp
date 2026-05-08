#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <sdlx.h>
#include <utils.h>

#include "svcs/Altitude/altitude.h"

// defines
#define ONE_SEC         1000000
#define DO_NOT_CREATE   false
#define READ_ONLY       true

#define EVID_PREV         1
#define EVID_NEXT         2
#define EVID_TODAY        4
#define EVID_INCR_GRAPH_MAX 5
#define EVID_DECR_GRAPH_MAX 6
#define EVID_INCR_GRAPH_MIN 7
#define EVID_DECR_GRAPH_MIN 8

#define GRAPH_H            800
#define GRAPH_Y_TOP        600
#define GRAPH_Y_BOTTOM     (GRAPH_Y_TOP + GRAPH_H - 1)

#define DEFAULT_GRAPH_MIN_ALT 0
#define DEFAULT_GRAPH_MAX_ALT 1000
#define GRAPH_INCR_DECR_AMOUNT 1000

// typedefs
typedef struct {
    int graph_min;
    int graph_max;
} params_t;

// variables
char *progname;
char *data_dir;
bool  end_program;

altitude_file_t *altitude_file;
double           X;
params_t         params;
    
// prototypes
void draw_display(void);
void process_event(sdlx_event_t *event);

// prototypes for utils
char *get_month_str(int month);
char *get_weekday_str(int y, int m, int d);
bool is_weekend(int y, int m, int d);
int days_in_month(int y, int m);
void get_current_ymd(int *y, int *m, int *d);

// -----------------  MAIN  ------------------------------------------

int initialize(void);
void cleanup(void);

int main(int argc, char **argv)
{
    sdlx_event_t event;
    int rc;

    // save args
    progname = argv[0];
    if (argc != 2) {
        printf("E %s: data_dir arg expected\n", progname);
        return 1;
    }
    data_dir = argv[1];
    printf("I %s: starting, data_dir=%s\n", progname, data_dir);

    // initialize
    rc = initialize();
    if (rc != 0) {
        printf("E %s: initialize failed\n", progname);
        return 1;
    }

    // runtime loop
    while (!end_program) {
        // init the backbuffer
        sdlx_display_init(COLOR_BLACK, PORTRAIT);

        // draw display
        draw_display();

        // present the display
        sdlx_display_present();

        // wait for event, with timeout
        sdlx_get_event(ONE_SEC, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process events
        process_event(&event);
    }

    // cleanup
    cleanup();

    // end program
    printf("I %s: terminating\n", progname);
    return 0;
}

int initialize(void)
{
    //time_t       t;
    //struct tm    tm;
    //int          y, m, d;

    // get current date
    //get_current_ymd(&year, &month, &day);

    // map altitude.dat file
    altitude_file = util_map_file("svcs/Altitude", ALTITUDE_FILENAME, sizeof(altitude_file_t),
                               DO_NOT_CREATE, READ_ONLY, NULL);
    if (altitude_file == NULL) {
        printf("E %s: failed to map %s\n", progname, ALTITUDE_FILENAME);
        return -1;
    }

    // validate altitude.dat version
    if (altitude_file->version != ALTITUDE_FILE_VERSION) {
        printf("E %s: unsupported altitude.dat version=%lx, expected=%lx\n",
               progname, altitude_file->version, ALTITUDE_FILE_VERSION);
        return -1;
    }

    // read params
    params.graph_min  = util_get_numeric_param(data_dir, "graph_min",  DEFAULT_GRAPH_MIN_ALT);
    params.graph_max  = util_get_numeric_param(data_dir, "graph_max",  DEFAULT_GRAPH_MAX_ALT);

    // success
    return 0;
}

void cleanup(void)
{
    if (altitude_file) {
        util_unmap_file(altitude_file, sizeof(altitude_file_t));
        altitude_file = NULL;
    }
}

// -----------------  DRAW DISPLAY  ------------------------------------

void display_day_graph(int x, int y, int m, int d);
void reg_event(int x, int y, int evid, char *str);

void draw_display(void)
{
    double altitude_ft;
    bool   alt_is_wgs84;
    int    x, y, m, d;

    // display current altitude
    util_get_location(NULL, NULL, &altitude_ft, &alt_is_wgs84);
    sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(1), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                           "Current");
    if (altitude_ft == INVALID_NUMBER) {
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(2), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                               "Alt = Unavailable");
    } else {
        sdlx_render_printf_ex2(sdlx_win_width/2, ROW2Y(2), FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                               "Alt = %0.0f ft %s",
                               altitude_ft,
                               alt_is_wgs84 ? "WGS84" : "MSL");
    }

    // xxx horizontal bar

#if 1
    // draw rectangle around the graph area
    // xxx move down, or delete, or just draw top and bottom lines
    sdlx_render_rect(0,
                     GRAPH_Y_TOP-5,
                     sdlx_win_width,
                     GRAPH_H+10,
                     5, // line_width
                     COLOR_WHITE);
#endif

    // display graph
    x = 0;
    get_current_ymd(&y, &m, &d);
    while (true) {
        if (x-X < sdlx_win_width && x-X > -sdlx_win_width) {
            display_day_graph(x-X, y, m, d);
        }

        if (x-X < -sdlx_win_width) break;

        x -= sdlx_win_width;

        if (--d < 0) {
            if (--m < 0) {
                m = 0;
                if (--y < 0) y = 0;
            }
            d = days_in_month(y, m) - 1;
        }
    }

    // xxx comment
    y = GRAPH_Y_BOTTOM + 3*sdlx_char_height_dflt;
    sdlx_render_printf_ex2(sdlx_win_width/2, y,
                           FONT_NORMAL, COLOR_WHITE, FLAG_XY_CTR, 
                           "%5d - %-5d", params.graph_min, params.graph_max);
    reg_event(sdlx_win_width/2-COL2X(2), y-100, EVID_INCR_GRAPH_MIN, "+");
    reg_event(sdlx_win_width/2-COL2X(2), y+100, EVID_DECR_GRAPH_MIN, "-");

    reg_event(sdlx_win_width/2+COL2X(2), y-100, EVID_INCR_GRAPH_MAX, "+");
    reg_event(sdlx_win_width/2+COL2X(2), y+100, EVID_DECR_GRAPH_MAX, "-");

    // register motion event, for horizontal scrolling of the graph
    sdlx_register_event(NULL, EVID_MOTION);

    // xxx
    sdlx_loc_t *loc;
    loc = sdlx_render_printf_ex1(0, sdlx_win_height-2*sdlx_char_height_dflt,
                                 FONT_NORMAL, COLOR_LIGHT_BLUE, "TODAY");
    sdlx_register_event(loc, EVID_TODAY);

    // register control event
    sdlx_register_control_events(EVID_PREV, "<",
                                 EVID_NEXT, ">",
                                 EVID_QUIT, "X",
                                 COLOR_WHITE, COLOR_BLACK);
}

void reg_event(int x, int y, int evid, char *str)
{
    sdlx_loc_t *loc;

    loc = sdlx_render_printf_ex2(x, y, FONT_NORMAL, COLOR_BLUE, FLAG_XY_CTR, "%s", str);
    sdlx_register_event(loc, evid);
}

void display_day_graph(int x, int y, int m, int d)
{
    double       w, h;
    //sdlx_color_t color;
    int          hour;
    double       alt_ft;
    int          max = -999999;
    int          min =  999999;
    bool         have_min_max = false;

    w = (double)(sdlx_win_width-10) / 24;  //xxx

    // display graph
    for (hour = 0; hour < 24; hour++) {
        alt_ft = altitude_file->altitude_ft[y][m][d][hour];
        if (alt_ft == NO_ALTITUDE_DATA) {
            // xxx check this path taken
            continue;
        }

        if (alt_ft > max) max = alt_ft;
        if (alt_ft < min) min = alt_ft;
        have_min_max = true;

        h = (alt_ft - params.graph_min) / (params.graph_max - params.graph_min) * GRAPH_H;
        if (h <= 0) continue;
        if (h > GRAPH_H) h = GRAPH_H;

        sdlx_render_fill_rect(x + 5+hour*w, GRAPH_Y_BOTTOM-h+1, w-6, h, COLOR_GREEN);

    }

    // display graph title lines
    // - example: Thu May 7 2026 xxx more comment
    sdlx_render_printf_ex2(x + sdlx_win_width/2, GRAPH_Y_TOP-2*sdlx_char_height_dflt, 
                           FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                           "%s %s %d", 
                           get_weekday_str(y, m, d),
                           get_month_str(m),
                           d+1);
    if (have_min_max) {
        sdlx_render_printf_ex2(x + sdlx_win_width/2, GRAPH_Y_TOP-sdlx_char_height_dflt, 
                               FONT_NORMAL, COLOR_WHITE, FLAG_X_CTR,
                               "%d - %d ft", min, max);
    }

    // display vertical line to delimit the days
    sdlx_render_fill_rect(x, GRAPH_Y_TOP, 5, GRAPH_H, COLOR_RED);
}

// -----------------  PROCESS EVENT  ---------------------------

void process_event(sdlx_event_t *event)
{
    switch (event->event_id) {
    case EVID_QUIT:
        end_program = true;
        break;
    case EVID_INCR_GRAPH_MAX:
        params.graph_max += GRAPH_INCR_DECR_AMOUNT;
        util_set_numeric_param(data_dir, "graph_max",  params.graph_max);
        break;
    case EVID_DECR_GRAPH_MAX:
        if (params.graph_max-GRAPH_INCR_DECR_AMOUNT > params.graph_min) {
            params.graph_max -= GRAPH_INCR_DECR_AMOUNT;
            util_set_numeric_param(data_dir, "graph_max",  params.graph_max);
        }
        break;
    case EVID_INCR_GRAPH_MIN:
        if (params.graph_min+GRAPH_INCR_DECR_AMOUNT < params.graph_max) {
            params.graph_min += GRAPH_INCR_DECR_AMOUNT;
            util_set_numeric_param(data_dir, "graph_min",  params.graph_min);
        }
        break;
    case EVID_DECR_GRAPH_MIN:
        params.graph_min -= GRAPH_INCR_DECR_AMOUNT;
        if (params.graph_min < 0) params.graph_min = 0;
        util_set_numeric_param(data_dir, "graph_min",  params.graph_min);
        break;
    case EVID_MOTION:
        X -= event->u.motion.xrel;
        if (X > 0) X = 0;
        break;
    case EVID_PREV:
        if (X / 1000 == nearbyint(X / 1000)) {
            X -= sdlx_win_width;
        } else {
            X = 1000 * floor(X / 1000);
        }
        break;
    case EVID_NEXT:
        if (X / 1000 == nearbyint(X / 1000)) {
            X += sdlx_win_width;
        } else {
            X = 1000 * ceil(X / 1000);
        }
        if (X > 0) X = 0;
        break;
    case EVID_TODAY: // xxx change to RESET
        X = 0;
        break;
    }
}

// -----------------  UTILS  --------------------------------------

char *month_str_tbl[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char *day_str_tbl[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

char *get_month_str(int m)
{
    return month_str_tbl[m];
}

char *get_weekday_str(int y, int m, int d)
{
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = y + YEAR0 - 1900;
    tm.tm_mon  = m;
    tm.tm_mday = d + 1;
    tm.tm_isdst = -1;  // system will determine dst

    t = mktime(&tm);
    localtime_r(&t, &tm);
    return day_str_tbl[tm.tm_wday];
}

bool is_weekend(int y, int m, int d)
{
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    tm.tm_year = y + YEAR0 - 1900;
    tm.tm_mon  = m;
    tm.tm_mday = d + 1;
    tm.tm_isdst = -1;  // system will determine dst

    t = mktime(&tm);
    localtime_r(&t, &tm);
    return tm.tm_wday == 0 || tm.tm_wday == 6;
}

int days_in_month(int y, int m)
{
    y += YEAR0;
    m += 1;

    if (m == 9 || m == 4 || m == 6 || m == 11) {
        return 30;
    } else if (m == 2) {
        bool leap_year = (((y % 4) == 0) && !((y % 100) == 0)) || ((y % 400) == 0);
        return leap_year ? 29 : 28;
    } else {
        return 31;
    }
}

void get_current_ymd(int *y, int *m, int *d)
{
    time_t t;
    struct tm tm;

    t = time(NULL);
    localtime_r(&t, &tm);
    *y = tm.tm_year + 1900 - YEAR0;   // y : 0 is year 2026
    *m = tm.tm_mon;                   // m : 0 - 11
    *d = tm.tm_mday - 1;              // d : 0 - 30
}
