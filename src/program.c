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

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
#define MAP_SIZE MAP_WIDTH*MAP_HEIGHT

#define PURPLE 0xFF7400B8 //CDB4DB
#define PINK 0xFFFB6F92 //FFAFCC
#define BLUE 0xFF758BFD //A2D2FF

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

typedef struct {
    float ray_len;
    u8 hit_side;
    v2 intersect_loc;
} ray_intersection;

float dot(v2 a, v2 b) {
    return a.x*b.x + a.y*b.y;
}

v2 scale(v2 a, float c) {
    return (v2){.x = a.x * c, .y = a.y * c};
}

v2 addv2(v2 a, v2 b) {
    return (v2){.x = a.x + b.x, .y = a.y + b.y};
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
SDL_Texture *map_texture;
u32 pixels[IMAGE_SIZE] = {0};
player p;
u32 colors[] = {0, 0, PURPLE, PINK, BLUE};
// 0=empty, 1=player, 2/3/4 = walls
u32 map_pixels[IMAGE_SIZE] = {0};
u8 map[MAP_HEIGHT][MAP_WIDTH] = {
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 4},
    {2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 4},
    {2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 4, 0, 4},
    {2, 3, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 4, 0, 4},
    {2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4}
};

void draw_vert_line(u16 x, u16 y_start, u16 y_end, u32 color) {
    assert(x >= 0 && x <= IMAGE_WIDTH);
    assert(y_start >= 0 && y_start <= y_end && y_start <= IMAGE_HEIGHT);
    assert(y_end >= 0 && y_end >= y_start && y_end <= IMAGE_HEIGHT);

    for (u16 i = y_start; i < y_end; ++i) {
        pixels[i*IMAGE_WIDTH + x] = color;
    }
}

void draw_vert_strip(u16 x_start, u16 x_end, u16 y_start, u16 y_end, u32 color) {
    assert(x_start >= 0 && x_start <= x_end && x_start <= IMAGE_WIDTH);
    assert(x_end >= 0 && x_end >= x_start && x_end <= IMAGE_WIDTH);
    assert(y_start >= 0 && y_start <= y_end && y_start <= IMAGE_HEIGHT);
    assert(y_end >= 0 && y_end >= y_start && y_end <= IMAGE_HEIGHT);

    for (u16 i = x_start; i < x_end; ++i) {
        draw_vert_line(i, y_start, y_end, color);
    }
}

