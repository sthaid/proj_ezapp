#include <apps/Reversi/common.h>

//
// notes:
// - black goes first
//

//
// defines
//

#define GAME_STATE_READY   1
#define GAME_STATE_ACTIVE  2
#define GAME_STATE_OVER    3

#define EVID_GAME_START          201
#define EVID_GAME_RESET          202
#define EVID_MOVE_PASS           203
#define EVID_PLAYER_BLACK_SELECT 205
#define EVID_PLAYER_WHITE_SELECT 206

//
// typedefs
//

//
// variables
//

//
// prototypes
//

static void game_init(board_t *b);
static bool is_game_over(board_t *b);
static bool humans_turn(board_t *b);
static char *player_name(int p);
static bool event_id_is_game_move(int evid);

static void update_display_init(void);
static void update_display_unload(void);
static void update_display_and_register_events(board_t *b, int game_state, char *eval_str);

// -----------------  MAIN  ------------------------------------

int main(int argc, char **argv)
{
    int     game_state;
    board_t board;
    char    eval_str[100];

   // save args
    if (argc != 2) {
        printf("ERROR: data_dir arg expected\n");
        return 1;
    }
    progname = argv[0];
    data_dir = argv[1];
    printf("INFO %s: starting, data_dir=%s\n", progname, data_dir);

    // init variables
    game_state = GAME_STATE_READY;
    game_init(&board);
    eval_str[0] = '\0';

    // seed random number generator
    // xxx commented out to compare picoc and linux versions
    //srandom(util_microsec_timer());

    // init sdl
    sdlx_init(SUBSYS_VIDEO);

    // xxx
    update_display_init();

    // loop until end program
    while (true) {
        // init display to black, and init print size/color
        sdlx_display_init(COLOR_BLACK);
        sdlx_print_init(20, COLOR_WHITE, COLOR_BLACK);

        // update the display and register events
        update_display_and_register_events(&board, game_state, eval_str);

        // present display
        sdlx_display_present();

        // if it is human's turn then 
        //   wait forever for an event
        // else if computer's turn then 
        //   poll for an event (no wait)
        // else 
        //   poll for an event (100 ms wait)
        // endif
        long timeout;  // microsecs
        sdlx_event_t event;
        if (game_state == GAME_STATE_ACTIVE && humans_turn(&board)) {
            timeout = -1;
        } else if (game_state == GAME_STATE_ACTIVE && !humans_turn(&board)) {
            timeout = 0;
        } else {
            timeout = 100000;  // 100 ms
        }
        sdlx_get_event(timeout, &event);

        // xxx motion or other unexpected events can be misinterpreted
        if (event.event_id == 9990 || event.event_id == 9991 || event.event_id == 9992) {
            continue;
        }

        // if no event and it is computer turn to move then 
        // get computer's move and apply it
        if (event.event_id == -1) {
            if (game_state == GAME_STATE_ACTIVE && !humans_turn(&board)) {
                int level = (board.whose_turn == BLACK ? board.player_black : board.player_white);
                int move  = cpu_get_move(level, &board, eval_str);
                apply_move(&board, move);
                if (is_game_over(&board)) {
                    game_state = GAME_STATE_OVER;
                }
            }
            continue;
        }

        // process the event
        // xxx need a way to interrupt a long running cpu_get_move
        if (event.event_id == EVID_QUIT) {
            break;
        } else if (event.event_id == EVID_GAME_RESET) {
            game_state = GAME_STATE_READY;
            game_init(&board);
            eval_str[0] = '\0';
        } else if (event.event_id == EVID_PLAYER_BLACK_SELECT) {
            board.player_black++;
            if (board.player_black > CPU(6)) board.player_black = HUMAN;  // xxx 3 on Android
            util_set_numeric_param(data_dir, "player_black", board.player_black);
        } else if (event.event_id == EVID_PLAYER_WHITE_SELECT) {
            board.player_white++;
            if (board.player_white > CPU(6)) board.player_white = HUMAN;  // xxx 3 on Android
            util_set_numeric_param(data_dir, "player_white", board.player_white);
        } else if (event.event_id == EVID_GAME_START) {
            game_state = GAME_STATE_ACTIVE;
        } else if (game_state == GAME_STATE_ACTIVE && 
                   humans_turn(&board) && 
                   event_id_is_game_move(event.event_id))
        {
            int move = (event.event_id == EVID_MOVE_PASS ? MOVE_PASS : event.event_id);
            apply_move(&board, move);
            if (is_game_over(&board)) {
                game_state = GAME_STATE_OVER;
            }
        } else {
            printf("ERROR %s: unexpected event_id %d\n", progname, event.event_id);
        }
    }

    // xxx
    update_display_unload();

    // exit sdl
    sdlx_quit(SUBSYS_VIDEO);

    // return success
    printf("INFO %s: terminating\n", progname);
    return 0;
}

