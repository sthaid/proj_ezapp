#include <std_hdrs.h>

#include <sdlx.h>
#include <logging.h>
#include <utils.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// xxx todo?
// - landscape
// - read pixels routine ? 
// - use of rint vs nearbyint
// - try SDL_SetRenderLogicalPresentation
// xxx review sdl3 port
// - routines now return succ
// - and use of floats instead of ints

//
// font defines
// 

#define FONT_FILE_PATH  "FreeMonoBold.ttf"

#define LARGE_FONT    10
#define DEFAULT_FONT  20

#define MIN_FONT_PTSIZE  10
#define MAX_FONT_PTSIZE  400

#define ONE_MS 1000
#define TEN_MS 10000

//
// typedefs
//

typedef struct {
    sdlx_loc_t loc;
    int       event_id;
} event_t;

//
// global variables
//

int sdlx_win_width;
int sdlx_win_height;
int sdlx_char_width;
int sdlx_char_height;

//
// variables
//

// used by other sdl*.c files
SDL_Window            * window;
double                  scale;

static SDL_Renderer   * renderer;

static TTF_Font        *font[MAX_FONT_PTSIZE];

static int              max_event;
static bool             evid_swipe_right_registered;
static bool             evid_swipe_left_registered;
static bool             evid_motion_registered;
static bool             evid_keybd_registered;

//
// prototypes
//

static void set_render_draw_color(int color);

//
// inline routines
//

//xxx static assert
// xxx [-Werror=strict-aliasing]
static inline SDL_Color sdlx_color(int color)
{
    SDL_Color val;
    memcpy(&val, &color, sizeof(color));
    return val;
}

// ----------------- INIT / EXIT --------------------------

// xxx temp for testing 
bool event_watcher(void* userdata, SDL_Event* event)
{
    static SDL_Renderer * save_renderer;

    switch (event->type) {
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            save_renderer = renderer;
            renderer = NULL;
            sleep(1);
            // Pause your game loop and background tasks
            INFO("App is about to be backgrounded\n");
            break;
        case SDL_EVENT_DID_ENTER_BACKGROUND:
            INFO("App is now in the background\n");
            break;
        case SDL_EVENT_WILL_ENTER_FOREGROUND:
            INFO("App is about to be foregrounded\n");
            break;
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            renderer = save_renderer;
            // Resume your game loop and tasks
            INFO("App is now in the foreground\n");
            break;
        default:
            break;
    }
    return 0;
}

#if 0 // xxx del later
    // set hints
    bool succ;
    succ = SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");
    if (!succ) {
        ERROR("failed to set SDL_HINT_ANDROID_BLOCK_ON_PAUSE\n");
    }
    succ = SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");  //xxx temp
    if (!succ) {
        ERROR("failed to set SDL_HINT_ENABLE_SCREEN_KEYBOARD\n");
    }
#endif

#if 0 //xxx make this a font routine
        // debug code to print font info
        for (int ptsize = MIN_FONT_PTSIZE; ptsize < MAX_FONT_PTSIZE; ptsize++) {
            TTF_Font *f = TTF_OpenFont(FONT_FILE_PATH, ptsize);
            TTF_SizeText(f, "X", &chw, &chh);
            TTF_CloseFont(f);
            INFO("font ptsize = %d  chw/chh = %d %d\n", ptsize, chw, chh);
        }
#endif

int sdlx_video_init(void)
{
    int    real_win_width, real_win_height;
    int    num, i;
    double aspect_ratio;

    INFO("initializing\n");

    // initialize SDL video
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        ERROR("SDL_Init VIDEO failed, %s\n", SDL_GetError());
        return -1;
    }

    // display available and current video drivers
    num = SDL_GetNumVideoDrivers();
    INFO("Available Video Drivers: ");
    for (i = 0; i < num; i++) {
        INFO("   %s\n",  SDL_GetVideoDriver(i));
    }

    // create SDL Window and Renderer
#ifdef ANDROID
    if (!SDL_CreateWindowAndRenderer("ezApp", 0, 0, SDL_WINDOW_FULLSCREEN, &window, &renderer)) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }
#else
    if (!SDL_CreateWindowAndRenderer("ezApp", 450, 975, 0, &window, &renderer)) {
        ERROR("SDL_CreateWindowAndRenderer failed\n");
        return -1;
    }
#endif

    // add the event watcher  xxx tbd
    SDL_AddEventWatch(event_watcher, NULL);

    // get real windows size and aspect ratio
    SDL_GetWindowSize(window, &real_win_width, &real_win_height);
    aspect_ratio = (double)real_win_height / real_win_width;
    INFO("real win_width x height = %d %d  aspect = %f\n", real_win_width, real_win_height, aspect_ratio);

    // xxx comment
    sdlx_win_width  = 1000;
    sdlx_win_height = rint(1000 * aspect_ratio);
    scale = (double)real_win_width / sdlx_win_width;
    INFO("logical sdlx_win_width x height = %d %d  scale = %f\n", sdlx_win_width, sdlx_win_height, scale);

    // initialize True Type Font
    if (!TTF_Init()) {
        ERROR("TTF_Init failed\n");
        return -1;
    }

    // init default fontsize, where DEFAULT_FONT is num chars across display;
    // and validate expected character size and columns
    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, COLOR_BLACK);
    INFO("sdlx_print_init(%d) sdlx_char_width=%d sdlx_char_height=%d\n", 
         DEFAULT_FONT, sdlx_char_width, sdlx_char_height);
    if (sdlx_char_width != 50 || sdlx_char_height != 83) {
        ERROR("chw,chh, expected = 50,83  actual = %d,%d\n", sdlx_char_width, sdlx_char_height);
    }

    // enable alpha blending
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // this is needed so that the first actual display present works
    sdlx_display_init(COLOR_BLACK);
    sdlx_display_present();

    // return success
    INFO("success\n");
    return 0;
}

