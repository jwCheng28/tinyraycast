#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h> 
#include <math.h>
#include <assert.h>
#include <SDL2/SDL.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 480

#define IMAGE_WIDTH SCREEN_WIDTH
#define IMAGE_HEIGHT SCREEN_HEIGHT
#define IMAGE_SIZE IMAGE_WIDTH*IMAGE_HEIGHT

#define MAP_WIDTH 10
#define MAP_HEIGHT 10
#define MAP_SIZE MAP_WIDTH*MAP_HEIGHT

#define PURPLE 0xCDB4DBFF
#define PINK 0xFFAFCCFF
#define BLUE 0xA2D2FFFF

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct {
    float x;
    float y;
} v2;

typedef struct {
    v2 pos;
    v2 dir;
    v2 plane;
} player;

float dot(v2 a, v2 b) {
    return a.x*b.x + a.y*b.y;
}

void printv2(v2 a) {
    printf("x: %f | y: %f\n", a.x, a.y);
}

v2 rotate(v2 a, double theta) {
    return (v2){.x = a.x * cos(theta) + a.y * sin(theta), 
                .y = a.y * cos(theta) - a.x * sin(theta)};
}

bool float_cmp(float a, float b, float precision) {
    return fabs(a-b) < precision;
}

i32 max(i32 a, i32 b) {
    if (a > b) return a;
    return b;
}

i32 min(i32 a, i32 b) {
    if (a < b) return a;
    return b;
}

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
u32 pixels[IMAGE_SIZE] = {0};
player p;
u32 colors[] = {0, 0, PURPLE, PINK, BLUE};
// 0=empty, 1=player, 2/3/4 = walls
u8 map[MAP_HEIGHT][MAP_WIDTH] = {
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
    {2, 0, 3, 3, 3, 3, 0, 4, 2, 2},
    {2, 0, 0, 0, 0, 0, 0, 4, 0, 2},
    {2, 0, 0, 0, 0, 0, 4, 4, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 4, 4, 2, 2},
    {2, 3, 3, 0, 0, 3, 3, 2, 2, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}
};

void draw_vert_line(u16 x, u16 y_start, u16 y_end, u32 color) {
    assert(x >= 0 && x <= IMAGE_WIDTH);
    assert(y_start >= 0 && y_start < y_end && y_start < IMAGE_HEIGHT);
    assert(y_end > 0 && y_end > y_start && y_end <= IMAGE_HEIGHT);

    for (u16 i = y_start; i < y_end; ++i) {
        pixels[i*IMAGE_WIDTH + x] = color;
    }
}

void draw_vert_strip(u16 x_start, u16 x_end, u16 y_start, u16 y_end, u32 color) {
    assert(x_start >= 0 && x_start < x_end && x_start < IMAGE_WIDTH);
    assert(x_end > 0 && x_end > x_start && x_end <= IMAGE_WIDTH);
    assert(y_start >= 0 && y_start < y_end && y_start < IMAGE_HEIGHT);
    assert(y_end > 0 && y_end > y_start && y_end <= IMAGE_HEIGHT);

    for (u16 i = x_start; i < x_end; ++i) {
        draw_vert_line(i, y_start, y_end, color);
    }
}

void update_display() {
    SDL_UpdateTexture(texture, NULL, pixels, sizeof(u32) * IMAGE_WIDTH);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void render_3d_raycast() {
    for (u16 i = 0; i < IMAGE_WIDTH; ++i) {
        float pane = 2.0 * (float) i / IMAGE_WIDTH - 1;

        float dirx = p.dir.x + p.plane.x * pane;
        float diry = p.dir.y + p.plane.y * pane;
        i8 stepx = dirx < 0 ? -1 : 1;
        i8 stepy = diry < 0 ? -1 : 1;
        u16 mapx = (u16) p.pos.x, mapy = (u16) p.pos.y;
        float raylen_per_x = dirx == 0 ? 1e20 : fabs(1/dirx);
        float raylen_per_y = diry == 0 ? 1e20 : fabs(1/diry);
        float raylen_x = dirx < 0 ? (p.pos.x - mapx) * raylen_per_x : 
                                    (mapx + 1 - p.pos.x) * raylen_per_x;
        float raylen_y = diry < 0 ? (p.pos.y - mapy) * raylen_per_y : 
                                    (mapy + 1 - p.pos.y) * raylen_per_y;

        u8 side = 0;
        while (true) {
            if (raylen_x < raylen_y) {
                raylen_x += raylen_per_x;
                mapx += stepx;
                side = 0;
            } else {
                raylen_y += raylen_per_y;
                mapy += stepy;
                side = 1;
            }

            assert(mapy >= 0 && mapy < MAP_HEIGHT);
            assert(mapx >= 0 && mapx < MAP_WIDTH);

            if (map[mapy][mapx] > 1) break;
        }

        float wall_dist = side == 1 ? raylen_y - raylen_per_y : raylen_x - raylen_per_x;
        if (wall_dist == 0) wall_dist = 1e-10;
        assert(wall_dist > 0);
        u16 height = (u16) min(IMAGE_HEIGHT, IMAGE_HEIGHT / wall_dist);
        u32 color = colors[map[mapy][mapx]];
        if (side == 1) color >>= 4;
        draw_vert_line(i, IMAGE_HEIGHT/2 - height/2, IMAGE_HEIGHT/2 + height/2, color);
    }
    update_display();
}

int main(int argc, char **argv) {
    p = (player) { (v2){3, 3}, (v2){1, 0}, (v2){0, 1} };

    window = SDL_CreateWindow("Tinyraycast Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 
                                IMAGE_WIDTH, IMAGE_HEIGHT);

    SDL_Event event;
    bool quit = false;
    float rotate_speed = M_PI / 16.0, move_speed = 0.5;
    while (1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                u8 sym = event.key.keysym.scancode;
                float nx = p.pos.x, ny = p.pos.y;
                if (sym == SDL_SCANCODE_ESCAPE) {
                    quit = true;
                    break;
                } else if (sym == SDL_SCANCODE_LEFT) {
                    p.dir = rotate(p.dir, rotate_speed);
                    p.plane = rotate(p.plane, rotate_speed);
                } else if (sym == SDL_SCANCODE_RIGHT) {
                    p.dir = rotate(p.dir, -rotate_speed);
                    p.plane = rotate(p.plane, -rotate_speed);
                } else if (sym == SDL_SCANCODE_UP) {
                    nx = p.pos.x + p.dir.x * move_speed;
                    ny = p.pos.y + p.dir.y * move_speed;
                } else if (sym == SDL_SCANCODE_DOWN) {
                    nx = p.pos.x - p.dir.x * move_speed;
                    ny = p.pos.y - p.dir.y * move_speed;
                }
                if (map[(u16) ny][(u16) nx] == 0) {
                    p.pos.x = nx;
                    p.pos.y = ny;
                }
            }
        }
        if (quit) break;

        memset(pixels, 0, sizeof(u32) * IMAGE_SIZE);
        render_3d_raycast();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
