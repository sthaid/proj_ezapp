#include "../interpreter.h"
#include <sdlx.h>
#include <utils.h>
#include <svcs.h>

struct StdVararg
{
    struct Value **Param;
    int NumArgs;
};

int StdioBasePrintf(struct ParseState *Parser, FILE *Stream, char *StrOut,
    int StrOutLen, char *Format, struct StdVararg *Args);

// -----------------  SDL PLATFORM ROUTINES  ----------------------------

//
// sdl initialization and termination, must be done once
//

void Sdl_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int subsys = Param[0]->Val->Integer;
    int ret;

    ret = sdlx_init(subsys);

    ReturnValue->Val->Integer = ret;
}

void Sdl_quit(struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int subsys = Param[0]->Val->Integer;

    sdlx_quit(subsys);
}

//
// display init and present, must be done for every display update
//

void Sdl_display_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;

    sdlx_display_init(color);
}

void Sdl_display_present (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_display_present();
}

//
// event registration and query
//

void Sdl_register_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_loc_t *loc      = (sdlx_loc_t*)Param[0]->Val->Pointer;
    int        event_id = Param[1]->Val->Integer;

    sdlx_register_event(loc, event_id);
}

void Sdl_register_control_events (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *evstr1   = (char*)Param[0]->Val->Pointer;
    char *evstr2   = (char*)Param[1]->Val->Pointer;
    char *evstr3   = (char*)Param[2]->Val->Pointer;
    int   fg_color = Param[3]->Val->Integer;
    int   bg_color = Param[4]->Val->Integer;
    int   evid1    = Param[5]->Val->Integer;
    int   evid2    = Param[6]->Val->Integer;
    int   evid3    = Param[7]->Val->Integer;

    sdlx_register_control_events(evstr1, evstr2, evstr3, fg_color, bg_color, evid1, evid2, evid3);
}

void Sdl_get_event (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    long         timeout_us = Param[0]->Val->LongInteger;
    sdlx_event_t *event      = Param[1]->Val->Pointer;

    sdlx_get_event(timeout_us, event);
}

void Sdl_get_input_str (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *prompt1       = Param[0]->Val->Pointer;
    char *prompt2       = Param[1]->Val->Pointer;
    bool  numeric_keybd = Param[2]->Val->Integer;
    int   bg_color      = Param[3]->Val->Integer;
    char *input_str;

    input_str = sdlx_get_input_str(prompt1, prompt2, numeric_keybd, bg_color);
    ReturnValue->Val->Pointer = input_str;
}

//
// create colors
//

void Sdl_create_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int r = Param[0]->Val->Integer;
    int g = Param[1]->Val->Integer;
    int b = Param[2]->Val->Integer;
    int a = Param[3]->Val->Integer;
    int color;

    color = sdlx_create_color(r, g, b, a);
    
    ReturnValue->Val->Integer = color;
}

void Sdl_scale_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;
    double inten = Param[1]->Val->FP;
    int scaled_color;

    scaled_color = sdlx_scale_color(color, inten);

    ReturnValue->Val->Integer = scaled_color;
}

void Sdl_set_color_alpha (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int color = Param[0]->Val->Integer;
    int alpha = Param[1]->Val->Integer;
    int ret_color;

    ret_color = sdlx_set_color_alpha(color, alpha);

    ReturnValue->Val->Integer = ret_color;
}

void Sdl_wavelength_to_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int wavelength = Param[0]->Val->Integer;
    int color;

    color = sdlx_wavelength_to_color(wavelength);

    ReturnValue->Val->Integer = color;
}

//
// render text
//

void Sdl_print_init (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double numchars = Param[0]->Val->FP;
    int    fg_color = Param[1]->Val->Integer;
    int    bg_color = Param[2]->Val->Integer;

    sdlx_print_init(numchars, fg_color, bg_color);
}

void Sdl_print_init_numchars (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double numchars = Param[0]->Val->FP;

    sdlx_print_init_numchars(numchars);
}

void Sdl_print_init_color (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int fg_color = Param[0]->Val->Integer;
    int bg_color = Param[1]->Val->Integer;

    sdlx_print_init_color(fg_color, bg_color);
}

void Sdl_print_save (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_print_state_t *print_state = (sdlx_print_state_t*)Param[0]->Val->Pointer;

    sdlx_print_save(print_state);
}

void Sdl_print_restore (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_print_state_t *print_state = (sdlx_print_state_t*)Param[0]->Val->Pointer;

    sdlx_print_restore(print_state);
}