static void game_init(board_t *b)
{
    memset(b, 0, sizeof(board_t));

    b->pos[4][4]      = WHITE;
    b->pos[4][5]      = BLACK;
    b->pos[5][4]      = BLACK;
    b->pos[5][5]      = WHITE;
    b->black_cnt      = 2;
    b->white_cnt      = 2;
    b->whose_turn     = BLACK;
    b->player_black   = util_get_numeric_param(data_dir, "player_black", HUMAN);
    b->player_white   = util_get_numeric_param(data_dir, "player_white", CPU(2));
}

static bool is_game_over(board_t *b)
{
    bool apm;

    // quick check, if all squares are filled the game is over
    if (b->black_cnt + b->white_cnt == 64) {
        return true;
    }

    // if the current player has possible moves then the game is not over
    if (any_possible_moves(b)) {
        return false;
    }

    // if the other player does not have possible moves the game is over
    b->whose_turn = OTHER_COLOR(b->whose_turn);
    apm = any_possible_moves(b);
    b->whose_turn = OTHER_COLOR(b->whose_turn);
    return !apm;
}

static bool humans_turn(board_t *b)
{
    return (b->whose_turn == BLACK && b->player_black == HUMAN) ||
           (b->whose_turn == WHITE && b->player_white == HUMAN);
}

static char *player_name(int p)
{
    static char str[20];

    if (p == HUMAN) {
        sprintf(str, "HUMAN");
    } else {
        sprintf(str, "CPU%d", p);
    }
    return str;
}

static bool event_id_is_game_move(int evid)
{
    int r, c;

    if (evid == EVID_MOVE_PASS) {
        return true;
    }

    move_to_rc(evid, &r, &c);
    return (r >= 1 && r <= 8 && c >= 1 && c <= 8);
}

// -----------------  UPDATE DISPLAY ------------------------------

//
// variables
//

static sdlx_loc_t rc_to_loc[10][10];

static int            piece_circle_radius;
static sdlx_texture_t *piece_black_circle;
static sdlx_texture_t *piece_white_circle;

static int            prompt_circle_radius;
static sdlx_texture_t *prompt_black_circle;
static sdlx_texture_t *prompt_white_circle;

static int            info_circle_radius;
static sdlx_texture_t *info_black_circle;
static sdlx_texture_t *info_white_circle;

// - - - - - - - - - - - - - - - - - 

static void update_display_init(void)
{
    int r,c;

    for (r = 1; r <= 8; r++) {
        for (c = 1; c <= 8; c++) {
            rc_to_loc[r][c].x = 1 + 125 * (c - 1);
            rc_to_loc[r][c].y = 1 + 125 * (r - 1);
            rc_to_loc[r][c].w = 124;
            rc_to_loc[r][c].h = 124;
        }
    }

    int sq_wh = 123;  // xxx is this correct

    piece_circle_radius  = nearbyint(0.4*sq_wh);   // xxx need rint,  use nearybint
    piece_black_circle   = sdlx_create_filled_circle_texture(piece_circle_radius, COLOR_BLACK);
    piece_white_circle   = sdlx_create_filled_circle_texture(piece_circle_radius, COLOR_WHITE);

    prompt_circle_radius = nearbyint(0.08*sq_wh);
    prompt_black_circle  = sdlx_create_filled_circle_texture(prompt_circle_radius, COLOR_BLACK);
    prompt_white_circle  = sdlx_create_filled_circle_texture(prompt_circle_radius, COLOR_WHITE);

    info_circle_radius = nearbyint(0.3*sq_wh);
    info_black_circle  = sdlx_create_filled_circle_texture(info_circle_radius, COLOR_BLACK);
    info_white_circle  = sdlx_create_filled_circle_texture(info_circle_radius, COLOR_WHITE);
}

