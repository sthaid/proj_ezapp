#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sdlx.h>
#include <utils.h>

// xxx
// - code cleanup and comments

// display locations
#define DISPLAY_Y_TOP       100
#define BUTTONS_X_LEFT      100
#define BUTTONS_Y_TOP       (DISPLAY_Y_TOP + 350)
#define BUTTONS_SPACING     200

// buttons
#define MAX_BUTTON_ROW 7
#define MAX_BUTTON_COL 5

#define BUTTON_HIGHLIGHT_DURATION_MS 100

#define EVID_64BIT   ('6' | '4' << 8)
#define EVID_32BIT   ('3' | '2' << 8)
#define EVID_DSP_HEX ('H' | 'E' << 8 | 'X' << 16)
#define EVID_DSP_DEC ('D' | 'E' << 8 | 'C' << 16)
#define EVID_CLR     ('C' | 'L' << 8 | 'R' << 16)
#define EVID_CE      ('C' | 'E' << 8)
#define EVID_SHL     ('<' | '<' << 8)
#define EVID_SHR     ('>' | '>' << 8)
#define BLANK 0

int button[MAX_BUTTON_ROW][MAX_BUTTON_COL] = {
    { EVID_32BIT, EVID_DSP_HEX,  BLANK,   EVID_CE,    EVID_CLR,   },
    {    'C',         'D',        'E',       'F',       'G',      },
    {    '8',         '9',        'A',       'B',       'M',      },
    {    '4',         '5',        '6',       '7',       'K',      },
    {    '0',         '1',        '2',       '3',       '~',      },
    {    '&',         '|',        '^',    EVID_SHL,   EVID_SHR,   },
    {    '+',         '-',        '*',       '/',      '=',       },
        };

// colors
#define BACKGROUND_COLOR         COLOR_LIGHT_GRAY
#define BUTTON_COLOR_NORMAL      COLOR_WHITE
#define BUTTON_COLOR_HIGHLIGHT   COLOR_GRAY
#define BUTTON_COLOR_TEXT        COLOR_BLACK
#define DISPLAY_NUMBER_COLOR     COLOR_BLACK

// values for display_state
#define RESULT     0
#define NO_VALUE   1
#define INPUTTING  2

// value for no operation
#define OP_NONE 0

// global variables
char *progname;
char *data_dir;

// prototypes
void update_numeric_display(unsigned long value, int bits, int display, bool error);
void draw_button(int row, int col, bool highlight);
void evid_to_button_row_and_col(int evid, int *button_row, int *button_col);
unsigned long process_op(int op, unsigned long operand1, unsigned long operand2, int bits, bool *error);
void cleanup(void);

// -----------------  MAIN  ----------------------------------

