#include <std_hdrs.h>
    
#include <sdlx.h>
#include <logging.h>
#include <utils.h>

#include <SDL3/SDL.h>

//
// defines
//

#define ONE_MS       1000
#define EVID_KEYBD   9999 

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
extern SDL_Window * window; // xxx move to private.h or sdlx.h

static event_t      event_tbl[100];
static int          max_event;

static bool         evid_motion_registered;
static bool         evid_keybd_registered;

static int          event_quit_rcvd;

static bool         event_box_enable=0; //xxx temp enable    move to sdlx.h

extern int          logical_win_width, logical_win_height;  // xxx move to sdlx.h
extern int          logical_win_width_portrait;

//
// prototypes
//

static void process_sdlx_event(SDL_Event *ev, sdlx_event_t *event);

// xxx cleanup and sections needed

// -----------------  EVENTS  -----------------------------

void sdlx_reset_events(void)
{
    max_event = 0;
    evid_motion_registered = false;
    evid_keybd_registered = false;
}

void sdlx_event_box_ctrl(bool enable)
{
    event_box_enable = enable;
}

void sdlx_register_event(sdlx_loc_t *loc, int event_id)
{
    sdlx_loc_t loc2;

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

    // xxx comment  
    if (event_box_enable) {
        sdlx_render_rect(loc2.x, loc2.y, loc2.w, loc2.h, 3, COLOR_WHITE);
    }

    // rotate the loc2  xxx comment
    if (orientation == LANDSCAPE) { 
        int x,y,w,h;
        x = logical_win_width_portrait - loc2.y - loc2.h;
        y = loc2.x;
        w = loc2.h;
        h = loc2.w;

        loc2.x = x;
        loc2.y = y;
        loc2.w = w;
        loc2.h = h;
    }

    event_tbl[max_event].loc = loc2;
    event_tbl[max_event].event_id  = event_id; 
    max_event++;
}

