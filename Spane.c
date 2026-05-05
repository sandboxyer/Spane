#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAIN_WINDOW_WIDTH  1000
#define MAIN_WINDOW_HEIGHT 700
#define GAME_AREA_WIDTH    800
#define GAME_AREA_HEIGHT   600
#define GAME_AREA_X        180
#define GAME_AREA_Y        50
#define SIDEBAR_WIDTH      160

#define TILE_SIZE     32
#define MAP_WIDTH     25
#define MAP_HEIGHT    18
#define PLAYER_SPEED  5

// Color definitions
typedef struct {
    unsigned long black;
    unsigned long white;
    unsigned long gray;
    unsigned long light_gray;
    unsigned long dark_gray;
    unsigned long blue;
    unsigned long green;
    unsigned long red;
    unsigned long yellow;
    unsigned long orange;
    unsigned long purple;
} Colors;

// Forward declarations
typedef struct Game Game;
typedef struct GameManager GameManager;

// Game interface - each game must implement these functions
struct Game {
    char name[32];
    void* data;  // Game-specific data
    int active;
    
    // Function pointers for game interface
    void (*init)(Game* game, Display* d, Window w, GC gc, Colors* colors);
    void (*handle_event)(Game* game, XEvent* e);
    void (*update)(Game* game);
    void (*render)(Game* game, Display* d, Window w, GC gc);
    void (*cleanup)(Game* game);
};

// Game Manager structure
struct GameManager {
    Display* display;
    Window main_window;
    Window game_area_window;  // Sub-window for game rendering
    GC gc;
    Colors colors;
    
    Game* games[5];  // Array of game pointers
    int game_count;
    int current_game;
    
    // UI state
    int button_states[5];  // For tracking button presses
    int hover_button;
    
    // FPS tracking
    int frame_count;
    time_t last_fps_update;
    double fps;
};

// ============================================
// RPG GAME IMPLEMENTATION
// ============================================

typedef struct {
    int map[18][25];
    int player_x, player_y;
    int player_dir;
    int player_frame;
    int moving;
    int key_up, key_down, key_left, key_right;
} RPGData;

