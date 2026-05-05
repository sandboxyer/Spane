#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define TILE_SIZE     32
#define MAP_WIDTH     25
#define MAP_HEIGHT    18
#define PLAYER_SPEED  5

// Map tiles: 0=grass, 1=wall, 2=water, 3=path, 4=tree, 5=house
int map[MAP_HEIGHT][MAP_WIDTH] = {
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

int is_walkable(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 0;
    int t = map[y][x];
    return (t == 0 || t == 3);  // Only grass and path are walkable
}

void draw_tile(Display *d, Window w, GC gc, int x, int y, int type) {
    switch(type) {
        case 0: XSetForeground(d, gc, 0x33AA33); break; // Grass
        case 1: XSetForeground(d, gc, 0x666666); break; // Wall
        case 2: XSetForeground(d, gc, 0x3355AA); break; // Water
        case 3: XSetForeground(d, gc, 0xCCAA66); break; // Path
        case 4: XSetForeground(d, gc, 0x226622); break; // Tree
        case 5: XSetForeground(d, gc, 0x884422); break; // House
    }
    
    XFillRectangle(d, w, gc, x, y, TILE_SIZE, TILE_SIZE);
    
    // Tree decoration
    if (type == 4) {
        XSetForeground(d, gc, 0x00CC00);
        XFillArc(d, w, gc, x-2, y-4, TILE_SIZE+4, 24, 0, 360*64);
        XSetForeground(d, gc, 0x664422);
        XFillRectangle(d, w, gc, x+13, y+16, 6, 16);
    }
    // House decoration
    else if (type == 5) {
        XSetForeground(d, gc, 0xCC3333);
        XFillRectangle(d, w, gc, x+4, y+8, 24, 24);
        XSetForeground(d, gc, 0x884422);
        XPoint roof[3] = {{x+4, y+8}, {x+16, y-4}, {x+28, y+8}};
        XFillPolygon(d, w, gc, roof, 3, Convex, CoordModeOrigin);
    }
}

void draw_player(Display *d, Window w, GC gc, int px, int py, int dir, int frame) {
    // Body
    XSetForeground(d, gc, 0x3366FF);
    XFillRectangle(d, w, gc, px+8, py+12, 16, 14);
    
    // Head
    XSetForeground(d, gc, 0xFFCC99);
    XFillArc(d, w, gc, px+8, py+2, 16, 16, 0, 360*64);
    
    // Eyes
    XSetForeground(d, gc, 0x000000);
    if (dir == 1) { // Left
        XFillRectangle(d, w, gc, px+9, py+8, 3, 3);
    } else if (dir == 2) { // Right
        XFillRectangle(d, w, gc, px+20, py+8, 3, 3);
    } else { // Down or Up
        XFillRectangle(d, w, gc, px+12, py+8, 3, 3);
        XFillRectangle(d, w, gc, px+19, py+8, 3, 3);
    }
    
    // Legs with walking animation
    XSetForeground(d, gc, 0x000088);
    if (frame % 20 < 10) {
        XFillRectangle(d, w, gc, px+9, py+26, 5, 8);
        XFillRectangle(d, w, gc, px+18, py+26, 5, 8);
    } else {
        XFillRectangle(d, w, gc, px+10, py+26, 5, 8);
        XFillRectangle(d, w, gc, px+17, py+26, 5, 8);
    }
}

int main(void) {
    Display *d;
    Window w;
    XEvent e;
    GC gc;
    int s;
    KeySym key;
    
    // Player variables
    int player_x = 5 * TILE_SIZE;
    int player_y = 5 * TILE_SIZE;
    int player_dir = 0;  // 0=down, 1=left, 2=right, 3=up
    int player_frame = 0;
    int moving = 0;
    
    // Keyboard state
    int key_up = 0, key_down = 0, key_left = 0, key_right = 0;
    
    d = XOpenDisplay(NULL);
    if (!d) {
        printf("Cannot open display\n");
        return 1;
    }
    
    s = DefaultScreen(d);
    
    w = XCreateSimpleWindow(d, RootWindow(d, s), 
                           100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 1,
                           BlackPixel(d, s), BlackPixel(d, s));
    
    XSelectInput(d, w, ExposureMask | KeyPressMask | KeyReleaseMask | 
                 ButtonPressMask | StructureNotifyMask);
    XStoreName(d, w, "RPG Game - Arrow Keys to Move");
    XMapWindow(d, w);
    
    gc = XCreateGC(d, w, 0, NULL);
    
    // Wait for window to appear
    do {
        XNextEvent(d, &e);
    } while (e.type != MapNotify);
    
    printf("Game started! Use arrow keys to move, ESC to quit.\n");
    
    int running = 1;
    while (running) {
        // Process all pending events
        while (XPending(d)) {
            XNextEvent(d, &e);
            
            if (e.type == KeyPress) {
                key = XLookupKeysym(&e.xkey, 0);
                
                if (key == XK_Escape) {
                    running = 0;
                }
                else if (key == XK_Up || key == XK_w || key == XK_W) {
                    key_up = 1;
                    player_dir = 3;
                    printf("UP pressed\n");
                }
                else if (key == XK_Down || key == XK_s || key == XK_S) {
                    key_down = 1;
                    player_dir = 0;
                    printf("DOWN pressed\n");
                }
                else if (key == XK_Left || key == XK_a || key == XK_A) {
                    key_left = 1;
                    player_dir = 1;
                    printf("LEFT pressed\n");
                }
                else if (key == XK_Right || key == XK_d || key == XK_D) {
                    key_right = 1;
                    player_dir = 2;
                    printf("RIGHT pressed\n");
                }
            }
            else if (e.type == KeyRelease) {
                key = XLookupKeysym(&e.xkey, 0);
                
                if (key == XK_Up || key == XK_w || key == XK_W) {
                    key_up = 0;
                }
                else if (key == XK_Down || key == XK_s || key == XK_S) {
                    key_down = 0;
                }
                else if (key == XK_Left || key == XK_a || key == XK_A) {
                    key_left = 0;
                }
                else if (key == XK_Right || key == XK_d || key == XK_D) {
                    key_right = 0;
                }
            }
            else if (e.type == ButtonPress) {
                if (e.xbutton.button == Button1) {
                    // Teleport to mouse click
                    int new_x = e.xbutton.x - 16;
                    int new_y = e.xbutton.y - 16;
                    
                    // Check if destination is walkable
                    if (is_walkable(new_x/TILE_SIZE, new_y/TILE_SIZE) &&
                        is_walkable((new_x+31)/TILE_SIZE, (new_y+31)/TILE_SIZE)) {
                        player_x = new_x;
                        player_y = new_y;
                        printf("Teleported to (%d, %d)\n", 
                               player_x/TILE_SIZE, player_y/TILE_SIZE);
                    }
                }
            }
        }
        
        // Handle movement
        moving = 0;
        int new_x = player_x;
        int new_y = player_y;
        
        if (key_up)    { new_y -= PLAYER_SPEED; moving = 1; }
        if (key_down)  { new_y += PLAYER_SPEED; moving = 1; }
        if (key_left)  { new_x -= PLAYER_SPEED; moving = 1; }
        if (key_right) { new_x += PLAYER_SPEED; moving = 1; }
        
        // Check collision at new position (check corners of player)
        if (moving) {
            if (is_walkable(new_x/TILE_SIZE, new_y/TILE_SIZE) &&
                is_walkable((new_x+31)/TILE_SIZE, new_y/TILE_SIZE) &&
                is_walkable(new_x/TILE_SIZE, (new_y+31)/TILE_SIZE) &&
                is_walkable((new_x+31)/TILE_SIZE, (new_y+31)/TILE_SIZE)) {
                player_x = new_x;
                player_y = new_y;
            }
        }
        
        // Keep player in bounds
        if (player_x < 0) player_x = 0;
        if (player_y < 0) player_y = 0;
        if (player_x > (MAP_WIDTH-1)*TILE_SIZE) player_x = (MAP_WIDTH-1)*TILE_SIZE;
        if (player_y > (MAP_HEIGHT-1)*TILE_SIZE) player_y = (MAP_HEIGHT-1)*TILE_SIZE;
        
        // Update animation
        if (moving) player_frame++;
        else player_frame = 0;
        
        // DRAW EVERYTHING
        // Clear screen
        XSetForeground(d, gc, 0x000000);
        XFillRectangle(d, w, gc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        // Draw map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                draw_tile(d, w, gc, x*TILE_SIZE, y*TILE_SIZE, map[y][x]);
            }
        }
        
        // Draw player
        draw_player(d, w, gc, player_x, player_y, player_dir, player_frame);
        
        // Draw status bar
        XSetForeground(d, gc, 0x222222);
        XFillRectangle(d, w, gc, 0, WINDOW_HEIGHT-30, WINDOW_WIDTH, 30);
        
        XSetForeground(d, gc, 0xFFFFFF);
        char status[200];
        snprintf(status, sizeof(status), 
                "Position: (%d,%d) | Arrow Keys/WASD: Move | Mouse Click: Teleport | ESC: Quit",
                player_x/TILE_SIZE, player_y/TILE_SIZE);
        XDrawString(d, w, gc, 10, WINDOW_HEIGHT-12, status, strlen(status));
        
        XFlush(d);
        usleep(16667);  // ~60 FPS
    }
    
    printf("Game ended.\n");
    XFreeGC(d, gc);
    XDestroyWindow(d, w);
    XCloseDisplay(d);
    return 0;
}