void sdlx_register_control_events(int evid1, char *evstr1, 
                                  int evid2, char *evstr2, 
                                  int evid3, char *evstr3, 
                                  sdlx_color_t print_color, sdlx_color_t bg_color)
{
    sdlx_loc_t *loc;
    int i, x, y;
    char *evstr[3];
    int  evid[3];

    evstr[0] = evstr1;
    evstr[1] = evstr2;
    evstr[2] = evstr3;

    evid[0] = evid1;
    evid[1] = evid2;
    evid[2] = evid3;

    // xxx these args not needed
    print_color = COLOR_WHITE;
    bg_color = COLOR_TEAL;

    // fill entire control events area with bg_color
    if (orientation == PORTRAIT) {
        y = logical_win_height - CONTROL_AREA_SIZE;
        sdlx_render_fill_rect(0, y, logical_win_width, logical_win_height-y, bg_color);
    } else {
        sdlx_render_fill_rect(logical_win_width - CONTROL_AREA_SIZE, 0,  // x,y
                              CONTROL_AREA_SIZE, logical_win_height,      // w,h  xxx name
                              bg_color);
    }

    // display the 3 control events at the display bottom
    for (i = 0; i < 3; i++) {
        int chw = sdlx_char_width(FONT_NORMAL);

        if (evstr[i] == NULL) {
            continue;
        }

        if (orientation == PORTRAIT) {
            x = (logical_win_width/3/2) + i * (logical_win_width/3);
            if (i == 0 && x < strlen(evstr[0]) * chw / 2) {
                x = strlen(evstr[0]) * chw / 2;
            }
            if (i == 2 && x > logical_win_width - (strlen(evstr[2]) * chw / 2)) {
                x = logical_win_width - (strlen(evstr[2]) * chw / 2);
            }
            y = logical_win_height - (CONTROL_AREA_SIZE / 2);
            loc = sdlx_render_printf_ex2(x, y, FONT_NORMAL, print_color, FLAG_XY_CTR, WRAP_NONE, "%s", evstr[i]);
        } else {
            y = (logical_win_height/3/2) + i * (logical_win_height/3);
            if (i == 0 && y < strlen(evstr[0]) * chw / 2) {
                y = strlen(evstr[0]) * chw / 2;
            }
            if (i == 2 && y > logical_win_height - (strlen(evstr[2]) * chw / 2)) {
                y = logical_win_height - (strlen(evstr[2]) * chw / 2);
            }
            x = logical_win_width - (CONTROL_AREA_SIZE / 2);
            loc = sdlx_render_printf_ex2(
                        x, logical_win_height - y,
                        FONT_NORMAL, print_color, FLAG_XY_CTR|FLAG_ROT_CTR_270, WRAP_NONE, "%s", evstr[i]);
        }

        sdlx_register_event(loc, evid[i]);
    }
}

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

    // if SDL_QUIT has been received then
    // repeat returning EVID_QUIT
    if (event_quit_rcvd > 0) {
        event_quit_rcvd--;
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
        x = ev->button.x / scale_events_x;
        y = ev->button.y / scale_events_y;

        if (ev->button.down) {
            last_pressed_x = x;
            last_pressed_y = y;
        } else {
            int delta_x = x - last_pressed_x;
            int delta_y = y - last_pressed_y;

            INFO("button released xy = %d %d, delta xy = %d %d\n", x, y, delta_x, delta_y);

            for (i = max_event-1; i >= 0; i--) {
                if (AT_LOC(x, y, event_tbl[i].loc)) {
                    break;
                }
            }
            if (i >= 0 &&
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
            if (orientation == PORTRAIT) {
                event->u.motion.x = ev->motion.x / scale_events_x;
                event->u.motion.y = ev->motion.y / scale_events_y;
                event->u.motion.xrel = ev->motion.xrel / scale_events_x;
                event->u.motion.yrel = ev->motion.yrel / scale_events_y;
            } else {
                event->u.motion.y = logical_win_height - ev->motion.x / scale_events_x;
                event->u.motion.x = ev->motion.y / scale_events_y;
                event->u.motion.yrel = -ev->motion.xrel / scale_events_x;
                event->u.motion.xrel = ev->motion.yrel / scale_events_y;
            }
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
        // xxx maybe support pinch
        // not used
        break; }
    case SDL_EVENT_QUIT: { //xxx
        // the event_quit_rcvd variable is set so that 
        // this routine will repeat returning EVID_QUIT, so that
        // a running app will first process the EVID_QUIT, and 
        // finally main will process EVID_QUIT
        event_quit_rcvd = 10;
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

char *sdlx_get_input_str(char *prompt1, char *prompt2, bool numeric_keybd, sdlx_color_t bg_color)
{
    static char        input[100];
    int                max_input, row;
    sdlx_loc_t        *loc;
    sdlx_event_t       event;

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

    //  xxx comment
    while (true) {
        // clear backbuffer to bg_color
        sdlx_display_init(bg_color, PORTRAIT);

        // display prompt line(s)
        row = 0;
        if (prompt1 && prompt1[0] != '\0') {
            row += 1;
            sdlx_render_printf_ex2(0, ROW2Y(row), FONT_NORMAL, COLOR_WHITE, 0, WRAP_NONE, "%s", prompt1);
        }
        if (prompt2 && prompt2[0] != '\0') {
            row += 1;
            sdlx_render_printf_ex2(0, ROW2Y(row), FONT_NORMAL, COLOR_WHITE, 0, WRAP_NONE, "%s", prompt2);
        }

        // display input value string
        row += 2;
        loc = sdlx_render_printf_ex2(0, ROW2Y(row), FONT_NORMAL, COLOR_WHITE, 0, WRAP_NONE, "? %s", input);
        sdlx_render_printf_ex2(loc->x+loc->w, loc->y, FONT_NORMAL, COLOR_WHITE, 0, WRAP_NONE, "%s", "_");

        // register cancel event;
        // this event is needed to deal with the keybd being dismissed
        row += 3;
        loc = sdlx_render_printf_ex2(0, ROW2Y(row), FONT_NORMAL, COLOR_WHITE, 0, WRAP_NONE, "Cancel");
        sdlx_register_event(loc, EVID_QUIT);

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

    // return input string
    return input;
}