// RPG map
int rpg_map[18][25] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,3,3,3,0,0,0,0,0,0,4,0,0,5,0,0,0,0,0,0,0,0,1},
    {1,0,0,3,3,3,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,1},
    {1,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,3,3,3,2,2,2,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,3,3,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,5,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,3,3,3,0,0,0,0,0,0,5,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,3,3,3,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static int rpg_is_walkable(RPGData* data, int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    int t = data->map[y][x];
    return (t == 0 || t == 3);
}

static void rpg_draw_tile(Display* d, Window w, GC gc, int x, int y, int type, Colors* c) {
    switch(type) {
        case 0: XSetForeground(d, gc, 0x33AA33); break; // Grass
        case 1: XSetForeground(d, gc, 0x666666); break; // Wall
        case 2: XSetForeground(d, gc, 0x3355AA); break; // Water
        case 3: XSetForeground(d, gc, 0xCCAA66); break; // Path
        case 4: XSetForeground(d, gc, 0x226622); break; // Tree
        case 5: XSetForeground(d, gc, 0x884422); break; // House
    }
    
    XFillRectangle(d, w, gc, x, y, TILE_SIZE, TILE_SIZE);
    
    if (type == 4) {
        XSetForeground(d, gc, 0x00CC00);
        XFillArc(d, w, gc, x-2, y-4, TILE_SIZE+4, 24, 0, 360*64);
        XSetForeground(d, gc, 0x664422);
        XFillRectangle(d, w, gc, x+13, y+16, 6, 16);
    }
    else if (type == 5) {
        XSetForeground(d, gc, 0xCC3333);
        XFillRectangle(d, w, gc, x+4, y+8, 24, 24);
        XSetForeground(d, gc, 0x884422);
        XPoint roof[3] = {{x+4, y+8}, {x+16, y-4}, {x+28, y+8}};
        XFillPolygon(d, w, gc, roof, 3, Convex, CoordModeOrigin);
    }
}

static void rpg_draw_player(Display* d, Window w, GC gc, int px, int py, int dir, int frame) {
    XSetForeground(d, gc, 0x3366FF);
    XFillRectangle(d, w, gc, px+8, py+12, 16, 14);
    
    XSetForeground(d, gc, 0xFFCC99);
    XFillArc(d, w, gc, px+8, py+2, 16, 16, 0, 360*64);
    
    XSetForeground(d, gc, 0x000000);
    if (dir == 1) {
        XFillRectangle(d, w, gc, px+9, py+8, 3, 3);
    } else if (dir == 2) {
        XFillRectangle(d, w, gc, px+20, py+8, 3, 3);
    } else {
        XFillRectangle(d, w, gc, px+12, py+8, 3, 3);
        XFillRectangle(d, w, gc, px+19, py+8, 3, 3);
    }
    
    XSetForeground(d, gc, 0x000088);
    if (frame % 20 < 10) {
        XFillRectangle(d, w, gc, px+9, py+26, 5, 8);
        XFillRectangle(d, w, gc, px+18, py+26, 5, 8);
    } else {
        XFillRectangle(d, w, gc, px+10, py+26, 5, 8);
        XFillRectangle(d, w, gc, px+17, py+26, 5, 8);
    }
}

static void rpg_init(Game* game, Display* d, Window w, GC gc, Colors* colors) {
    RPGData* data = (RPGData*)calloc(1, sizeof(RPGData));
    memcpy(data->map, rpg_map, sizeof(rpg_map));
    data->player_x = 5 * TILE_SIZE;
    data->player_y = 5 * TILE_SIZE;
    data->player_dir = 0;
    data->player_frame = 0;
    game->data = data;
    printf("RPG Game initialized!\n");
}

static void rpg_handle_event(Game* game, XEvent* e) {
    RPGData* data = (RPGData*)game->data;
    
    if (e->type == KeyPress) {
        KeySym key = XLookupKeysym(&e->xkey, 0);
        
        if (key == XK_Up || key == XK_w || key == XK_W) {
            data->key_up = 1;
            data->player_dir = 3;
        }
        else if (key == XK_Down || key == XK_s || key == XK_S) {
            data->key_down = 1;
            data->player_dir = 0;
        }
        else if (key == XK_Left || key == XK_a || key == XK_A) {
            data->key_left = 1;
            data->player_dir = 1;
        }
        else if (key == XK_Right || key == XK_d || key == XK_D) {
            data->key_right = 1;
            data->player_dir = 2;
        }
    }
    else if (e->type == KeyRelease) {
        KeySym key = XLookupKeysym(&e->xkey, 0);
        
        if (key == XK_Up || key == XK_w || key == XK_W) data->key_up = 0;
        else if (key == XK_Down || key == XK_s || key == XK_S) data->key_down = 0;
        else if (key == XK_Left || key == XK_a || key == XK_A) data->key_left = 0;
        else if (key == XK_Right || key == XK_d || key == XK_D) data->key_right = 0;
    }
    else if (e->type == ButtonPress) {
        if (e->xbutton.button == Button1) {
            // Check if click is within game area
            if (e->xbutton.x >= GAME_AREA_X && e->xbutton.x < GAME_AREA_X + GAME_AREA_WIDTH &&
                e->xbutton.y >= GAME_AREA_Y && e->xbutton.y < GAME_AREA_Y + GAME_AREA_HEIGHT) {
                
                int new_x = e->xbutton.x - GAME_AREA_X - 16;
                int new_y = e->xbutton.y - GAME_AREA_Y - 16;
                
                if (rpg_is_walkable(data, new_x/TILE_SIZE, new_y/TILE_SIZE) &&
                    rpg_is_walkable(data, (new_x+31)/TILE_SIZE, (new_y+31)/TILE_SIZE)) {
                    data->player_x = new_x;
                    data->player_y = new_y;
                }
            }
        }
    }
}

static void rpg_update(Game* game) {
    RPGData* data = (RPGData*)game->data;
    
    data->moving = 0;
    int new_x = data->player_x;
    int new_y = data->player_y;
    
    if (data->key_up)    { new_y -= PLAYER_SPEED; data->moving = 1; }
    if (data->key_down)  { new_y += PLAYER_SPEED; data->moving = 1; }
    if (data->key_left)  { new_x -= PLAYER_SPEED; data->moving = 1; }
    if (data->key_right) { new_x += PLAYER_SPEED; data->moving = 1; }
    
    if (data->moving) {
        if (rpg_is_walkable(data, new_x/TILE_SIZE, new_y/TILE_SIZE) &&
            rpg_is_walkable(data, (new_x+31)/TILE_SIZE, new_y/TILE_SIZE) &&
            rpg_is_walkable(data, new_x/TILE_SIZE, (new_y+31)/TILE_SIZE) &&
            rpg_is_walkable(data, (new_x+31)/TILE_SIZE, (new_y+31)/TILE_SIZE)) {
            data->player_x = new_x;
            data->player_y = new_y;
        }
    }
    
    if (data->player_x < 0) data->player_x = 0;
    if (data->player_y < 0) data->player_y = 0;
    if (data->player_x > (MAP_WIDTH-1)*TILE_SIZE) data->player_x = (MAP_WIDTH-1)*TILE_SIZE;
    if (data->player_y > (MAP_HEIGHT-1)*TILE_SIZE) data->player_y = (MAP_HEIGHT-1)*TILE_SIZE;
    
    if (data->moving) data->player_frame++;
    else data->player_frame = 0;
}

static void rpg_render(Game* game, Display* d, Window w, GC gc) {
    RPGData* data = (RPGData*)game->data;
    Colors* c = NULL;  // Not used but available
    
    // Clear game area
    XSetForeground(d, gc, 0x000000);
    XFillRectangle(d, w, gc, GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT);
    
    // Draw map
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            rpg_draw_tile(d, w, gc, 
                         GAME_AREA_X + x*TILE_SIZE, 
                         GAME_AREA_Y + y*TILE_SIZE, 
                         data->map[y][x], c);
        }
    }
    
    // Draw player
    rpg_draw_player(d, w, gc, 
                   GAME_AREA_X + data->player_x, 
                   GAME_AREA_Y + data->player_y, 
                   data->player_dir, data->player_frame);
    
    // Draw game title and info
    XSetForeground(d, gc, 0xFFFFFF);
    char info[200];
    snprintf(info, sizeof(info), 
            "RPG Adventure - Pos: (%d,%d) - Arrow Keys/WASD to Move",
            data->player_x/TILE_SIZE, data->player_y/TILE_SIZE);
    XDrawString(d, w, gc, GAME_AREA_X + 10, GAME_AREA_Y + GAME_AREA_HEIGHT - 10, 
               info, strlen(info));
}