int main(int argc, char **argv)
{
    int           rc;
    sdlx_event_t  event;
    int           highlight_button_row = -1;
    int           highlight_button_col = -1;

    int           bits          = EVID_32BIT; 
    int           display_fmt   = EVID_DSP_HEX;
    int           display_state = RESULT;
    int           op            = OP_NONE;
    unsigned long display_value = 0;
    unsigned long operand_value = 0;
    bool          error         = false;

    // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init sdl video subsystem
    rc = sdlx_init(SUBSYS_VIDEO);
    if (rc != 0) {
        printf("ERROR %s: sdlx_init failed\n", progname);
        return 1;
    }

    // runtime loop
    while (true) {
        // -------------------------
        // display update processing
        // -------------------------

        // init the backbuffer
        sdlx_display_init(BACKGROUND_COLOR);

        // register control event to end program
        sdlx_register_control_events(NULL, NULL, "X", BUTTON_COLOR_TEXT, BACKGROUND_COLOR, 0, 0, EVID_QUIT);

        // display calculator buttons, which will also register the button events
        for (int row = 0; row < MAX_BUTTON_ROW; row++) {
            for (int col = 0; col < MAX_BUTTON_COL; col++) {
                bool highlight = (row == highlight_button_row && col == highlight_button_col);
                draw_button(row, col, highlight);
            }
        }
        
        // update numeric display
        update_numeric_display(display_value, bits, display_fmt, error);

        // present the display
        sdlx_display_present();

        // --------------------
        // event processing
        // --------------------

        // wait for event;
        // if a button is highlighted then use a short timeout to clear the button highlight;
        // otherwise use infinite timeout
        sdlx_get_event(highlight_button_row != -1 ? BUTTON_HIGHLIGHT_DURATION_MS * 1000 : -1, &event);

        // if sdlx_get_event timed out then clear the button highlight, and continue
        if (event.event_id == -1) {
            highlight_button_row = -1;
            highlight_button_col = -1;
            continue;
        }

        // if quit event then end program
        if (event.event_id == EVID_QUIT) {
            break;
        }

        // calculator button has been pressed ...

        // if in error state then ignore all button presses, except CLR
        if (error && event.event_id != EVID_CLR) {
            continue;
        }

        // set highlight_button_row/col so the button will be briefly hightlighted
        evid_to_button_row_and_col(event.event_id, &highlight_button_row, &highlight_button_col);

        // process the calculator button
        switch (event.event_id) {
        // calculator control buttons: clear, clear entry, bits select, display format select
        case EVID_CLR:
            display_state = RESULT;
            op            = OP_NONE;
            display_value = 0;
            operand_value = 0;
            error         = false;
            break;
        case EVID_CE:
            if (display_state == INPUTTING) {
                display_value = 0;
            }
            break;
        case EVID_64BIT: case EVID_32BIT:
            bits = (bits == EVID_64BIT ? EVID_32BIT : EVID_64BIT);
            button[0][0] = bits;
            if (bits == EVID_32BIT) {
                display_value = (unsigned int)display_value;
                operand_value = (unsigned int)operand_value;
            }
            break;
        case EVID_DSP_HEX: case EVID_DSP_DEC:
            display_fmt = (display_fmt == EVID_DSP_HEX ? EVID_DSP_DEC : EVID_DSP_HEX);
            button[0][1] = display_fmt;
            break;

        // number input events
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        case 'A': case 'B': case 'C':  case 'D':  case 'E':  case 'F': {
            int n = (event.event_id <= '9' ? event.event_id - '0' : event.event_id - 'A' + 0xa);

            if (display_state == RESULT || display_state == NO_VALUE) {
                display_value = 0;
                display_state = INPUTTING;
            }

            if (bits == EVID_64BIT) {
                if ((display_value & (0xful << 60)) == 0) {
                    display_value = (display_value << 4) | n;
                }
            } else {
                if ((display_value & (0xful << 28)) == 0) {
                    display_value = (display_value << 4) | n;
                }
            }
            break; }

        // number constant input
        case 'G':
            display_value = 0x40000000ul;
            display_state = RESULT;
            break;
        case 'M':
            display_value = 0x100000ul;
            display_state = RESULT;
            break;
        case 'K':
            display_value = 0x400;
            display_state = RESULT;
            break;
            
        // unary ops
        case '~':
            display_value = ~display_value;
            if (bits == EVID_32BIT) {
                display_value = (unsigned int)display_value;
            }
            display_state = RESULT;
            break;

        // binary ops
        case '&': case '|': case '^': case EVID_SHL: case EVID_SHR:
        case '+': case '-': case '*': case '/':
            if (display_state == NO_VALUE) {
                break;
            }
            if (op != OP_NONE) {
                display_value = process_op(op, operand_value, display_value, bits, &error);
            }
            operand_value = display_value;
            op = event.event_id;
            display_state = NO_VALUE;
            break;

        // equals op
        case '=':
            if (op != OP_NONE && display_state != NO_VALUE) {
                display_value = process_op(op, operand_value, display_value, bits, &error);
            }
            operand_value = 0;
            op = OP_NONE;
            display_state = RESULT;
        }
    }

    // cleanup and end program
    cleanup();
    sdlx_quit(SUBSYS_VIDEO);
    printf("INFO %s: terminating\n", progname);
    return 0;
}

// -----------------  SUPPORT ROUTINES  ----------------------------

