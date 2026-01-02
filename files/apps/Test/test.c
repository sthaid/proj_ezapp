#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

#include <sdlx.h>
#include <utils.h>

#include "apps/Test/common.h"

//
// defines
//

//
// variables
//

static bool  end_program;

//
// prototypes
//

static void page_hndlr(void);

static void page_0_draw(void);
static void page_0_process_event(sdlx_event_t *event);

static void page_1_draw(void);

static void page_2_draw(void);

static void page_3_init(void);
static void page_3_draw(void);
static void page_3_process_event(sdlx_event_t *event);
static void page_3_exit(void);

static void page_4_draw(void);

static void page_5_init(void);
static void page_5_draw(void);
static void page_5_exit(void);

static void page_6_draw(void);

static void page_7_init(void);
static void page_7_draw(void);
static void page_7_process_event(sdlx_event_t *event);
static void page_7_exit(void);

static void page_8_init(void);
static void page_8_draw(void);
static void page_8_exit(void);

static void page_9_init(void);
static void page_9_draw(void);

static void page_10_draw(void);

static void page_11_init(void);
static void page_11_draw(void);
static void page_11_process_event(sdlx_event_t *ev);
static void page_11_exit(void);

// -----------------  MAIN  ------------------------------------------

int main(int argc, char **argv)
{
    int rc;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl
    rc = sdlx_init(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // print window and char sized, these are global variables from sdl.c;
    // the initial char size provides 20 chars across the display width
    printf("INFO %s: sdlx_win_width/height  = %d %d\n", progname, sdlx_win_width, sdlx_win_height);
    printf("INFO %s: sdlx_char_width/height = %d %d\n", progname, sdlx_char_width, sdlx_char_height);

    // test calling a routine that is defined in another file
    test1_proc();

    // test reading a file in the 'data_dir' dir
    int file_len;
    char *file_content = util_read_file(data_dir, "common.h", &file_len);
    if (file_content == NULL) {
        printf("ERROR %s: failed to read file common.h\n", data_dir);
    } else {
        printf("INFO %s: read file common.h okay, file_len = %d\n", data_dir, file_len);
    }

    // toast test
    sdlx_show_toast("TOAST TEST");

    // call handler routine for the current page
    while (true) {
        page_hndlr();
        if (end_program) {
            break;
        }
    }

    // exit sdl
    sdlx_quit(SUBSYS_VIDEO | SUBSYS_AUDIO | SUBSYS_SENSOR);

    // return success
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  SUPPORT PROCS FOR ALL PAGES  ------------

// xxx check this about picoc
// picoc: picoc does not support this being static, causes crash
char *page_title[] = {     // Page
        "Unit Test",       //   0
        "Font",            //   1
        "Sizeof",          //   2
        "Multi Lines",     //   3
        "Drawing",         //   4
        "Textures",        //   5
        "Colors",          //   6
        "Audio",           //   7
        "Sensor Info",     //   8
        "Sensor Values",   //   9
        "Location",        //  10
        "Playback Capture" //  11
            };
static int pagenum = 11;

#define LAST_PAGE 11

#define EVID_PREV_PAGE 1
#define EVID_NEXT_PAGE 2

static void page_hndlr()
{
    sdlx_event_t event;
    int         new_pagenum = -1;

    // call the page specific init routine, if provided
    switch (pagenum) {
    case 3: page_3_init(); break;
    case 5: page_5_init(); break;
    case 7: page_7_init(); break;
    case 8: page_8_init(); break;
    case 9: page_9_init(); break;
    case 11: page_11_init(); break;
    }

    while (true) {
        // init the backbuffer, and print font/color
        sdlx_display_init(COLOR_BLACK);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

        // draw title line
        sdlx_render_text_xyctr(sdlx_win_width/2, sdlx_char_height/2, page_title[pagenum]);

        // register control events
        // "<" - previous page
        // ">" - next page
        // 'X' - end prorgram
        sdlx_register_control_events("<", ">", "X", COLOR_WHITE, COLOR_BLACK,
                                    EVID_PREV_PAGE, EVID_NEXT_PAGE, EVID_QUIT);

        // register swipe events, also used to change page
        sdlx_register_event(NULL, EVID_SWIPE_LEFT);
        sdlx_register_event(NULL, EVID_SWIPE_RIGHT);

        // draw display
        switch (pagenum) {
        case 0: page_0_draw(); break;
        case 1: page_1_draw(); break;
        case 2: page_2_draw(); break;
        case 3: page_3_draw(); break;
        case 4: page_4_draw(); break;
        case 5: page_5_draw(); break;
        case 6: page_6_draw(); break;
        case 7: page_7_draw(); break;
        case 8: page_8_draw(); break;
        case 9: page_9_draw(); break;
        case 10: page_10_draw(); break;
        case 11: page_11_draw(); break;
        default:
            printf("ERROR %s: invalid pagenum %d\n", progname, pagenum);
            end_program = true;
            return;
        }

        // present the display
        sdlx_display_present();

        // wait for an event with 50 ms timeout;
        // if no event available, then redraw display
        sdlx_get_event(50000, &event);
        if (event.event_id == -1) {
            continue;
        }

        // process common events
        switch (event.event_id) {
        case EVID_QUIT:
            end_program = true;
            break;      
        case EVID_SWIPE_RIGHT: case EVID_PREV_PAGE:
            new_pagenum = pagenum - 1;
            if (new_pagenum < 0) {
                new_pagenum = LAST_PAGE;
            }
            break;      
        case EVID_SWIPE_LEFT: case EVID_NEXT_PAGE:
            new_pagenum = pagenum + 1;
            if (new_pagenum > LAST_PAGE) {
                new_pagenum = 0;
            }
            break;      
        }

        // if the page has been changed or the program is terminating
        // then break out of the loop
        if (new_pagenum != -1 || end_program) {
            break;
        }

        // it wasn't a common event;
        // call the page specific event hndlr, if provided
        switch (pagenum) {
        case 0: page_0_process_event(&event); break;
        case 3: page_3_process_event(&event); break;
        case 7: page_7_process_event(&event); break;
        case 11: page_11_process_event(&event); break;
        }
    }

    // call the page specific exit routine, if provided
    switch (pagenum) {
    case 3: page_3_exit(); break;
    case 5: page_5_exit(); break;
    case 7: page_7_exit(); break;
    case 8: page_8_exit(); break;
    case 11: page_11_exit(); break;
    }

    // update pagenum
    pagenum = new_pagenum;
}

// -----------------  PAGE 0: CLOCK  --------------------------

#define EVID_TOGGLE_FOREGROUND  10
#define EVID_FLASH_OFF          11
#define EVID_FLASH_ON           12

static void page_0_draw(void)
{
    time_t t;
    struct tm *tm;
    char str[100];
    long usecs, delta_ms;
    static long usecs_last, usecs_first;
    
    // print the time, hh:mm:ss
    time(&t);
    tm = localtime(&t);
    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    sdlx_render_text_xyctr(sdlx_win_width/2, ROW2Y(5), str);

    // print the time in microsecs
    usecs = util_get_real_time_microsec();
    util_time2str(str, usecs, false, true, false);
    sdlx_render_text_xyctr(sdlx_win_width/2, ROW2Y(7), str);

    // print microsecs since this page is first viewed, and
    // print the delta time since last display update
    usecs = util_microsec_timer();
    if (usecs_first == 0) {
        usecs_first = usecs;
    }
    delta_ms = (usecs - usecs_last) / 1000;
    usecs_last = usecs;
    sdlx_render_printf_xyctr(sdlx_win_width/2, ROW2Y(9), "%0.3f delta=%ld ms", 
        (usecs-usecs_first)/1000000., delta_ms);

    // print ipaddr
    sdlx_render_printf_xyctr(sdlx_win_width/2, ROW2Y(11), "%s", util_get_ipaddr());

#ifdef NOTDEF
    sdlx_loc_t *loc;

    // register toggle foreground event
    sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
    loc = sdlx_render_text(
            0, 
            sdlx_win_height-8*sdlx_char_height, 
            util_is_foreground_enabled() ? "Foreground_Enabled" : "Foreground_Disabled");
    sdlx_register_event(loc, EVID_TOGGLE_FOREGROUND);
    sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);

    // register flashlight on/off events
    sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
    loc = sdlx_render_text(
            0, 
            sdlx_win_height-10*sdlx_char_height, 
            "FlashOff");
    sdlx_register_event(loc, EVID_FLASH_OFF);
    loc = sdlx_render_text(
            sdlx_win_width/2,
            sdlx_win_height-10*sdlx_char_height, 
            "FlashOn");
    sdlx_register_event(loc, EVID_FLASH_ON);
    sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);
#endif
}

