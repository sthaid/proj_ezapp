#include <std_hdrs.h>
    
#include <sdlx.h>
#include <logging.h>
#include <utils.h>

#include <SDL3/SDL.h>

//
// defines
//

#define ONE_MS 1000

//
// typedefs
//

typedef struct {
    sdlx_loc_t loc;
    int       event_id;
} event_t;

//
// variables
//

// defined in sdlx_video.c
extern SDL_Window * window;
extern double       scale;

static event_t      event_tbl[100];
static int          max_event;

static bool         evid_swipe_right_registered;
static bool         evid_swipe_left_registered;
static bool         evid_motion_registered;
static bool         evid_keybd_registered;

//
// prototypes
//

static void process_sdlx_event(SDL_Event *ev, sdlx_event_t *event);

// xxx cleanup and sections needed

// -----------------  EVENTS  -----------------------------

void sdlx_reset_events(void)
{
    max_event = 0;
    evid_swipe_right_registered = false;
    evid_swipe_left_registered = false;
    evid_motion_registered = false;
    evid_keybd_registered = false;
}

void sdlx_register_event(sdlx_loc_t *loc, int event_id)
{
    sdlx_loc_t loc2;

    if (event_id == EVID_SWIPE_RIGHT) {
        evid_swipe_right_registered = true;
        return;
    }
    if (event_id == EVID_SWIPE_LEFT) {
        evid_swipe_left_registered = true;
        return;
    }
    if (event_id == EVID_MOTION) {
        evid_motion_registered = true;
        return;
    }
    if (event_id == EVID_KEYBD) {
        evid_keybd_registered = true;
        return;
    }

    if (loc == NULL || loc->w == 0 || loc->h == 0) {
        ERROR("invalid loc, event_id=%d\n", event_id);
        return;
    }

    // enforce minimum w,h
    loc2 = *loc;
    if (loc2.w < 150) {
        int delta = 150 - loc2.w;
        loc2.w += delta;
        loc2.x -= delta/2;
    }
    if (loc2.h < 150) {
        int delta = 150 - loc2.h;
        loc2.h += delta;
        loc2.y -= delta/2;
    }

    // xxx temporary
    sdlx_render_rect(loc2.x, loc2.y, loc2.w, loc2.h, 1, COLOR_WHITE);

    event_tbl[max_event].loc = loc2;
    event_tbl[max_event].event_id  = event_id; 
    max_event++;
}

void sdlx_register_control_events(char *evstr1, char *evstr2, char *evstr3, 
                                  int fg_color, int bg_color,
                                  int evid1, int evid2, int evid3)
{
    sdlx_loc_t *loc;
    int i, x, y;
    char *evstr[3];
    int  evid[3];
    sdlx_print_state_t print_state;

    evstr[0] = evstr1;
    evstr[1] = evstr2;
    evstr[2] = evstr3;

    evid[0] = evid1;
    evid[1] = evid2;
    evid[2] = evid3;

    sdlx_print_save(&print_state);

    sdlx_print_init(DEFAULT_FONT, fg_color, bg_color);

    // xxx trial, fill entire control events area with bg_color
    y = sdlx_win_height - 150;
    sdlx_render_fill_rect(0, y, sdlx_win_width, sdlx_win_height-y, bg_color);

    for (i = 0; i < 3; i++) {
        if (evstr[i] == NULL) {
            continue;
        }

        x = (sdlx_win_width/3/2) + i * (sdlx_win_width/3);
        if (i == 0 && x < strlen(evstr[0]) * sdlx_char_width / 2) {
            x = strlen(evstr[0]) * sdlx_char_width / 2;
        }
        if (i == 2 && x > sdlx_win_width - (strlen(evstr[2]) * sdlx_char_width / 2)) {
            x = sdlx_win_width - (strlen(evstr[2]) * sdlx_char_width / 2);
        }
        y = sdlx_win_height - 75;
        loc = sdlx_render_text_xyctr(x, y, evstr[i]);
        sdlx_register_event(loc, evid[i]);
    }

    sdlx_print_restore(&print_state);
}

static int sdlx_event_quit_rcvd;  //xxx cleanup