void Sdl_render_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x   = Param[0]->Val->Integer;
    int         y   = Param[1]->Val->Integer;
    char       *str = Param[2]->Val->Pointer;
    sdlx_loc_t  *loc;

    loc = sdlx_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x   = Param[0]->Val->Integer;
    int   y   = Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdlx_loc_t       *loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdlx_render_text(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_text_xyctr (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int         x   = Param[0]->Val->Integer;
    int         y   = Param[1]->Val->Integer;
    char       *str = Param[2]->Val->Pointer;
    sdlx_loc_t  *loc;

    loc = sdlx_render_text_xyctr(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_printf_xyctr (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x   = Param[0]->Val->Integer;
    int   y   = Param[1]->Val->Integer;
    char *fmt = Param[2]->Val->Pointer;

    struct StdVararg PrintfArgs;
    char             str[200] = "";
    sdlx_loc_t       *loc;

    PrintfArgs.Param = Param + 2;
    PrintfArgs.NumArgs = NumArgs - 3;
    StdioBasePrintf(Parser, NULL, str, sizeof(str), fmt, &PrintfArgs);

    loc = sdlx_render_text_xyctr(x, y, str);

    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_multiline_text (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int    y_top           = Param[0]->Val->Integer;
    int    y_display_begin = Param[1]->Val->Integer;
    int    y_display_end   = Param[2]->Val->Integer;
    char **lines           = Param[3]->Val->Pointer;
    int    n               = Param[4]->Val->Integer;

    sdlx_render_multiline_text(y_top, y_display_begin, y_display_end, lines, n);
}

//
// render rectangle, lines, circles, points
//

void Sdl_render_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x          = Param[0]->Val->Integer;
    int   y          = Param[1]->Val->Integer;
    int   w          = Param[2]->Val->Integer;
    int   h          = Param[3]->Val->Integer;
    int   line_width = Param[4]->Val->Integer;
    int   color      = Param[5]->Val->Integer;

    sdlx_render_rect(x, y, w, h, line_width, color);
}

void Sdl_render_fill_rect (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x          = Param[0]->Val->Integer;
    int   y          = Param[1]->Val->Integer;
    int   w          = Param[2]->Val->Integer;
    int   h          = Param[3]->Val->Integer;
    int   color      = Param[4]->Val->Integer;

    sdlx_render_fill_rect(x, y, w, h, color);
}

void Sdl_render_line (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x1    = Param[0]->Val->Integer;
    int y1    = Param[1]->Val->Integer;
    int x2    = Param[2]->Val->Integer;
    int y2    = Param[3]->Val->Integer;
    int color = Param[4]->Val->Integer;

    sdlx_render_line(x1, y1, x2, y2, color);
}

void Sdl_render_lines (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_point_t *points = (sdlx_point_t*)Param[0]->Val->Pointer;
    int count           = Param[1]->Val->Integer;
    int color           = Param[2]->Val->Integer;

    sdlx_render_lines(points, count, color);
}

void Sdl_render_circle (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int  x_ctr      = Param[0]->Val->Integer;
    int  y_ctr      = Param[1]->Val->Integer;
    int  radius     = Param[2]->Val->Integer;
    int  line_width = Param[3]->Val->Integer;
    int  color      = Param[4]->Val->Integer;

    sdlx_render_circle(x_ctr, y_ctr, radius, line_width, color);
}

void Sdl_render_point (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int x          = Param[0]->Val->Integer;
    int y          = Param[1]->Val->Integer;
    int color      = Param[2]->Val->Integer;
    int point_size = Param[3]->Val->Integer;

    sdlx_render_point(x, y, color, point_size);
}

void Sdl_render_points (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_point_t *points = (sdlx_point_t*)Param[0]->Val->Pointer;
    int count           = Param[1]->Val->Integer;
    int color           = Param[2]->Val->Integer;
    int point_size      = Param[3]->Val->Integer;

    sdlx_render_points(points, count, color, point_size);
}

//
// render using textures
//

void Sdl_create_texture_from_pixels (struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    unsigned char  *pixels = Param[0]->Val->Pointer;
    int             w      = Param[1]->Val->Integer;
    int             h      = Param[2]->Val->Integer;
    sdlx_texture_t *texture;

    texture = sdlx_create_texture_from_pixels(pixels, w, h);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_filled_circle_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int radius = Param[0]->Val->Integer;
    int color  = Param[1]->Val->Integer;
    sdlx_texture_t *texture;

    texture = sdlx_create_filled_circle_texture(radius, color);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_create_text_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *str = (char*)Param[0]->Val->Pointer;
    sdlx_texture_t *texture;

    texture = sdlx_create_text_texture(str);
    ReturnValue->Val->Pointer = (char*)texture; 
}

void Sdl_render_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int            x       = Param[0]->Val->Integer;
    int            y       = Param[1]->Val->Integer;
    int            w       = Param[2]->Val->Integer;
    int            h       = Param[3]->Val->Integer;
    sdlx_texture_t *texture = (sdlx_texture_t*)Param[4]->Val->Pointer;
    sdlx_loc_t     *loc;

    loc = sdlx_render_texture(x, y, w, h, texture);
    ReturnValue->Val->Pointer = loc;
}

void Sdl_render_texture_ex (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int            x       = Param[0]->Val->Integer;
    int            y       = Param[1]->Val->Integer;
    int            w       = Param[2]->Val->Integer;
    int            h       = Param[3]->Val->Integer;
    double         angle   = Param[4]->Val->FP;
    sdlx_texture_t *texture = (sdlx_texture_t*)Param[5]->Val->Pointer;

    sdlx_render_texture_ex(x, y, w, h, angle, texture);
}

void Sdl_render_texture_ex2 (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int            x       = Param[0]->Val->Integer;
    int            y       = Param[1]->Val->Integer;
    int            w       = Param[2]->Val->Integer;
    int            h       = Param[3]->Val->Integer;
    double         angle   = Param[4]->Val->FP;
    int            xctr    = Param[5]->Val->Integer;
    int            yctr    = Param[6]->Val->Integer;
    sdlx_texture_t *texture = (sdlx_texture_t*)Param[7]->Val->Pointer;

    sdlx_render_texture_ex2(x, y, w, h, angle, xctr, yctr, texture);
}

void Sdl_destroy_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_texture_t *texture = (sdlx_texture_t*)Param[0]->Val->Pointer;

    sdlx_destroy_texture(texture);
}

void Sdl_query_texture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_texture_t *texture = (sdlx_texture_t*)Param[0]->Val->Pointer;
    int           *width   = (int*)Param[1]->Val->Pointer;
    int           *height  = (int*)Param[2]->Val->Pointer;

    sdlx_query_texture(texture, width, height);
}

void Sdl_read_display_pixels (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int   x        = Param[0]->Val->Integer;
    int   y        = Param[1]->Val->Integer;
    int   w        = Param[2]->Val->Integer;
    int   h        = Param[3]->Val->Integer;
    int  *w_pixels = Param[4]->Val->Pointer;
    int  *h_pixels = Param[5]->Val->Pointer;
    unsigned char *pixels;

    pixels = sdlx_read_display_pixels(x, y, w, h, w_pixels, h_pixels);
    ReturnValue->Val->Pointer = pixels; 
}

//
// plot
//

void Sdl_plot_create (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *title           = Param[0]->Val->Pointer;
    int xleft             = Param[1]->Val->Integer;
    int xright            = Param[2]->Val->Integer;
    int ybottom           = Param[3]->Val->Integer;
    int ytop              = Param[4]->Val->Integer;
    double xval_left      = Param[5]->Val->FP;
    double xval_right     = Param[6]->Val->FP;
    double yval_bottom    = Param[7]->Val->FP;
    double yval_top       = Param[8]->Val->FP;
    double yval_of_x_axis = Param[9]->Val->FP;

    void *plot_cx;

    plot_cx = sdlx_plot_create(title, 
                              xleft, xright, ybottom, ytop, 
                              xval_left, xval_right, yval_bottom, yval_top, 
                              yval_of_x_axis);

    ReturnValue->Val->Pointer = plot_cx;
}

