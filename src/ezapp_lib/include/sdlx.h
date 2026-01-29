#ifndef __SDLX_H__
#define __SDLX_H__

#ifdef __cplusplus
extern "C" {
#endif

// --------------------
// INIT / QUIT
// --------------------

#define SUBSYS_VIDEO  1
#define SUBSYS_AUDIO  2
#define SUBSYS_SENSOR 4

int sdlx_init(int subsys);
void sdlx_quit(int subsys);

// --------------------
// VIDEO    
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

typedef struct {
    int x, y, w, h;
} sdlx_loc_t;

extern int sdlx_win_width;
extern int sdlx_win_height;
extern int sdlx_char_width;
extern int sdlx_char_height;

// - - - - - - - - - - - - - 
// display init and present
// - - - - - - - - - - - - - 
void sdlx_display_init(int color);
void sdlx_display_present(void);

// - - - - - 
// colors
// - - - - - 
int sdlx_create_color(int r, int g, int b, int a);
int sdlx_scale_color(int color, double inten);
int sdlx_set_color_alpha(int color, int alpha);
int sdlx_wavelength_to_color(int wavelength);

// - - - - - - - 
// render text
// - - - - - - - 
#define ROW2Y(r) ((r) * sdlx_char_height)
#define COL2X(c) ((c) * sdlx_char_width)

#define FONT_TINY     40
#define FONT_SMALL    30
#define FONT_NORMAL   20
#define FONT_LARGE    10

#define WRAP_NONE    -1
#define WRAP_NEWLINE  0

#define FLAG_X_CTR  1
#define FLAG_Y_CTR  2
#define FLAG_XY_CTR (FLAG_X_CTR | FLAG_Y_CTR)

typedef struct {
    int color;
    int ptsize;
    int char_width;
    int char_height;
} sdlx_print_state_t;

void sdlx_print_set(double numchars, int color);
void sdlx_print_set_numchars(double numchars);
void sdlx_print_set_color(int color);
void sdlx_print_save(sdlx_print_state_t *save);
void sdlx_print_restore(sdlx_print_state_t *restore);

sdlx_loc_t *sdlx_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));
sdlx_loc_t *sdlx_render_printf_ex(int x, int y, int flags, int wrap, char *fmt, ...) __attribute__ ((format (printf, 5, 6)));
void sdlx_render_multiline_text(int x, int y, int y_top, int y_bottom, char **lines, int num_lines);

// - - - - - - - - - - - - - - - - - - - - -
// render rectangle, lines, circles, points
// - - - - - - - - - - - - - - - - - - - - -
typedef struct {
    int x, y;
} sdlx_point_t;

void sdlx_render_rect(int x, int y, int w, int h, int line_width, int color);
void sdlx_render_fill_rect(int x, int y, int w, int h, int color);
void sdlx_render_line(int x1, int y1, int x2, int y2, int color);
void sdlx_render_lines(sdlx_point_t *points, int count, int color);
void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, int color);
// xxx make filled circle proc
void sdlx_render_point(int x, int y, int color, int point_size);
void sdlx_render_points(sdlx_point_t *points, int count, int color, int point_size);

// - - - - - 
// textures
// - - - - - 
typedef struct sdlx_texture sdlx_texture_t;

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

// - - - - - -
// plotting
// - - - - - -
// xxx rework or delete
typedef struct {
    double xval;
    double yval;
} sdlx_plot_point_t;

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

// --------------------
// AUDIO
// --------------------

#define FRAMES_PER_SEC 48000

#define AUDIO_STATE_IDLE                0
#define AUDIO_STATE_STOPPING            1
#define AUDIO_STATE_PAUSED              2
#define AUDIO_STATE_PLAY_FILE           3
#define AUDIO_STATE_PLAY_TONES          4
#define AUDIO_STATE_PLAY_BUFF           5
#define AUDIO_STATE_RECORD_FROM_MIC     6
#define AUDIO_STATE_RECORD_FROM_DEVICE  7

typedef struct {
    short freq;
    short intvl_ms;
} sdlx_tone_t;

typedef struct {
    int  state;
    int  play_current_ms;
    int  play_total_ms;
    int  record_ms;
    int  volume;
    char pathname[200];  // xxx or is this filename?
} sdlx_audio_state_t;

// - - - - - -
// control
// - - - - - -
int sdlx_audio_stop(void);
void sdlx_audio_pause(void);
void sdlx_audio_resume(void);
void sdlx_audio_state(sdlx_audio_state_t * state);
int sdlx_audio_file_duration_ms(char *dir, char *filename);

// - - - - - -
// playback
// - - - - - -
int sdlx_audio_play_file(char *dir, char *filename);
int sdlx_audio_play_tones(sdlx_tone_t *tones);
int sdlx_audio_play_buff(short *samples, int num_samples, int num_channels,
                         int loops, bool free_samples_when_done);

// - - - - - -
// record
// - - - - - -
int sdlx_audio_record_from_mic(char *dir, char *filename, int max_duration_secs, int auto_stop_secs, bool append);
int sdlx_audio_record_from_device(char *dir, char *filename);

// --------------------
// SENSORS
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
// EVENTS   
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

// - - - - - - - - - - 
// event registration
// - - - - - - - - - - 
void sdlx_register_event(sdlx_loc_t *loc, int event_id);
void sdlx_register_control_events(int evid1, char *evstr1,
                                  int evid2, char *evstr2,
                                  int evid3, char *evstr3,
                                  int print_color, int bg_color);

// - - - - - - - - 
// wait for event
// - - - - - - - - 
void sdlx_get_event(long timeout_us, sdlx_event_t *event);


// --------------------
// MISC
// --------------------

#define INVALID_NUMBER 999999999

// android show toast
void sdlx_show_toast(char *message);

// get string, using virtual keyboard when on Android
char *sdlx_get_input_str(char *prompt1, char *prompt2, bool numeric_keybd, int bg_color);

// --------------------
// NOT AVAILABLE IN PICOC
// --------------------

// sdlx_video.c
int sdlx_video_init(void);
void sdlx_video_quit(void);
void sdlx_minimize_window(void);

// sdlx_audio.c
#ifdef ANDROID
    #define DEFAULT_RECORD_GAIN 5
#else
    #define DEFAULT_RECORD_GAIN 1
#endif
#define DEFAULT_RECORD_SILENCE 10

typedef struct {
    double record_gain;
    double record_silence;
} sdlx_audio_params_t;

int sdlx_audio_init(void);
void sdlx_audio_quit(void);
void sdlx_audio_set_params(sdlx_audio_params_t *ap);
void sdlx_audio_get_params(sdlx_audio_params_t *ap);

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
// xxx is this needed
#define sdlx_create_detached_thread(thread_fn, cx) \
    do { \
        sdlx_create_detached_thread_private(thread_fn, #thread_fn, cx); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif
