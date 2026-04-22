#ifndef __SDLX_H__
#define __SDLX_H__

#ifdef __cplusplus
extern "C" {
#endif

// --------------------
// VIDEO    
// --------------------

typedef unsigned int sdlx_color_t;

typedef struct {
    int x, y, w, h;
} sdlx_loc_t;

typedef struct {
    int x, y;
} sdlx_point_t;

typedef struct sdlx_texture sdlx_texture_t;

extern int sdlx_win_width;
extern int sdlx_win_height;

// - - - - - - - - - - - - - 
// display init and present
// - - - - - - - - - - - - - 

#define PORTRAIT  0
#define LANDSCAPE 1

void sdlx_display_init(sdlx_color_t color, int orientation);
void sdlx_display_present(void);

// - - - - - 
// colors
// - - - - - 

#define BYTES_PER_PIXEL   4

// reference: https://www.w3schools.com/colors/colors_converter.asp
// these colors are opaque (alpha equals 255)
#define COLOR_BLACK       ((sdlx_color_t)(   0  |    0<<8 |    0<<16 |  255<<24 ))
#define COLOR_WHITE       ((sdlx_color_t)( 255  |  255<<8 |  255<<16 |  255<<24 ))
#define COLOR_RED         ((sdlx_color_t)( 255  |    0<<8 |    0<<16 |  255<<24 ))
#define COLOR_ORANGE      ((sdlx_color_t)( 255  |  128<<8 |    0<<16 |  255<<24 ))
#define COLOR_YELLOW      ((sdlx_color_t)( 255  |  255<<8 |    0<<16 |  255<<24 ))
#define COLOR_GREEN       ((sdlx_color_t)(   0  |  255<<8 |    0<<16 |  255<<24 ))
#define COLOR_BLUE        ((sdlx_color_t)(   0  |    0<<8 |  255<<16 |  255<<24 ))
#define COLOR_INDIGO      ((sdlx_color_t)(  75  |    0<<8 |  130<<16 |  255<<24 ))
#define COLOR_VIOLET      ((sdlx_color_t)( 238  |  130<<8 |  238<<16 |  255<<24 ))
#define COLOR_PURPLE      ((sdlx_color_t)( 127  |    0<<8 |  255<<16 |  255<<24 ))
#define COLOR_LIGHT_BLUE  ((sdlx_color_t)(   0  |  255<<8 |  255<<16 |  255<<24 ))
#define COLOR_LIGHT_GREEN ((sdlx_color_t)( 144  |  238<<8 |  144<<16 |  255<<24 ))
#define COLOR_PINK        ((sdlx_color_t)( 255  |  105<<8 |  180<<16 |  255<<24 ))
#define COLOR_TEAL        ((sdlx_color_t)(   0  |  128<<8 |  128<<16 |  255<<24 ))
#define COLOR_LIGHT_GRAY  ((sdlx_color_t)( 192  |  192<<8 |  192<<16 |  255<<24 ))
#define COLOR_GRAY        ((sdlx_color_t)( 128  |  128<<8 |  128<<16 |  255<<24 ))
#define COLOR_DARK_GRAY   ((sdlx_color_t)(  64  |   64<<8 |   64<<16 |  255<<24 ))

sdlx_color_t sdlx_create_color(int r, int g, int b, int a);
sdlx_color_t sdlx_scale_color(sdlx_color_t color, double inten);
sdlx_color_t sdlx_set_color_alpha(sdlx_color_t color, int alpha);
sdlx_color_t sdlx_wavelength_to_color(int wavelength);

// - - - - - - - 
// render text
// - - - - - - - 

#define FONT_TINY     40   // 40 chars fit in display width
#define FONT_SMALL    30   // 30 chars fit in display width
#define FONT_NORMAL   20   // etc
#define FONT_LARGE    10

// The following work with the default font.
// The default font settings (fontid and color) are specified by call to sdlx_print_set_default.
void sdlx_print_set_default(int fontid, sdlx_color_t color);
extern int sdlx_char_width_dflt;
extern int sdlx_char_height_dflt;
#define ROW2Y(r) ((r) * sdlx_char_height_dflt)
#define COL2X(c) ((c) * sdlx_char_width_dflt)
sdlx_loc_t *sdlx_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

// The following work with the font specified by fontid, and the 
// specified color. These also support the flag and wrap params
#define FLAG_NONE          0x000
#define FLAG_X_CTR         0x001
#define FLAG_Y_CTR         0x002
#define FLAG_XY_CTR        (FLAG_X_CTR | FLAG_Y_CTR)
#define FLAG_ROT_CTR_90    0x010
#define FLAG_ROT_CTR_180   0x020
#define FLAG_ROT_CTR_270   0x040
#define FLAG_BG_BLACK      0x100
#define FLAG_BG_WHITE      0x200
#define WRAP_NONE    -1
#define WRAP_NEWLINE  0
int sdlx_char_width(int fontid);
int sdlx_char_height(int fontid);
sdlx_loc_t *sdlx_render_printf_ex1(int x, int y, int fontid, sdlx_color_t color, 
                                   char * fmt, ...)
                                   __attribute__ ((format (printf, 5, 6)));
sdlx_loc_t *sdlx_render_printf_ex2(int x, int y, int fontid, sdlx_color_t color,
                                   int flags, int wrap, 
                                   char *fmt, ...) 
                                   __attribute__ ((format (printf, 7, 8)));
void sdlx_render_multiline_text(int x, int y, int y_top, int y_bottom, 
                                int fontid, char **lines, sdlx_color_t *colors, int num_lines);

// - - - - - - - - - - - - - - - - - - - - -
// render rectangle, lines, circles, points
// - - - - - - - - - - - - - - - - - - - - -

void sdlx_render_rect(int x, int y, int w, int h, int line_width, sdlx_color_t color);
void sdlx_render_fill_rect(int x, int y, int w, int h, sdlx_color_t color);
void sdlx_render_line(int x1, int y1, int x2, int y2, sdlx_color_t color);
void sdlx_render_lines(sdlx_point_t *points, int count, sdlx_color_t color);
void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, sdlx_color_t color);
void sdlx_render_fill_circle(int x_ctr, int y_ctr, int radius, sdlx_color_t color);
#define MAX_POINT_SIZE 9
void sdlx_render_point(int x, int y, sdlx_color_t color, int point_size);
void sdlx_render_points(sdlx_point_t *points, int count, sdlx_color_t color, int point_size);