void Sdl_plot_axis (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    void *plot_cx  = Param[0]->Val->Pointer;
    char *xmin_str = Param[1]->Val->Pointer;
    char *xmax_str = Param[2]->Val->Pointer;
    char *ymin_str = Param[3]->Val->Pointer;
    char *ymax_str = Param[4]->Val->Pointer;

    sdlx_plot_axis(plot_cx, xmin_str, xmax_str, ymin_str, ymax_str);
}

void Sdl_plot_points (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    void *plot_cx         = Param[0]->Val->Pointer;
    sdlx_plot_point_t *pts = Param[1]->Val->Pointer;
    int num_pts           = Param[2]->Val->Integer;

    sdlx_plot_points(plot_cx, pts, num_pts);
}

void Sdl_plot_bars (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    void *plot_cx             = Param[0]->Val->Pointer;
    sdlx_plot_point_t *pts_avg = Param[1]->Val->Pointer;
    sdlx_plot_point_t *pts_min = Param[2]->Val->Pointer;;
    sdlx_plot_point_t *pts_max = Param[3]->Val->Pointer;
    int num_pts               = Param[4]->Val->Integer;
    double bar_wval           = Param[5]->Val->FP;

    sdlx_plot_bars(plot_cx, pts_avg, pts_min, pts_max, num_pts, bar_wval);
}

void Sdl_plot_free (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    void *plot_cx = Param[0]->Val->Pointer;

    sdlx_plot_free(plot_cx);
}

//
// audio
//

void Sdl_audio_play (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir      = Param[0]->Val->Pointer;
    char *filename = Param[1]->Val->Pointer;
    int rc;

    rc = sdlx_audio_play(dir, filename);
    ReturnValue->Val->Integer = rc; 
}

void Sdl_audio_record (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir                = Param[0]->Val->Pointer;
    char *filename           = Param[1]->Val->Pointer;
    int   max_duration_secs  = Param[2]->Val->Integer;
    int   auto_stop_secs     = Param[3]->Val->Integer;
    bool  append             = Param[4]->Val->Integer;
    int   rc;

    rc = sdlx_audio_record(dir, filename, max_duration_secs, auto_stop_secs, append);
    ReturnValue->Val->Integer = rc; 
}

void Sdl_audio_play_tones (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_tone_t *tones = Param[0]->Val->Pointer;
    int         rc;

    rc = sdlx_audio_play_tones(tones);
    ReturnValue->Val->Integer = rc; 
}

void Sdl_audio_file_duration (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir      = Param[0]->Val->Pointer;
    char *filename = Param[1]->Val->Pointer;
    int   secs;

    secs = sdlx_audio_file_duration(dir, filename);
    ReturnValue->Val->Integer = secs; 
}

void Sdl_audio_ctl (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int req = Param[0]->Val->Integer;

    sdlx_audio_ctl(req);
}

void Sdl_audio_state (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_audio_state_t *state = Param[0]->Val->Pointer;

    sdlx_audio_state(state);
}

void Sdl_audio_print_device_info (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_audio_print_devices_info();
}

void Sdl_audio_create_test_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir           = Param[0]->Val->Pointer;
    char *filename      = Param[1]->Val->Pointer;
    int   duration_secs = Param[2]->Val->Integer;
    int   freq          = Param[3]->Val->Integer;

    sdlx_audio_create_test_file(dir, filename, duration_secs, freq);
}

void Sdl_audio_play_new (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir      = Param[0]->Val->Pointer;
    char *filename = Param[1]->Val->Pointer;
    int rc;

    rc = sdlx_audio_play_new(dir, filename);  //xxx name
    ReturnValue->Val->Integer = rc; 
}

void Sdl_start_playbackcapture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir      = Param[0]->Val->Pointer;
    char *filename = Param[1]->Val->Pointer;

    sdlx_start_playbackcapture(dir, filename);
}

void Sdl_stop_playbackcapture (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    sdlx_stop_playbackcapture();
}

//
// sensors
//

void Sdl_sensor_get_info_tbl (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int *num_sensors = Param[0]->Val->Pointer;
    sdlx_sensor_info_t *sit;

    sit = sdlx_sensor_get_info_tbl(num_sensors);
    ReturnValue->Val->Pointer = sit;
}

void Sdl_sensor_find (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int type = Param[0]->Val->Integer;
    int id;

    id = sdlx_sensor_find(type);
    ReturnValue->Val->Integer = id;
}