static void page_0_process_event(sdlx_event_t *ev)
{
    switch (ev->event_id) {
    case EVID_TOGGLE_FOREGROUND:
        if (util_is_foreground_enabled()) {
            util_stop_foreground();
        } else {
            util_start_foreground();
        }
        break;
    case EVID_FLASH_OFF:
        util_turn_flashlight_off();
        break;
    case EVID_FLASH_ON:
        util_turn_flashlight_on();
        break;
    }
}

// -----------------  PAGE 1: FONT  ---------------------------

static void page_1_draw(void)
{
    int i, ch=0;
    char str[32];

    for (i = 0; i < 16; i++) {
        sprintf(str, "%02x %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                i*16,
                ch+0, ch+1, ch+2, ch+3, ch+4, ch+5, ch+6, ch+7,
                ch+8, ch+9, ch+10, ch+11, ch+12, ch+13, ch+14, ch+15);

        sdlx_render_text(0, ROW2Y(i+2), str);
        ch += 16;
    }
}

// -----------------  PAGE 2: SIZEOF  -------------------------

static void page_2_draw(void)
{
    int r = 2;

    sdlx_render_printf(0, ROW2Y(r++), "sizoef(char)   = %zd", sizeof(char));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(short)  = %zd", sizeof(short));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(int)    = %zd", sizeof(int));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(long)   = %zd", sizeof(long));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(size_t) = %zd", sizeof(size_t));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(off_t)  = %zd", sizeof(off_t));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(time_t) = %zd", sizeof(time_t));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(clock_t)= %zd", sizeof(clock_t));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(float)  = %zd", sizeof(float));
    sdlx_render_printf(0, ROW2Y(r++), "sizoef(double) = %zd", sizeof(double));
    sdlx_render_printf(0, ROW2Y(r++), "sizeof(1)      = %zd", sizeof(1));
    sdlx_render_printf(0, ROW2Y(r++), "sizeof(1L)     = %zd", sizeof(1L));
}