// - - - - - 
// textures
// - - - - - 

sdlx_texture_t *sdlx_create_texture(int w, int h);
void sdlx_destroy_texture(sdlx_texture_t *t);
void sdlx_query_texture(sdlx_texture_t *t, int *w, int *h);
void sdlx_clear_texture(sdlx_texture_t *t, sdlx_color_t color);
void sdlx_color_mod_texture(sdlx_texture_t *t, float r, float g, float b);

void sdlx_set_texture_pixels(sdlx_texture_t *t, unsigned int *pixels);
unsigned int *sdlx_get_texture_pixels(sdlx_texture_t *t, int *w, int *h);

void sdlx_render_texture(sdlx_texture_t *t, int x, int y);
void sdlx_render_texture_ex1(sdlx_texture_t *t, int x, int y, int w, int h);
void sdlx_render_texture_ex2(sdlx_texture_t *t, int x, int y, int w, int h, double angle);
void sdlx_render_texture_ex3(sdlx_texture_t *texture, int x, int y, int w, int h, double angle, int xctr, int yctr);

void sdlx_set_render_target(sdlx_texture_t *t);

// --------------------
// AUDIO
// --------------------

#define FRAMES_PER_SEC 48000

#define AUDIO_STATE_IDLE                0
#define AUDIO_STATE_PLAY_FILE           1
#define AUDIO_STATE_PLAY_TONES_SEQUENCE 2
#define AUDIO_STATE_PLAY_BUFF           3
#define AUDIO_STATE_RECORD_FROM_MIC     4
#define AUDIO_STATE_RECORD_FROM_DEVICE  5

#define GET_SAMPLES_MONO          0
#define GET_SAMPLES_LEFT_CHANNEL  1
#define GET_SAMPLES_RIGHT_CHANNEL 2

