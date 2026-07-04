#ifndef __SDLX_H__
#define __SDLX_H__

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_NUMBER 999999999

// --------------------
// VIDEO    
// --------------------

// xxx explain sdlx this is a layer on SDL
// xxx intro
// - w x h
// - landscape / portrait
// - general flow
// xxx backbuffer?
// describe how rendering takes place to a texture based on PORTRAIT / LANDSCAPE

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

// This is the first step when rendering the display.
// The backbuffer is set to the color specified. And the
// display orientation to be used (PORTRAIT or LANDSCAPE) 
// is specified.
void sdlx_display_init(sdlx_color_t color, int orientation);

// This routine will present the dispaly. Making the backbuffer
// visible by exchanging the backbuffer and frontbuffer.
void sdlx_display_present(void);

// - - - - - 
// colors
// - - - - - 

#define BYTES_PER_PIXEL   4

// reference: https://www.w3schools.com/colors/colors_converter.asp
// these colors are opaque (alpha equals 255)
//                                         Red      Green       Blue      Alpha
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

// The sdlx_color_t is the 32 bit RGBA value.
// These routines perform their function and return the 32 bit RGBA value.
sdlx_color_t sdlx_create_color(int r, int g, int b, int a);
sdlx_color_t sdlx_scale_color(sdlx_color_t color, double intensity);
sdlx_color_t sdlx_set_color_alpha(sdlx_color_t color, int alpha);
sdlx_color_t sdlx_wavelength_to_color(int wavelength);

// - - - - - - - 
// render text
// - - - - - - - 

// xxx come back here
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
// specified color. These also support the flag param.
#define FLAG_WRAP_MASK     0x00000fff
#define FLAG_X_CTR         0x00001000
#define FLAG_Y_CTR         0x00002000
#define FLAG_ROT_CTR_90    0x00004000
#define FLAG_ROT_CTR_180   0x00008000
#define FLAG_ROT_CTR_270   0x00010000
#define FLAG_BG_BLACK      0x00020000
#define FLAG_BG_WHITE      0x00040000
#define FLAG_XY_CTR        (FLAG_X_CTR | FLAG_Y_CTR)
int sdlx_char_width(int fontid);
int sdlx_char_height(int fontid);
sdlx_loc_t *sdlx_render_printf_ex1(int x, int y, int fontid, sdlx_color_t color, 
                                   char * fmt, ...)
                                   __attribute__ ((format (printf, 5, 6)));
sdlx_loc_t *sdlx_render_printf_ex2(int x, int y, int fontid, sdlx_color_t color, unsigned int flags,
                                   char *fmt, ...) 
                                   __attribute__ ((format (printf, 6, 7)));
void sdlx_render_multiline_text(int x, int y, int y_top, int y_bottom, 
                                int fontid, char **lines, sdlx_color_t *colors, int num_lines);

// - - - - - - - - - - - - - - - - - - - - -
// render rectangle, lines, circles, points
// - - - - - - - - - - - - - - - - - - - - -

#define MAX_POINT_SIZE 9

void sdlx_render_rect(int x, int y, int w, int h, int line_width, sdlx_color_t color);
void sdlx_render_fill_rect(int x, int y, int w, int h, sdlx_color_t color);
void sdlx_render_line(int x1, int y1, int x2, int y2, sdlx_color_t color);
void sdlx_render_lines(sdlx_point_t *points, int count, sdlx_color_t color);
void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, sdlx_color_t color);
void sdlx_render_fill_circle(int x_ctr, int y_ctr, int radius, sdlx_color_t color);
void sdlx_render_point(int x, int y, sdlx_color_t color, int point_size);
void sdlx_render_points(sdlx_point_t *points, int count, sdlx_color_t color, int point_size);

// - - - - - 
// textures
// - - - - - 

// Textures are GPU memory buffers that can be selected as render target.
// The texture can be rendered to a location in the backbuffer; during the              xxx reword
// rendering to the backbuffer, the texture can optionally be scaled and rotated.
//
// For example, the Compass miniApp uses a texture to display the compass image:
// - initialization
//   . the compass image pixels are read from a png file
//   - sdlx_create_texture is called to create a texture of the same width/height as the png file
//   - sdlx_set_texture_pixels is called to copy the png file pixels to the texture
// - run time
//   - the compass_heading is determined from the Android magnetic field sensor
//   - sdlx_render_texture_ex2 is called to copy the compass texture to the backbuffer;   xxx
//     the compass texture is scaled to fit the backbuffer x,y,w,h; and rotated by 
//     the compass heading angle
// - termination
//   - sdlx_destroy_texture is called to free the GPU memory 

// Another way textures can be initialized is by using sdlx_set_render_target.
// For example, during initialization, create a 100x100 texture containing a yellow circle:
//    sdlx_texture_t *circle = sdlx_create_texture(100, 100);
//    sdlx_set_render_target(circle);
//    sdlx_render_fill_circle(50, 50, 50, COLOR_YELLOW);
//    sdlx_set_render_target(NULL); // restores rendering to default rendering texture  xxx explain
// At runtime, the circle texture can be scaled to fill the entire dispaly:
//    sdlx_render_texture_ex1(circle, 0, 0, sdlx_win_width, sdlx_win_height);
// Destroy the circle texture when the program terminates, freeing GPU memory:
//    sdlx_destroy_texture(circle);

// xxx comment all of these
sdlx_texture_t *sdlx_create_texture(int w, int h);
// xxx
void sdlx_destroy_texture(sdlx_texture_t *t);
// returns texture width and height
void sdlx_query_texture(sdlx_texture_t *t, int *w, int *h);
// xxx
void sdlx_clear_texture(sdlx_texture_t *t, sdlx_color_t color);
// adjusts r,g,b intensity; r,g,b args are range 0-1
void sdlx_color_mod_texture(sdlx_texture_t *t, float r, float g, float b);

// xxx
void sdlx_set_texture_pixels(sdlx_texture_t *t, unsigned int *pixels);
// xxx
unsigned int *sdlx_get_texture_pixels(sdlx_texture_t *t, int *w, int *h);

// xxx comments here
void sdlx_render_texture(sdlx_texture_t *t, int x, int y);
// xxx
void sdlx_render_texture_ex1(sdlx_texture_t *t, int x, int y, int w, int h);
// xxx
void sdlx_render_texture_ex2(sdlx_texture_t *t, int x, int y, int w, int h, double angle);
// xxx
void sdlx_render_texture_ex3(sdlx_texture_t *texture, int x, int y, int w, int h, double angle, int xctr, int yctr);

// xxx
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
    int    play_tones_freq;
    int    play_tones_seqnum;
    double volume;
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
void sdlx_audio_set_play_file_time(int secs);
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

// define common events
#define EVID_MOTION  9990
#define EVID_KEYBD   9991 
#define EVID_QUIT    9992

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
                                  int evid3, char *evstr3);

// - - - - - - - - 
// wait for event
// - - - - - - - - 

void sdlx_get_event(long timeout_us, sdlx_event_t *event);

// --------------------
// MISC
// --------------------

// android show toast
void sdlx_show_toast(char *message);

// get string, uses virtual keyboard on Android
char *sdlx_get_input_str(char *prompt, bool numeric_keybd, char *dflt_input_str);

#ifdef __cplusplus
}
#endif

#endif
