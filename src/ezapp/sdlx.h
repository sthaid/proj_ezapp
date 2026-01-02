#ifndef __SDLX_H__
#define __SDLX_H__

#ifdef __cplusplus
extern "C" {
#endif

// --------------------
// init / quit
// --------------------

#define SUBSYS_VIDEO  1
#define SUBSYS_AUDIO  2
#define SUBSYS_SENSOR 4

int sdlx_init(int subsys);
void sdlx_quit(int subsys);

// --------------------
// video    
// --------------------

#define BYTES_PER_PIXEL   4

// https://www.w3schools.com/colors/colors_converter.asp
// these colors are opaque (alpha equals 255)
//                          red      green       blue      alpha
#define COLOR_BLACK       (   0  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_WHITE       ( 255  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_RED         ( 255  |    0<<8 |    0<<16 |  255<<24 )
#define COLOR_ORANGE      ( 255  |  128<<8 |    0<<16 |  255<<24 )
#define COLOR_YELLOW      ( 255  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_GREEN       (   0  |  255<<8 |    0<<16 |  255<<24 )
#define COLOR_BLUE        (   0  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_INDIGO      (  75  |    0<<8 |  130<<16 |  255<<24 )
#define COLOR_VIOLET      ( 238  |  130<<8 |  238<<16 |  255<<24 )
#define COLOR_PURPLE      ( 127  |    0<<8 |  255<<16 |  255<<24 )
#define COLOR_LIGHT_BLUE  (   0  |  255<<8 |  255<<16 |  255<<24 )
#define COLOR_LIGHT_GREEN ( 144  |  238<<8 |  144<<16 |  255<<24 )
#define COLOR_PINK        ( 255  |  105<<8 |  180<<16 |  255<<24 )
#define COLOR_TEAL        (   0  |  128<<8 |  128<<16 |  255<<24 )
#define COLOR_LIGHT_GRAY  ( 192  |  192<<8 |  192<<16 |  255<<24 )
#define COLOR_GRAY        ( 128  |  128<<8 |  128<<16 |  255<<24 )
#define COLOR_DARK_GRAY   (  64  |   64<<8 |   64<<16 |  255<<24 )

#define ROW2Y(r) ((r) * sdlx_char_height)
#define COL2X(c) ((c) * sdlx_char_width)

#define SMALLEST_FONT 40
#define SMALL_FONT    30
#define DEFAULT_FONT  20
#define LARGE_FONT    10

//
// typedefs
//

typedef struct {
    int x, y, w, h;
} sdlx_loc_t;

typedef struct {
    int x, y;
} sdlx_point_t;

typedef struct sdlx_texture sdlx_texture_t;

typedef struct {
    int ptsize;
    int char_width;
    int char_height;
    int bg_color;
    int fg_color;
} sdlx_print_state_t;

typedef struct {
    double xval;
    double yval;
} sdlx_plot_point_t;

//
// global variables
//

extern int sdlx_win_width;
extern int sdlx_win_height;
extern int sdlx_char_width;
extern int sdlx_char_height;

//
// prototypes
//

// display init and present, must be done for every display update
void sdlx_display_init(int color);
void sdlx_display_present(void);

// create colors
int sdlx_create_color(int r, int g, int b, int a);
int sdlx_scale_color(int color, double inten);
int sdlx_set_color_alpha(int color, int alpha);
int sdlx_wavelength_to_color(int wavelength);

// render text
void sdlx_print_init(double numchars, int fg_color, int bg_color);
void sdlx_print_init_numchars(double numchars);
void sdlx_print_init_color(int fg_color, int bg_color);
void sdlx_print_save(sdlx_print_state_t *save);
void sdlx_print_restore(sdlx_print_state_t *restore);
sdlx_loc_t *sdlx_render_text(int x, int y, char *str);
sdlx_loc_t *sdlx_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
sdlx_loc_t *sdlx_render_text_xyctr(int x, int y, char *str);
sdlx_loc_t *sdlx_render_printf_xyctr(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
void sdlx_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char **lines, int n);

// render rectangle, lines, circles, points
void sdlx_render_rect(int x, int y, int w, int h, int line_width, int color);
void sdlx_render_fill_rect(int x, int y, int w, int h, int color);
void sdlx_render_line(int x1, int y1, int x2, int y2, int color);
void sdlx_render_lines(sdlx_point_t *points, int count, int color);
void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);
void sdlx_render_point(int x, int y, int color, int point_size);
void sdlx_render_points(sdlx_point_t *points, int count, int color, int point_size);