static void rpg_cleanup(Game* game) {
    if (game->data) {
        free(game->data);
        game->data = NULL;
    }
}

// ============================================
// SNAKE GAME IMPLEMENTATION
// ============================================

#define SNAKE_GRID_SIZE 20
#define SNAKE_MAX_LENGTH 500
#define SNAKE_AREA_WIDTH 800
#define SNAKE_AREA_HEIGHT 600
#define SNAKE_COLS (SNAKE_AREA_WIDTH / SNAKE_GRID_SIZE)
#define SNAKE_ROWS (SNAKE_AREA_HEIGHT / SNAKE_GRID_SIZE)

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point snake[SNAKE_MAX_LENGTH];
    int length;
    int direction;  // 0=right, 1=down, 2=left, 3=up
    int next_direction;
    Point food;
    int score;
    int game_over;
    int frame_count;
    int speed;
} SnakeData;

static void snake_spawn_food(SnakeData* data) {
    int valid;
    do {
        valid = 1;
        data->food.x = rand() % SNAKE_COLS;
        data->food.y = rand() % SNAKE_ROWS;
        
        // Make sure food doesn't spawn on snake
        for (int i = 0; i < data->length; i++) {
            if (data->snake[i].x == data->food.x && data->snake[i].y == data->food.y) {
                valid = 0;
                break;
            }
        }
    } while (!valid);
}

