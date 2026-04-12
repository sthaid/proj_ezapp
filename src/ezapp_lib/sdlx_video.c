#include <std_hdrs.h>
#include <sys/queue.h>

#include <sdlx.h>
#include <logging.h>
#include <utils.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

// xxx review sdl3 port
// - routines now return succ
// - and use of floats instead of ints

//
// defines
// 

#define FONT_FILE_PATH  "FreeMonoBold.ttf"

#define ONE_MS 1000
#define TEN_MS 10000

#define MIN_FONT_PTSIZE  10
#define MAX_FONT_PTSIZE  400

#define USE_SET_RENDER_LOGICAL_PRESENTATION  //xxx

//
// typedefs
//

typedef struct {
    sdlx_loc_t loc;
    int       event_id;
} event_t;

typedef struct {
    TTF_Font *font;
    int       chw;
    int       chh;
} font_t;

//
// global variables
//

int sdlx_win_width;
int sdlx_win_height;
int sdlx_char_width_dflt;
int sdlx_char_height_dflt;

//
// variables
//

// used by other sdl*.c files
SDL_Window            * window;
double                  scale_render;
double                  scale_events;
double                  scale_render_save;

static SDL_Renderer   * renderer;

static font_t           font[MAX_FONT_PTSIZE];

//
// prototypes
//

static void set_render_draw_color(sdlx_color_t color);

//
// inline routines
//

// this routine is used, instead of type punning, 
// to prevent strict-aliasing warnings
static inline SDL_Color sdlx_color(sdlx_color_t color)
{
    SDL_Color val;
    memcpy(&val, &color, sizeof(color));
    return val;
}

// ----------------- INIT / EXIT --------------------------

static bool event_watcher(void* userdata, SDL_Event* event);

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

    // xxx   1080 x 2340    2.1666    = 19.5/9
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

    // add the event watcher
    SDL_AddEventWatch(event_watcher, NULL);

    // get real windows size and aspect ratio
    SDL_GetWindowSize(window, &real_win_width, &real_win_height);
    aspect_ratio = (double)real_win_height / real_win_width;
    INFO("real win_width x height = %d %d  aspect = %f\n", real_win_width, real_win_height, aspect_ratio);

    // xxx comment
    sdlx_win_width  = 1000;
    sdlx_win_height = rint(sdlx_win_width * aspect_ratio);
#ifndef USE_SET_RENDER_LOGICAL_PRESENTATION
    scale_render = (double)real_win_width / sdlx_win_width;
    scale_render_save = scale_render;
    scale_events = (double)real_win_width / sdlx_win_width;
#else
    // xxx remove scaling
    INFO("XXXXXXXXXXXXXXXXXXXX USE_SET_RENDER_LOGICAL_PRESENTATION\n");
    scale_render = 1;
    scale_render_save = scale_render;
    scale_events = (double)real_win_width / sdlx_win_width;
    //SDL_SetRenderLogicalPresentation(renderer, sdlx_win_width, sdlx_win_height, 
                                      //SDL_LOGICAL_PRESENTATION_STRETCH);
#endif
    INFO("logical sdlx_win_width x height = %d %d  scale = %f %f\n", 
         sdlx_win_width, sdlx_win_height, scale_render, scale_events);
    int w, h;
    SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
    INFO("GetCurrentRenderOutputSize = %d %d\n", w, h);

    // initialize True Type Font
    if (!TTF_Init()) {
        ERROR("TTF_Init failed\n");
        return -1;
    }

    // init default fontsize, where value of FONT_NORMAL is num chars across display;
    // and validate expected character size and columns
    sdlx_print_set_default(FONT_NORMAL, COLOR_WHITE);
    INFO("sdlx_print_set(%d) char_width=%d char_height=%d\n", FONT_NORMAL, sdlx_char_width_dflt, sdlx_char_height_dflt);
    if (sdlx_char_width_dflt != 50 || sdlx_char_height_dflt != 83) {
        // xxx this is off by 1
        ERROR("chw,chh, expected = 50,83  actual = %d,%d\n", sdlx_char_width_dflt, sdlx_char_height_dflt);
    }

    // enable alpha blending
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // this is needed so that the first actual display present works
    sdlx_display_init(COLOR_BLACK, true);
    sdlx_display_present();
    usleep(50000);  // xxx comment
    sdlx_event_t event;
    sdlx_get_event(0, &event);

    // return success
    INFO("success\n");
    return 0;
}