void Sdl_sensor_read_raw (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    int     id         = Param[0]->Val->Integer;
    double *data       = Param[1]->Val->Pointer;
    int     num_values = Param[2]->Val->Integer;
    int     rc;

    rc = sdlx_sensor_read_raw(id, data, num_values);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_step_counter (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *step_count = Param[0]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_step_counter(step_count);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_mag_heading (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *mag_heading = Param[0]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_mag_heading(mag_heading);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_accelerometer (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *ax = Param[0]->Val->Pointer;
    double *ay = Param[1]->Val->Pointer;
    double *az = Param[2]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_accelerometer(ax, ay, az);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_roll_pitch (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *roll = Param[0]->Val->Pointer;
    double *pitch = Param[1]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_roll_pitch(roll, pitch);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_pressure (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *millibars = Param[0]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_pressure(millibars);
    ReturnValue->Val->Integer = rc;
}

void Sdl_sensor_read_temperature (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *degrees_c = Param[0]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_temperature(degrees_c);
    ReturnValue->Val->Integer = rc;
}

// xxx use Sdlx
void Sdl_sensor_read_humidity (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    double *percent = Param[0]->Val->Pointer;
    int     rc;

    rc = sdlx_sensor_read_humidity(percent);
    ReturnValue->Val->Integer = rc;
}

//
// misc
//

void Sdl_show_toast (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *msg = Param[0]->Val->Pointer;
    sdlx_show_toast(msg);
}

//
// SDL REGISTRATION
//

void SdlSetupFunction(Picoc *pc)
{
    #define PLATFORM_VAR(name, type, writeable) \
        do { \
            VariableDefinePlatformVar(pc, NULL, #name, type, \
                                      (union AnyValue *)&name, writeable); \
        } while (0)
        
    PLATFORM_VAR(sdlx_win_width, &pc->IntType, false);
    PLATFORM_VAR(sdlx_win_height, &pc->IntType, false);
    PLATFORM_VAR(sdlx_char_width, &pc->IntType, false);
    PLATFORM_VAR(sdlx_char_height, &pc->IntType, false);
}

struct LibraryFunction SdlFunctions[] = {
    // sdl initialization and termination, must be done once
    { Sdl_init,            "int sdlx_init(int subsys);" },
    { Sdl_quit,            "void sdlx_quit(int subsys);" },

    // display init and present, must be done for every display update
    { Sdl_display_init,    "void sdlx_display_init(int color);" },
    { Sdl_display_present, "void sdlx_display_present(void);" },

    // event registration and query
    { Sdl_register_event,  "void sdlx_register_event(sdlx_loc_t *loc, int event_id);" },
    { Sdl_register_control_events, 
                           "void sdlx_register_control_events(char *evstr1, char *evstr2, char *evstr3, int fg_color, int bg_color, int evid1, int evid2, int evid3); " },
    { Sdl_get_event,       "void sdlx_get_event(long timeout_us, sdlx_event_t *event);" },
    { Sdl_get_input_str,   "char *sdlx_get_input_str(char *prompt1, char *prompt2, bool numeric_keybd, int bg_color);" },

    // create colors
    { Sdl_create_color,    "int sdlx_create_color(int r, int g, int b, int a);" },
    { Sdl_scale_color,     "int sdlx_scale_color(int color, double inten);" },
    { Sdl_set_color_alpha, "int sdlx_set_color_alpha(int color, int alpha);" },
    { Sdl_wavelength_to_color, "int sdlx_wavelength_to_color(int wavelength);" },

    // render text
    { Sdl_print_init,              "void sdlx_print_init(double numchars, int fg_color, int bg_color);" },
    { Sdl_print_init_numchars,     "void sdlx_print_init_numchars(double numchars);" },
    { Sdl_print_init_color,        "void sdlx_print_init_color(int fg_color, int bg_color);" },
    { Sdl_print_save,              "void sdlx_print_save(sdlx_print_state_t *save);" },
    { Sdl_print_restore,           "void sdlx_print_restore(sdlx_print_state_t *restore);" },
    { Sdl_render_text,             "sdlx_loc_t *sdlx_render_text(int x, int y, char *str);" },
    { Sdl_render_printf,           "sdlx_loc_t *sdlx_render_printf(int x, int y, char *fmt, ...);" },
    { Sdl_render_text_xyctr,       "sdlx_loc_t *sdlx_render_text_xyctr(int x, int y, char *str);" },
    { Sdl_render_printf_xyctr,     "sdlx_loc_t *sdlx_render_printf_xyctr(int x, int y, char *fmt, ...);" },
    { Sdl_render_multiline_text,   "void sdlx_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char **lines, int n);" },

    // render rectangle, lines, circles, points
    { Sdl_render_rect,     "void sdlx_render_rect(int x, int y, int w, int h, int line_width, int color);" },
    { Sdl_render_fill_rect,"void sdlx_render_fill_rect(int x, int y, int w, int h, int color);" },
    { Sdl_render_line,     "void sdlx_render_line(int x1, int y1, int x2, int y2, int color);" },
    { Sdl_render_lines,    "void sdlx_render_lines(sdlx_point_t *points, int count, int color);" },
    { Sdl_render_circle,   "void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);" },
    { Sdl_render_point,    "void sdlx_render_point(int x, int y, int color, int point_size);" },
    { Sdl_render_points,   "void sdlx_render_points(sdlx_point_t *points, int count, int color, int point_size);" },

    // render using textures
    { Sdl_create_texture_from_pixels,   "sdlx_texture_t *sdlx_create_texture_from_pixels(unsigned char *pixels, int w, int h);" },
    { Sdl_create_filled_circle_texture, "sdlx_texture_t *sdlx_create_filled_circle_texture(int radius, int color);" },
    { Sdl_create_text_texture,          "sdlx_texture_t *sdlx_create_text_texture(char *str);" },
    { Sdl_render_texture,               "sdlx_loc_t *sdlx_render_texture(int x, int y, int w, int h, sdlx_texture_t *texture);" },
    { Sdl_render_texture_ex,            "void sdlx_render_texture_ex(int x, int y, int w, int h, double angle, sdlx_texture_t *texture);" },
    { Sdl_render_texture_ex2,           "void sdlx_render_texture_ex2(int x, int y, int w, int h, double angle, int xctr, int yctr, sdlx_texture_t *texture);" },
    { Sdl_destroy_texture,              "void sdlx_destroy_texture(sdlx_texture_t *texture);" },
    { Sdl_query_texture,                "void sdlx_query_texture(sdlx_texture_t *texture, int *width, int *height);" },
    { Sdl_read_display_pixels,          "void *sdlx_read_display_pixels(int x, int y, int w, int h, int *w_pixels, int *h_pixels);" },

    // plotting
    { Sdl_plot_create,                   "void *sdlx_plot_create("
                                              "char *title, "
                                              "int xleft, int xright, int ybottom, int ytop, "
                                              "double xval_left, double xval_right, double yval_bottom, double yval_top, "
                                              "double yval_of_x_axis);" },
    { Sdl_plot_axis,                     "void sdlx_plot_axis("
                                              "void *cx, "
                                              "char *xmin_str, char *xmax_str, "
                                              "char *ymin_str, char *ymax_str);" },
    { Sdl_plot_points,                   "void sdlx_plot_points(void *cx, sdlx_plot_point_t *pts, int num_pts);" },
    { Sdl_plot_bars,                     "void sdlx_plot_bars(void *cx,"
                                              "sdlx_plot_point_t *pts_avg, sdlx_plot_point_t *pts_min, "
                                              "sdlx_plot_point_t *pts_max, int num_pts, double bar_wval);" },
    { Sdl_plot_free,                     "void sdlx_plot_free(void *cx);" },

    // audio
    { Sdl_audio_play,                   "int sdlx_audio_play(char *dir, char *filename);" },
    { Sdl_audio_record,                 "int sdlx_audio_record(char *dir, char *filename, int max_duration_secs, int auto_stop_secs, bool append); "},
    { Sdl_audio_play_tones,             "int sdlx_audio_play_tones(sdlx_tone_t *tones);" },
    { Sdl_audio_file_duration,          "int sdlx_audio_file_duration(char *dir, char *filename);" },
    { Sdl_audio_ctl,                    "void sdlx_audio_ctl(int req);" },
    { Sdl_audio_state,                  "void sdlx_audio_state(sdlx_audio_state_t * state);" },
    { Sdl_audio_print_device_info,      "void sdlx_audio_print_devices_info(void);" },
    { Sdl_audio_create_test_file,       "void sdlx_audio_create_test_file(char *dir, char *filename, int duration_secs, int freq);" },
    { Sdl_audio_play_new,               "int sdlx_audio_play_new(char *dir, char *filename);" },
    { Sdl_start_playbackcapture,        "void sdlx_start_playbackcapture(char *dir, char *filename);" },
    { Sdl_stop_playbackcapture,         "void sdlx_stop_playbackcapture(void);" },

    // sensors
    { Sdl_sensor_get_info_tbl,          "sdlx_sensor_info_t *sdlx_sensor_get_info_tbl(int *num_sensors);" },
    { Sdl_sensor_find,                  "int sdlx_sensor_find(int type);" },
    { Sdl_sensor_read_raw,              "int sdlx_sensor_read_raw(int id, double *data, int num_values);" },
    { Sdl_sensor_read_step_counter,     "int sdlx_sensor_read_step_counter(double *step_count);" },
    { Sdl_sensor_read_mag_heading,      "int sdlx_sensor_read_mag_heading(double *mag_heading);" },
    { Sdl_sensor_read_accelerometer,    "int sdlx_sensor_read_accelerometer(double *ax, double *ay, double *az);" },
    { Sdl_sensor_read_roll_pitch,       "int sdlx_sensor_read_roll_pitch(double *roll, double *pitch);" },
    { Sdl_sensor_read_pressure,         "int sdlx_sensor_read_pressure(double *millibars);" },
    { Sdl_sensor_read_temperature,      "int sdlx_sensor_read_temperature(double *degrees_c);" },
    { Sdl_sensor_read_humidity,         "int sdlx_sensor_read_humidity(double *percent);" },

    // misc
    { Sdl_show_toast,                   "void sdlx_show_toast(char *msg);" },

    { NULL, NULL } };

const char SdlDefs[] = "\
typedef struct sdlx_texture sdlx_texture_t; \n\
typedef struct { \n\
    int x; \n\
    int y; \n\
    int w; \n\
    int h; \n\
} sdlx_loc_t; \n\
typedef struct { \n\
    int x; \n\
    int y; \n\
} sdlx_point_t; \n\
typedef struct { \n\
    int event_id; \n\
    union { \n\
        struct { \n\
            double x; double y; double xrel; double yrel; \n\
        } motion; \n\
    } u; \n\
} sdlx_event_t; \n\
typedef struct { \n\
    short freq; \n\
    short intvl_ms; \n\
} sdlx_tone_t; \n\
typedef struct { \n\
    int  state; \n\
    bool paused; \n\
    int  processed_ms; \n\
    int  total_ms; \n\
    int  volume; \n\
    char filename[100]; \n\
} sdlx_audio_state_t; \n\
typedef struct { \n\
    int   id; \n\
    int   type; \n\
    char *name; \n\
} sdlx_sensor_info_t; \n\
typedef struct { \n\
    int ptsize; \n\
    int char_width; \n\
    int char_height; \n\
    int bg_color; \n\
    int fg_color; \n\
} sdlx_print_state_t; \n\
typedef struct { \n\
    double xval; \n\
    double yval; \n\
} sdlx_plot_point_t; \n\
\n\
#define SUBSYS_VIDEO  1 \n\
#define SUBSYS_AUDIO  2 \n\
#define SUBSYS_SENSOR 4 \n\
\n\
#define BYTES_PER_PIXEL  4 \n\
#define COLOR_BLACK       (   0  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_WHITE       ( 255  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_RED         ( 255  |    0<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_ORANGE      ( 255  |  128<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_YELLOW      ( 255  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_GREEN       (   0  |  255<<8 |    0<<16 |  255<<24 ) \n\
#define COLOR_BLUE        (   0  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_INDIGO      (  75  |    0<<8 |  130<<16 |  255<<24 ) \n\
#define COLOR_VIOLET      ( 238  |  130<<8 |  238<<16 |  255<<24 ) \n\
#define COLOR_PURPLE      ( 127  |    0<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_BLUE  (   0  |  255<<8 |  255<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_GREEN ( 144  |  238<<8 |  144<<16 |  255<<24 ) \n\
#define COLOR_PINK        ( 255  |  105<<8 |  180<<16 |  255<<24 ) \n\
#define COLOR_TEAL        (   0  |  128<<8 |  128<<16 |  255<<24 ) \n\
#define COLOR_LIGHT_GRAY  ( 192  |  192<<8 |  192<<16 |  255<<24 ) \n\
#define COLOR_GRAY        ( 128  |  128<<8 |  128<<16 |  255<<24 ) \n\
#define COLOR_DARK_GRAY   (  64  |   64<<8 |   64<<16 |  255<<24 ) \n\
\n\
#define SMALLEST_FONT 40 \n\
#define SMALL_FONT    30 \n\
#define DEFAULT_FONT  20 \n\
#define LARGE_FONT    10 \n\
 \n\
#define ROW2Y(r) ((r) * sdlx_char_height) \n\
#define COL2X(c) ((c) * sdlx_char_width) \n\
\n\
#define EVID_SWIPE_RIGHT       9990 \n\
#define EVID_SWIPE_LEFT        9991 \n\
#define EVID_MOTION            9992 \n\
#define EVID_QUIT              9999 \n\
\n\
#define AUDIO_STATE_IDLE          0 \n\
#define AUDIO_STATE_PLAY_FILE     1 \n\
#define AUDIO_STATE_PLAY_TONES    2 \n\
#define AUDIO_STATE_RECORD        3 \n\
#define AUDIO_STATE_RECORD_APPEND 4 \n\
\n\
#define AUDIO_REQ_STOP     1 \n\
#define AUDIO_REQ_PAUSE    2 \n\
#define AUDIO_REQ_UNPAUSE  3 \n\
\n\
#define ASENSOR_TYPE_ACCELEROMETER       1 \n\
#define ASENSOR_TYPE_MAGNETIC_FIELD      2 \n\
#define ASENSOR_TYPE_GYROSCOPE           4 \n\
#define ASENSOR_TYPE_LIGHT               5 \n\
#define ASENSOR_TYPE_PRESSURE            6 \n\
#define ASENSOR_TYPE_PROXIMITY           8 \n\
#define ASENSOR_TYPE_GRAVITY             9 \n\
#define ASENSOR_TYPE_LINEAR_ACCELERATION 10 \n\
#define ASENSOR_TYPE_ROTATION_VECTOR     11 \n\
#define ASENSOR_TYPE_RELATIVE_HUMIDITY   12 \n\
#define ASENSOR_TYPE_AMBIENT_TEMPERATURE 13 \n\
#define ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED 14 \n\
#define ASENSOR_TYPE_GAME_ROTATION_VECTOR 15 \n\
#define ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED 16 \n\
#define ASENSOR_TYPE_SIGNIFICANT_MOTION 17 \n\
#define ASENSOR_TYPE_STEP_DETECTOR 18 \n\
#define ASENSOR_TYPE_STEP_COUNTER 19 \n\
#define ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR 20 \n\
#define ASENSOR_TYPE_HEART_RATE 21 \n\
#define ASENSOR_TYPE_POSE_6DOF 28 \n\
#define ASENSOR_TYPE_STATIONARY_DETECT 29 \n\
#define ASENSOR_TYPE_MOTION_DETECT 30 \n\
#define ASENSOR_TYPE_HEART_BEAT 31 \n\
#define ASENSOR_TYPE_DYNAMIC_SENSOR_META 32 \n\
#define ASENSOR_TYPE_ADDITIONAL_INFO 33 \n\
#define ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT 34 \n\
#define ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED 35 \n\
#define ASENSOR_TYPE_HINGE_ANGLE 36 \n\
#define ASENSOR_TYPE_HEAD_TRACKER 37 \n\
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES 38 \n\
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES 39 \n\
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES_UNCALIBRATED 40 \n\
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES_UNCALIBRATED 41 \n\
#define ASENSOR_TYPE_HEADING 42 \n\
\n\
#define INVALID_NUMBER 999999999 \n\
";

// -----------------  UTILS PLATFORM ROUTINES  --------------------------

//
// utils time routines
//

void Util_microsec_timer (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    ReturnValue->Val->LongInteger = util_microsec_timer();
}

void Util_get_real_time_microsec (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    ReturnValue->Val->LongInteger = util_get_real_time_microsec();
}

void Util_time2str (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *str          = Param[0]->Val->Pointer;
    long  us           = Param[1]->Val->LongInteger;
    int   gmt          = Param[2]->Val->Integer;
    int   display_ms   = Param[3]->Val->Integer;
    int   display_date = Param[4]->Val->Integer;
    char *s;

    s = util_time2str(str, us, gmt, display_ms, display_date);
    ReturnValue->Val->Pointer = s;
}

//
// utils file write / read routines
//

void Util_write_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir  = Param[0]->Val->Pointer;
    char *fn   = Param[1]->Val->Pointer;
    void *data = Param[2]->Val->Pointer;
    int   len  = Param[3]->Val->Integer;
    int   ret;

    ret = util_write_file(dir, fn, data, len);
    ReturnValue->Val->Integer = ret;
}

void Util_read_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir  = Param[0]->Val->Pointer;
    char *fn   = Param[1]->Val->Pointer;
    int  *len  = Param[2]->Val->Pointer;
    void *file_contents;

    file_contents = util_read_file(dir, fn, len);
    ReturnValue->Val->Pointer = file_contents;
}

void Util_delete_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir  = Param[0]->Val->Pointer;
    char *fn   = Param[1]->Val->Pointer;

    util_delete_file(dir, fn);
}

void Util_file_exists (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir   = Param[0]->Val->Pointer;
    char *fn    = Param[1]->Val->Pointer;
    bool exists;

    exists = util_file_exists(dir, fn);
    ReturnValue->Val->Integer = exists;
}

void Util_file_mtime (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir   = Param[0]->Val->Pointer;
    char *fn    = Param[1]->Val->Pointer;
    long mtime;

    mtime = util_file_mtime(dir, fn);
    ReturnValue->Val->LongInteger = mtime;
}

void Util_file_size (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir   = Param[0]->Val->Pointer;
    char *fn    = Param[1]->Val->Pointer;
    long size;

    size = util_file_size(dir, fn);
    ReturnValue->Val->LongInteger = size;
}

//
// utils directory routines
//

void Util_delete_dir (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir            = Param[0]->Val->Pointer;
    char *dir_to_delete  = Param[1]->Val->Pointer;

    util_delete_dir(dir, dir_to_delete);
}

//
// utils file map routines
//

void Util_map_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *dir              = Param[0]->Val->Pointer;
    char *file             = Param[1]->Val->Pointer;
    int   len              = Param[2]->Val->Integer;
    bool  create_if_needed = Param[3]->Val->Integer;
    bool  read_only        = Param[4]->Val->Integer;
    int   *created_flag    = Param[5]->Val->Pointer;
    void *addr;

    addr = util_map_file(dir, file, len, create_if_needed, read_only, created_flag);
    ReturnValue->Val->Pointer = addr;
}

void Util_unmap_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *addr = Param[0]->Val->Pointer;
    int   len  = Param[1]->Val->Integer;

    util_unmap_file(addr, len);
}