// -----------------  PAGE 3: MULTI LINE TEXT  ----------------

static double y_top;
static int y_display_begin;
static int y_display_end;
static char *lines[100];

static void page_3_init(void)
{
    for (int i = 0; i < 100; i++) {
        lines[i] = malloc(50);
        sprintf(lines[i], "Line %d\nHello World\n", i);
    }

    y_top = ROW2Y(2); 
    y_display_begin = ROW2Y(2);
    y_display_end = sdlx_win_height-3*sdlx_char_height;
}

static void page_3_draw(void)
{
    sdlx_register_event(NULL, EVID_MOTION);

    sdlx_render_multiline_text(y_top, y_display_begin, y_display_end, lines, 100);
}

static void page_3_process_event(sdlx_event_t *event)
{
    if (event->event_id == EVID_MOTION) {
        y_top += event->u.motion.yrel;
        if (y_top >= y_display_begin) {
            y_top = y_display_begin;
        }
    }
}

static void page_3_exit(void)
{
    for (int i = 0; i < 100; i++) {
        free(lines[i]);
        lines[i] = NULL;
    }
}

// -----------------  PAGE 4: DRAWING  ------------------------

static void add_point(sdlx_point_t **p, int x, int y);

static void page_4_draw(void)
{
    // draw rect around perimeter
    sdlx_render_rect(0, 0, sdlx_win_width, sdlx_win_height, 2, COLOR_PURPLE);

    // draw fill rect, y = 170 .. 400
    sdlx_render_fill_rect(100, 170, 800, 230, COLOR_RED);

    // draw circles, y = 400 .. 500
    sdlx_render_circle(1*sdlx_win_width/4, 450, 50, 3, COLOR_YELLOW);
    sdlx_render_circle(2*sdlx_win_width/4, 450, 50, 3, COLOR_YELLOW);
    sdlx_render_circle(3*sdlx_win_width/4, 450, 50, 3, COLOR_YELLOW);

    // draw 6 lines, y = 500 .. 600
    for (int y = 500; y <= 600; y += 20) {
        sdlx_render_line(0, y, 1000, y, COLOR_WHITE);
    }

    // draw 3 lines to make a triangle, y = 600 .. 800
    sdlx_point_t pts[4], *ptsx=pts;
    add_point(&ptsx, 500, 600);
    add_point(&ptsx, 700, 800);
    add_point(&ptsx, 300, 800);
    add_point(&ptsx, 500, 600);
    sdlx_render_lines(pts, 4, COLOR_RED);

    // draw 2 squares and vary intensity and wavelen, y = 800 .. 900
    static double inten;
    int color;
    inten = inten + 0.01;
    if (inten > 1) inten = 0;
    color = sdlx_scale_color(COLOR_YELLOW, inten);
    sdlx_render_fill_rect(100, 800, 100, 100, color);

    static double wavelen = 750;
    wavelen -= 2;
    if (wavelen < 440) wavelen = 750;
    color = sdlx_wavelength_to_color(wavelen);
    sdlx_render_fill_rect(800, 800, 100, 100, color);

    // draw points with varying size, y = 1000
    color = sdlx_create_color(0, 255, 0, 255);
    for (int pointsize = 0; pointsize <= 9; pointsize++) {
        sdlx_render_point(pointsize*100+50, 1000, color, pointsize);
    }

    // draw 10 points of the same size, y = 1100
    sdlx_point_t points[10];
    for (int i = 0; i < 10; i++) {
        points[i].x = i*100+50;
        points[i].y = 1100;
    }
    sdlx_render_points(points, 10, COLOR_PURPLE, 5);
}