// arg timeout_us:
//   -1:     wait forever
//    0:     don't wait
//    usecs: timeout
void sdlx_get_event(long timeout_us, sdlx_event_t *event)
{
    SDL_Event ev;
    long waited = 0;
    bool got_event;

    // xxx move
    memset(event, 0, sizeof(*event));
    event->event_id = -1;

    // xxx comment
    if (sdlx_event_quit_rcvd > 0) {
        INFO("XXXXX quit pending, %d\n", sdlx_event_quit_rcvd);
        sdlx_event_quit_rcvd--;
        event->event_id = EVID_QUIT;
        return;
    }

try_again:
    //SDL_UpdateSensors(); // xxx is this needed?

    // get event
    got_event = SDL_PollEvent(&ev);

    // no event available, either return error or try again to get event
    if (!got_event) {
        if (timeout_us == 0) {
            // dont wait
            return;
        } else if (timeout_us < 0 || waited < timeout_us) {
            // either wait forever or time waited is less than timeout_us
            usleep(ONE_MS);
            waited += ONE_MS;
            goto try_again;
        } else {
            // time waited exceeds timeout_us
            return;
        }
    }

    // process the sdlx_event; this may or may not return an event
    process_sdlx_event(&ev, event);
    if (event->event_id == -1) {
        goto try_again;
    }

    // an event was returned from process_sdlx_event
    return;
}

static void process_sdlx_event(SDL_Event *ev, sdlx_event_t *event)
{
    #define AT_LOC(X,Y,loc) (((X) >= (loc).x)            && \
                             ((X) <  (loc).x + (loc).w)  && \
                             ((Y) >= (loc).y)            && \
                             ((Y) <  (loc).y + (loc).h))

    int i;

    switch (ev->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        static int last_pressed_x = -1;
        static int last_pressed_y = -1;
        int x, y;
#if 0
       INFO("MOUSE_BUTTON button=%s state=%s x=%d y=%d\n",
               (ev->button.button == SDL_BUTTON_LEFT   ? "LEFT" :
                ev->button.button == SDL_BUTTON_MIDDLE ? "MIDDLE" :
                ev->button.button == SDL_BUTTON_RIGHT  ? "RIGHT" : "???"),
               (ev->button.down ? "DOWN" : "UP"),
               ev->button.x,
               ev->button.y);
#endif
        x = ev->button.x / scale;
        y = ev->button.y / scale;

        if (ev->button.down) {
            last_pressed_x = x;
            last_pressed_y = y;
        } else {
            int delta_x = x - last_pressed_x;
            int delta_y = y - last_pressed_y;

            INFO("button released xy = %d %d, delta xy = %d %d\n", x, y, delta_x, delta_y);

            if (delta_x > 300 && evid_swipe_right_registered) {
                INFO("got EVID_SWIPE_RIGHT %d %d\n", delta_x, delta_y);
                event->event_id = EVID_SWIPE_RIGHT;
                break;
            } else if (delta_x < -300 && evid_swipe_left_registered) {
                INFO("got EVID_SWIPE_LEFT %d %d\n", delta_x, delta_y);
                event->event_id = EVID_SWIPE_LEFT;
                break;
            }

            for (i = 0; i < max_event; i++) {
                if (AT_LOC(x, y, event_tbl[i].loc)) {
                    break;
                }
            }
            if (i < max_event &&
                AT_LOC(last_pressed_x, last_pressed_y, event_tbl[i].loc))
            {
                event->event_id = event_tbl[i].event_id;
            }
        }
        break; }
    case SDL_EVENT_MOUSE_MOTION: {
        if ((ev->motion.state & SDL_BUTTON_LMASK) && evid_motion_registered) {
            //INFO("MOUSE_MOTION x=%f y=%f xrel=%f yrel=%f\n",
            //    ev->motion.x,
            //    ev->motion.y,
            //    ev->motion.xrel,
            //    ev->motion.yrel);

            event->event_id = EVID_MOTION;
            event->u.motion.x = ev->motion.x / scale;
            event->u.motion.y = ev->motion.y / scale;
            event->u.motion.xrel = ev->motion.xrel / scale;
            event->u.motion.yrel = ev->motion.yrel / scale;
        }
        break; }
    case SDL_EVENT_SENSOR_UPDATE: {
        SDL_SensorEvent *x = &ev->sensor;
        // xxx why is step counter not working
        // xxx cleanup
        if (x->which == 14 || x->which == 15) { // xxx clean up these prints
            // xxx long stepc = *(long*)x->data;
            unsigned long stepc;
            memcpy(&stepc, x->data, sizeof(stepc));
            INFO("SENSOR: which=%d data=%f %f %f %f %f %f stepc=%ld timestamp=%ld\n",
                 x->which,
                 x->data[0], x->data[1], x->data[2], x->data[3], x->data[4], x->data[5],
                 stepc, x->sensor_timestamp);
        }
        break; }