static void update_display_unload(void)
{
    sdlx_destroy_texture(piece_black_circle);
    sdlx_destroy_texture(piece_white_circle);

    sdlx_destroy_texture(prompt_black_circle);
    sdlx_destroy_texture(prompt_white_circle);

    sdlx_destroy_texture(info_black_circle);
    sdlx_destroy_texture(info_white_circle);
}

static void update_display_and_register_events(board_t *b, int game_state, char *eval_str)
{
    possible_moves_t pm;
    sdlx_loc_t *ploc;
    int w, h;

    // display game state lines (1 or 2 lines), directly below board
    char *str = "";
    if (game_state == GAME_STATE_READY)  str = "READY";  
    if (game_state == GAME_STATE_ACTIVE) str = "IN PROGRESS";  
    if (game_state == GAME_STATE_OVER)   str = "GAME OVER";  
    sdlx_render_text_xyctr(sdlx_win_width/2, 1000+0.5*sdlx_char_height, str);

    // xxx
    if (game_state == GAME_STATE_OVER) {
        if (b->black_cnt > b->white_cnt) {
            sdlx_render_printf_xyctr(sdlx_win_width/2, 1000+1.5*sdlx_char_height,
                "BLACK WINS BY %d", b->black_cnt - b->white_cnt);
        } else if (b->white_cnt > b->black_cnt) {
            sdlx_render_printf_xyctr(sdlx_win_width/2, 1000+1.5*sdlx_char_height,
                "WHITE WINS BY %d", b->white_cnt - b->black_cnt);
        } else {
            sdlx_render_printf_xyctr(sdlx_win_width/2, 1000+1.5*sdlx_char_height,
                "TIE");
        }
    }       

    // xxx
    if (game_state == GAME_STATE_ACTIVE || game_state == GAME_STATE_OVER) {
        sdlx_render_text(0, sdlx_win_height - 3 * sdlx_char_height, eval_str);  //xxx need char height of font 10
    }

    // display player info, and register for events to change the players
    for (int i = 0; i < 2; i++) {
        int player                 = (i == 0 ? b->player_black : b->player_white);
        int evid                   = (i == 0 ? EVID_PLAYER_BLACK_SELECT : EVID_PLAYER_WHITE_SELECT);
        sdlx_texture_t *info_circle = (i == 0 ? info_black_circle : info_white_circle);
        int x_origin               = (i == 0 ? 0 : 500);
        int y_origin               = 1300;
        int piece_cnt              = (i == 0 ? b->black_cnt : b->white_cnt);
        bool is_turn               = (i == 0 ? b->whose_turn == BLACK : b->whose_turn == WHITE);

        sdlx_render_fill_rect(x_origin, y_origin, info_circle_radius*2, info_circle_radius*2, COLOR_GREEN);
        sdlx_query_texture(info_circle, &w, &h);
        sdlx_render_texture(x_origin, y_origin, w, h, info_circle);

        if (game_state == GAME_STATE_ACTIVE) {
            sdlx_render_text(x_origin, y_origin+100, player_name(player));
        } else {
            sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
            ploc = sdlx_render_text(x_origin, y_origin+100, player_name(player));
            sdlx_register_event(ploc, evid);
        }

        if (game_state == GAME_STATE_ACTIVE || game_state == GAME_STATE_OVER) {
            if (game_state == GAME_STATE_OVER) is_turn = false;
            sdlx_render_printf(x_origin+100, y_origin, "%c %d", is_turn ? '*' : ' ', piece_cnt);
        }
    }

    // register control event to end program
    sdlx_register_control_events(NULL, NULL, "X", COLOR_WHITE, COLOR_BLACK, 0, 0, EVID_QUIT);

    // register game start and reset events
    sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
    if (game_state == GAME_STATE_READY) {
        ploc = sdlx_render_text(0, 1600, "START");
        sdlx_register_event(ploc, EVID_GAME_START);
    } else {
        ploc = sdlx_render_text(0, 1600, "RESET");
        sdlx_register_event(ploc, EVID_GAME_RESET);
    }
    sdlx_print_init_color(COLOR_WHITE, COLOR_BLACK);

    // if the game is in progress and it is the humans turn then
    // register events for the human players possible moves
    if (game_state == GAME_STATE_ACTIVE && humans_turn(b)) {
        get_possible_moves(b, &pm);
        if (pm.max == 0) {
            sdlx_print_init_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
            ploc = sdlx_render_text(0, 1750, "PASS");  // xxx move this
            sdlx_register_event(ploc, EVID_MOVE_PASS);
        } else {
            for (int i = 0; i < pm.max; i++) {
                int r, c;
                sdlx_loc_t loc;

                move_to_rc(pm.move[i], &r, &c);
                loc = rc_to_loc[r][c];
                sdlx_register_event(&loc, pm.move[i]);
            }
        }
    }

    // draw the empty board, using green background and 
    // lines to separate the squares
    sdlx_render_fill_rect(1, 1, 998, 998, COLOR_GREEN);
    for (int i = 0; i < 9; i++) {
        int x1, y1, x2, y2;

        x1 = x2 = 125 * i;
        y1 = 0;
        y2 = 999;
        sdlx_render_line(x1, y1, x2, y2, COLOR_BLACK);

        x1 = 0;
        x2 = 999;
        y1 = y2 = 125 * i;
        sdlx_render_line(x1, y1, x2, y2, COLOR_BLACK);
    }

    // draw the black and white pieces 
    for (int r = 1; r <= 8; r++) {
        for (int c = 1; c <= 8; c++) {
            if (b->pos[r][c] != NONE) {
                sdlx_texture_t *piece;
                sdlx_loc_t loc;
                int offset;

                piece = (b->pos[r][c] == BLACK ? piece_black_circle : piece_white_circle);
                loc = rc_to_loc[r][c];
                offset = loc.w / 2 - piece_circle_radius;
                sdlx_query_texture(piece, &w, &h);
                sdlx_render_texture(loc.x+offset, loc.y+offset, w, h, piece);
            }
        }
    }

    // display the human player's possilbe moves as small circles
    if (game_state == GAME_STATE_ACTIVE && humans_turn(b)) {
        sdlx_texture_t *prompt = (b->whose_turn == BLACK ? prompt_black_circle : prompt_white_circle);
        for (int i = 0; i < pm.max; i++) {
            int r, c, offset;
            sdlx_loc_t loc;

            move_to_rc(pm.move[i], &r, &c);
            loc = rc_to_loc[r][c];
            offset = loc.w / 2 - prompt_circle_radius;
            sdlx_query_texture(prompt, &w, &h);
            sdlx_render_texture(loc.x+offset, loc.y+offset, w, h, prompt);
        }
    }
}