static void add_point(sdlx_point_t **p, int x, int y)
{
    (*p)->x = x;
    (*p)->y = y;
    (*p)++;
}

// -----------------  PAGE 5: TEXTURES  -----------------------

static sdlx_texture_t *circle;
static sdlx_texture_t *text;

static void page_5_init(void)
{
    circle = sdlx_create_filled_circle_texture(100, COLOR_RED);
    text   = sdlx_create_text_texture("XXXXX");
}

static void page_5_draw(void)
{
    int ret, w, h;
    sdlx_texture_t *t;
    unsigned char *pixels;
    int w_pixels, h_pixels;

    // render the circle texture at varying x location, y = 200 .. 400
    static int circle_x=-200;
    sdlx_query_texture(circle, &w, &h);
    sdlx_render_texture(circle_x, 200, w, h, circle);
    circle_x += 10;
    if (circle_x > 1000) circle_x = -200;

    // render the circle texture using scaling, y = 400 .. 600
    sdlx_render_texture(500-200, 400, 400, 200, circle);

    // render text texture, at y = 500
    sdlx_query_texture(text, &w, &h);
    sdlx_render_texture(0, 500-h/2, w, h, text);

    // rotate and render the text texture at y = 600 .. 850
    static double angle = 0;
    angle += 5;
    sdlx_render_texture_ex(500-w/2, 600+w/2-h/2, w, h, angle, text);

    // create unit_test_pixels file from the top row of the display
    pixels = sdlx_read_display_pixels(0, 0, sdlx_win_width, sdlx_char_height, &w_pixels, &h_pixels);
    if (pixels == NULL) {
        printf("ERROR %s: failed to read display pixels\n", progname);
        return;
    }

    ret = util_write_png_file(data_dir, "test5.png", pixels, w_pixels, h_pixels);
    if (ret != 0) {
        printf("ERROR %s: failed to write test5.png\n", progname);
        free(pixels);
        return;
    }

    free(pixels);

    // read the png file created above
    // create a texture from the pixels, and
    // display the texture rotated 180 degrees
    ret = util_read_png_file(data_dir, "test5.png", &pixels, &w_pixels, &h_pixels);
    if (ret != 0) {
        printf("ERROR %s: failed to read test5.png\n", progname);
        return;
    }

    t = sdlx_create_texture_from_pixels(pixels, w_pixels, h_pixels);
    sdlx_render_texture_ex(0, 900, sdlx_win_width, sdlx_char_height, 180, t);
    sdlx_destroy_texture(t);

    free(pixels);
}

static void page_5_exit(void)
{
    sdlx_destroy_texture(circle);
    sdlx_destroy_texture(text);
}

// -----------------  PAGE 6: COLORS  -------------------------

static void color_test(int idx, char *color_name, int color);
static void alpha_test(int idx, char *test_name, int bg_color, int fg_color);

static void page_6_draw(void)
{
    int idx = 0;

    color_test(idx++, "WHITE", COLOR_WHITE);
    color_test(idx++, "RED",   COLOR_RED);
    color_test(idx++, "ORANGE", COLOR_ORANGE);
    color_test(idx++, "YELLOW", COLOR_YELLOW);
    color_test(idx++, "GREEN", COLOR_GREEN);
    color_test(idx++, "BLUE", COLOR_BLUE);
    color_test(idx++, "INDIGO", COLOR_INDIGO);
    color_test(idx++, "VIOLET", COLOR_VIOLET);
    color_test(idx++, "PURPLE", COLOR_PURPLE);
    color_test(idx++, "LIGHT_BLUE", COLOR_LIGHT_BLUE);
    color_test(idx++, "LIGHT_GREEN", COLOR_LIGHT_GREEN);
    color_test(idx++, "PINK", COLOR_PINK);
    color_test(idx++, "TEAL", COLOR_TEAL);
    color_test(idx++, "LIGHT_GRAY", COLOR_LIGHT_GRAY);
    color_test(idx++, "GRAY", COLOR_GRAY);
    color_test(idx++, "DARK_GRAY", COLOR_DARK_GRAY);
    alpha_test(idx++, "ALPHA_TEST", COLOR_WHITE, COLOR_BLUE);
}

static void color_test(int idx, char *color_name, int color)
{
    int y = 2 * sdlx_char_height + idx * 100;

    sdlx_render_text(0, y, color_name);
    sdlx_render_fill_rect(500, y, 500, sdlx_char_height, color);
}

static void alpha_test(int idx, char *test_name, int bg_color, int fg_color)
{
    int y = 2 * sdlx_char_height + idx * 100;
    int alpha, x, color;

    sdlx_render_text(0, y, test_name);
    sdlx_render_fill_rect(500, y, 500, sdlx_char_height, bg_color);

    for (x = 500; x < 1000; x+=2) {
        alpha = (x - 500) / 2;  // will range from 0 to 249
        color = sdlx_set_color_alpha(fg_color, alpha);
        sdlx_render_line(x, y, x, y+sdlx_char_height, color);
        sdlx_render_line(x+1, y, x+1, y+sdlx_char_height, color);
    }

}