void update_display(void *new_pixels) {
    SDL_UpdateTexture(texture, NULL, new_pixels, sizeof(u32) * IMAGE_WIDTH);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

v2 ray_dir(float plane_scale) {
    return addv2(p.dir, scale(p.plane, plane_scale));
}

ray_intersection ray_intersect(float plane_scale) {
    v2 dir = ray_dir(plane_scale);
    i8 stepx = dir.x < 0 ? -1 : 1;
    i8 stepy = dir.y < 0 ? -1 : 1;
    u16 mapx = (u16) p.pos.x, mapy = (u16) p.pos.y;
    float raylen_per_x = dir.x == 0 ? 1e20 : fabs(1/dir.x);
    float raylen_per_y = dir.y == 0 ? 1e20 : fabs(1/dir.y);
    float raylen_x = dir.x < 0 ? (p.pos.x - mapx) * raylen_per_x : 
                                 (mapx + 1 - p.pos.x) * raylen_per_x;
    float raylen_y = dir.y < 0 ? (p.pos.y - mapy) * raylen_per_y : 
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
    return (ray_intersection) {wall_dist, side, (v2) {mapx, mapy}};
}

void render_2d_raycast() {
    u16 length = min(IMAGE_WIDTH, IMAGE_HEIGHT);
    u16 ratio = length / MAP_WIDTH;
    u16 y_offset = (IMAGE_HEIGHT - length) / 2;
    u16 x_offset = (IMAGE_WIDTH - length) / 2;

    u16 px = p.pos.x * ratio + x_offset;
    u16 py = p.pos.y * ratio + y_offset;
    i16 dx = p.dir.x * ratio;
    i16 dy = p.dir.y * ratio;
    i16 plx = p.plane.x * ratio;
    i16 ply = p.plane.y * ratio;

    SDL_Point player_points[ratio*ratio/25];
    u16 k = 0;
    SDL_RenderClear(renderer);
    for (u16 i = y_offset; i < IMAGE_HEIGHT-y_offset; ++i) {
        for (u16 j = x_offset; j < IMAGE_WIDTH-x_offset; ++j) {
            map_pixels[i*IMAGE_WIDTH + j] = colors[map[(u16) (i - y_offset)/ratio][(u16) (j - x_offset)/ratio]];
            if (i >= py && i < py+ratio/5 && j >= px && j < px+ratio/5) player_points[k++] = (SDL_Point){j, i};//map_pixels[i*IMAGE_WIDTH + j] = 0xFFFFFFFF;
        }
    }

    SDL_UpdateTexture(texture, NULL, map_pixels, sizeof(u32) * IMAGE_WIDTH);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_SetRenderDrawColor(renderer, 0xFC, 0xF6, 0xBD, 0xFF);
    v2 ray_end;
    u8 r = 2;
    ray_intersection ri;
    for (u16 i = py+r; i < py+ratio/5-r; ++i) {
        for (u16 j = px+r; j < px+ratio/5-r; ++j) {
            for (float ps = -1; ps <= 1; ps+=0.01) {
                ri = ray_intersect(ps);
                ray_end = scale(ray_dir(ps), ri.ray_len * ratio);
                SDL_RenderDrawLine(renderer, j, i, px + (i16) ray_end.x, py + (i16) ray_end.y);
            }
        }
    }
    SDL_SetRenderDrawColor(renderer, 0xCE, 0xD4, 0xDA, 0xFF);
    SDL_RenderDrawPoints(renderer, player_points, k);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    SDL_RenderPresent(renderer);
}

void render_3d_raycast() {
    for (u16 i = 0; i < IMAGE_WIDTH; ++i) {
        float pane = 2.0 * (float) i / IMAGE_WIDTH - 1;
        ray_intersection ri = ray_intersect(pane);
        u16 height = (u16) min(IMAGE_HEIGHT, IMAGE_HEIGHT / ri.ray_len);
        u32 color = colors[map[(u16)ri.intersect_loc.y][(u16)ri.intersect_loc.x]];
        if (ri.hit_side == 1) {
            u32 r = color & 0xFF0000, g = color & 0x00FF00, b = color & 0x0000FF;
            color = 0xFF000000 | (r * 0xED / 0xFF) | (g * 0xFA / 0xFF) | (b * 0xFD / 0xFF);

        }
        draw_vert_line(i, 0, IMAGE_HEIGHT/2 - height/2, 0xFFCAF0F8);
        draw_vert_line(i, IMAGE_HEIGHT/2 - height/2, IMAGE_HEIGHT/2 + height/2, color);
        draw_vert_line(i, IMAGE_HEIGHT/2 + height/2, IMAGE_HEIGHT, 0xFFE3D5CA);
    }
    update_display(pixels);
}

int main(int argc, char **argv) {
    p = (player) { (v2){8, 8}, (v2){1, 0}, (v2){0, 1} };

    window = SDL_CreateWindow("Tinyraycast Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
                                IMAGE_WIDTH, IMAGE_HEIGHT);

    SDL_Event event;
    bool quit = false;
    u8 view3d = 0;
    float rotate_speed = M_PI / 16.0, move_speed = 0.1;
    while (1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (event.type == SDL_KEYDOWN) {
                u8 sym = event.key.keysym.scancode;
                float nx = p.pos.x, ny = p.pos.y;
                if (sym == SDL_SCANCODE_SPACE) {
                    view3d ^= 1;
                } else if (sym == SDL_SCANCODE_ESCAPE) {
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
        memset(map_pixels, 0, sizeof(u32) * IMAGE_SIZE);
        if (!view3d) render_2d_raycast();
        else render_3d_raycast();
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