// -----------------  COMMON SUPPORT ROUTINES  ---------------------------

void move_to_rc(int move, int *r, int *c)
{
    *r = move / 10;
    *c = move % 10;
}

int rc_to_move(int r, int c)
{
    return r * 10 + c;
}

static int r_incr_tbl[8] = {0, -1, -1, -1,  0,  1, 1, 1};
static int c_incr_tbl[8] = {1,  1,  0, -1, -1, -1, 0, 1};

void apply_move(board_t *b, int move)
{
    int  r, c, i, j, my_color, other_color;
    int *my_color_cnt, *other_color_cnt;
    bool succ;

    //printf("apply_move called: move=%d color=%d\n", move, b->whose_turn);

    if (move == MOVE_PASS) {
        b->whose_turn = OTHER_COLOR(b->whose_turn);
        return;
    }

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    succ = false;
    move_to_rc(move, &r, &c);
    if (b->pos[r][c] != NONE) {
        printf("ERROR %s: pos[%d][%d] is occupied, color=%d\n", progname, r, c, b->pos[r][c]);
        return;
    }

    if (my_color == BLACK) {
        my_color_cnt    = &b->black_cnt;
        other_color_cnt = &b->white_cnt;
    } else {
        my_color_cnt    = &b->white_cnt;
        other_color_cnt = &b->black_cnt;
    }

    b->pos[r][c] = my_color;
    *my_color_cnt += 1;

    for (i = 0; i < 8; i++) {
        int r_incr = r_incr_tbl[i];
        int c_incr = c_incr_tbl[i];
        int r_next = r + r_incr;
        int c_next = c + c_incr;
        int cnt    = 0;

        while (b->pos[r_next][c_next] == other_color) {
            r_next += r_incr;
            c_next += c_incr;
            cnt++;
        }

        if (cnt > 0 && b->pos[r_next][c_next] == my_color) {
            succ = true;
            r_next = r;
            c_next = c;
            for (j = 0; j < cnt; j++) {
                r_next += r_incr;
                c_next += c_incr;
                b->pos[r_next][c_next] = my_color;
            }
            *my_color_cnt += cnt;
            *other_color_cnt -= cnt;
        }
    }

    if (!succ) {
        printf("ERROR %s: invalid call to apply_move, move=%d\n", progname, move);
        return;
    }

    b->whose_turn = OTHER_COLOR(b->whose_turn);
}

