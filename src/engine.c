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

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

#define IMAGE_WIDTH SCREEN_WIDTH
#define IMAGE_HEIGHT SCREEN_HEIGHT
#define IMAGE_SIZE IMAGE_WIDTH*IMAGE_HEIGHT

#define MAP_WIDTH 32
#define MAP_HEIGHT 32
#define MAP_SIZE MAP_WIDTH*MAP_HEIGHT

#define BOUNDARY 8
#define MAP_VIEW_WIDTH 320
#define MAP_VIEW_HEIGHT 320
#define MAP_VIEW_SIZE MAP_VIEW_WIDTH*MAP_VIEW_HEIGHT
#define MAP_IMAGE_WIDTH (MAP_VIEW_WIDTH + 2*BOUNDARY)
#define MAP_IMAGE_HEIGHT (MAP_VIEW_HEIGHT + 2*BOUNDARY)
#define MAP_IMAGE_SIZE MAP_IMAGE_WIDTH*MAP_IMAGE_HEIGHT

#define PURPLE 0xFF7400B8 //CDB4DB
#define PINK 0xFFFB6F92 //FFAFCC
#define BLUE 0xFF758BFD //A2D2FF
#define SKY_COLOR 0xFFCAF0F8
#define GROUND_COLOR 0xFFE3D5CA

#define SIGN(x) (x < 0 ? -1 : (x > 0 ? 1 : 0))
#define BOUND(x, mn, mx) (max(min(x, mx), mn))
#define INBOUND(x, mn, mx) (mn <= x && x <= mx)

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
    float z;
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
SDL_Rect map_viewport = (SDL_Rect) {.x=0, .y=0, .w=MAP_IMAGE_WIDTH, .h=MAP_IMAGE_HEIGHT};
SDL_Rect full_viewport = (SDL_Rect) {.x=0, .y=0, .w=IMAGE_WIDTH, .h=IMAGE_HEIGHT};
u32 pixels[IMAGE_SIZE] = {0};
player p;
u32 colors[] = {0, 0, PURPLE, PINK, BLUE};
// 0=empty, 1=player, 2/3/4 = walls
u32 map_pixels[MAP_IMAGE_SIZE] = {0};
u8 map[MAP_HEIGHT][MAP_WIDTH] = {
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4},
    {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}
};

void draw_line(u32 *pixels, u16 image_width, u16 image_height, u32 x_start, u32 x_end, u32 y_start, u32 y_end, u32 color) {
    assert(x_start >= 0 && x_start <= image_width);
    assert(x_end >= 0 && x_end <= image_width);
    assert(y_start >= 0 && y_start <= image_height);
    assert(y_end >= 0 && y_end <= image_height);

    i16 dx = x_end - x_start, dy = y_end - y_start;
    i8 stepx = SIGN(dx), stepy = SIGN(dy);
    dx = abs(dx), dy = abs(dy);

    i16 m, err, step_err;
    i16 x = x_start, y = y_start;
    if (dx > dy) {
        m = 2 * dy;
        err = -dx;
        step_err = -2 * dx;
        while (x != x_end) {
            pixels[y * image_width + x] = color;
            err += m;
            x += stepx;
            if (err >= 0) {
                y += stepy;
                err += step_err;
            }
        }
    } else {
        m = 2 * dx;
        err = -dy;
        step_err = -2 * dy;
        while (y != y_end) {
            pixels[y * image_width + x] = color;
            err += m;
            y += stepy;
            if (err >= 0) {
                x += stepx;
                err += step_err;
            }
        }

    }
}

void draw_vert_line(u32 *pixels, u16 image_width, u16 image_height, u16 x, u16 y_start, u16 y_end, u32 color) {
    assert(x >= 0 && x <= image_width);
    assert(y_start >= 0 && y_start <= y_end && y_start <= image_height);
    assert(y_end >= 0 && y_end >= y_start && y_end <= image_height);

    for (u16 i = y_start; i < y_end; ++i) {
        pixels[i*image_width + x] = color;
    }
}

void draw_hor_line(u32 *pixels, u16 image_width, u16 image_height, u16 x_start, u16 x_end, u16 y, u32 color) {
    assert(y >= 0 && y <= image_height);
    assert(x_start >= 0 && x_start <= x_end && x_start <= image_width);
    assert(x_end >= 0 && x_end >= x_start && x_end <= image_width);

    for (u16 i = x_start; i < x_end; ++i) {
        pixels[y*image_width + i] = color;
    }
}

void draw_vert_strip(u32 *pixels, u16 image_width, u16 image_height, u16 x_start, u16 x_end, u16 y_start, u16 y_end, u32 color) {
    assert(x_start >= 0 && x_start <= x_end && x_start <= image_width);
    assert(x_end >= 0 && x_end >= x_start && x_end <= image_width);
    assert(y_start >= 0 && y_start <= y_end && y_start <= image_height);
    assert(y_end >= 0 && y_end >= y_start && y_end <= image_height);

    for (u16 i = x_start; i < x_end; ++i) {
        draw_vert_line(pixels, image_width, image_height, i, y_start, y_end, color);
    }
}