// render using textures
sdlx_texture_t *sdlx_create_texture_from_pixels(unsigned char *pixels, int w, int h);  // xxx color  xxx pixel_t ?
sdlx_texture_t *sdlx_create_filled_circle_texture(int radius, int color);  // xxx color
sdlx_texture_t *sdlx_create_text_texture(char *str);  // xxx color
sdlx_loc_t *sdlx_render_texture(int x, int y, int w, int h, sdlx_texture_t *texture);
void sdlx_render_texture_ex(int x, int y, int w, int h, double angle, sdlx_texture_t *texture);
void sdlx_render_texture_ex2(int x, int y, int w, int h, double angle, int xctr, int yctr,
                            sdlx_texture_t *texture);
void sdlx_destroy_texture(sdlx_texture_t *texture);
void sdlx_query_texture(sdlx_texture_t *texture, int *w, int *h);
unsigned char *sdlx_read_display_pixels(int x, int y, int w, int h, int *w_pixels, int *h_pixels);

// plotting  xxx rework or delete
void *sdlx_plot_create(char *title,
                      int xleft, int xright, int ybottom, int ytop,
                      double xval_left, double xval_right, double yval_bottom, double yval_top,
                      double yval_of_x_axis);
void sdlx_plot_axis(void *cx_arg, char *xmin_str, char *xmax_str, char *ymin_str, char *ymax_str);
void sdlx_plot_points(void *cx, sdlx_plot_point_t *pts, int num_pts);
void sdlx_plot_bars(void *cx,
                   sdlx_plot_point_t *pts_avg, sdlx_plot_point_t *pts_min, sdlx_plot_point_t *pts_max,
                   int num_pts, double bar_wval);
void sdlx_plot_free(void *cx);

// misc
void sdlx_show_toast(char *message);

// --------------------
// audio
// --------------------

#define AUDIO_REQ_STOP     1
#define AUDIO_REQ_PAUSE    2
#define AUDIO_REQ_UNPAUSE  3

#define AUDIO_STATE_IDLE           0
#define AUDIO_STATE_PLAY_FILE      1
#define AUDIO_STATE_PLAY_TONES     2
#define AUDIO_STATE_RECORD         3
#define AUDIO_STATE_RECORD_APPEND  4

typedef struct {
    short freq;
    short intvl_ms;
} sdlx_tone_t;

typedef struct {
    int  state;
    bool paused;
    int  processed_ms;
    int  total_ms;
    int  volume;
    char filename[100];
} sdlx_audio_state_t;

int sdlx_audio_play(char *dir, char *filename);
int sdlx_audio_record(char *dir, char *filename, int max_duration_secs, int auto_stop_secs, bool append);
int sdlx_audio_play_tones(sdlx_tone_t *tones);
int sdlx_audio_file_duration(char *dir, char *filename);

int sdlx_audio_play_new(char *dir, char *filename);

void sdlx_audio_ctl(int req);
void sdlx_audio_state(sdlx_audio_state_t * state);

void sdlx_audio_print_devices_info(void);
void sdlx_audio_create_test_file(char *dir, char *filename, int duration_secs, int freq);

void sdlx_start_playbackcapture(char *dir, char *filename);
void sdlx_stop_playbackcapture(void);

// not available in picoc
#ifdef ANDROID
    #define DEFAULT_RECORD_SCALE 5
#else
    #define DEFAULT_RECORD_SCALE 1
#endif
#define DEFAULT_RECORD_SILENCE 10
typedef struct {
    double record_scale;
    double record_silence;
} sdlx_audio_params_t;
void sdlx_audio_set_params(sdlx_audio_params_t *ap);
void sdlx_audio_get_params(sdlx_audio_params_t *ap);

// --------------------
// sensors
// --------------------