static void snake_init(Game* game, Display* d, Window w, GC gc, Colors* colors) {
    SnakeData* data = (SnakeData*)calloc(1, sizeof(SnakeData));
    
    // Initialize snake in the middle
    data->length = 3;
    int start_x = SNAKE_COLS / 2;
    int start_y = SNAKE_ROWS / 2;
    
    for (int i = 0; i < data->length; i++) {
        data->snake[i].x = start_x - i;
        data->snake[i].y = start_y;
    }
    
    data->direction = 0;
    data->next_direction = 0;
    data->score = 0;
    data->game_over = 0;
    data->frame_count = 0;
    data->speed = 10;
    
    snake_spawn_food(data);
    game->data = data;
    printf("Snake Game initialized!\n");
}

static void snake_handle_event(Game* game, XEvent* e) {
    SnakeData* data = (SnakeData*)game->data;
    
    if (e->type == KeyPress) {
        KeySym key = XLookupKeysym(&e->xkey, 0);
        
        if (data->game_over) {
            if (key == XK_r || key == XK_R) {
                // Reset game
                data->length = 3;
                int start_x = SNAKE_COLS / 2;
                int start_y = SNAKE_ROWS / 2;
                for (int i = 0; i < data->length; i++) {
                    data->snake[i].x = start_x - i;
                    data->snake[i].y = start_y;
                }
                data->direction = 0;
                data->next_direction = 0;
                data->score = 0;
                data->game_over = 0;
                data->frame_count = 0;
                snake_spawn_food(data);
            }
            return;
        }
        
        switch(key) {
            case XK_Up: case XK_w: case XK_W:
                if (data->direction != 1) data->next_direction = 3;
                break;
            case XK_Down: case XK_s: case XK_S:
                if (data->direction != 3) data->next_direction = 1;
                break;
            case XK_Left: case XK_a: case XK_A:
                if (data->direction != 0) data->next_direction = 2;
                break;
            case XK_Right: case XK_d: case XK_D:
                if (data->direction != 2) data->next_direction = 0;
                break;
        }
    }
}

static void snake_update(Game* game) {
    SnakeData* data = (SnakeData*)game->data;
    
    if (data->game_over) return;
    
    data->frame_count++;
    if (data->frame_count < data->speed) return;
    data->frame_count = 0;
    
    // Update direction
    data->direction = data->next_direction;
    
    // Move snake
    Point new_head = data->snake[0];
    switch(data->direction) {
        case 0: new_head.x++; break;
        case 1: new_head.y++; break;
        case 2: new_head.x--; break;
        case 3: new_head.y--; break;
    }
    
    // Check wall collision
    if (new_head.x < 0 || new_head.x >= SNAKE_COLS ||
        new_head.y < 0 || new_head.y >= SNAKE_ROWS) {
        data->game_over = 1;
        return;
    }
    
    // Check self collision
    for (int i = 0; i < data->length; i++) {
        if (data->snake[i].x == new_head.x && data->snake[i].y == new_head.y) {
            data->game_over = 1;
            return;
        }
    }
    
    // Check food
    int ate_food = (new_head.x == data->food.x && new_head.y == data->food.y);
    
    // Move body
    for (int i = data->length - 1; i > 0; i--) {
        data->snake[i] = data->snake[i-1];
    }
    data->snake[0] = new_head;
    
    if (ate_food) {
        data->length++;
        data->score += 10;
        if (data->speed > 3) data->speed--;
        snake_spawn_food(data);
    }
}