int possible_move_tbl[] = {
    11, 18, 81, 88, 
    33, 34, 35, 36, 
    43, 44, 45, 46, 
    53, 54, 55, 56, 
    63, 64, 65, 66, 
    23, 24, 25, 26, 
    73, 74, 75, 76, 
    32, 42, 52, 62, 
    37, 47, 57, 67, 
    13, 14, 15, 16, 
    83, 84, 85, 86, 
    31, 41, 51, 61, 
    38, 48, 58, 68, 
    12, 21, 17, 28,
    71, 82, 78, 87, 
    22, 27, 72, 77 };

void get_possible_moves(board_t *b, possible_moves_t *pm)
{
    int r, c, i, k, move, my_color, other_color;

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    pm->max = 0;

    for (k = 0; k < 64; k++) {
        move = possible_move_tbl[k];
        move_to_rc(move, &r, &c);

        if (b->pos[r][c] != NONE) {
            continue;
        }

        for (i = 0; i < 8; i++) {
            int r_incr = r_incr_tbl[i];
            int c_incr = c_incr_tbl[i];
            int r_next = r + r_incr;
            int c_next = c + c_incr;
            int cnt    = 0;

            while (b->pos[r_next][c_next] == other_color) {
                r_next += r_incr;
                c_next += c_incr;
                cnt++;
            }

            if (cnt > 0 && b->pos[r_next][c_next] == my_color) {
                pm->move[pm->max++] = rc_to_move(r, c);
                break;
            }
        }
    }
}

bool any_possible_moves(board_t *b)
{
    int r, c, i, my_color, other_color;

    my_color = b->whose_turn;
    other_color = OTHER_COLOR(my_color);

    for (r = 1; r <= 8; r++) {
        for (c = 1; c <= 8; c++) {
            if (b->pos[r][c] != NONE) {
                continue;
            }

            for (i = 0; i < 8; i++) {
                int r_incr = r_incr_tbl[i];
                int c_incr = c_incr_tbl[i];
                int r_next = r + r_incr;
                int c_next = c + c_incr;
                int cnt    = 0;

                while (b->pos[r_next][c_next] == other_color) {
                    r_next += r_incr;
                    c_next += c_incr;
                    cnt++;
                }

                if (cnt > 0 && b->pos[r_next][c_next] == my_color) {
                    return true;
                }
            }
        }
    }

    return false;
}