void sdlx_video_quit(void)
{
    int i;

    INFO("quitting\n");

    // close fonts
    for (i = 0; i < MAX_FONT_PTSIZE; i++) {
        if (font[i].font != NULL) {
            TTF_CloseFont(font[i].font);
        }
    }
    TTF_Quit();

    // destroy the renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // quit SDL video
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void sdlx_minimize_window(void)
{   
    SDL_MinimizeWindow(window);
}

// xxx will this be needed
static bool event_watcher(void* userdata, SDL_Event* event)
{
    static SDL_Renderer * save_renderer;

    switch (event->type) {
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        save_renderer = renderer;
        renderer = NULL;
        sleep(1);
        // pause app here
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
        // resume app here
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
    succ = SDL_SetHint(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1");
    if (!succ) {
        ERROR("failed to set SDL_HINT_ENABLE_SCREEN_KEYBOARD\n");
    }
#endif

// ----------------- DISPLAY INIT / PRESENT ---------------
//xxx cleanup needed here
sdlx_texture_t *landscape;
int landscape_w;
int landscape_h;

sdlx_texture_t *portrait;
int portrait_w;
int portrait_h;

bool portrait_flag;

void sdlx_display_init(sdlx_color_t color, bool portrait_orientation)
{
    portrait_flag = portrait_orientation;

    sdlx_reset_events();

#if 0
    // xxx orig
    set_render_draw_color(color);
    SDL_RenderClear(renderer);
#endif

    // xxx create and destroy texture
    if (portrait_flag) {
        if (portrait == NULL) {
            portrait_w = sdlx_win_width;
            portrait_h = sdlx_win_height;
            portrait = sdlx_create_texture(portrait_w, portrait_h);
        }

        if (portrait) {
            sdlx_set_render_target(portrait);
            sdlx_clear_texture(portrait, COLOR_PURPLE);
        }
    } else {
        if (landscape == NULL) {
            landscape_w = sdlx_win_height;
            landscape_h = sdlx_win_width;
            landscape = sdlx_create_texture(landscape_w, landscape_h);
            printf("XXXXXXXXXXXXX LAND W h %d %d\n", landscape_w, landscape_h);
        }

        if (landscape) {
            sdlx_set_render_target(landscape);
            sdlx_clear_texture(landscape, COLOR_PURPLE);
        }
    }
}

//void commit_register_control_events(void);

void sdlx_display_present(void)
{
    sdlx_set_render_target(NULL);

    if (landscape) {
        sdlx_render_texture_ex3(landscape,
                                0, 0, 975, 450,
                                90,           // clockwise rotation angle
                                450/2, 450/2);
    } else if (portrait) {
        sdlx_render_texture_ex1(portrait, 0, 0, 450, 975);
    }

    //commit_register_control_events();

    SDL_RenderPresent(renderer);
}

// -----------------  COLORS  -----------------------------

sdlx_color_t sdlx_create_color(int r, int g, int b, int a)
{
    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

sdlx_color_t sdlx_scale_color(sdlx_color_t color, double inten)
{
    unsigned int r = (color >> 0) & 0xff;
    unsigned int g = (color >> 8) & 0xff;
    unsigned int b = (color >> 16) & 0xff;
    unsigned int a = (color >> 24) & 0xff;

    if (inten < 0) inten = 0;
    if (inten > 1) inten = 1;

    r *= inten;
    g *= inten;
    b *= inten;

    return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

sdlx_color_t sdlx_set_color_alpha(sdlx_color_t color, int alpha)
{
    return (color & 0x00ffffff) | ((alpha & 0xff) << 24);
}

// ported from http://www.noah.org/wiki/Wavelength_to_RGB_in_Python
sdlx_color_t sdlx_wavelength_to_color(int wavelength_arg)
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

static void set_render_draw_color(sdlx_color_t color)
{
    unsigned int r = (color >> 0) & 0xff;
    unsigned int g = (color >> 8) & 0xff;
    unsigned int b = (color >> 16) & 0xff;
    unsigned int a = (color >> 24) & 0xff;

    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

// -----------------  RENDER TEXT  ------------------------

// - - - - - - - - - set print default - - - - - - - - - - - 

static struct {
    int          fontid;
    sdlx_color_t color;
} print_dflt;

static void font_create(int ptsize)
{
    font_t *f;
    TTF_Font *fnt;

    // validate ptsize
    if (ptsize < MIN_FONT_PTSIZE || ptsize >= MAX_FONT_PTSIZE) {
        ERROR("ptsize %d out of range\n", ptsize);
        return;
    }

    // if font already exists then return
    f = &font[ptsize];
    if (f->font != NULL) {
        return;
    }

    // open TTF font
    fnt = TTF_OpenFont(FONT_FILE_PATH, ptsize);
    if (fnt == NULL) {
        ERROR("TTF_OpenFont failed, ptsize=%d, %s\n", ptsize, SDL_GetError());
        return;
    }

    // initialize the font structure
    f->font = fnt;
    f->chh  = nearbyint(ptsize / scale_render);
    f->chw  = nearbyint(f->chh * 0.6);
}

void sdlx_print_set_default(int fontid, sdlx_color_t color)
{
    int ptsize;

    // save print default settings
    print_dflt.fontid = fontid;
    print_dflt.color  = color;

    // set global variables containing the char width,height of the default font
    ptsize = ((sdlx_win_width / fontid) * scale_render) / 0.6;
    sdlx_char_height_dflt = nearbyint(ptsize / scale_render);
    sdlx_char_width_dflt  = nearbyint(sdlx_char_height_dflt * 0.6);
}

int sdlx_char_width(int fontid)
{
    int ptsize, chh, chw;

    // return char width, based on fontid
    ptsize = ((sdlx_win_width / fontid) * scale_render) / 0.6;
    chh = nearbyint(ptsize / scale_render);
    chw  = nearbyint(chh * 0.6);
    return chw;
}

int sdlx_char_height(int fontid)
{
    int ptsize, chh;

    // return char height, based on fontid
    ptsize = ((sdlx_win_width / fontid) * scale_render) / 0.6;
    chh = nearbyint(ptsize / scale_render);
    return chh;
}

// - - - - - - - - - render text - - - - - - - - - - - - - 

#define MAX_HASH_LIST   293  // using a prime number is recomended
#define MAX_HASH_ENTRY  200

static TAILQ_HEAD(age_list, ht_entry_s)   age_list_head;
static TAILQ_HEAD(hash_list, ht_entry_s)  hash_list_head[MAX_HASH_LIST];

typedef struct ht_entry_s {
    char                   *key;
    SDL_Texture            *texture;
    int                     surface_w;
    int                     surface_h;
    TAILQ_ENTRY(ht_entry_s) age_list_entry;
    TAILQ_ENTRY(ht_entry_s) hash_list_entry;
} ht_entry_t;

static void render_text_texture(SDL_Texture *t, SDL_FRect *pos, int flags)
{
    #define SWAP_FLOAT(a,b) \
        do { float tmp = (a); (a) = (b); (b) = tmp; } while (0)

    bool flip = (flags & FLAG_FLIP);

    if (flags & FLAG_ROT_CTR_90) {
        SDL_RenderTextureRotated(renderer,
                                 t, 
                                 NULL,      // source, NULL means the entire texture
                                 pos,       // dest rectangle
                                 90,        // rotation angle
                                 NULL,      // point around which dest will be rotated
                                 flip ? SDL_FLIP_HORIZONTAL_AND_VERTICAL : SDL_FLIP_NONE);
        pos->x = pos->x + pos->w/2 - pos->h/2;
        pos->y = pos->y + pos->h/2 - pos->w/2;
        SWAP_FLOAT(pos->w, pos->h);
    } else if (flags & FLAG_ROT_CTR_180) {
        SDL_RenderTextureRotated(renderer,
                                 t, 
                                 NULL,      // source, NULL means the entire texture
                                 pos,       // dest rectangle
                                 180,       // rotation angle
                                 NULL,      // point around which dest will be rotated
                                 flip ? SDL_FLIP_HORIZONTAL_AND_VERTICAL : SDL_FLIP_NONE);
    } else if (flags & FLAG_ROT_CTR_270) {
        SDL_RenderTextureRotated(renderer,
                                 t, 
                                 NULL,      // source, NULL means the entire texture
                                 pos,       // dest rectangle
                                 270,       // rotation angle
                                 NULL,      // point around which dest will be rotated
                                 flip ? SDL_FLIP_HORIZONTAL_AND_VERTICAL : SDL_FLIP_NONE);
        pos->x = pos->x + pos->w/2 - pos->h/2;
        pos->y = pos->y + pos->h/2 - pos->w/2;
        SWAP_FLOAT(pos->w, pos->h);
    } else if (flip) {
        // flip with 0 rotation
        SDL_RenderTextureRotated(renderer,
                                 t, 
                                 NULL,      // source, NULL means the entire texture
                                 pos,       // dest rectangle
                                 0,         // rotation angle
                                 NULL,      // point around which dest will be rotated
                                 SDL_FLIP_HORIZONTAL_AND_VERTICAL);
    } else {
        SDL_RenderTexture(renderer, t, NULL, pos);
    }
}

static unsigned int calc_hash_idx(char *key)
{
    unsigned int hash_value = 0;

    for (int i = 0; key[i] != '\0'; i++) {
        hash_value = hash_value * 31 + key[i];
    }

    return (hash_value % MAX_HASH_LIST);
}

// xxx prints for number of hash hits and misses
static sdlx_loc_t *render_text(int x, int y, int fontid, sdlx_color_t color, int flags, int wrap, char *str)
{
    char         key[1000];
    ht_entry_t  *entry;
    SDL_FRect    pos;
    bool         found;
    int          hash_idx, ptsize;
    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;

    static sdlx_loc_t loc;
    static int        num_allocated = 0;

    // if font has not been created then do so
    ptsize = ((sdlx_win_width / fontid) * scale_render) / 0.6;
    if (font[ptsize].font == NULL) {
        font_create(ptsize);
        if (font[ptsize].font == NULL) {
            ERROR("failed to create font ptsize %d\n", ptsize);
            loc.x = loc.y = loc.w = loc.h = 0;
            return &loc;
        }
    }

    // if zero length string, which will fail to print, then
    // set the str arg to single space char
    if (str[0] == '\0') {
        str = " ";
    }

    // calculate hash key and idx
    snprintf(key, sizeof(key), "%x-%d-%s", color, ptsize, str);
    hash_idx = calc_hash_idx(key);

    // search the hash_list_head[hash_idx] for entry matching hash key
    found = false;
    TAILQ_FOREACH(entry, &hash_list_head[hash_idx], hash_list_entry) {
        if (strcmp(entry->key, key) == 0) {
            found = true;
            break;
        }
    }

    // matching entry found, render it, and return
    if (found) {
        pos.x = x*scale_render;
        pos.y = y*scale_render;
        pos.w = entry->surface_w;
        pos.h = entry->surface_h;
        if (flags & FLAG_X_CTR) pos.x -= pos.w / 2;
        if (flags & FLAG_Y_CTR) pos.y -= pos.h / 2;
        render_text_texture(entry->texture, &pos, flags);

        TAILQ_REMOVE(&age_list_head, entry, age_list_entry);
        TAILQ_INSERT_HEAD(&age_list_head, entry, age_list_entry);

        goto return_loc;
    }

    // hash table entry not found ...

    // xxx put prints in here to test this
    // if number of textures allocated is max then
    //   discard the least recently used texture
    // endif
    if (num_allocated >= MAX_HASH_ENTRY) {
        entry = TAILQ_LAST(&age_list_head, age_list);
        TAILQ_REMOVE(&age_list_head, entry, age_list_entry);
        TAILQ_REMOVE(&hash_list_head[hash_idx], entry, hash_list_entry);
        SDL_DestroyTexture(entry->texture);
        free(entry->key);
        free(entry);
        num_allocated--;
    }

    // create surface containing the rendered text
    if (wrap != WRAP_NONE) {
        surface = TTF_RenderText_Solid_Wrapped(
                        font[ptsize].font, str, 0,
                        sdlx_color(color),
                        wrap * scale_render);
    } else {
        surface = TTF_RenderText_Solid(
                        font[ptsize].font, str, 0,
                        sdlx_color(color));
    }
    if (surface == NULL) {
        ERROR("TTF_RenderTextSolid failed, %s\n", SDL_GetError());
        goto return_error;
    }

    // create texture from the surface
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        ERROR("SDL_CreateTextureFromSurface failed, %s\n", SDL_GetError());
        goto return_error;
    }

    // allocate and init new hash table entry
    entry = malloc(sizeof(ht_entry_t));
    entry->key = strdup(key);
    entry->texture = texture;
    entry->surface_w = surface->w;
    entry->surface_h = surface->h;
    TAILQ_INSERT_HEAD(&age_list_head, entry, age_list_entry);
    TAILQ_INSERT_HEAD(&hash_list_head[hash_idx], entry, hash_list_entry);
    num_allocated++;

    // render the texture
    pos.x = x*scale_render;
    pos.y = y*scale_render;
    pos.w = surface->w;
    pos.h = surface->h;
    if (flags & FLAG_X_CTR) pos.x -= pos.w / 2;
    if (flags & FLAG_Y_CTR) pos.y -= pos.h / 2;
    render_text_texture(texture, &pos, flags);

    // the surface is no longer needed, destroy the surface
    SDL_DestroySurface(surface);
    surface = NULL;

    // return the display location where the text was rendered;
return_loc:
    loc.x = pos.x / scale_render;
    loc.y = pos.y / scale_render;
    loc.w = pos.w / scale_render;
    loc.h = pos.h / scale_render;
    return &loc;

    // error return path
return_error:
    if (surface) {
        SDL_DestroySurface(surface);
        surface = NULL;
    }
    loc.x = 0;
    loc.y = 0;
    loc.w = 0;
    loc.h = 0;
    return &loc;
}

// - - - - - - - - -  print api routines - - - - - - - - - 

sdlx_loc_t *sdlx_render_printf(int x, int y, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return render_text(x, y, print_dflt.fontid, print_dflt.color, 0, WRAP_NONE, str);
}

sdlx_loc_t *sdlx_render_printf_ex1(int x, int y, int fontid, sdlx_color_t color, char * fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return render_text(x, y, fontid, color, 0, WRAP_NONE, str);
}

sdlx_loc_t *sdlx_render_printf_ex2(int x, int y, int fontid, sdlx_color_t color, int flags, int wrap, char *fmt, ...)
{
    char str[1000];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);

    return render_text(x, y, fontid, color, flags, wrap, str);
}

// each line may have embedded newline chars
void sdlx_render_multiline_text(int x, int y, int y_top, int y_bottom, int fontid, 
                                char **lines, unsigned int *colors, int num_lines)
{
    int   y2 = y;
    int   n = 0, k = 0, len;
    char *ptr;
    char  str[1000];
    int   chh = sdlx_char_height(fontid);

    while (n < num_lines) {
        // if y pos of line is below the bottom of the
        // display region then break
        if (y2 > y_bottom - chh) {
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

        // if y location of line (y2) is at or below the begining of the display
        // region then render the line
        if (y2 >= y_top) {
            sdlx_color_t color = (colors ? colors[n] : print_dflt.color);
            render_text(x, y2, fontid, color, 0, WRAP_NONE, str);
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

        // advance y for the next line
        y2 += chh;
    }
}

// -----------------  RENDER RECTANGLES, LINES, CIRCLES, POINTS  --------------------

void sdlx_render_rect(int x, int y, int w, int h, int line_width, sdlx_color_t color)
{
    SDL_FRect rect;
    int i;

    rect.x = x * scale_render;
    rect.y = y * scale_render;
    rect.w = w * scale_render;
    rect.h = h * scale_render;

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

void sdlx_render_fill_rect(int x, int y, int w, int h, sdlx_color_t color)
{
    SDL_FRect rect;

    rect.x = x * scale_render;
    rect.y = y * scale_render;
    rect.w = w * scale_render;
    rect.h = h * scale_render;

    set_render_draw_color(color);
    SDL_RenderFillRect(renderer, &rect);
}

void sdlx_render_line(int x1, int y1, int x2, int y2, sdlx_color_t color)
{
    sdlx_point_t points[2] = { {x1,y1}, {x2,y2} };
    sdlx_render_lines(points, 2, color);
}

void sdlx_render_lines(sdlx_point_t *points, int count, sdlx_color_t color)
{
    SDL_FPoint stack_scaled_points[100];
    SDL_FPoint *scaled_points;

    if (count <= 1) {
        return;
    }

    if (count > 100) {
        scaled_points = malloc(count * sizeof(SDL_FPoint));
    } else {
        scaled_points = stack_scaled_points;
    }

    for (int i = 0; i < count; i++) {
        scaled_points[i].x = points[i].x * scale_render;
        scaled_points[i].y = points[i].y * scale_render;
    }

    set_render_draw_color(color);
    SDL_RenderLines(renderer, scaled_points, count);

    if (count > 100) {
        free(scaled_points);
    }
}

void sdlx_render_circle(int x_ctr, int y_ctr, int radius, int line_width, sdlx_color_t color)
{
    int count = 0, i, angle, x, y;
    int x_center, y_center;
    SDL_FPoint points[370];

    static int sin_table[370];
    static int cos_table[370];
    static bool first_call = true;

    // apply scale factor
    x_center = nearbyint(x_ctr * scale_render);
    y_center = nearbyint(y_ctr * scale_render);
    radius   = nearbyint(radius * scale_render);

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

// Bresenham’s circle algorithm)
void sdlx_render_fill_circle(int x_ctr, int y_ctr, int radius, sdlx_color_t color)
{
    int x, y, error;

    x_ctr  = nearbyint(x_ctr * scale_render);
    y_ctr  = nearbyint(y_ctr * scale_render);
    radius = nearbyint(radius * scale_render);

    x     = radius;
    y     = 0;
    error = 1 - radius;

    set_render_draw_color(color);

    #define draw_horizontal_line(y, x1, x2) SDL_RenderLine(renderer, x1, y, x2, y)
    
    while(x >= y) {
        draw_horizontal_line(y+y_ctr, -x+x_ctr, x+x_ctr);
        draw_horizontal_line(x+y_ctr, -y+x_ctr, y+x_ctr);
        draw_horizontal_line(-y+y_ctr, -x+x_ctr, x+x_ctr);
        draw_horizontal_line(-x+y_ctr, -y+x_ctr, y+x_ctr);
        y++;
        if (error<0) {
            error += 2 * y + 1;
        } else {
            x--;
            error += 2 * (y - x) + 1;
        }
    }
}

void sdlx_render_point(int x, int y, sdlx_color_t color, int point_size)
{
    sdlx_point_t point = {x,y};

    sdlx_render_points(&point, 1, color, point_size);
}

void sdlx_render_points(sdlx_point_t *points, int count, sdlx_color_t color, int point_size)
{
    static struct point_extend_s {
        int max;
        struct point_extend_offset_s {
            int x;
            int y;
        } offset[280];
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

    static_assert(sizeof(point_extend)/sizeof(point_extend[0]) == MAX_POINT_SIZE+1, "static assert failed");

    if (count <= 0) {
        return;
    }
    if (point_size < 0) {
        point_size = 0;
    }
    if (point_size > MAX_POINT_SIZE) {
        point_size = MAX_POINT_SIZE;
    }

    #define MAX_SDL_POINTS 1000
    int i, j, x, y;
    SDL_FPoint sdlx_points[MAX_SDL_POINTS];
    int sdlx_points_count = 0;
    struct point_extend_s * pe = &point_extend[point_size];
    struct point_extend_offset_s * peo = pe->offset;

    set_render_draw_color(color);

    for (i = 0; i < count; i++) {
        for (j = 0; j < pe->max; j++) {
            x = nearbyint((points[i].x + peo[j].x) * scale_render);
            y = nearbyint((points[i].y + peo[j].y) * scale_render);
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

// - - - - - - create/destroy/query/clear/color_mod texture  - - - - - - - 

sdlx_texture_t *sdlx_create_texture(int width, int height)
{
    sdlx_texture_t *texture;

    // create texture
    texture = (sdlx_texture_t*)
              SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ABGR8888,
                                SDL_TEXTUREACCESS_TARGET,  // SDL_TEXTUREACCESS_STREAMING,
                                width, height);
    if (texture == NULL) {
        ERROR("failed to allocate texture\n");
        return NULL;
    }

    // init texture pixels to transparent black (alpha=0)
    sdlx_clear_texture(texture, 0);

    // return the created texture
    return texture;
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
    *width  = nearbyint(w_float);
    *height = nearbyint(h_float);
}

void sdlx_clear_texture(sdlx_texture_t *t_arg, sdlx_color_t color)
{
    SDL_Texture *initial_render_target = SDL_GetRenderTarget(renderer);
    SDL_Texture *t = (SDL_Texture *)t_arg;

    if (t != initial_render_target) {
        SDL_SetRenderTarget(renderer, t);
        set_render_draw_color(color);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, initial_render_target);
    } else {
        set_render_draw_color(color);
        SDL_RenderClear(renderer);
    }
}

void sdlx_color_mod_texture(sdlx_texture_t *t, float r, float g, float b)
{
    bool succ;

    succ = SDL_SetTextureColorModFloat((SDL_Texture*)t, r, g, b);
    if (!succ) {
        ERROR("failed color mod, %s\n", SDL_GetError());
    }
}

// - - - - - - set/get pixels from texture - - - - - - - - 

void sdlx_set_texture_pixels(sdlx_texture_t *texture, unsigned int *pixels)
{
    int w, h;

    sdlx_query_texture(texture, &w, &h);

    SDL_UpdateTexture((SDL_Texture*)texture, NULL, pixels, w * BYTES_PER_PIXEL);
}

unsigned int *sdlx_get_texture_pixels(sdlx_texture_t *t, int *w_arg, int *h_arg)
{
    SDL_Texture *initial_render_target;
    bool succ;
    SDL_Surface *surface;
    unsigned int *pixels, *p;
    int w, h, row;

    // save the initial_render_target, so it can be restored at then end 
    // of this routine 
    initial_render_target = SDL_GetRenderTarget(renderer);

    // set render target to the caller supplied texture
    succ = SDL_SetRenderTarget(renderer, (SDL_Texture*)t);
    if (!succ) {
        ERROR("SDL_SetRenderTarget failed, %s\n", SDL_GetError());
        return NULL;
    }

    // create surface containing the texture pixels
    surface = SDL_RenderReadPixels(renderer, NULL);
    if (surface == NULL) {
        ERROR("SDL_RenderReadPixels failed, %s\n", SDL_GetError());
        SDL_SetRenderTarget(renderer, initial_render_target);
        return NULL;
    }

    // get the width/height, and
    // allocate memory to hold the pixels
    SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
    p = pixels = malloc(w * h * BYTES_PER_PIXEL);

    // copy the pixels from the surface to the allocated buffer
    for (row = 0; row < h; row++) {
        memcpy(p, 
               surface->pixels + (row * surface->pitch), 
               w * BYTES_PER_PIXEL);
        p += w;
    }

    // destroy the surface
    SDL_DestroySurface(surface);

    // restore render target
    SDL_SetRenderTarget(renderer, initial_render_target);

    // return w, h, and pixels; caller must free pixels
    *w_arg = w;
    *h_arg = h;
    return pixels;
}

// - - - - - - render texture - - - - - 

void sdlx_render_texture(sdlx_texture_t *texture, int x, int y)
{
    int w, h;
    SDL_FRect dest;

    if (texture == NULL) {
        return;
    }

    sdlx_query_texture(texture, &w, &h);

    dest.x = x * scale_render;
    dest.y = y * scale_render;
    dest.w = w * scale_render;
    dest.h = h * scale_render;

    SDL_RenderTexture(renderer, (SDL_Texture*)texture, NULL, &dest);
}

void sdlx_render_texture_ex1(sdlx_texture_t *texture, int x, int y, int w, int h)
{
    SDL_FRect dest;

    if (texture == NULL) {
        return;
    }

    dest.x = x * scale_render;
    dest.y = y * scale_render;
    dest.w = w * scale_render;
    dest.h = h * scale_render;

    SDL_RenderTexture(renderer, (SDL_Texture*)texture, NULL, &dest);
}

void sdlx_render_texture_ex2(sdlx_texture_t *texture, int x, int y, int w, int h, double angle)
{
    SDL_FRect dest;

    if (texture == NULL) {
        return;
    }

    dest.x = x * scale_render;
    dest.y = y * scale_render;
    dest.w = w * scale_render;
    dest.h = h * scale_render;

    SDL_RenderTextureRotated(renderer,
                             (SDL_Texture*)texture, 
                             NULL,      // source, NULL means the entire texture
                             &dest,     // dest rectangle
                             angle,     // rotation angle
                             NULL,      // point around which dest will be rotated
                             SDL_FLIP_NONE);
}

void sdlx_render_texture_ex3(sdlx_texture_t *texture, int x, int y, int w, int h, double angle, int xctr, int yctr)
{
    SDL_FRect dest;
    SDL_FPoint ctr;

    if (texture == NULL) {
        return;
    }

    dest.x = x * scale_render;
    dest.y = y * scale_render;
    dest.w = w * scale_render;
    dest.h = h * scale_render;

    ctr.x = xctr * scale_render;
    ctr.y = yctr * scale_render;

    SDL_RenderTextureRotated(renderer,
                             (SDL_Texture*)texture, 
                             NULL,      // source, NULL means the entire texture
                             &dest,     // dest rectangle
                             angle,     // rotation angle
                             &ctr,      // point around which dest will be rotated
                             SDL_FLIP_NONE);
}

// - - - - - - set render target - - - - - - - - 

void sdlx_set_render_target(sdlx_texture_t *t)
{
    bool succ;

    succ = SDL_SetRenderTarget(renderer, (SDL_Texture*)t);
    if (!succ) {
        ERROR("SDL_SetRenderTarget failed, %s\n", SDL_GetError());
    }

    scale_render = (t ? 1 : scale_render_save);
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