v2 ray_dir(float plane_scale) {
    return addv2(p.dir, scale(p.plane, plane_scale));
}

ray_intersection ray_intersect(float plane_scale) {
    v2 dir = ray_dir(plane_scale);
    i8 stepx = SIGN(dir.x);
    i8 stepy = SIGN(dir.y);
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

        if (!(mapy >= 0 && mapy < MAP_HEIGHT) ||
            !(mapx >= 0 && mapx < MAP_WIDTH)) break;

        if (map[mapy][mapx] > 1) break;
    }

    float wall_dist = side == 1 ? raylen_y - raylen_per_y : raylen_x - raylen_per_x;
    if (wall_dist == 0) wall_dist = 1e-10;
    assert(wall_dist > 0);
    return (ray_intersection) {wall_dist, side, (v2) {mapx, mapy}};
}

void render_2d_raycast() {
    u16 length = MAP_VIEW_HEIGHT;
    u16 ratio = length / MAP_WIDTH;
    u16 y_offset = BOUNDARY;
    u16 x_offset = BOUNDARY;

    u16 px = p.pos.x * ratio + x_offset;
    u16 py = p.pos.y * ratio + y_offset;

    for (u16 i = 0; i < MAP_IMAGE_HEIGHT; ++i) {
        for (u16 j = 0; j < MAP_IMAGE_WIDTH; ++j) {
            if ( (i >= y_offset && i < MAP_IMAGE_HEIGHT-y_offset) &&
                 (j >= x_offset && j < MAP_IMAGE_WIDTH-x_offset) ) continue;
            map_pixels[i*MAP_IMAGE_WIDTH + j] = 0xFFA98467;
        }
    }

    for (u16 i = y_offset; i < MAP_IMAGE_HEIGHT-y_offset; ++i) {
        for (u16 j = x_offset; j < MAP_IMAGE_WIDTH-x_offset; ++j) {
            map_pixels[i*MAP_IMAGE_WIDTH + j] = colors[map[(u16) (i - y_offset)/ratio][(u16) (j - x_offset)/ratio]];
        }
    }

    v2 ray_end;
    u8 r = 1;
    ray_intersection ri;
    for (u16 i = py+r; i < py+ratio/3-r; ++i) {
        for (u16 j = px+r; j < px+ratio/3-r; ++j) {
            for (float ps = -1; ps <= 1; ps+=0.01) {
                ri = ray_intersect(ps);
                ray_end = scale(ray_dir(ps), ri.ray_len * ratio);
                draw_line(map_pixels, MAP_IMAGE_WIDTH, MAP_IMAGE_HEIGHT, j, px + (i16) ray_end.x, i, py + (i16) ray_end.y, 0xFFFCF6BD);
            }
        }
    }

    for (u16 i = py; i < py+ratio/3; ++i) {
        for (u16 j = px; j < px+ratio/3; ++j) {
            map_pixels[i*MAP_IMAGE_WIDTH + j] = 0xFF8D99AE;
        }
    }


    SDL_UpdateTexture(map_texture, NULL, map_pixels, sizeof(u32) * MAP_IMAGE_WIDTH);
    SDL_RenderSetViewport(renderer, &map_viewport);
    SDL_RenderCopy(renderer, map_texture, NULL, NULL);
}