void Util_sync_file (struct ParseState *Parser, struct Value *ReturnValue,
	struct Value **Param, int NumArgs)
{
    char *addr = Param[0]->Val->Pointer;
    int   len  = Param[1]->Val->Integer;

    util_sync_file(addr, len);
}

//
// utils params
//

void Util_get_str_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir           = Param[0]->Val->Pointer;
    char *name          = Param[1]->Val->Pointer;
    char *default_value = Param[2]->Val->Pointer;
    char *value;

    value = util_get_str_param(dir, name, default_value);
    ReturnValue->Val->Pointer = value;
}

void Util_set_str_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir   = Param[0]->Val->Pointer;
    char *name  = Param[1]->Val->Pointer;
    char *value = Param[2]->Val->Pointer;

    util_set_str_param(dir, name, value);
}

void Util_get_numeric_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char  *dir           = Param[0]->Val->Pointer;
    char  *name          = Param[1]->Val->Pointer;
    double default_value = Param[2]->Val->FP;
    double value;

    value = util_get_numeric_param(dir, name, default_value);
    ReturnValue->Val->FP = value;
}

void Util_set_numeric_param(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char  *dir   = Param[0]->Val->Pointer;
    char  *name  = Param[1]->Val->Pointer;
    double value = Param[2]->Val->FP;

    util_set_numeric_param(dir, name, value);
}