// -----------------  PAGE 7: AUDIO  --------------------------

#define EVID_AUDIO_PLAY_TONE        10
#define EVID_AUDIO_PLAY_FREQ_SWEEP  11
#define EVID_AUDIO_PLAY_SQUARE_WAVE 12
#define EVID_AUDIO_PLAY_MORSE_CODE  13
#define EVID_AUDIO_PLAY_RECORDING   19
#define EVID_AUDIO_RECORD           20
#define EVID_AUDIO_RECORD_APPEND    21
#define EVID_AUDIO_STOP             30
#define EVID_AUDIO_PAUSE            31
#define EVID_AUDIO_CONT             32

static void add_tone(sdlx_tone_t **t, int freq, int intvl_ms);
static void add_gap(sdlx_tone_t **t, int intvl_ms);
static void add_terminator(sdlx_tone_t **t);
static char *audio_state_str(int x);
static void generate_morse_code_tones(sdlx_tone_t **t, char *letters, int wpm);
       
static void page_7_init(void)
{
    sdlx_audio_print_devices_info();
}

static void page_7_draw(void)
{
    sdlx_loc_t *loc;
    sdlx_audio_state_t state;
    int y;
    sdlx_print_state_t print_state;

    //
    // get audio state
    //

    sdlx_audio_state(&state);

    //
    // record section
    //

    sdlx_print_save(&print_state);

    sdlx_print_init_color(state.state == AUDIO_STATE_RECORD ? COLOR_RED : COLOR_WHITE, COLOR_BLACK);
    loc = sdlx_render_text(0, 200, "RECORD");
    sdlx_register_event(loc, EVID_AUDIO_RECORD);

    sdlx_print_init_color(state.state == AUDIO_STATE_RECORD_APPEND ? COLOR_RED : COLOR_WHITE, COLOR_BLACK);
    loc = sdlx_render_text(sdlx_win_width/2, 200, "APPEND");
    sdlx_register_event(loc, EVID_AUDIO_RECORD_APPEND);

    sdlx_print_restore(&print_state);

    //
    // play section
    //

    y = 400;

    sdlx_render_text_xyctr(sdlx_win_width/2, y, "--- PLAY ---");
    y += 150;

    loc = sdlx_render_text(0, y, "RECORDING");
    sdlx_register_event(loc, EVID_AUDIO_PLAY_RECORDING);
    y += 150;

    loc = sdlx_render_text(0, y, "TONE");
    sdlx_register_event(loc, EVID_AUDIO_PLAY_TONE);
    y += 150;

    loc = sdlx_render_text(0, y, "FREQ_SWEEP");
    sdlx_register_event(loc, EVID_AUDIO_PLAY_FREQ_SWEEP);
    y += 150;

    loc = sdlx_render_text(0, y, "SQUARE_WAVE");
    sdlx_register_event(loc, EVID_AUDIO_PLAY_SQUARE_WAVE);
    y += 150;

    loc = sdlx_render_text(0, y, "MORSE_CODE");
    sdlx_register_event(loc, EVID_AUDIO_PLAY_MORSE_CODE);
    y += 150;

    //
    // state section
    //

    if (state.state != AUDIO_STATE_IDLE) {
        y = sdlx_win_height-650;

        // state, processed/total time, and paused
        sdlx_render_printf(0, y, "%s %d/%d", 
                          audio_state_str(state.state), state.processed_ms/1000, state.total_ms/1000);
        if (state.paused) {
            sdlx_render_printf(sdlx_win_width-sdlx_char_width, y, "%s", "P");
        }
        y += sdlx_char_height;

        // volume
        sdlx_render_fill_rect(0, y, sdlx_win_width * state.volume / 100, sdlx_char_height, COLOR_RED);
        sdlx_render_rect(0, y, sdlx_win_width, sdlx_char_height, 2, COLOR_WHITE);
        y += sdlx_char_height;

        // filename
        if (state.filename[0]) {
            sdlx_render_printf(0, y, "%s", state.filename);
            y += sdlx_char_height;
        }

    }

    //
    // stop, pause, cont controls section
    //

    loc = sdlx_render_text(0, sdlx_win_height-300, "STOP");
    sdlx_register_event(loc, EVID_AUDIO_STOP);

    loc = sdlx_render_text(sdlx_win_width/2-2.5*sdlx_char_width, sdlx_win_height-300, "PAUSE");
    sdlx_register_event(loc, EVID_AUDIO_PAUSE);

    loc = sdlx_render_text(sdlx_win_width-4*sdlx_char_width, sdlx_win_height-300, "CONT");
    sdlx_register_event(loc, EVID_AUDIO_CONT);
}