void update_numeric_display(unsigned long value, int bits, int display_fmt, bool error)
{
    int max_chars;
    char fmt[20];

    if (display_fmt == EVID_DSP_HEX) {
        max_chars = (bits == EVID_32BIT ? 8 : 16);
    } else {
        max_chars = (bits == EVID_32BIT ? 10 : 20);
    }

    sdlx_print_init(max_chars, DISPLAY_NUMBER_COLOR, BACKGROUND_COLOR);

    if (error) {
        sprintf(fmt, "%%%ds", max_chars);
        sdlx_render_printf(0, DISPLAY_Y_TOP, fmt, "error");
    } else if (display_fmt == EVID_DSP_HEX) {
        sprintf(fmt, "%%%dlX", max_chars);
        sdlx_render_printf(0, DISPLAY_Y_TOP, fmt, value);
    } else {
        sprintf(fmt, "%%%dlu", max_chars);
        sdlx_render_printf(0, DISPLAY_Y_TOP, fmt, value);
    }
}

sdlx_texture_t *button_texture;
sdlx_texture_t *highlighted_button_texture;

void draw_button(int row, int col, bool highlight)
{
    sdlx_loc_t loc;
    int x, y, radius;
    char str[8];

    static int texture_w, texture_h;

    if (button_texture == NULL) {
        radius = BUTTONS_SPACING * 45 / 100;
        button_texture = sdlx_create_filled_circle_texture(radius, BUTTON_COLOR_NORMAL);
        highlighted_button_texture = sdlx_create_filled_circle_texture(radius,BUTTON_COLOR_HIGHLIGHT);
        sdlx_query_texture(button_texture, &texture_w, &texture_h);
    }

    if (button[row][col] == 0) {
        return;
    }    

    x = BUTTONS_X_LEFT + col * BUTTONS_SPACING;
    y = BUTTONS_Y_TOP + row * BUTTONS_SPACING;

    sdlx_render_texture(x-texture_w/2, y-texture_h/2, texture_w, texture_h,
        !highlight ? button_texture : highlighted_button_texture);

    sdlx_print_init(20, BUTTON_COLOR_TEXT, !highlight ? BUTTON_COLOR_NORMAL : BUTTON_COLOR_HIGHLIGHT);
    memset(str, 0, sizeof(str));
    memcpy(str, &button[row][col], 4);
    sdlx_render_printf_xyctr(x, y, "%s", str);

    loc.x = x - texture_w/2;
    loc.y = y - texture_h/2;
    loc.w = texture_w;
    loc.h = texture_h;
    sdlx_register_event(&loc, button[row][col]);
}

void evid_to_button_row_and_col(int evid, int *button_row, int *button_col)
{
    int row, col;

    for (row = 0; row < MAX_BUTTON_ROW; row++) {
        for (col = 0; col < MAX_BUTTON_COL; col++) {
            if (button[row][col] == evid) {
                *button_row = row;
                *button_col = col;
                return;
            }
        }
    }

    *button_row = -1;
    *button_col = -1;
}

unsigned long process_op(int op, unsigned long operand1, unsigned long operand2, int bits, bool *error)
{
    unsigned long result;

    if (op == '/' && operand2 == 0) {
        *error = true;
        return 0;
    }

    switch (op) {
    case '&':      result = operand1 & operand2; break;
    case '|':      result = operand1 | operand2; break;
    case '^':      result = operand1 ^ operand2; break;
    case EVID_SHL: result = operand1 << operand2; break;
    case EVID_SHR: result = operand1 >> operand2; break;
    case '+':      result = operand1 + operand2; break;
    case '-':      result = operand1 - operand2; break;
    case '*':      result = operand1 * operand2; break;
    case '/':      result = operand1 / operand2; break;
    default:
        printf("ERROR %s: BUG process_op, op=%x\n", progname, op);
        return 0;
    }

    if (bits == EVID_32BIT) {
        result = (unsigned int)result;
    }

    return result;
}

void cleanup(void)
{
    sdlx_destroy_texture(button_texture);
    sdlx_destroy_texture(highlighted_button_texture);
}