#if 0
    case SDL_EVENT_TEXT_INPUT: {
        SDL_TextInputEvent *x = &ev->text;
        INFO("SDL_EVENT_TEXT_INPUT: '%s'\n", x->text);
        break; }
    case SDL_EVENT_TEXT_EDITING: {
        SDL_TextEditingEvent *x = &ev->edit;
        INFO("SDL_EVENT_TEXT_EDITING: '%s' %d %d\n", x->text, x->start, x->length);
        break; }
#endif
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        SDL_KeyboardEvent *x = &ev->key;
        bool shift = (x->mod & SDL_KMOD_SHIFT) != 0;
        SDL_Keycode keycode;

        if (!evid_keybd_registered || x->down) {
            break;
        }

        keycode = SDL_GetKeyFromScancode(x->scancode, x->mod, false);
        INFO("GOT keycode 0x%x  shift=%d\n", keycode, shift);
        event->event_id = EVID_KEYBD;
        event->u.keybd.ch = keycode;
        break; }
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_MOTION: {
        // not used
        break; }
    case SDL_EVENT_QUIT: {
        sdlx_event_quit_rcvd = 10;
        event->event_id = EVID_QUIT;
        break; }

    // xxx  these dont seem to be invoked
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
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
        // Resume your game loop and tasks
        INFO("App is now in the foreground\n");
        break;

    default: {
        //INFO("event_type %d - not supported\n", ev->type);
        break; }
    }
}

char *sdlx_get_input_str(char *prompt1, char *prompt2, bool numeric_keybd, int bg_color)
{
    static char        input[100];
    int                max_input, row;
    sdlx_loc_t        *loc;
    sdlx_event_t       event;
    sdlx_print_state_t print_state;

    // xxx comments

    // init
    memset(input, 0, sizeof(input));
    max_input = 0;

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(
            props, 
            SDL_PROP_TEXTINPUT_TYPE_NUMBER, 
            numeric_keybd ?  SDL_TEXTINPUT_TYPE_NUMBER : SDL_TEXTINPUT_TYPE_TEXT);
    SDL_StartTextInputWithProperties(window, props);

    // set print size and color
    sdlx_print_save(&print_state);
    sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, bg_color);

    //  xxx comment
    while (true) {
        // clear backbuffer to bg_color
        sdlx_display_init(bg_color);

        // display prompt line(s)
        row = 0;
        if (prompt1 && prompt1[0] != '\0') {
            row += 1;
            sdlx_render_printf(0, ROW2Y(row), "%s", prompt1);
        }
        if (prompt2 && prompt2[0] != '\0') {
            row += 1;
            sdlx_render_printf(0, ROW2Y(row), "%s", prompt2);
        }

        // display input value string
        row += 2;
        loc = sdlx_render_printf(0, ROW2Y(row), "? %s", input);
        sdlx_render_printf(loc->x+loc->w, loc->y, "%s", "_");

        // register cancel event;
        // this event is needed to deal with the keybd being dismissed
        row += 3;
        sdlx_print_init(DEFAULT_FONT, COLOR_LIGHT_BLUE, bg_color);
        loc = sdlx_render_printf(0, ROW2Y(row), "Cancel");
        sdlx_register_event(loc, EVID_QUIT);
        sdlx_print_init(DEFAULT_FONT, COLOR_WHITE, bg_color);

        // register for keyboard events
        sdlx_register_event(NULL, EVID_KEYBD);

        // present display
        sdlx_display_present();

        // wait for event
        sdlx_get_event(-1, &event);

        // process event
        if (event.event_id == EVID_KEYBD) {
            int ch = event.u.keybd.ch;

            if (ch >= 0x20 && ch < 0x7f) {
                if (max_input < sizeof(input)) {
                    // sometimes the './-' key on numeric keybd 
                    // does not work, so allow ',' to be used instead
                    if (numeric_keybd && ch == ',') ch = '.';
                    input[max_input++] = ch;
                }
            } else if (ch == '\b') {
                if (max_input > 0) {
                    input[--max_input] = '\0';
                }
            } if (ch == '\r') {
                break;
            }
        }

        if (event.event_id == EVID_QUIT) {
            input[0] = '\0';
            break;
        }
    }

    // cleanup
    SDL_StopTextInput(window);
    SDL_DestroyProperties(props);
    sdlx_print_restore(&print_state);

    // return input string
    return input;
}