static void page_7_process_event(sdlx_event_t *ev)
{
    int rc, i, freq;
    sdlx_tone_t tones[5000];
    sdlx_tone_t *t;
    sdlx_audio_state_t state;

    switch (ev->event_id) {
    case EVID_AUDIO_PLAY_TONE:
        sdlx_audio_create_test_file(data_dir, "audio_test.raw", 10, 1000);
        rc = sdlx_audio_play(data_dir, "audio_test.raw");
        if (rc != 0) {
            printf("ERROR %s: sdlx_audio_play audio_test.raw failed\n", progname);
        }
        util_delete_file(data_dir, "audio_test.raw");
        break;
    case EVID_AUDIO_PLAY_RECORDING:
        rc = sdlx_audio_play(data_dir, "recording.raw");
        if (rc != 0) {
            printf("ERROR %s: sdlx_audio_play recording.raw failed\n", progname);
        }
        break;
    case EVID_AUDIO_PLAY_FREQ_SWEEP:
        t = tones;
        for (freq = 100; freq <= 3000; freq += 100) {
            add_tone(&t, freq, 500);
        }
        add_terminator(&t);
        sdlx_audio_play_tones(tones);
        break;
    case EVID_AUDIO_PLAY_SQUARE_WAVE:
        t = tones;
        for (i = 0; i < 10; i++) {
            add_tone(&t, 500, 500);  // freq=500 dur=500ms
            add_gap(&t, 500);        // dur=500ms
        }
        add_terminator(&t);
        sdlx_audio_play_tones(tones);
        break;
    case EVID_AUDIO_PLAY_MORSE_CODE:
        t = tones;
        generate_morse_code_tones(&t, "CQ CQ HELLO WORLD CQ CQ", 10);  // 10 words-per-minute
        sdlx_audio_play_tones(tones);
        break;
    case EVID_AUDIO_RECORD:
        sdlx_audio_state(&state);
        if (state.state == AUDIO_STATE_RECORD) {
            sdlx_audio_ctl(AUDIO_REQ_STOP);
            break;
        }

        // 30 sec max, 3 sec auto stop, new recording
        rc = sdlx_audio_record(data_dir, "recording.raw", 30, 3, false);
        if (rc != 0) {
            printf("ERROR %s: sdlx_audio_record failed\n", progname);
        }
        break;
    case EVID_AUDIO_RECORD_APPEND:
        sdlx_audio_state(&state);
        if (state.state == AUDIO_STATE_RECORD_APPEND) {
            sdlx_audio_ctl(AUDIO_REQ_STOP);
            break;
        }

        // 30 sec max, 3 sec auto stop, append
        rc = sdlx_audio_record(data_dir, "recording.raw", 30, 3, true);
        if (rc != 0) {
            printf("ERROR %s: sdlx_audio_record append failed\n", progname);
        }
        break;
    case EVID_AUDIO_STOP:
        sdlx_audio_ctl(AUDIO_REQ_STOP);
        break;
    case EVID_AUDIO_PAUSE:
        sdlx_audio_ctl(AUDIO_REQ_PAUSE);
        break;
    case EVID_AUDIO_CONT:
        sdlx_audio_ctl(AUDIO_REQ_UNPAUSE);
        break;
    }
}

static void page_7_exit(void)
{
    sdlx_audio_ctl(AUDIO_REQ_STOP);
}

static char *audio_state_str(int x)
{
    if (x == AUDIO_STATE_IDLE)          return "IDLE";
    if (x == AUDIO_STATE_PLAY_FILE)     return "PLAY_FILE";
    if (x == AUDIO_STATE_PLAY_TONES)    return "PLAY_TONES";
    if (x == AUDIO_STATE_RECORD)        return "RECORD";
    if (x == AUDIO_STATE_RECORD_APPEND) return "RECORD_APPEND";
    return "INVLD_STATE";
}

#define MORSE_FREQ 1000