void Util_print_params(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *dir = Param[0]->Val->Pointer;

    util_print_params(dir);
}

//
// utils network
//

void Util_get_ipaddr(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *ipaddr;

    ipaddr = util_get_ipaddr();
    ReturnValue->Val->Pointer = ipaddr;
}

//
// utils json decoder
//

void Util_json_parse(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *str =      (char*)Param[0]->Val->Pointer;
    char **end_ptr = (char**)Param[1]->Val->Pointer;
    void *json_root;

    json_root = util_json_parse(str, end_ptr);

    ReturnValue->Val->Pointer = json_root;
}

void Util_json_free(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    void *json_root = Param[0]->Val->Pointer;

    util_json_free(json_root);
}

void Util_json_get_value(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    void         *json_item = Param[0]->Val->Pointer;
    struct        Value *ThisArg = Param[0];
    int           i;
    char         *va[10];
    json_value_t *value;

    memset(va, 0, sizeof(va)); // xxx check for too many args  and/or add a few more

    for (i = 0; i < NumArgs-1; i++) {
        ThisArg = (struct Value*)((char*)ThisArg +
                            MEM_ALIGN(sizeof(struct Value)+TypeStackSizeValue(ThisArg)));
        va[i] = (char*)ThisArg->Val->Pointer;
    }

    value = util_json_get_value(
                json_item,
                va[0], va[1], va[2], va[3], va[4], va[5], va[6], va[7], va[8], va[9]);

    ReturnValue->Val->Pointer = value;
}