static void snake_render(Game* game, Display* d, Window w, GC gc) {
    SnakeData* data = (SnakeData*)game->data;
    
    // Clear game area
    XSetForeground(d, gc, 0x1A1A2E);
    XFillRectangle(d, w, gc, GAME_AREA_X, GAME_AREA_Y, SNAKE_AREA_WIDTH, SNAKE_AREA_HEIGHT);
    
    // Draw grid lines (subtle)
    XSetForeground(d, gc, 0x252540);
    for (int i = 0; i <= SNAKE_COLS; i++) {
        XDrawLine(d, w, gc, 
                 GAME_AREA_X + i * SNAKE_GRID_SIZE, GAME_AREA_Y,
                 GAME_AREA_X + i * SNAKE_GRID_SIZE, GAME_AREA_Y + SNAKE_AREA_HEIGHT);
    }
    for (int i = 0; i <= SNAKE_ROWS; i++) {
        XDrawLine(d, w, gc, 
                 GAME_AREA_X, GAME_AREA_Y + i * SNAKE_GRID_SIZE,
                 GAME_AREA_X + SNAKE_AREA_WIDTH, GAME_AREA_Y + i * SNAKE_GRID_SIZE);
    }
    
    // Draw food
    XSetForeground(d, gc, 0xFF3333);
    XFillArc(d, w, gc, 
            GAME_AREA_X + data->food.x * SNAKE_GRID_SIZE + 2,
            GAME_AREA_Y + data->food.y * SNAKE_GRID_SIZE + 2,
            SNAKE_GRID_SIZE - 4, SNAKE_GRID_SIZE - 4, 0, 360*64);
    
    // Draw snake
    for (int i = 0; i < data->length; i++) {
        if (i == 0) {
            XSetForeground(d, gc, 0x00FF00);  // Head
        } else {
            XSetForeground(d, gc, 0x00AA00);  // Body
        }
        
        XFillRectangle(d, w, gc,
                      GAME_AREA_X + data->snake[i].x * SNAKE_GRID_SIZE + 1,
                      GAME_AREA_Y + data->snake[i].y * SNAKE_GRID_SIZE + 1,
                      SNAKE_GRID_SIZE - 2, SNAKE_GRID_SIZE - 2);
    }
    
    // Draw score
    XSetForeground(d, gc, 0xFFFFFF);
    char score_text[50];
    snprintf(score_text, sizeof(score_text), "Score: %d", data->score);
    XDrawString(d, w, gc, GAME_AREA_X + 10, GAME_AREA_Y + 20, score_text, strlen(score_text));
    
    // Game over
    if (data->game_over) {
        XSetForeground(d, gc, 0x000000);
        XFillRectangle(d, w, gc, 
                      GAME_AREA_X + 250, GAME_AREA_Y + 250, 300, 100);
        XSetForeground(d, gc, 0xFF0000);
        char gameover_text[] = "GAME OVER!";
        char restart_text[] = "Press R to restart";
        XDrawString(d, w, gc, GAME_AREA_X + 320, GAME_AREA_Y + 280, 
                   gameover_text, strlen(gameover_text));
        XDrawString(d, w, gc, GAME_AREA_X + 310, GAME_AREA_Y + 310, 
                   restart_text, strlen(restart_text));
    }
    
    // Game info
    XSetForeground(d, gc, 0x888888);
    char info[100];
    snprintf(info, sizeof(info), "Snake Game - Arrow Keys/WASD to Move - Length: %d", data->length);
    XDrawString(d, w, gc, GAME_AREA_X + 10, GAME_AREA_Y + SNAKE_AREA_HEIGHT - 10, 
               info, strlen(info));
}

static void snake_cleanup(Game* game) {
    if (game->data) {
        free(game->data);
        game->data = NULL;
    }
}

// ============================================
// GAME MANAGER FUNCTIONS
// ============================================