static void generate_morse_code_tones(sdlx_tone_t **t, char *letters, int wpm)
{
    char *morse_chars[] = {  // xxx this cant be static due to picoc limitation
                    /* A */ ".-",      /* B */ "-...",    /* C */ "-.-.",
                    /* D */ "-..",     /* E */ ".",       /* F */ "..-.",
                    /* G */ "--.",     /* H */ "....",    /* I */ "..",
                    /* J */ ".---",    /* K */ "-.-",     /* L */ ".-..",
                    /* M */ "--",      /* N */ "-.",      /* O */ "---",
                    /* P */ ".--.",    /* Q */ "--.-",    /* R */ ".-.",
                    /* S */ "...",     /* T */ "-",       /* U */ "..-",
                    /* V */ "...-",    /* W */ ".--",     /* X */ "-..-",
                    /* Y */ "-.--",    /* Z */ "--..", };
    int dit_dur         = 1200 / wpm;   // millisecs
    int dah_dur         = 3 * dit_dur;
    int dit_dah_gap_dur = dit_dur;
    int char_gap_dur    = 2 * dit_dur;
    int word_gap_dur    = 4 * dit_dur;

    for (int i = 0; letters[i]; i++) {
        int ch = letters[i];
        ch = toupper(ch);
        if (ch >= 'A' && ch <='Z') {
            for (int j = 0; morse_chars[ch-'A'][j]; j++) {
                int intvl_ms = (morse_chars[ch-'A'][j] == '.') ? dit_dur : dah_dur;
                add_tone(t, MORSE_FREQ, intvl_ms);
                add_gap(t, dit_dah_gap_dur);
            }
            add_gap(t, char_gap_dur);
        } else if (ch == ' ' || ch == '\n') {
            add_gap(t, word_gap_dur);
        }
    }
    add_terminator(t);
}

static void add_tone(sdlx_tone_t **t, int freq, int intvl_ms)
{       
    (*t)->freq = freq;
    (*t)->intvl_ms = intvl_ms;
    *t = *t + 1;
}       
            
static void add_gap(sdlx_tone_t **t, int intvl_ms)
{       
    (*t)->freq = 0;
    (*t)->intvl_ms = intvl_ms;
    *t = *t + 1;
}           
        
static void add_terminator(sdlx_tone_t **t)
{       
    (*t)->freq = 0;
    (*t)->intvl_ms = 0;
    *t = *t + 1;
}

// -----------------  PAGE 8: SENSOR INFO TBL -----------------

static sdlx_sensor_info_t *sit;
static int                max_sit;
static char              *sit_lines[100];

static void page_8_init(void)
{
    char str[200];

    sit = sdlx_sensor_get_info_tbl(&max_sit);
    if (sit == NULL) {
        printf("ERROR %s: sdlx_sensor_get_info_tbl failed\n", progname);
    }

    y_top = ROW2Y(2); 
    y_display_begin = ROW2Y(2);
    y_display_end = sdlx_win_height-3*sdlx_char_height;

    for (int i = 0; i < max_sit; i++) {
        sprintf(str, "%2d %2d %s", sit[i].id, sit[i].type, sit[i].name);
        sit_lines[i] = strdup(str);
    }
}

static void page_8_draw(void)
{
    sdlx_print_init(SMALL_FONT, COLOR_WHITE, COLOR_BLACK);
    sdlx_render_multiline_text(y_top, y_display_begin, y_display_end, sit_lines, max_sit);
}

static void page_8_exit(void)
{
    for (int i = 0; i < max_sit; i++) {
        free(sit_lines[i]);
    }
}

// -----------------  PAGE 9: SENSOR DATA ---------------------

#define MAX_SENSOR_TEST_TBL 6

struct sensor_test_s {
    char *name;
    int   type;
    int   id;
} sensor_test_tbl[MAX_SENSOR_TEST_TBL];

static void page_9_init(void)
{
    sensor_test_tbl[0].name =  "stepc";
    sensor_test_tbl[0].type =  ASENSOR_TYPE_STEP_COUNTER;
    sensor_test_tbl[1].name =  "magf";
    sensor_test_tbl[1].type =  ASENSOR_TYPE_MAGNETIC_FIELD;
    sensor_test_tbl[2].name =  "accel";
    sensor_test_tbl[2].type =  ASENSOR_TYPE_ACCELEROMETER;
    sensor_test_tbl[3].name =  "press";
    sensor_test_tbl[3].type =  ASENSOR_TYPE_PRESSURE;
    sensor_test_tbl[4].name =  "temp";
    sensor_test_tbl[4].type =  ASENSOR_TYPE_AMBIENT_TEMPERATURE;
    sensor_test_tbl[5].name =  "humid";
    sensor_test_tbl[5].type =  ASENSOR_TYPE_RELATIVE_HUMIDITY; // xxx add routine for this

    for (int i = 0; i < MAX_SENSOR_TEST_TBL; i++) {
        struct sensor_test_s *x = &sensor_test_tbl[i];
        x->id = sdlx_sensor_find(x->type);
    }
}