//
// utils read/write 32-bit RGBA png files
//

void Util_read_png_file(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char           *dir      = Param[0]->Val->Pointer;
    char           *filename = Param[1]->Val->Pointer;
    unsigned char **pixels   = Param[2]->Val->Pointer;
    int            *w        = Param[3]->Val->Pointer;
    int            *h        = Param[4]->Val->Pointer;
    int             rc;

    rc = util_read_png_file(dir, filename, pixels, w, h);

    ReturnValue->Val->Integer = rc;
}

void Util_write_png_file(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char          *dir      = Param[0]->Val->Pointer;
    char          *filename = Param[1]->Val->Pointer;
    unsigned char *pixels   = Param[2]->Val->Pointer;
    int            w        = Param[3]->Val->Integer;
    int            h        = Param[4]->Val->Integer;
    int            rc;

    rc = util_write_png_file(dir, filename, pixels, w, h);

    ReturnValue->Val->Integer = rc;
}

//
// utils java methods
//

// get location: latitude, longitude, and altitude
void Util_get_location(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    double *lat = (double*)Param[0]->Val->Pointer;
    double *lng = (double*)Param[1]->Val->Pointer;
    double *alt = (double*)Param[2]->Val->Pointer;

    util_get_location(lat, lng, alt);
}

// text to speech
void Util_text_to_speech(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *text = Param[0]->Val->Pointer;

    util_text_to_speech(text);
}

void Util_text_to_speech_stop(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_text_to_speech_stop();
}

// foreground control 
void Util_start_foreground(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_start_foreground();
}

void Util_stop_foreground(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_stop_foreground();
}

void Util_is_foreground_enabled(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    bool is_enabled;

    is_enabled = util_is_foreground_enabled();
    ReturnValue->Val->Integer = is_enabled;
}

// flashlight
void Util_turn_flashlight_on(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_turn_flashlight_on();
}

void Util_turn_flashlight_off(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_turn_flashlight_off();
}

void Util_toggle_flashlight(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_toggle_flashlight();
}

void Util_is_flashlight_on(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    bool is_on;

    is_on = util_is_flashlight_on();
    ReturnValue->Val->Integer = is_on;
}

// playback capture   
void Util_start_playbackcapture(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_start_playbackcapture();
}

void Util_stop_playbackcapture(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    util_stop_playbackcapture();
}

void Util_get_playbackcapture_audio(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    short *array = Param[0]->Val->Pointer;
    int    num_elements = Param[1]->Val->Integer;

    util_get_playbackcapture_audio(array, num_elements);
}

//
// UTILS REGISTRATION
//

void UtilsSetupFunction(Picoc *pc)
{
}