#define ASENSOR_TYPE_ACCELEROMETER       1
#define ASENSOR_TYPE_MAGNETIC_FIELD      2
#define ASENSOR_TYPE_GYROSCOPE           4
#define ASENSOR_TYPE_LIGHT               5
#define ASENSOR_TYPE_PRESSURE            6
#define ASENSOR_TYPE_PROXIMITY           8
#define ASENSOR_TYPE_GRAVITY             9
#define ASENSOR_TYPE_LINEAR_ACCELERATION 10
#define ASENSOR_TYPE_ROTATION_VECTOR     11
#define ASENSOR_TYPE_RELATIVE_HUMIDITY   12
#define ASENSOR_TYPE_AMBIENT_TEMPERATURE 13
#define ASENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED 14
#define ASENSOR_TYPE_GAME_ROTATION_VECTOR 15
#define ASENSOR_TYPE_GYROSCOPE_UNCALIBRATED 16
#define ASENSOR_TYPE_SIGNIFICANT_MOTION 17
#define ASENSOR_TYPE_STEP_DETECTOR 18
#define ASENSOR_TYPE_STEP_COUNTER 19
#define ASENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR 20
#define ASENSOR_TYPE_HEART_RATE 21
#define ASENSOR_TYPE_POSE_6DOF 28
#define ASENSOR_TYPE_STATIONARY_DETECT 29
#define ASENSOR_TYPE_MOTION_DETECT 30
#define ASENSOR_TYPE_HEART_BEAT 31
#define ASENSOR_TYPE_DYNAMIC_SENSOR_META 32
#define ASENSOR_TYPE_ADDITIONAL_INFO 33
#define ASENSOR_TYPE_LOW_LATENCY_OFFBODY_DETECT 34
#define ASENSOR_TYPE_ACCELEROMETER_UNCALIBRATED 35
#define ASENSOR_TYPE_HINGE_ANGLE 36
#define ASENSOR_TYPE_HEAD_TRACKER 37
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES 38
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES 39
#define ASENSOR_TYPE_ACCELEROMETER_LIMITED_AXES_UNCALIBRATED 40
#define ASENSOR_TYPE_GYROSCOPE_LIMITED_AXES_UNCALIBRATED 41
#define ASENSOR_TYPE_HEADING 42

#define INVALID_NUMBER 999999999

typedef struct {
    int   id;
    int   type;  // ASENSOR_TYPE
    char *name;
} sdlx_sensor_info_t;

sdlx_sensor_info_t *sdlx_sensor_get_info_tbl(int *max);
int sdlx_sensor_find(int type);  // returns sensor id, or -1 if not found
int sdlx_sensor_read_raw(int id, double *data, int num_values);

int sdlx_sensor_read_step_counter(double *step_count);  // xxx just return INVALID_NUMBER
int sdlx_sensor_read_mag_heading(double *mag_heading);
int sdlx_sensor_read_accelerometer(double *ax, double *ay, double *az);
int sdlx_sensor_read_roll_pitch(double *roll, double *pitch);
int sdlx_sensor_read_pressure(double *millibars);
int sdlx_sensor_read_temperature(double *degrees_c);
int sdlx_sensor_read_humidity(double *percent);

// --------------------
// events   
// --------------------

#define EVID_SWIPE_RIGHT       9990  // xxx are these 2 used
#define EVID_SWIPE_LEFT        9991
#define EVID_MOTION            9992
#define EVID_KEYBD             9993  // xxx move define
#define EVID_QUIT              9999  // xxx review where this is used

typedef struct {
    int event_id;
    union {
        struct {
            double x, y, xrel, yrel;
        } motion;
        struct {
            int ch;
        } keybd;
    } u;
} sdlx_event_t;

// event registration and query
void sdlx_register_event(sdlx_loc_t *loc, int event_id);
void sdlx_register_control_events(char *evstr1, char *evstr2, char *evstr3, 
                                  int fg_color, int bg_color,
                                  int evid1, int evid2, int evid3);
void sdlx_get_event(long timeout_us, sdlx_event_t *event);

// get string, from virtual keyboard when on Android
char *sdlx_get_input_str(char *prompt1, char *prompt2, bool numeric_keybd, int bg_color);

// --------------------
// not made available in picoc
// --------------------

// sdlx_video.c
int sdlx_video_init(void);
void sdlx_video_quit(void);
void sdlx_minimize_window(void);

// sdlx_audio.c
int sdlx_audio_init(void);
void sdlx_audio_quit(void);

// sdlx_sensor.c
int sdlx_sensor_init(void);
void sdlx_sensor_quit(void);

// sdlx_event.c
void sdlx_reset_events(void);

// sdlx_misc.c
char *sdlx_get_storage_path(void);
void sdlx_copy_asset_file(char *asset_filename, char *dest_dir);
int sdlx_get_permission(char *name);
int sdlx_create_detached_thread_private(int (*thread_fn)(void*), char *thread_name, void *cx);
#define sdlx_create_detached_thread(thread_fn, cx) \
    do { \
        sdlx_create_detached_thread_private(thread_fn, #thread_fn, cx); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