static void page_9_draw(void)
{
    double data[3];
    int    row = 2;
    int    rc;
    double step_count;
    double mag_heading, roll, pitch, millibars, degrees_c, percent;
    double ax, ay, az;

    sdlx_print_init(SMALL_FONT, COLOR_WHITE, COLOR_BLACK);

    for (int i = 0; i < MAX_SENSOR_TEST_TBL; i++) {
        struct sensor_test_s *x = &sensor_test_tbl[i];
        if (x->id != -1) {
            sdlx_sensor_read_raw(x->id, data, 3);
            sdlx_render_printf(0, ROW2Y(row++), "%-5s %6.2f %6.2f %6.2f", x->name, data[0], data[1], data[2]);
        }
    }

    row++;

    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);

    rc = sdlx_sensor_read_step_counter(&step_count);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "stepc=% .0f", step_count);
    }

    rc = sdlx_sensor_read_mag_heading(&mag_heading);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "magh =% 3.0f", mag_heading);
    }

    rc = sdlx_sensor_read_accelerometer(&ax, &ay, &az);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "accel=% 4.1f % 4.1f % 4.1f", ax, ay, az);;
    }

    rc = sdlx_sensor_read_roll_pitch(&roll, &pitch);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "r/p  =% 4.1f % 4.1f", roll, pitch);
    }

    rc = sdlx_sensor_read_pressure(&millibars);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "press=% 5.0f", millibars);
    }

    rc = sdlx_sensor_read_temperature(&degrees_c);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "temp =% 4.0f", degrees_c);
    }

    rc = sdlx_sensor_read_humidity(&percent);
    if (rc == 0) {
        sdlx_render_printf(0, ROW2Y(row++), "humid=% 4.0f", percent);
    }
}

// -----------------  PAGE 10: LOCATION  ----------------------

static char *num2str(double num, char *fmt, char *s);

static void page_10_draw(void)
{
    double lat, lng, alt;
    int    row=2;
    char   s[50];

    util_get_location(&lat, &lng, &alt);

    sdlx_render_printf(0, ROW2Y(row++), "Lat  = %s", num2str(lat,"%9.4f",s));
    sdlx_render_printf(0, ROW2Y(row++), "Long = %s", num2str(lng,"%9.4f",s));
    sdlx_render_printf(0, ROW2Y(row++), "Alt  = %s", num2str(alt,"%9.4f",s));
}

static char *num2str(double num, char *fmt, char *s)
{
    if (num == INVALID_NUMBER) {
        sprintf(s, "invld");
    } else {
        sprintf(s, fmt, num);
    }
    return s;
}

// -----------------  PAGE 11: PLAYBACK CAPTURE  --------------

//xxx wip
#define EVID_START_PLAYBACKCAPTURE  10
#define EVID_STOP_PLAYBACKCAPTURE   11
#define EVID_TEST_PLAYBACKCAPTURE   12
#define EVID_PLAY_PCM               13
#define EVID_PLAY_MP3               14

static void page_11_init(void)
{
}

static void page_11_draw(void)
{
    sdlx_loc_t *loc;

    // xxx is it okay to call start or stop repeated
    // xxx need method to check if it is running

    // register start / stop playbackcapture events
    sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);

    loc = sdlx_render_text( 0, ROW2Y(2), "StartCapture");
    sdlx_register_event(loc, EVID_START_PLAYBACKCAPTURE);

    loc = sdlx_render_text( 0, ROW2Y(6), "StopCapture");
    sdlx_register_event(loc, EVID_STOP_PLAYBACKCAPTURE);

    //loc = sdlx_render_text( 0, ROW2Y(10), "Test");
    //sdlx_register_event(loc, EVID_TEST_PLAYBACKCAPTURE);

    //loc = sdlx_render_text( 0, ROW2Y(14), "PlayPcm");
    //sdlx_register_event(loc, EVID_PLAY_PCM);

    loc = sdlx_render_text( 0, ROW2Y(18), "PlayMp3");
    sdlx_register_event(loc, EVID_PLAY_MP3);

    sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);
}

static void page_11_process_event(sdlx_event_t *ev)
{
    switch (ev->event_id) {
    case EVID_START_PLAYBACKCAPTURE:
        //util_start_playbackcapture();
        sdlx_start_playbackcapture(data_dir, "pbc.mp3");
        break;
    case EVID_STOP_PLAYBACKCAPTURE:
        //util_stop_playbackcapture();
        sdlx_stop_playbackcapture();
        break;
#if 0
    case EVID_TEST_PLAYBACKCAPTURE: {  //xxx cleanup
        short array[8000];
        int i, j, sum;

        util_start_playbackcapture();
        for (i = 0; i < 100; i++) {
            memset(array,0,sizeof(array));
            util_get_playbackcapture_audio(array, sizeof(array)/2);
            sum = 0;
            for (j = 0; j < 8000; j++) {
                sum += array[j];
            }
            printf("SUM = %d\n", sum);
        }
        util_stop_playbackcapture();
        break; }
#endif
    //case EVID_PLAY_PCM:
        //sdlx_audio_play_new(data_dir, "captured_audio.pcm");
        //break;
    case EVID_PLAY_MP3:
        sdlx_audio_play_new(data_dir, "pbc.mp3");
        break;
    }
}

static void page_11_exit(void)
{
    util_stop_playbackcapture();
}