void sdlx_video_quit(void)
{
    int i;

    INFO("quitting\n");

    // close fonts
    for (i = MIN_FONT_PTSIZE; i < MAX_FONT_PTSIZE; i++) {
        if (font[i] != NULL) {
            TTF_CloseFont(font[i]);
        }
    }
    TTF_Quit();

    // destroy the renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // quit SDL video
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

#if 0
SDL_Window *sdlx_get_window(void)
{
    return window;
}
#endif

void sdlx_minimize_window(void)
{   
    SDL_MinimizeWindow(window);
}

// ----------------- DISPLAY INIT / PRESENT ---------------

void sdlx_display_init(int color)
{
    sdlx_reset_events();
    // xxx need a routine in sdlx_event
    max_event = 0;
    evid_swipe_right_registered = false;
    evid_swipe_left_registered = false;
    evid_motion_registered = false;
    evid_keybd_registered = false;

    set_render_draw_color(color);
    SDL_RenderClear(renderer);
}

void sdlx_display_present(void)
{
    SDL_RenderPresent(renderer);
}

// -----------------  COLORS  -----------------------------

int sdlx_create_color(int r, int g, int b, int a)
{
    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

int sdlx_scale_color(int color, double inten)
{
    int r = (color >> 0) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 16) & 0xff;
    int a = (color >> 24) & 0xff;

    if (inten < 0) inten = 0;
    if (inten > 1) inten = 1;

    r *= inten;
    g *= inten;
    b *= inten;

    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

int sdlx_set_color_alpha(int color, int alpha)
{
    return (color & 0x00ffffff) | ((alpha & 0xff) << 24);
}

// ported from http://www.noah.org/wiki/Wavelength_to_RGB_in_Python
int sdlx_wavelength_to_color(int wavelength_arg)
{
    double wavelength = wavelength_arg;
    double attenuation;
    double gamma = 0.8;
    double R,G,B;

    if (wavelength >= 380 && wavelength <= 440) {
        double attenuation = 0.3 + 0.7 * (wavelength - 380) / (440 - 380);
        R = pow((-(wavelength - 440) / (440 - 380)) * attenuation, gamma);
        G = 0.0;
        B = pow(1.0 * attenuation, gamma);
    } else if (wavelength >= 440 && wavelength <= 490) {
        R = 0.0;
        G = pow((wavelength - 440) / (490 - 440), gamma);
        B = 1.0;
    } else if (wavelength >= 490 && wavelength <= 510) {
        R = 0.0;
        G = 1.0;
        B = pow(-(wavelength - 510) / (510 - 490), gamma);
    } else if (wavelength >= 510 && wavelength <= 580) {
        R = pow((wavelength - 510) / (580 - 510), gamma);
        G = 1.0;
        B = 0.0;
    } else if (wavelength >= 580 && wavelength <= 645) {
        R = 1.0;
        G = pow(-(wavelength - 645) / (645 - 580), gamma);
        B = 0.0;
    } else if (wavelength >= 645 && wavelength <= 750) {
        attenuation = 0.3 + 0.7 * (750 - wavelength) / (750 - 645);
        R = pow(1.0 * attenuation, gamma);
        G = 0.0;
        B = 0.0;
    } else {
        R = 0.0;
        G = 0.0;
        B = 0.0;
    }

    if (R < 0) R = 0; else if (R > 1) R = 1;
    if (G < 0) G = 0; else if (G > 1) G = 1;
    if (B < 0) B = 0; else if (B > 1) B = 1;

    return sdlx_create_color(R*255, G*255, B*255, 255);
}

static void set_render_draw_color(int color)
{
    int r = (color >> 0) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 16) & 0xff;
    int a = (color >> 24) & 0xff;

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// -----------------  RENDER TEXT  ------------------------

static sdlx_print_state_t print_state;

void sdlx_print_save(sdlx_print_state_t *save)
{
    *save = print_state;
}

void sdlx_print_restore(sdlx_print_state_t *restore)
{
    print_state = *restore;
    sdlx_char_width = print_state.char_width;
    sdlx_char_height = print_state.char_height;
}

void sdlx_print_init_color(int fg_color, int bg_color)
{
    print_state.fg_color = fg_color;
    print_state.bg_color = bg_color;
}

void sdlx_print_init_numchars(double numchars)
{
    int ptsize;
    double chw_fp, chh_fp;

    // determine real font ptsize to use;
    // note: rint() not used here so ptsize will round down
    chw_fp = (sdlx_win_width / numchars) * scale;
    chh_fp = chw_fp / 0.6;
    ptsize = chh_fp;

    // ensure ptiszie is in range
    if (ptsize < MIN_FONT_PTSIZE) ptsize = MIN_FONT_PTSIZE;
    if (ptsize >= MAX_FONT_PTSIZE) ptsize = MAX_FONT_PTSIZE-1;

    // if the requested font pointsize has not yet been opened
    // then do so
    if (font[ptsize] == NULL) {
        font[ptsize] = TTF_OpenFont(FONT_FILE_PATH, ptsize);
        if (font[ptsize] == NULL) {
            ERROR("TTF_OpenFont failed, ptsize=%d\n", ptsize);
            return;
        }
    }

    // save new point size and character width/height xxx comment
    sdlx_char_width  = rint(sdlx_win_width / numchars);  // xxx nearbyint
    sdlx_char_height = rint(sdlx_char_width / 0.6);

    // save new font ptsize and sdlx_char width/height to print_state
    print_state.ptsize = ptsize;
    print_state.char_width = sdlx_char_width;
    print_state.char_height = sdlx_char_height;
}

void sdlx_print_init(double numchars, int fg_color, int bg_color)
{
    sdlx_print_init_numchars(numchars);
    sdlx_print_init_color(fg_color, bg_color);
}

static sdlx_loc_t *render_text(bool xy_is_ctr, int x, int y, char * str)
{
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_FRect     pos;
    static sdlx_loc_t loc;

    //printf("xy_is_ctr = %d x=%d y=%d str='%s'\n", xy_is_ctr, x, y, str);

    // if font not initialized then return error
    if (font[print_state.ptsize] == NULL) {
        ERROR("font ptsize %d, not initialized\n", print_state.ptsize);
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    // if zero len str then return
    if (str[0] == '\0') {
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    // render the string to a surface xxx cleanup
    surface = TTF_RenderText_Solid(font[print_state.ptsize], str, 0, 
                                         sdlx_color(print_state.fg_color));
                                         //sdlx_color(print_state.bg_color));
    if (surface == NULL) {
        ERROR("TTF_RenderText_Solid returned NULL\n");
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    // determine the real display position to render the text
    // xxx dont need rint now
    if (!xy_is_ctr) {
        pos.x = x*scale;
        pos.y = y*scale;
        pos.w = surface->w;
        pos.h = surface->h;
    } else {
        pos.x = x*scale - surface->w/2.;
        pos.y = y*scale - surface->h/2.;
        pos.w = surface->w;
        pos.h = surface->h;
    }

    // create texture from the surface, and render the texture
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderTexture(renderer, texture, NULL, &pos);

    // clean up
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);

    // return the display location where the text was rendered;
    loc.x = pos.x / scale;
    loc.y = pos.y / scale;
    loc.w = pos.w / scale;
    loc.h = pos.h / scale;
    return &loc;
}

// note: each line may have embedded newline chars
void sdlx_render_multiline_text(int y_top, int y_display_begin, int y_display_end, char **lines, int num_lines)
{
    int y = y_top;
    int n = 0, k = 0, len;
    char *ptr;
    char str[200];

    while (n < num_lines) {
        // if y pos of line is below the bottom of the
        // display region then break
        if (y > y_display_end - sdlx_char_height) {
            break;
        }

        // extract str from the line currently being processed
        ptr = strchr(&lines[n][k], '\n');
        if (ptr) {
            len = ptr - &lines[n][k];
            memcpy(str, &lines[n][k], len);
            str[len] = '\0';
        } else {
            strcpy(str, &lines[n][k]);
            len = strlen(str);
        }

        // advance k and n
        k += len;
        if (lines[n][k] == '\n') {
            k++;
        }
        if (lines[n][k] == '\0') {
            k = 0;
            n++;
        }

        // if y loc of line is at or below the begining of the display
        // region then render the line
        if (y >= y_display_begin && str[0] != '\0') {
            render_text(false, 0, y, str);
        }

        // advance y for the next line
        y += sdlx_char_height;
    }
}

sdlx_loc_t *sdlx_render_text(int x, int y, char * str)
{
    return render_text(false, x, y, str);
}

sdlx_loc_t *sdlx_render_text_xyctr(int x, int y, char * str)
{
    return render_text(true, x, y, str);
}

sdlx_loc_t *sdlx_render_printf(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return sdlx_render_text(x, y, str);
}

sdlx_loc_t *sdlx_render_printf_xyctr(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return sdlx_render_text_xyctr(x, y, str);
}

// -----------------  RENDER RECTANGLES, LINES, CIRCLES, POINTS  --------------------

void sdlx_render_rect(int x, int y, int w, int h, int line_width, int color)
{
    SDL_FRect rect;
    int i;

    rect.x = x * scale;
    rect.y = y * scale;
    rect.w = w * scale;
    rect.h = h * scale;

    set_render_draw_color(color);

    for (i = 0; i < line_width; i++) {
        SDL_RenderRect(renderer, &rect);
        if (rect.w < 2 || rect.h < 2) {
            break;
        }
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
    }
}

void sdlx_render_fill_rect(int x, int y, int w, int h, int color)
{
    SDL_FRect rect;

    rect.x = x * scale;
    rect.y = y * scale;
    rect.w = w * scale;
    rect.h = h * scale;

    set_render_draw_color(color);
    SDL_RenderFillRect(renderer, &rect);
}

void sdlx_render_line(int x1, int y1, int x2, int y2, int color)
{
    sdlx_point_t points[2] = { {x1,y1}, {x2,y2} };
    sdlx_render_lines(points, 2, color);
}

void sdlx_render_lines(sdlx_point_t *points, int count, int color)
{
    SDL_FPoint scaled_points[100];  // xxx malloc this

    if (count <= 1) {
        return;
    }

    for (int i = 0; i < count; i++) {
        scaled_points[i].x = points[i].x * scale;
        scaled_points[i].y = points[i].y * scale;
    }

    set_render_draw_color(color);

    SDL_RenderLines(renderer, scaled_points, count);
}

// xxx change args to x_ctr_arg ..
void sdlx_render_circle(int x_ctr_arg, int y_ctr_arg, int radius, int line_width, int color)
{
    int count = 0, i, angle, x, y;
    int x_center, y_center;
    SDL_FPoint points[370];

    static int sin_table[370];
    static int cos_table[370];
    static bool first_call = true;

    // xxx comment
    x_center = rint(x_ctr_arg * scale);
    y_center = rint(y_ctr_arg * scale);
    radius   = rint(radius * scale);

    // on first call make table of sin and cos indexed by degrees
    if (first_call) {
        for (angle = 0; angle < 362; angle++) {
            sin_table[angle] = sin(angle*(2*M_PI/360)) * (1<<10);
            cos_table[angle] = cos(angle*(2*M_PI/360)) * (1<<10);
        }
        first_call = false;
    }

    // set the color
    set_render_draw_color(color);

    // loop over line_width
    for (i = 0; i < line_width; i++) {
        // draw circle
        for (angle = 0; angle < 362; angle++) {
            x = x_center + ((radius * sin_table[angle]) >> 10);
            y = y_center + ((radius * cos_table[angle]) >> 10);
            points[count].x = x;
            points[count].y = y;
            count++;
        }
        SDL_RenderLines(renderer, points, count);
        count = 0;

        // reduce radius by 1
        radius--;
        if (radius <= 0) {
            break;
        }
    }
}

void sdlx_render_point(int x, int y, int color, int point_size)
{
    sdlx_point_t point = {x,y};

    sdlx_render_points(&point, 1, color, point_size);
}

void sdlx_render_points(sdlx_point_t *points, int count, int color, int point_size)
{
    #define MAX_SDL_POINTS 1000

    static struct point_extend_s {
        int max;
        struct point_extend_offset_s {
            int x;
            int y;
        } offset[300];
    } point_extend[10] = {
    { 1, {
        {0,0}, 
            } },
    { 5, {
        {-1,0}, 
        {0,-1}, {0,0}, {0,1}, 
        {1,0}, 
            } },
    { 21, {
        {-2,-1}, {-2,0}, {-2,1}, 
        {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, 
        {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, 
        {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, 
        {2,-1}, {2,0}, {2,1}, 
            } },
    { 37, {
        {-3,-1}, {-3,0}, {-3,1}, 
        {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, 
        {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, 
        {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, 
        {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, 
        {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, 
        {3,-1}, {3,0}, {3,1}, 
            } },
    { 61, {
        {-4,-1}, {-4,0}, {-4,1}, 
        {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, 
        {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, 
        {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, 
        {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, 
        {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, 
        {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, 
        {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, 
        {4,-1}, {4,0}, {4,1}, 
            } },
    { 89, {
        {-5,-1}, {-5,0}, {-5,1}, 
        {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, 
        {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, 
        {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, 
        {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, 
        {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, 
        {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, 
        {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, 
        {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, 
        {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, 
        {5,-1}, {5,0}, {5,1}, 
            } },
    { 121, {
        {-6,-1}, {-6,0}, {-6,1}, 
        {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, 
        {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, 
        {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, 
        {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, 
        {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, 
        {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, 
        {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, 
        {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, 
        {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, 
        {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, 
        {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, 
        {6,-1}, {6,0}, {6,1}, 
            } },
    { 177, {
        {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, 
        {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, 
        {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, 
        {-4,-6}, {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-4,5}, {-4,6}, 
        {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, 
        {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, 
        {-1,-7}, {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {-1,7}, 
        {0,-7}, {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, 
        {1,-7}, {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, 
        {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, 
        {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, 
        {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, 
        {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}, 
        {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, 
        {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2}, 
            } },
    { 221, {
        {-8,-2}, {-8,-1}, {-8,0}, {-8,1}, {-8,2}, 
        {-7,-4}, {-7,-3}, {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, {-7,3}, {-7,4}, 
        {-6,-5}, {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, {-6,5}, 
        {-5,-6}, {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, {-5,6}, 
        {-4,-7}, {-4,-6}, {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-4,5}, {-4,6}, {-4,7}, 
        {-3,-7}, {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, {-3,7}, 
        {-2,-8}, {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, {-2,8}, 
        {-1,-8}, {-1,-7}, {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {-1,7}, {-1,8}, 
        {0,-8}, {0,-7}, {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {0,8}, 
        {1,-8}, {1,-7}, {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, 
        {2,-8}, {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, {2,8}, 
        {3,-7}, {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {3,7}, 
        {4,-7}, {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}, 
        {5,-6}, {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}, {5,6}, 
        {6,-5}, {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, {6,5}, 
        {7,-4}, {7,-3}, {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2}, {7,3}, {7,4}, 
        {8,-2}, {8,-1}, {8,0}, {8,1}, {8,2}, 
            } },
    { 277, {
        {-9,-2}, {-9,-1}, {-9,0}, {-9,1}, {-9,2}, 
        {-8,-4}, {-8,-3}, {-8,-2}, {-8,-1}, {-8,0}, {-8,1}, {-8,2}, {-8,3}, {-8,4}, 
        {-7,-6}, {-7,-5}, {-7,-4}, {-7,-3}, {-7,-2}, {-7,-1}, {-7,0}, {-7,1}, {-7,2}, {-7,3}, {-7,4}, {-7,5}, {-7,6}, 
        {-6,-7}, {-6,-6}, {-6,-5}, {-6,-4}, {-6,-3}, {-6,-2}, {-6,-1}, {-6,0}, {-6,1}, {-6,2}, {-6,3}, {-6,4}, {-6,5}, {-6,6}, {-6,7}, 
        {-5,-7}, {-5,-6}, {-5,-5}, {-5,-4}, {-5,-3}, {-5,-2}, {-5,-1}, {-5,0}, {-5,1}, {-5,2}, {-5,3}, {-5,4}, {-5,5}, {-5,6}, {-5,7}, 
        {-4,-8}, {-4,-7}, {-4,-6}, {-4,-5}, {-4,-4}, {-4,-3}, {-4,-2}, {-4,-1}, {-4,0}, {-4,1}, {-4,2}, {-4,3}, {-4,4}, {-4,5}, {-4,6}, {-4,7}, {-4,8}, 
        {-3,-8}, {-3,-7}, {-3,-6}, {-3,-5}, {-3,-4}, {-3,-3}, {-3,-2}, {-3,-1}, {-3,0}, {-3,1}, {-3,2}, {-3,3}, {-3,4}, {-3,5}, {-3,6}, {-3,7}, {-3,8}, 
        {-2,-9}, {-2,-8}, {-2,-7}, {-2,-6}, {-2,-5}, {-2,-4}, {-2,-3}, {-2,-2}, {-2,-1}, {-2,0}, {-2,1}, {-2,2}, {-2,3}, {-2,4}, {-2,5}, {-2,6}, {-2,7}, {-2,8}, {-2,9}, 
        {-1,-9}, {-1,-8}, {-1,-7}, {-1,-6}, {-1,-5}, {-1,-4}, {-1,-3}, {-1,-2}, {-1,-1}, {-1,0}, {-1,1}, {-1,2}, {-1,3}, {-1,4}, {-1,5}, {-1,6}, {-1,7}, {-1,8}, {-1,9}, 
        {0,-9}, {0,-8}, {0,-7}, {0,-6}, {0,-5}, {0,-4}, {0,-3}, {0,-2}, {0,-1}, {0,0}, {0,1}, {0,2}, {0,3}, {0,4}, {0,5}, {0,6}, {0,7}, {0,8}, {0,9}, 
        {1,-9}, {1,-8}, {1,-7}, {1,-6}, {1,-5}, {1,-4}, {1,-3}, {1,-2}, {1,-1}, {1,0}, {1,1}, {1,2}, {1,3}, {1,4}, {1,5}, {1,6}, {1,7}, {1,8}, {1,9}, 
        {2,-9}, {2,-8}, {2,-7}, {2,-6}, {2,-5}, {2,-4}, {2,-3}, {2,-2}, {2,-1}, {2,0}, {2,1}, {2,2}, {2,3}, {2,4}, {2,5}, {2,6}, {2,7}, {2,8}, {2,9}, 
        {3,-8}, {3,-7}, {3,-6}, {3,-5}, {3,-4}, {3,-3}, {3,-2}, {3,-1}, {3,0}, {3,1}, {3,2}, {3,3}, {3,4}, {3,5}, {3,6}, {3,7}, {3,8}, 
        {4,-8}, {4,-7}, {4,-6}, {4,-5}, {4,-4}, {4,-3}, {4,-2}, {4,-1}, {4,0}, {4,1}, {4,2}, {4,3}, {4,4}, {4,5}, {4,6}, {4,7}, {4,8}, 
        {5,-7}, {5,-6}, {5,-5}, {5,-4}, {5,-3}, {5,-2}, {5,-1}, {5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}, {5,6}, {5,7}, 
        {6,-7}, {6,-6}, {6,-5}, {6,-4}, {6,-3}, {6,-2}, {6,-1}, {6,0}, {6,1}, {6,2}, {6,3}, {6,4}, {6,5}, {6,6}, {6,7}, 
        {7,-6}, {7,-5}, {7,-4}, {7,-3}, {7,-2}, {7,-1}, {7,0}, {7,1}, {7,2}, {7,3}, {7,4}, {7,5}, {7,6}, 
        {8,-4}, {8,-3}, {8,-2}, {8,-1}, {8,0}, {8,1}, {8,2}, {8,3}, {8,4}, 
        {9,-2}, {9,-1}, {9,0}, {9,1}, {9,2}, 
            } },
                };

    int i, j, x, y;
    SDL_FPoint sdlx_points[MAX_SDL_POINTS];
    int sdlx_points_count = 0;
    struct point_extend_s * pe = &point_extend[point_size];
    struct point_extend_offset_s * peo = pe->offset;

    if (count < 0) {
        return;
    }
    if (point_size < 0) {
        point_size = 0;
    }
    if (point_size > 9) {
        point_size = 9;
    }

    set_render_draw_color(color);

    for (i = 0; i < count; i++) {
        for (j = 0; j < pe->max; j++) {
            x = rint((points[i].x + peo[j].x) * scale);
            y = rint((points[i].y + peo[j].y) * scale);
            sdlx_points[sdlx_points_count].x = x;
            sdlx_points[sdlx_points_count].y = y;
            sdlx_points_count++;

            if (sdlx_points_count == MAX_SDL_POINTS) {
                SDL_RenderPoints(renderer, sdlx_points, sdlx_points_count);
                sdlx_points_count = 0;
            }
        }
    }

    if (sdlx_points_count > 0) {
        SDL_RenderPoints(renderer, sdlx_points, sdlx_points_count);
        sdlx_points_count = 0;
    }
}

// -----------------  RENDER USING TEXTURES  ---------------------------- 


sdlx_texture_t *sdlx_create_texture_from_pixels(unsigned char *pixels, int w, int h)
{
    sdlx_texture_t *texture;

    // create the texture
    texture = (sdlx_texture_t*)
              SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                w, h);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        return NULL;
    }

    // update the texture with the pixels
    SDL_UpdateTexture((SDL_Texture*)texture, NULL, pixels, w * BYTES_PER_PIXEL);

    // return the texture
    return texture;
}

sdlx_texture_t *sdlx_create_filled_circle_texture(int radius, int color)
{
    radius *= scale;

    int width = 2 * radius + 1;
    int x = radius;
    int y = 0;
    int radiusError = 1-x;
    int (*pixels)[width];
    sdlx_texture_t * texture;

    #define DRAWLINE(Y, XS, XE, V) \
        do { \
            int i; \
            for (i = XS; i <= XE; i++) { \
                pixels[Y][i] = (V); \
            } \
        } while (0)

    // alloc zeroed pixels
    pixels = calloc(width * width, BYTES_PER_PIXEL);

    // initialize pixels
    while(x >= y) {
        DRAWLINE(y+radius, -x+radius, x+radius, color);
        DRAWLINE(x+radius, -y+radius, y+radius, color);
        DRAWLINE(-y+radius, -x+radius, x+radius, color);
        DRAWLINE(-x+radius, -y+radius, y+radius, color);
        y++;
        if (radiusError<0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x) + 1;
        }
    }

    // create the texture and copy the pixels to the texture
    texture = (sdlx_texture_t*)
              SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                width, width);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        free(pixels);
        return NULL;
    }
    SDL_SetTextureBlendMode((SDL_Texture*)texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture((SDL_Texture*)texture, NULL, pixels, width*BYTES_PER_PIXEL);

    free(pixels);

    // return texture
    return texture;
}

sdlx_texture_t *sdlx_create_text_texture(char * str)
{
    SDL_Surface * surface;
    SDL_Texture * texture;

    if (str[0] == '\0') {
        return NULL;
    }

    // if font not initialized then return error
    if (font[print_state.ptsize] == NULL) {
        ERROR("font ptsize %d, not initialized\n", print_state.ptsize);
        return NULL;
    }

    // render the text to a surface,  xxx cleanup
    // create a texture from the surface
    // free the surface
    surface = TTF_RenderText_Solid(font[print_state.ptsize], str, 0, 
                                    sdlx_color(print_state.fg_color));
                                    //sdlx_color(print_state.bg_color));
    if (surface == NULL) {
        ERROR("failed to allocate surface\n");
        return NULL;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        SDL_DestroySurface(surface);
        return NULL;
    }
    SDL_DestroySurface(surface);

    // return the texture which contains the text
    return (sdlx_texture_t*)texture;
}

sdlx_loc_t *sdlx_render_texture(int x, int y, int w, int h, sdlx_texture_t *texture)
{
    SDL_FRect dest;
    static sdlx_loc_t loc;

    if (texture == NULL) {
        loc.x = x; loc.y = y; loc.w = 0; loc.h = 0;
        return &loc;
    }

    dest.x = x * scale;
    dest.y = y * scale;
    dest.w = w * scale;
    dest.h = h * scale;

    SDL_RenderTextureRotated(renderer, (SDL_Texture*)texture, NULL, &dest, 0, NULL, false);

    // return the display location where the text was rendered;
    loc.x = x;
    loc.y = y;
    loc.w = w;
    loc.h = h;
    return &loc;
}

void sdlx_render_texture_ex(int x, int y, int w, int h, double angle, sdlx_texture_t *texture)
{
    SDL_FRect dest;

    if (texture == NULL) {
        return;
    }

    dest.x = x * scale;
    dest.y = y * scale;
    dest.w = w * scale;
    dest.h = h * scale;

    SDL_RenderTextureRotated(renderer, (SDL_Texture*)texture, NULL, &dest, angle, NULL, false);
}

void sdlx_render_texture_ex2(int x, int y, int w, int h, double angle, int xctr, int yctr,
                            sdlx_texture_t *texture)
{
    SDL_FRect dest;
    SDL_FPoint ctr;

    if (texture == NULL) {
        return;
    }

    dest.x = x * scale;
    dest.y = y * scale;
    dest.w = w * scale;
    dest.h = h * scale;

    ctr.x = xctr * scale;
    ctr.y = yctr * scale;

    SDL_RenderTextureRotated(renderer, (SDL_Texture*)texture, NULL, &dest, angle, &ctr, false);
}

void sdlx_destroy_texture(sdlx_texture_t *texture)
{
    if (texture == NULL) {
        return;
    }

    SDL_DestroyTexture((SDL_Texture *)texture);
}

void sdlx_query_texture(sdlx_texture_t *texture, int * width, int * height)
{
    float w_float, h_float;

    if (texture == NULL) {
        *width = 0;
        *height = 0;
        return;
    }

    SDL_GetTextureSize((SDL_Texture *)texture, &w_float, &h_float);
    *width = rint(w_float / scale);
    *height = rint(h_float / scale);
}

// caller must free returned ptr to pixels
unsigned char *sdlx_read_display_pixels(int x, int y, int w, int h, int *w_pixels, int *h_pixels)
{
    SDL_Rect       loc;
    SDL_Surface   *surface;
    unsigned char *pixels;
    unsigned char *pixels_malloced;

    // init display location to read the pixels from
    loc.x = x * scale; // xxx rint
    loc.y = y * scale;
    loc.w = w * scale;
    loc.h = h * scale;

    // read the pixels to SDL_Surface  xxx check all rets 
    surface = SDL_RenderReadPixels(renderer, &loc);
    if (surface == NULL) {
        ERROR("SDL_RenderReadPixels, %s\n", SDL_GetError());
        return NULL;
    }

    // allocate memory for the return array of pixels
    pixels_malloced = malloc(loc.w * loc.h * BYTES_PER_PIXEL);
    if (pixels_malloced == NULL) {
        ERROR("allocate pixels failed\n");
        SDL_DestroySurface(surface);
        return NULL;
    }

    // copy pixels from 'surface' to pixels buffer
    pixels = pixels_malloced;
    for (int row = 0; row < loc.h; row++) {
        memcpy(pixels, 
               surface->pixels + (row * surface->pitch), 
               loc.w * BYTES_PER_PIXEL);
        pixels += (loc.w * BYTES_PER_PIXEL);
    }

    // destroy surface
    SDL_DestroySurface(surface);

    // success, return pixels
    *w_pixels = loc.w;
    *h_pixels = loc.h;
    return pixels_malloced;
}

// -----------------  PLOTTING  ----------------------------------------- 

// xxx either test and improve this, or delete it

typedef struct {
    char   title[64];
    int    xleft;
    int    xright;
    int    xspan;
    int    ybottom;
    int    ytop;
    int    yspan;
    double xval_min;
    double xval_max;
    double xval_span;
    double yval_min;
    double yval_max;
    double yval_span;
    double yval_of_x_axis;
} plot_cx_t;

static int xval2x(plot_cx_t *cx, double xval)
{
    int x;

    x = cx->xleft + (xval - cx->xval_min) * (cx->xspan / cx->xval_span);  // xxx add cvt constant to cx
    return x;
}

static int yval2y(plot_cx_t *cx, double yval)
{
    int y;

    y = cx->ybottom - (yval - cx->yval_min) * (cx->yspan / cx->yval_span);  // xxx add cvt constant to cx
    return y;
}

void *sdlx_plot_create(char *title, 
                      int xleft, int xright, int ybottom, int ytop,
                      double xval_left, double xval_right, double yval_bottom, double yval_top,
                      double yval_of_x_axis)
{
    plot_cx_t *cx;

    // alloc cx and save params in cx
    cx = calloc(1, sizeof(plot_cx_t));
    strcpy(cx->title, title);
    cx->xleft          = xleft;
    cx->xright         = xright;
    cx->xspan          = xright - xleft;
    cx->ybottom        = ybottom;
    cx->ytop           = ytop;
    cx->yspan          = ybottom - ytop;
    cx->xval_min       = xval_left;
    cx->xval_max       = xval_right;
    cx->xval_span      = xval_right - xval_left;
    cx->yval_min       = yval_bottom;
    cx->yval_max       = yval_top;
    cx->yval_span      = yval_top - yval_bottom;
    cx->yval_of_x_axis = yval_of_x_axis;

    // return cx
    return cx;
}

void sdlx_plot_axis(void *cx_arg, char *xmin_str, char *xmax_str, char *ymin_str, char *ymax_str)
{
    plot_cx_t        *cx = (plot_cx_t*)cx_arg;
    sdlx_print_state_t print_state;
    int               i, y;

    // print save and init
    sdlx_print_save(&print_state);
    sdlx_print_init(SMALLEST_FONT, COLOR_WHITE, COLOR_BLACK);

    // draw rectangle around the plot area
    sdlx_render_rect(cx->xleft, cx->ytop, cx->xspan, cx->yspan, 3, COLOR_BLUE);

    // draw and label x-axis xxx option to not do this
    if (cx->yval_of_x_axis != INVALID_NUMBER) {
        y = yval2y(cx, cx->yval_of_x_axis);
        for (i = -1; i <= 1; i++) {
            sdlx_render_line(cx->xleft, y+i, cx->xright, y+i, COLOR_BLUE);
        }
        y = yval2y(cx, cx->yval_of_x_axis);
        if ((xmin_str != NULL) && (xmin_str[0] != '\0')) {
            sdlx_render_printf(cx->xleft+3, y+3, "%s", xmin_str);
        }
        if ((xmax_str != NULL) && (xmax_str[0] != '\0')) {
            sdlx_render_printf(cx->xright-3-strlen(xmax_str)*sdlx_char_width, y+3, "%s", xmax_str);
        }
    }

    // label y-axis
    if ((ymin_str != NULL) && (ymin_str[0] != '\0')) {
        sdlx_render_printf(cx->xleft+3, cx->ybottom-3-sdlx_char_height, "%s", ymin_str);
        sdlx_render_printf(cx->xright-3-strlen(ymin_str)*sdlx_char_width, cx->ybottom-3-sdlx_char_height, "%s", ymin_str);
    }
    if ((ymax_str != NULL) && (ymax_str[0] != '\0')) {
        sdlx_render_printf(cx->xleft+3, cx->ytop+3, "%s", ymax_str);
        sdlx_render_printf(cx->xright-3-strlen(ymax_str)*sdlx_char_width, cx->ytop+3, "%s", ymax_str);
    }

    // restore saved print state
    sdlx_print_restore(&print_state);
}

void sdlx_plot_points(void *cx_arg, sdlx_plot_point_t *pts, int num_pts)
{
    plot_cx_t   *cx = (plot_cx_t*)cx_arg;
    sdlx_point_t *points;
    int          i, n=0, point_size=5;

    points = malloc(num_pts * sizeof(sdlx_point_t));

    for (i = 0; i < num_pts; i++) {
        points[n].x = xval2x(cx, pts[i].xval);
        points[n].y = yval2y(cx, pts[i].yval);
        n++;
    }
    sdlx_render_points(points, n, COLOR_WHITE, point_size);

    free(points);
}

void sdlx_plot_bars(void *cx_arg, 
                   sdlx_plot_point_t *pts_avg, sdlx_plot_point_t *pts_min, sdlx_plot_point_t *pts_max,
                   int num_pts, double bar_wval)
{
    int        i, x, y, w, h;
    double     wval, hval, xval, yval;
    plot_cx_t *cx = (plot_cx_t*)cx_arg;

    static bool first = 1;

    //xxxpts_min[0].yval = pts_max[0].yval = pts_avg[0].yval = 1010;

    for (i = 0; i < num_pts; i++) {
        wval = bar_wval;
        hval = pts_max[i].yval - pts_min[i].yval;
        xval = pts_min[i].xval - wval/2;
        yval = pts_max[i].yval;

        x = xval2x(cx, xval);
        y = yval2y(cx, yval);
        w = wval * cx->xspan / cx->xval_span;
        h = hval * cx->yspan / cx->yval_span;

        if (h < 7) {
            y -= (7-h) / 2;
            h = 7;
        }

        if (first) printf("%d: %f %f %f\n", i, 
                         pts_min[i].yval, pts_avg[i].yval, pts_max[i].yval);
        if (first) printf("    %d %d %d %d - hval=%f yspan=%d yval_span=%f\n", 
               x, y, w, h,
               hval, cx->yspan, cx->yval_span);

        sdlx_render_fill_rect(x, y, w, h, COLOR_PURPLE);
    }

    sdlx_plot_points(cx, pts_avg, num_pts);

    first = 0;
}

void sdlx_plot_free(void *cx)
{
    // free cx
    free(cx);
}

// -----------------  MISC  --------------------------------------------- 

void sdlx_show_toast(char *message)
{ 
    INFO("%s\n", message);

#ifdef ANDROID
    #define DURATION_SHORT  0
    #define DURATION_LONG   1
    #define GRAVITY_CENTER  17
    SDL_ShowAndroidToast(message, DURATION_LONG, GRAVITY_CENTER, 0, 0);
#endif
}