static void gm_init_colors(GameManager* gm) {
    int s = DefaultScreen(gm->display);
    gm->colors.black = BlackPixel(gm->display, s);
    gm->colors.white = WhitePixel(gm->display, s);
    
    // Create a colormap for custom colors
    Colormap cmap = DefaultColormap(gm->display, s);
    XColor color;
    
    // Parse and allocate colors
    XParseColor(gm->display, cmap, "#333333", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.gray = color.pixel;
    
    XParseColor(gm->display, cmap, "#CCCCCC", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.light_gray = color.pixel;
    
    XParseColor(gm->display, cmap, "#1A1A1A", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.dark_gray = color.pixel;
    
    XParseColor(gm->display, cmap, "#3355AA", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.blue = color.pixel;
    
    XParseColor(gm->display, cmap, "#33AA33", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.green = color.pixel;
    
    XParseColor(gm->display, cmap, "#CC3333", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.red = color.pixel;
    
    XParseColor(gm->display, cmap, "#CCAA33", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.yellow = color.pixel;
    
    XParseColor(gm->display, cmap, "#FF8833", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.orange = color.pixel;
    
    XParseColor(gm->display, cmap, "#8844AA", &color); XAllocColor(gm->display, cmap, &color);
    gm->colors.purple = color.pixel;
}

static void gm_add_game(GameManager* gm, Game* game) {
    if (gm->game_count < 5) {
        gm->games[gm->game_count] = game;
        gm->game_count++;
    }
}

static void gm_switch_game(GameManager* gm, int index) {
    if (index >= 0 && index < gm->game_count && index != gm->current_game) {
        printf("Switching to game: %s\n", gm->games[index]->name);
        gm->current_game = index;
    }
}

static void gm_draw_button(Display* d, Window w, GC gc, int x, int y, int w_btn, int h_btn, 
                          const char* text, int is_active, int is_hover, Colors* c) {
    // Button background
    if (is_active) {
        XSetForeground(d, gc, c->blue);
    } else if (is_hover) {
        XSetForeground(d, gc, c->yellow);
    } else {
        XSetForeground(d, gc, c->gray);
    }
    XFillRectangle(d, w, gc, x, y, w_btn, h_btn);
    
    // Button border
    XSetForeground(d, gc, c->light_gray);
    XDrawRectangle(d, w, gc, x, y, w_btn, h_btn);
    
    // Button text
    XSetForeground(d, gc, c->white);
    int text_width = XTextWidth(XQueryFont(d, XGContextFromGC(gc)), text, strlen(text));
    XDrawString(d, w, gc, 
               x + (w_btn - text_width) / 2, 
               y + h_btn/2 + 5, 
               text, strlen(text));
}

static void gm_draw_sidebar(GameManager* gm) {
    Display* d = gm->display;
    Window w = gm->main_window;
    GC gc = gm->gc;
    
    // Sidebar background
    XSetForeground(d, gc, gm->colors.dark_gray);
    XFillRectangle(d, w, gc, 0, 0, SIDEBAR_WIDTH, MAIN_WINDOW_HEIGHT);
    
    // Title
    XSetForeground(d, gc, gm->colors.white);
    char title[] = "GAME HUB";
    int title_w = XTextWidth(XQueryFont(d, XGContextFromGC(gc)), title, strlen(title));
    XDrawString(d, w, gc, (SIDEBAR_WIDTH - title_w)/2, 30, title, strlen(title));
    
    // Separator
    XSetForeground(d, gc, gm->colors.light_gray);
    XDrawLine(d, w, gc, 10, 45, SIDEBAR_WIDTH-10, 45);
    
    // Game buttons
    int btn_y = 70;
    int btn_height = 40;
    int btn_spacing = 15;
    
    for (int i = 0; i < gm->game_count; i++) {
        int is_active = (i == gm->current_game);
        int is_hover = (i == gm->hover_button);
        
        gm_draw_button(d, w, gc, 15, btn_y, SIDEBAR_WIDTH-30, btn_height,
                      gm->games[i]->name, is_active, is_hover, &gm->colors);
        btn_y += btn_height + btn_spacing;
    }
    
    // Info section at bottom
    btn_y = MAIN_WINDOW_HEIGHT - 120;
    XSetForeground(d, gc, gm->colors.light_gray);
    XDrawLine(d, w, gc, 10, btn_y, SIDEBAR_WIDTH-10, btn_y);
    
    btn_y += 15;
    XSetForeground(d, gc, gm->colors.white);
    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", gm->fps);
    XDrawString(d, w, gc, 15, btn_y, fps_text, strlen(fps_text));
    
    btn_y += 20;
    char help[] = "ESC to quit";
    XDrawString(d, w, gc, 15, btn_y, help, strlen(help));
}

static void gm_draw_border(GameManager* gm) {
    Display* d = gm->display;
    Window w = gm->main_window;
    GC gc = gm->gc;
    
    // Draw border around game area
    XSetForeground(d, gc, gm->colors.light_gray);
    for (int i = 0; i < 3; i++) {
        XDrawRectangle(d, w, gc, 
                      GAME_AREA_X - i, GAME_AREA_Y - i,
                      GAME_AREA_WIDTH + 2*i, GAME_AREA_HEIGHT + 2*i);
    }
    
    // Current game label
    XSetForeground(d, gc, gm->colors.white);
    char game_label[64];
    snprintf(game_label, sizeof(game_label), "Current Game: %s", 
            gm->games[gm->current_game]->name);
    XDrawString(d, w, gc, GAME_AREA_X, GAME_AREA_Y - 15, game_label, strlen(game_label));
}

static void gm_handle_sidebar_click(GameManager* gm, int x, int y) {
    if (x < 0 || x > SIDEBAR_WIDTH) return;
    
    int btn_y = 70;
    int btn_height = 40;
    int btn_spacing = 15;
    
    for (int i = 0; i < gm->game_count; i++) {
        if (y >= btn_y && y < btn_y + btn_height && x >= 15 && x < SIDEBAR_WIDTH - 15) {
            gm_switch_game(gm, i);
            return;
        }
        btn_y += btn_height + btn_spacing;
    }
}

static void gm_update_fps(GameManager* gm) {
    gm->frame_count++;
    time_t now = time(NULL);
    if (now - gm->last_fps_update >= 1) {
        gm->fps = gm->frame_count / (double)(now - gm->last_fps_update);
        gm->frame_count = 0;
        gm->last_fps_update = now;
    }
}

// ============================================
// MAIN PROGRAM
// ============================================

int main(void) {
    GameManager gm;
    memset(&gm, 0, sizeof(gm));
    
    // Initialize display
    gm.display = XOpenDisplay(NULL);
    if (!gm.display) {
        printf("Cannot open display\n");
        return 1;
    }
    
    gm_init_colors(&gm);
    
    int s = DefaultScreen(gm.display);
    
    // Create main window
    gm.main_window = XCreateSimpleWindow(gm.display, 
                                         RootWindow(gm.display, s),
                                         50, 50, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT, 1,
                                         BlackPixel(gm.display, s),
                                         gm.colors.dark_gray);
    
    XSelectInput(gm.display, gm.main_window, 
                ExposureMask | KeyPressMask | KeyReleaseMask | 
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                StructureNotifyMask);
    XStoreName(gm.display, gm.main_window, "Game Hub - Multi-Game Platform");
    XMapWindow(gm.display, gm.main_window);
    
    gm.gc = XCreateGC(gm.display, gm.main_window, 0, NULL);
    
    // Initialize games
    Game rpg_game;
    memset(&rpg_game, 0, sizeof(Game));
    strcpy(rpg_game.name, "RPG Adventure");
    rpg_game.init = rpg_init;
    rpg_game.handle_event = rpg_handle_event;
    rpg_game.update = rpg_update;
    rpg_game.render = rpg_render;
    rpg_game.cleanup = rpg_cleanup;
    rpg_game.active = 1;
    
    Game snake_game;
    memset(&snake_game, 0, sizeof(Game));
    strcpy(snake_game.name, "Snake");
    snake_game.init = snake_init;
    snake_game.handle_event = snake_handle_event;
    snake_game.update = snake_update;
    snake_game.render = snake_render;
    snake_game.cleanup = snake_cleanup;
    snake_game.active = 1;
    
    // Add games to manager
    gm_add_game(&gm, &rpg_game);
    gm_add_game(&gm, &snake_game);
    gm.current_game = 0;
    
    // Initialize all games
    for (int i = 0; i < gm.game_count; i++) {
        gm.games[i]->init(gm.games[i], gm.display, gm.main_window, gm.gc, &gm.colors);
    }
    
    // Seed random for snake game
    srand(time(NULL));
    
    // Wait for window to appear
    XEvent e;
    do {
        XNextEvent(gm.display, &e);
    } while (e.type != MapNotify);
    
    printf("Game Hub started! Use buttons to switch games, ESC to quit.\n");
    
    int running = 1;
    gm.last_fps_update = time(NULL);
    
    while (running) {
        // Process all pending events
        while (XPending(gm.display)) {
            XNextEvent(gm.display, &e);
            
            if (e.type == KeyPress) {
                KeySym key = XLookupKeysym(&e.xkey, 0);
                if (key == XK_Escape) {
                    running = 0;
                } else if (key == XK_F1) {
                    gm_switch_game(&gm, 0);  // Switch to RPG
                } else if (key == XK_F2) {
                    gm_switch_game(&gm, 1);  // Switch to Snake
                } else {
                    // Forward to current game
                    if (gm.games[gm.current_game]->active) {
                        gm.games[gm.current_game]->handle_event(
                            gm.games[gm.current_game], &e);
                    }
                }
            }
            else if (e.type == KeyRelease) {
                if (gm.games[gm.current_game]->active) {
                    gm.games[gm.current_game]->handle_event(
                        gm.games[gm.current_game], &e);
                }
            }
            else if (e.type == ButtonPress) {
                int x = e.xbutton.x;
                int y = e.xbutton.y;
                
                if (x < SIDEBAR_WIDTH) {
                    gm_handle_sidebar_click(&gm, x, y);
                } else {
                    if (gm.games[gm.current_game]->active) {
                        gm.games[gm.current_game]->handle_event(
                            gm.games[gm.current_game], &e);
                    }
                }
            }
            else if (e.type == MotionNotify) {
                gm.hover_button = -1;
                if (e.xmotion.x < SIDEBAR_WIDTH) {
                    int btn_y = 70;
                    int btn_height = 40;
                    int btn_spacing = 15;
                    for (int i = 0; i < gm.game_count; i++) {
                        if (e.xmotion.y >= btn_y && e.xmotion.y < btn_y + btn_height &&
                            e.xmotion.x >= 15 && e.xmotion.x < SIDEBAR_WIDTH - 15) {
                            gm.hover_button = i;
                            break;
                        }
                        btn_y += btn_height + btn_spacing;
                    }
                }
            }
        }
        
        // Update current game
        if (gm.games[gm.current_game]->active) {
            gm.games[gm.current_game]->update(gm.games[gm.current_game]);
        }
        
        // Render everything
        XSetForeground(gm.display, gm.gc, gm.colors.dark_gray);
        XFillRectangle(gm.display, gm.main_window, gm.gc, 
                      0, 0, MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT);
        
        // Draw sidebar
        gm_draw_sidebar(&gm);
        
        // Draw game area border and label
        gm_draw_border(&gm);
        
        // Render current game
        if (gm.games[gm.current_game]->active) {
            gm.games[gm.current_game]->render(
                gm.games[gm.current_game], gm.display, gm.main_window, gm.gc);
        }
        
        XFlush(gm.display);
        
        gm_update_fps(&gm);
        usleep(16667);  // ~60 FPS
    }
    
    // Cleanup
    printf("Shutting down...\n");
    for (int i = 0; i < gm.game_count; i++) {
        if (gm.games[i]->active) {
            gm.games[i]->cleanup(gm.games[i]);
        }
    }
    
    XFreeGC(gm.display, gm.gc);
    XDestroyWindow(gm.display, gm.main_window);
    XCloseDisplay(gm.display);
    
    return 0;
}