typedef struct {
    short freq;
    short intvl_ms;
} sdlx_tone_t;

typedef struct {
    int    state;
    bool   stopping;
    bool   paused;
    int    play_current_secs;
    int    play_total_secs;
    int    record_secs;
    char   pathname[100];  // xxx if not used then delete
    int    play_tones_freq;
    int    play_tones_seqnum;
    double volume;
    // xxx add fps and num_channels ?
} sdlx_audio_state_t;

// - - - - - -
// control
// - - - - - -

int sdlx_audio_stop(void);
void sdlx_audio_pause(void);
void sdlx_audio_resume(void);
void sdlx_audio_get_state(sdlx_audio_state_t * state);
int sdlx_audio_file_duration_secs(char *dir, char *filename);

// - - - - - -
// get/downsample most recent samples
// - - - - - -

void sdlx_get_audio_samples(int num_ret_samples, int num_downsample, int which_channel, float *ret_samples);

// - - - - - -
// playback
// - - - - - -

int sdlx_audio_play_file(char *dir, char *filename);
int sdlx_audio_play_tones(sdlx_tone_t *tones);
int sdlx_audio_play_buff(float *samples, int num_samples, int num_channels,
                         int loops, bool free_samples_when_done);

// - - - - - -
// record
// - - - - - -

int sdlx_audio_record_from_mic(char *dir, char *filename, int auto_stop_secs, bool append, bool start_paused);
int sdlx_audio_record_from_device(char *dir, char *filename, bool append, bool start_paused);

// - - - - - -
// create test file
// - - - - - -

void sdlx_create_test_file(char *dir, char *filename, int freq1, int freq2, int duration_secs);

// --------------------
// SENSORS
// --------------------

// these are from ndk android/sensor.h
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
int sdlx_sensor_read_raw(int id, float *data, int num_values);

int sdlx_sensor_read_step_counter(unsigned long *step_count);
int sdlx_sensor_read_mag_heading(double *mag_heading);
int sdlx_sensor_read_accelerometer(double *ax, double *ay, double *az);
int sdlx_sensor_read_roll_pitch(double *roll, double *pitch);
int sdlx_sensor_read_pressure(double *millibars);
int sdlx_sensor_read_temperature(double *degrees_c);
int sdlx_sensor_read_humidity(double *percent);

// --------------------
// EVENTS   
// --------------------

#define EVID_MOTION  9990
#define EVID_QUIT    9991

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
                                  sdlx_color_t print_color, sdlx_color_t bg_color);

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

// get string, uses virtual keyboard on Android
char *sdlx_get_input_str(char *prompt1, char *prompt2, bool numeric_keybd, sdlx_color_t bg_color);

// --------------------
// NOT AVAILABLE IN PICOC
// --------------------

// xxx put these in private.h  ???

// sdlx_video.c
// xxx put event_box_enable flag here
extern double scale_render;
extern double scale_events_x;
extern double scale_events_y;
extern int    orientation;

int sdlx_video_init(void);
void sdlx_video_quit(void);
void sdlx_minimize_window(void);

// sdlx_audio.c
#ifdef ANDROID
    #define DEFAULT_RECORD_GAIN 5
#else
    #define DEFAULT_RECORD_GAIN 1
#endif
#define DEFAULT_RECORD_SILENCE 0.1

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
#define CONTROL_AREA_SIZE 150

void sdlx_reset_events(void);
void sdlx_event_box_ctrl(bool event_box_enable);

// sdlx_misc.c
#define SUBSYS_VIDEO  1
#define SUBSYS_AUDIO  2
#define SUBSYS_SENSOR 4

int sdlx_init(int subsys);
void sdlx_quit(int subsys);
char *sdlx_get_storage_path(void);
void sdlx_copy_asset_file(char *asset_filename, char *dest_dir);
int sdlx_get_permission(char *name);
int sdlx_create_detached_thread_private(int (*thread_fn)(void*), char *thread_name, void *cx);
#define sdlx_create_detached_thread(thread_fn, cx) \
    do { sdlx_create_detached_thread_private(thread_fn, #thread_fn, cx); } while (0)

#ifdef __cplusplus
}
#endif

#endif