struct LibraryFunction UtilsFunctions[] = {
    // time
    { Util_microsec_timer,   "long util_microsec_timer(void);" },
    { Util_get_real_time_microsec, "long util_get_real_time_microsec(void);" },
    { Util_time2str,         "char *util_time2str(char * str, long us, bool gmt, bool display_ms, bool display_date);" },
    // file utils     
    { Util_write_file,       "int util_write_file(char *dir, char *fn, void *data, int len);" },
    { Util_read_file,        "void *util_read_file(char *dir, char *fn, int *len);" },
    { Util_delete_file,      "void *util_delete_file(char *dir, char *fn);" },
    { Util_file_exists,      "bool util_file_exists(char *dir, char *fn);" },
    { Util_file_mtime,       "long util_file_mtime(char *dir, char *fn);" },
    { Util_file_size,        "long util_file_size(char *dir, char *fn);" },
    // directory utils
    { Util_delete_dir,       "void util_delete_dir(char *dir, char *dir_to_delete);" },
    // file map
    { Util_map_file,         "void *util_map_file(char *dir, char *file, int len, bool create_if_needed, bool read_only, int *created_flag);" },
    { Util_unmap_file,       "void util_unmap_file(void *addr, int len);" },
    { Util_sync_file,        "void util_sync_file(void *addr, int len);" },
    // params get/set
    { Util_get_str_param,    "char *util_get_str_param(char *dir, char *name, char *default_value);" },
    { Util_set_str_param,    "void util_set_str_param(char *dir, char *name, char *value);" },
    { Util_get_numeric_param,"double util_get_numeric_param(char *dir, char *name, double default_value);" },
    { Util_set_numeric_param,"void util_set_numeric_param(char *dir, char *name, double value);" },
    { Util_print_params,     "void util_print_params(char *dir);" },
    // network
    { Util_get_ipaddr,       "char *util_get_ipaddr(void);" },
    // json
    { Util_json_parse,       "void *util_json_parse(char *str, char **end_ptr);" },
    { Util_json_free,        "void util_json_free(void *json_root);" },
    { Util_json_get_value,   "json_value_t *util_json_get_value(void *json_item, ...);" },
    // png file read/write
    { Util_read_png_file,    "int util_read_png_file(char *dir, char *filename, unsigned char **pixels, int *w, int *h);" },
    { Util_write_png_file,   "int util_write_png_file(char *dir, char *filename, unsigned char *pixels, int w, int h);" },
    // call java: location
    { Util_get_location,     "void util_get_location(double *latitude, double *longitude, double *altitude);" },
    // call java: text to speech
    { Util_text_to_speech,      "void util_text_to_speech(char *text);" },
    { Util_text_to_speech_stop, "void util_text_to_speech_stop(void);" },
    // call java: start / stop foreground
    { Util_start_foreground,      "void util_start_foreground(void);" },
    { Util_stop_foreground,       "void util_stop_foreground(void);" },
    { Util_is_foreground_enabled, "bool util_is_foreground_enabled(void);" },
    // call java: flashlight
    { Util_turn_flashlight_on,  "void util_turn_flashlight_on(void);" },
    { Util_turn_flashlight_off, "void util_turn_flashlight_off(void);" },
    { Util_toggle_flashlight,   "void util_toggle_flashlight(void);" },
    { Util_is_flashlight_on,    "bool util_is_flashlight_on(void);" },
    // call java: playbackcapture
    { Util_start_playbackcapture,     "void util_start_playbackcapture(void);" },
    { Util_stop_playbackcapture,      "void util_stop_playbackcapture(void);" },
    { Util_get_playbackcapture_audio, "void util_get_playbackcapture_audio(short *array, int num_array_elements);" },

    { NULL, NULL } };

const char UtilsDefs[] = "\
#define JSON_TYPE_UNDEFINED 0 \n\
#define JSON_TYPE_FLAG      1 \n\
#define JSON_TYPE_NUMBER    2 \n\
#define JSON_TYPE_STRING    3 \n\
#define JSON_TYPE_ARRAY     4 \n\
#define JSON_TYPE_OBJECT    5 \n\
\n\
typedef struct { \n\
    int type; \n\
    union { \n\
        bool   flag; \n\
        double number; \n\
        char  *string; \n\
        void  *array; \n\
        void  *object; \n\
    } u; \n\
} json_value_t; \n\
";

// -----------------  SVCS PLATFORM ROUTINES  --------------------------

//
// routines called by apps
//

void Svc_make_req(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char *svc_name     = Param[0]->Val->Pointer;
    int   req_id       = Param[1]->Val->Integer;
    char *req_data     = Param[2]->Val->Pointer;
    int   req_data_len = Param[3]->Val->Integer;
    int   timeout_secs = Param[4]->Val->Integer;
    int   ret;

    ret = svc_make_req(svc_name, req_id, req_data, req_data_len, timeout_secs);
    ReturnValue->Val->Integer = ret;
}

//
// routines called by svcs
//

void Svc_wait_for_req(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    char       *svc_name             = Param[0]->Val->Pointer;
    svc_req_t **req                  = Param[1]->Val->Pointer;
    long        timeout_abstime_secs = Param[2]->Val->LongInteger;
    int         ret;

    ret = svc_wait_for_req(svc_name, req, timeout_abstime_secs);
    ReturnValue->Val->Integer = ret;
}

void Svc_req_completed(struct ParseState *Parser, struct Value *ReturnValue,
        struct Value **Param, int NumArgs)
{
    svc_req_t *req         = Param[0]->Val->Pointer;
    int        comp_status = Param[1]->Val->Integer;

    svc_req_completed(req, comp_status);
}

//
// SVCS REGISTRATION
//

void SvcsSetupFunction(Picoc *pc)
{
}

struct LibraryFunction SvcsFunctions[] = {
    // routines called by apps
    { Svc_make_req,              "int svc_make_req(char *svc_name, int req_id, char *req_data, int req_data_len, int timeout_secs);" },

    // routines called by svcs
    { Svc_wait_for_req,          "int svc_wait_for_req(char *svc_name, svc_req_t **req, long timeout_abstime_secs);" },
    { Svc_req_completed,         "void svc_req_completed(svc_req_t *req, int comp_status);" },

    { NULL, NULL } };

const char SvcsDefs[] = "\
// common values for req_id \n\
#define SVC_REQ_ID_STOP 1 \n\
\n\
// values returned by svc_make_req \n\
#define SVC_REQ_OK                     0 \n\
#define SVC_REQ_ERROR_NOT_COMPLETED    1 \n\
#define SVC_REQ_ERROR_DATA_LEN         2 \n\
#define SVC_REQ_ERROR_SVC_NOT_FOUND    3 \n\
#define SVC_REQ_ERROR_SVC_NOT_RUNNING  4 \n\
#define SVC_REQ_ERROR_QUEUE_FULL       5 \n\
#define SVC_REQ_ERROR_INVALID_REQ      6 \n\
#define SVC_REQ_ERROR                  7 \n\
\n\
// values returned by svc_wait_for_req \n\
#define SVC_REQ_WAIT_OK                    0 \n\
#define SVC_REQ_WAIT_ERROR_SVC_NOT_FOUND   1 \n\
#define SVC_REQ_WAIT_ERROR_TIMEDOUT        2 \n\
\n\
// sizeof of req->data \n\
#define MAX_SVC_REQ_DATA 100 \n\
\n\
typedef struct { \n\
    int  req_id; \n\
    bool completed; \n\
    int  status; \n\
    char data[MAX_SVC_REQ_DATA]; \n\
} svc_req_t; \n\
";

// -----------------  PLATFORM INIT PROC  -------------------------------

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(
        pc, 
        "sdlx.h", 
        SdlSetupFunction,
        SdlFunctions, 
        SdlDefs);

    IncludeRegister(
        pc, 
        "utils.h", 
        UtilsSetupFunction,
        UtilsFunctions, 
        UtilsDefs);

    IncludeRegister(
        pc, 
        "svcs.h", 
        SvcsSetupFunction,
        SvcsFunctions, 
        SvcsDefs);
}
