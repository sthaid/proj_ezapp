#ifndef __SDLX_H__
#define __SDLX_H__

#ifdef __cplusplus
extern "C" {
#endif

// The sdlx routines are built upon the SDL, SDL_mixer, and SDL_ttf libraries.
// The purpose of these routines is to provide easy to use access to SDL.
// 
// Sdlx provides routines for:
// - video:   graphics rendering to the display
// - audio:   audio play and record
// - sensors: read Android device sensors
// - events:  registration and detection of events
// 
// Details provided in following sections.

#define INVALID_NUMBER 999999999

// --------------------
// VIDEO    
// --------------------

// Sdlx_video defines a logical display that has pixel dimensions based on orientation:
// - Portrait WxH:  1000 x 2200
// - Landscape WxH: 2200 x 1000
// 
// A texture of either (WxH) 1000x2200 (portrait), or 2200x1000 (landscape) is used by sdlx.
// Rendering is performed to this texture. When preenting the display, this texture
// is scaled (and rotated when in landscape mode), to fit the display.
// 
// The top left of the display area is (x,y) = (0,0).
// In Portrait Mode, the bottom right of the display is (x,y) = (999,2199).
// In Landscape Mode, the bottom right of the display is (x,y) = (2199,999).
// 
// Sdlx video example:
//    sdlx_display_init(COLOR_BLACK, PORTRAIT);
//    int x_ctr = sdlx_win_width/2;
//    int y_ctr = sdlx_win_height/2;
//    int radius = 250;
//    sdlx_render_fill_circle(x_ctr, y_ctr, radius, COLOR_PURPLE);
//    sdlx_display_present();

// xxx explain dflt and current texture
// xxx xywh
// xxx pixels are always logical

typedef unsigned int sdlx_color_t;

typedef struct {
    int x, y, w, h;
} sdlx_loc_t;

typedef struct {
    int x, y;
} sdlx_point_t;

typedef struct sdlx_texture sdlx_texture_t;

// These external variables provide the display area size. They are set based
// on the orientation (PORTRAIT or LANDSCAPE) value passed to sdlx_display_init.
//  Orientation     sdlx_win_width      sdlx_win_height
//  -----------     --------------      ---------------
//  PORTRAIT            1000                2200
//  LANDSCAPE           2200                1000
extern int sdlx_win_width;
extern int sdlx_win_height;

// - - - - - - - - - - - - - 
// Display Init and Present
// - - - - - - - - - - - - - 

#define PORTRAIT  0
#define LANDSCAPE 1

// This is the first step when rendering the display.
// The texture being rendered to is set to the color specified.
// And the display orientation to be used (PORTRAIT or LANDSCAPE) is specified.
void sdlx_display_init(sdlx_color_t color, int orientation);

// Calling this routine will make the renderings visible.
void sdlx_display_present(void);

// - - - - - 
// Colors
// - - - - - 

#define BYTES_PER_PIXEL   4

// Reference: https://www.w3schools.com/colors/colors_converter.asp
// These colors are opaque (alpha equals 255)
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
// Example: sdlx_create_color(255,0,0,255) is equivalent to the above '#define COLOR_RED'.
sdlx_color_t sdlx_create_color(int r, int g, int b, int a);
sdlx_color_t sdlx_scale_color(sdlx_color_t color, double intensity);
sdlx_color_t sdlx_set_color_alpha(sdlx_color_t color, int alpha);
sdlx_color_t sdlx_wavelength_to_color(int wavelength);

// - - - - - - - - - - - 
// Render Text, Basic API
// - - - - - - - - - - - 

// xxx global rename to FONTID_NORMAL etc

// The font used by ezApp is 'FreeMonoBold.ttf', which is a fixed width font.

// When a miniApp starts the default fontid is preset to FONT_NORMAL, COLOR_WHITE.

// These values use used for the fontid arg, and specify the size of the font.
// Other values are okay to use, in range xxx (biggest) to xxx (smallest).
#define FONT_TINY     40   // 40 chars fit across portrait display
#define FONT_SMALL    30   // 30 chars fit across portrait display
#define FONT_NORMAL   20   // etc
#define FONT_LARGE    10

// This routine changes the default fontid and color.
void sdlx_print_set_default(int fontid, sdlx_color_t color);

// This printf routine prints to the current texture.
// X,y specify the upper left corner location of the print.
// The default fontid and color are used.
sdlx_loc_t *sdlx_render_printf(int x, int y, char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

// External variables providing the size of a font character,
// for the currently selected default fontid.
extern int sdlx_char_width_dflt;
extern int sdlx_char_height_dflt;

// Macros to convert from default font row,column location to x,y.
#define ROW2Y(r) ((r) * sdlx_char_height_dflt)
#define COL2X(c) ((c) * sdlx_char_width_dflt)

// - - - - - - - - - - - - -
// Render Text, Advanced API
// - - - - - - - - - - - - -

// The Render Text Advanced API does not use the default font and color that are
// used in the Basic API.

// These printf routines are similar to sdlx_render_printf. 
// - The main difference is the fontid, and color are provided as args,
//   instead of using default fontid and color.
// - The flags arg to sdlx_render_printf_ex2 provides additional capabilities, see below.
sdlx_loc_t *sdlx_render_printf_ex1(int x, int y, int fontid, sdlx_color_t color, 
                                   char * fmt, ...)
                                   __attribute__ ((format (printf, 5, 6)));
sdlx_loc_t *sdlx_render_printf_ex2(int x, int y, int fontid, sdlx_color_t color, unsigned int flags,
                                   char *fmt, ...) 
                                   __attribute__ ((format (printf, 6, 7)));

// The sdlx_render_multiline_text routine displays text that spans multiple lines.
// - The x,y args specify the location of the top left corner of the text.
//   The x and y values can be negative. The x and y values are often adjusted 
//   on receipt of the EVID_MOTION event.
// - The y_top and y_bottom args specify range of y locations to which text will be printed;
//   text is not printed above y_top or below y_bottom.
// - The lines and colors args are parallel arrays. Color[n] specifies the color of line[n].
// - There are 2 typical use cases:
//     a) Just one line, with embedded newline chars.
//     b) multiple lines, without embedded newline chars.
// - For an example of the one line use case, refer to show_file routine in apps/lib/lib.c.
void sdlx_render_multiline_text(int x, int y, int y_top, int y_bottom, 
                                int fontid, char **lines, sdlx_color_t *colors, int num_lines);

// These routines return the character witdh and height based on fontid.
int sdlx_char_width(int fontid);
int sdlx_char_height(int fontid);

// Values for the flags arg to sdlx_render_printf_ex2 ...
#define FLAG_WRAP_MASK     0x00000fff   // wrap text at this number of pixels; 
                                        // use zero for wrapping text only on newline char
#define FLAG_X_CTR         0x00001000   // x arg is text center instead of left
#define FLAG_Y_CTR         0x00002000   // y arg is text center instead of top
#define FLAG_ROT_CTR_90    0x00004000   // rotate text about center
#define FLAG_ROT_CTR_180   0x00008000
#define FLAG_ROT_CTR_270   0x00010000
#define FLAG_BG_BLACK      0x00020000   // use black background
#define FLAG_BG_WHITE      0x00040000   // use white background
#define FLAG_XY_CTR        (FLAG_X_CTR | FLAG_Y_CTR)

// - - - - - - - - - - - - - - - - - - - - -
// Render Rectangle, Lines, Circles, Points
// - - - - - - - - - - - - - - - - - - - - -

#define MAX_POINT_SIZE 9  // for call to sdlx_render_point and sdlx_render_points

// These routines perform the function implied by their name.
// xxx comments needed about where rendered to 
// - For sdlx_render_rect and sdlx_render_fill_rect, x,y are the top left corner
//   of the rectangle.
// - For sdlx_render_circle and sdlx_render_fill_circle, x_ctr,y_ctr are the 
//   center of the circle.
// - The point_size arg to sdlx_render_point and sdlx_render_points, ranges from
//   0 (smallest) to MAX_POINT_SIZE (largest).
void sdlx_render_rect(int x, int y, int w, int h, int line_width, sdlx_color_t color);
void sdlx_render_fill_rect(int x, int y, int w, int h, sdlx_color_t color);
void sdlx_render_line(int x1, int y1, int x2, int y2, sdlx_color_t color);
void sdlx_render_lines(sdlx_point_t *points, int count, sdlx_color_t color);
void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, sdlx_color_t color);
void sdlx_render_fill_circle(int x_ctr, int y_ctr, int radius, sdlx_color_t color);
void sdlx_render_point(int x, int y, sdlx_color_t color, int point_size);
void sdlx_render_points(sdlx_point_t *points, int count, sdlx_color_t color, int point_size);

// - - - - - 
// Textures
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

// Create a texture with width w and height h.
sdlx_texture_t *sdlx_create_texture(int w, int h);
// Destroy a texture.
void sdlx_destroy_texture(sdlx_texture_t *t);
// Query a texture for its width and height.
void sdlx_query_texture(sdlx_texture_t *t, int *w, int *h);
// Set all pixels of a texture to the color.
void sdlx_clear_texture(sdlx_texture_t *t, sdlx_color_t color);
// Adjust all pixels in the texture; r,g,b args each range from 0 to 1.
// For example, setting each of r,g,b to 0.5 would reduce the pixel intensity by half.
void sdlx_color_mod_texture(sdlx_texture_t *t, float r, float g, float b);

// Copy pixels to the texture. The pixels arg must contain texture_w * texture_h pixels.
void sdlx_set_texture_pixels(sdlx_texture_t *t, unsigned int *pixels); // xxx use sdlx_color_t ?
// Returns array containing the texture pixels; also returns texture width & height
// in the w, h ptr args. Caller must free the returned pixels array.
unsigned int *sdlx_get_texture_pixels(sdlx_texture_t *t, int *w, int *h);  // xxx sdlx_color_t

// Copy entire texture to location x,y of the current render target.
// The current render target would usually be the default (portrait or landscape) texture;
// unless another texture has been specified by call to sdlx_set_render_target.
void sdlx_render_texture(sdlx_texture_t *t, int x, int y);
// Copy entire texture to location x,y,w,h of the current render target. 
// The texture is scaled to fit w X h.
void sdlx_render_texture_ex1(sdlx_texture_t *t, int x, int y, int w, int h);
// Same as above, except that the texture is also rotated by the specified angle, in degrees.
// The texture is rotated about its center.
void sdlx_render_texture_ex2(sdlx_texture_t *t, int x, int y, int w, int h, double angle);
// Same as above, except the xctr, yctr args specify the point around which the 
// texture is rotated. This is equivalent to the previous routine when xctr=w/2 and yctr=h/2.
void sdlx_render_texture_ex3(sdlx_texture_t *texture, int x, int y, int w, int h, double angle, int xctr, int yctr);

// Set the current render target to texture t.
// It t==NULL, the current render target is set to the default (portraint/landscape) texture.
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
// Control
// - - - - - -

int sdlx_audio_stop(void);
void sdlx_audio_pause(void);
void sdlx_audio_resume(void);
void sdlx_audio_get_state(sdlx_audio_state_t * state);
int sdlx_audio_file_duration_secs(char *dir, char *filename);

// - - - - - -
// Get/Downsample Most Recent Samples
// - - - - - -

void sdlx_get_audio_samples(int num_ret_samples, int num_downsample, int which_channel, float *ret_samples);

// - - - - - -
// Playback
// - - - - - -

int sdlx_audio_play_file(char *dir, char *filename);
void sdlx_audio_set_play_file_time(int secs);
int sdlx_audio_play_tones(sdlx_tone_t *tones);
int sdlx_audio_play_buff(float *samples, int num_samples, int num_channels,
                         int loops, bool free_samples_when_done);

// - - - - - -
// Record
// - - - - - -

int sdlx_audio_record_from_mic(char *dir, char *filename, int auto_stop_secs, bool append, bool start_paused);
int sdlx_audio_record_from_device(char *dir, char *filename, bool append, bool start_paused);

// - - - - - -
// Create Test File
// - - - - - -

void sdlx_create_test_file(char *dir, char *filename, int freq1, int freq2, int duration_secs);

// --------------------
// SENSORS
// --------------------

// - - - - - - - - - - - - 
// High-Level Sensor Access
// - - - - - - - - - - - - 

// These routines read the Android device sensors.
// The return value is 0 for success, and -1 for failure. 
// When returning -1 for failure, the returned arg values are INVALID_NUMBER.

// Provides number of steps since the Android device was booted.
int sdlx_sensor_read_step_counter(unsigned long *step_count);

// The magnetic sensor is read, this sensor provide magnetic field stength in the x,y,z directions.
// The magnetic heading of the device is then calculated based on these 3 field strength values,
// and adjusting for the device roll & pitch.
// The device magnetic heading is provided in range 0 to 359.999 degrees,
// referenced to the top of the device.
int sdlx_sensor_read_mag_heading(double *mag_heading);

// Reads accelerometer sensor. Units are m/s^2.
// - x-axis: left to right
// - y-axis: bottom to top
// - z-axis: perpendicular to the screen pointing to user
int sdlx_sensor_read_accelerometer(double *ax, double *ay, double *az);

// The accelerometer sensor is read, providing acceleration values in the x,y,z directions.
// The device roll & pitch is calculated from these 3 acceleration values.
// The roll and pitch values are provided in degrees.
// When the device is level, the roll and pitch are 0.
// Roll is positive when the right side of the device is lowered.
// Pitch is positive when the top of the device is raised.
int sdlx_sensor_read_roll_pitch(double *roll, double *pitch);

// Provides  atmospheric pressure in millibars.
// Standard atmospheric pressure at sea level is 1,013.25 millibars.
int sdlx_sensor_read_pressure(double *millibars);

// Provides ambient temperature in Celsius.
// Note that most Android devices do not have an ambient temperature sensor.
int sdlx_sensor_read_temperature(double *degrees_c);

// Provides relative humidity in percent.
// Note that most Android devices do not have a humidity sensor.
int sdlx_sensor_read_humidity(double *percent);

// - - - - - - - - - - - - 
// Low-Level Sensor Access
// - - - - - - - - - - - - 

// Android device sensors each have an:
// - id:   identifies the sensor
// - type  the type of sensor
// - name: name of the sensor

// The sensor type values can be found here:
// ~/android/sdk/ndk/*/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/android/sensor.h

typedef struct {
    int   id;
    int   type;
    char *name;
} sdlx_sensor_info_t;

// Returns table of sdlx_sensor_into_t.
// For an example see: files/apps/Test/test.c "PAGE 8: SENSOR INFO TBL".
sdlx_sensor_info_t *sdlx_sensor_get_info_tbl(int *max);

// Search for a sensor of the type specified.
// Returns -1 if not found; otherwise the found sensor id is returned.
// xxx rework this test
int sdlx_sensor_find(int type);

// Read the raw data from the specified sensor id.
// xxx how to deal with step_count
// xxx document that most sensors return float values
// xxx doc how this routine can be used to read the step counter
int sdlx_sensor_read_raw(int id, float *data, int num_values);

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
// Event Registration
// - - - - - - - - - - 

void sdlx_register_event(sdlx_loc_t *loc, int event_id);
void sdlx_register_control_events(int evid1, char *evstr1,
                                  int evid2, char *evstr2,
                                  int evid3, char *evstr3);

// - - - - - - - - 
// Wait For Event
// - - - - - - - - 

void sdlx_get_event(long timeout_us, sdlx_event_t *event);

// --------------------
// MISC
// --------------------

// Display a 'Toast' popup message.
void sdlx_show_toast(char *message);

// Read input text string from user.
// The return value points to a static variable, and must not be freed.
// When numeric_keybd is true the Android numeric keyboard is displayed,
// when false, the full keyboard is displayed.
char *sdlx_get_input_str(char *prompt_optional, bool numeric_keybd, char *dflt_input_str_optional);

#ifdef __cplusplus
}
#endif

#endif