void render_3d_raycast() {
    for (u16 i = 0; i < IMAGE_WIDTH; ++i) {
        float pane = 2.0 * (float) i / IMAGE_WIDTH - 1;
        ray_intersection ri = ray_intersect(pane);
        u16 height = (u16) IMAGE_HEIGHT / ri.ray_len;
        bool outside = (ri.intersect_loc.y < 0 || ri.intersect_loc.y >= MAP_HEIGHT || 
                        ri.intersect_loc.x < 0 || ri.intersect_loc.x >= MAP_WIDTH);
        u32 color;
        if (outside) color = SKY_COLOR;
        else color = colors[map[(u16)ri.intersect_loc.y][(u16)ri.intersect_loc.x]];
        if (!outside && ri.hit_side == 1) {
            u32 r = color & 0xFF0000, g = color & 0x00FF00, b = color & 0x0000FF;
            color = 0xFF000000 | (r * 0xED / 0xFF) | (g * 0xFA / 0xFF) | (b * 0xFD / 0xFF);

        }

        u16 wall_start = BOUND(IMAGE_HEIGHT/2 - height/2 + p.z, 0, IMAGE_HEIGHT);
        u16 wall_end = BOUND(IMAGE_HEIGHT/2 - height/2 + p.z + height, 0, IMAGE_HEIGHT);
        draw_vert_line(pixels, IMAGE_WIDTH, IMAGE_HEIGHT, i, 0, wall_start, SKY_COLOR);
        draw_vert_line(pixels, IMAGE_WIDTH, IMAGE_HEIGHT, i, wall_start, wall_end, color);
        draw_vert_line(pixels, IMAGE_WIDTH, IMAGE_HEIGHT, i, wall_end, IMAGE_HEIGHT, GROUND_COLOR);
    }
    SDL_UpdateTexture(texture, NULL, pixels, sizeof(u32) * IMAGE_WIDTH);
    SDL_RenderSetViewport(renderer, &full_viewport);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

int main(int argc, char **argv) {
    p = (player) { (v2){16, 4}, (v2){0, 1}, (v2){-1, 0}, 0 };

    window = SDL_CreateWindow("Tinyraycast Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                               SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    texture = SDL_CreateTexture(renderer, 
                                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
                                IMAGE_WIDTH, IMAGE_HEIGHT);
    map_texture = SDL_CreateTexture(renderer, 
                                    SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 
                                    MAP_IMAGE_WIDTH, MAP_IMAGE_HEIGHT);
    SDL_Event event;
    i8 z_count = 0, editor_color = 4;
    bool quit = false, view_map = false, editor_mode = false, mouse_pressed = false;
    float rotate_speed = M_PI / 16.0, move_speed = 0.1, z_speed = 100;
    while (1) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }
            if (event.type == SDL_MOUSEBUTTONUP) mouse_pressed = false;
            if (event.type == SDL_MOUSEBUTTONDOWN) mouse_pressed = true;
            if (event.type == SDL_MOUSEMOTION) {
                if (mouse_pressed) {
                    u16 mousex = event.motion.x, mousey = event.motion.y;
                    if (view_map && editor_mode &&
                        INBOUND(mousex, BOUNDARY, MAP_IMAGE_WIDTH-BOUNDARY-1) &&
                        INBOUND(mousey, BOUNDARY, MAP_IMAGE_HEIGHT-BOUNDARY-1)) {
                        u8 mapx = (mousex - BOUNDARY) / (MAP_VIEW_WIDTH / MAP_WIDTH);
                        u8 mapy = (mousey - BOUNDARY) / (MAP_VIEW_HEIGHT / MAP_HEIGHT);
                        if (mapy == (u16)p.pos.y && mapx == (u16)p.pos.x) continue;
                        map[mapy][mapx] = editor_color;
                    }
                }
            }
            if (event.type == SDL_KEYDOWN) {
                u8 sym = event.key.keysym.scancode;
                if (sym == SDL_SCANCODE_ESCAPE) {
                    quit = true;
                    break;
                }

                if (sym == SDL_SCANCODE_SPACE) {
                    view_map ^= 1;
                } else if (sym == SDL_SCANCODE_E) {
                    editor_mode = true;
                } else if (sym == SDL_SCANCODE_N) {
                    editor_mode = false;
                }

                float nx = p.pos.x, ny = p.pos.y;
                if (sym == SDL_SCANCODE_LEFT) {
                    nx = p.pos.x - p.plane.x * move_speed;
                    ny = p.pos.y - p.plane.y * move_speed;
                } else if (sym == SDL_SCANCODE_RIGHT) {
                    nx = p.pos.x + p.plane.x * move_speed;
                    ny = p.pos.y + p.plane.y * move_speed;
                } else if (sym == SDL_SCANCODE_UP) {
                    nx = p.pos.x + p.dir.x * move_speed;
                    ny = p.pos.y + p.dir.y * move_speed;
                } else if (sym == SDL_SCANCODE_DOWN) {
                    nx = p.pos.x - p.dir.x * move_speed;
                    ny = p.pos.y - p.dir.y * move_speed;
                } else if (sym == SDL_SCANCODE_W && z_count < 3) {
                    p.z += z_speed;
                    z_count++;
                } else if (sym == SDL_SCANCODE_S && z_count > -3) {
                    p.z -= z_speed;
                    z_count--;
                } else if (sym == SDL_SCANCODE_A) {
                    p.dir = rotate(p.dir, rotate_speed);
                    p.plane = rotate(p.plane, rotate_speed);
                } else if (sym == SDL_SCANCODE_D) {
                    p.dir = rotate(p.dir, -rotate_speed);
                    p.plane = rotate(p.plane, -rotate_speed);
                }
                if (map[(u16) ny][(u16) nx] == 0) {
                    p.pos.x = nx;
                    p.pos.y = ny;
                }

                if (!editor_mode) continue;
                if (sym == SDL_SCANCODE_1) {
                    editor_color = 0;
                } else if (sym == SDL_SCANCODE_2) {
                    editor_color = 2;
                } else if (sym == SDL_SCANCODE_3) {
                    editor_color = 3;
                } else if (sym == SDL_SCANCODE_4) {
                    editor_color = 4;
                } 
            }
        }
        if (quit) break;

        memset(pixels, 0, sizeof(u32) * IMAGE_SIZE);
        memset(map_pixels, 0, sizeof(u32) * MAP_IMAGE_SIZE);
        SDL_RenderSetViewport(renderer, &full_viewport);
        SDL_RenderClear(renderer);

        render_3d_raycast();
        if (view_map) render_2d_raycast();
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
