/*
* Copyright (C) 2008  Maciek Borzecki
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <signal.h>
#include <cmath>
#include "logger.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <GL/gl.h>
#include <SOIL/SOIL.h>

#define VIDEO_W 800
#define VIDEO_H 600
#define VIDEO_BPP 32

#define KEY_UP SDLK_w
#define KEY_DOWN SDLK_s
#define KEY_LEFT SDLK_a
#define KEY_RIGHT SDLK_d

#define AUDIO_RATE 22050
#define AUDIO_FORMAT AUDIO_S16SYS
#define AUDIO_CHANNELS 1
#define AUDIO_BUFFERS 4096

#define SHOOT_SND "data/shoot.wav"
#define RELOAD_SND "data/reload.wav"

#define PLAYER_IMG "data/player.png"
#define GROUND_IMG "data/ground.png"
#define CROSS_IMG "data/cross.png"
#define ARROW_IMG "data/arrow.png"
#define MONSTER_IMG "data/monster.png"


/* static stuff */
volatile bool do_run = false;
Mix_Chunk * shoot_snd = NULL;
Mix_Chunk * reload_snd = NULL;
SDL_Surface * player_img = NULL;
int32_t player_x = 0;
int32_t player_y = 0;
int32_t player_moving_x = 0;
int32_t player_moving_y = 0;
int32_t player_angle = 0;
uint32_t pointer_x = 0;
uint32_t pointer_y = 0;
uint32_t last_tick = 0;
double center_x = VIDEO_W / 2;
double center_y = VIDEO_H / 2;
uint32_t frames = 0;

typedef enum {
    TEXTURE_GROUND = 0,
    TEXTURE_CROSS,
    TEXTURE_ARROW,
    TEXTURE_PLAYER,
    TEXTURE_MONSTER,
    TEXTURE_MAX
} texture_id_t;

struct texture_info_s {
    texture_id_t id;
    const char * file;
} texture_info[TEXTURE_MAX + 1] = { 
                    { TEXTURE_GROUND, GROUND_IMG },
                    { TEXTURE_CROSS, CROSS_IMG },
                    { TEXTURE_ARROW, ARROW_IMG },
                    { TEXTURE_PLAYER, PLAYER_IMG},
                    { TEXTURE_MONSTER, MONSTER_IMG},
                    { (texture_id_t) 0, NULL } 
                    };

uint32_t textures[TEXTURE_MAX] = {0};


void sig_handle(int signal) {
    do_run = false;
}

void draw_ground(void) {
    glPushMatrix();
    glLoadIdentity();
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_GROUND]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); 
    glVertex3d(0, 0, 0);
    glTexCoord2f(1.0, 0); 
    glVertex3d(VIDEO_W, 0, 0);
    glTexCoord2f(1.0, 1.0); 
    glVertex3d(VIDEO_W, VIDEO_H, 0);
    glTexCoord2f(0, 1.0); 
    glVertex3d(0, VIDEO_H, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
}

int load_texture(const char * file_name, uint32_t * texture_id) {
    *texture_id = SOIL_load_OGL_texture(file_name, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    if (0 == *texture_id ) {
        LOG_ERR("failed to load texture file: " << SOIL_last_result());
        return -1;
    }   
    return 0;
}

int load_all_textures(void) {
    for (int i = 0; i < TEXTURE_MAX; i++) {
        if (NULL == texture_info[i].file)
            break;
        if (-1 == load_texture(texture_info[i].file, &textures[texture_info[i].id])) {
            LOG_ERR("failed to load texture file: " << texture_info[i].file);
            return -1;
        }
    }
    return 0;
}

void draw_player(uint32_t x, uint32_t y, double direction_angle) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    glRotated(direction_angle, 0, 0, 1);
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_PLAYER]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2d(-15, 15);
    glTexCoord2f(1.0, 0);
    glVertex2d(15, 15);
    glTexCoord2f(1.0, 1.0);
    glVertex2d(15, -15);
    glTexCoord2f(0, 1.0);
    glVertex2d(-15, -15);
    glEnd();
    glPopMatrix();
}

void draw_cross(uint32_t x, uint32_t y) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    /*
    glRotated(direction_angle, 0, 0, 1);
    */
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_CROSS]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2d(-15, -15);
    glTexCoord2f(1.0, 0);
    glVertex2d(15, -15);
    glTexCoord2f(1.0, 1.0);
    glVertex2d(15, 15);
    glTexCoord2f(0, 1.0);
    glVertex2d(-15, 15);
    glEnd();
    glPopMatrix();
}

void draw_arrow(uint32_t x, uint32_t y, double direction_angle) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    glRotated(direction_angle, 0, 0, 1);
    glTranslatef(0, -50, 0);
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_ARROW]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1.0);
    glVertex2d(-10, -10);
    glTexCoord2f(1.0, 1.0);
    glVertex2d(10, -10);
    glTexCoord2f(1.0, 0);
    glVertex2d(10, 10);
    glTexCoord2f(0, 0);
    glVertex2d(-10, 10);
    glEnd();
    glPopMatrix();
}

void draw_monster(uint32_t x, uint32_t y, double direction_angle) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    glRotated(direction_angle, 0, 0, 1);
    glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_MONSTER]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1.0);
    glVertex2d(-10, -10);
    glTexCoord2f(1.0, 1.0);
    glVertex2d(10, -10);
    glTexCoord2f(1.0, 0);
    glVertex2d(10, 10);
    glTexCoord2f(0, 0);
    glVertex2d(-10, 10);
    glEnd();
    glPopMatrix();
}

void init_GL_attrs(void) {
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
}

void init_GL(void) {
    glShadeModel(GL_SMOOTH);
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    /*glHint .. any? */
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_TEXTURE_2D);
    /*glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, 1.0);*/
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void set2D(void) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void unset2D(void) {
    /*
    glMatrixMode(GL_MODELVIEW); 
    */
    glPopMatrix();
}

int main (int argc, char * argv[]) {
    uint32_t video_flags = 0x0;
    const SDL_VideoInfo * video_info = NULL;
    SDL_Surface * surface = NULL;

    signal(SIGINT, sig_handle);
    signal(SIGTERM, sig_handle);

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0) {
        LOG_ERR("SDL init failed: " << SDL_GetError());
        return 0;
    }

    video_info = SDL_GetVideoInfo();
    LOG_INFO("video mem (kb): " << video_info->video_mem);

    video_flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;
    if (video_info->hw_available) {
        LOG_INFO("VIDEO: surface stored in HW");
        video_flags |= SDL_HWSURFACE;
    } else {
        LOG_WARN("VIDEO: surface stored in SW");
        video_flags |= SDL_SWSURFACE;
    }
    
    if (video_info->blit_hw) {
        LOG_INFO("VIDEO: HW accelerated blit");
        video_flags |= SDL_HWACCEL;
    } else {
        LOG_WARN("VIDEO: no HW accelerated blit");
    }

    init_GL_attrs();
    surface = SDL_SetVideoMode(VIDEO_W, VIDEO_H, VIDEO_BPP, video_flags);
    if (NULL != surface) {
        LOG_INFO("surface ready");
    } else {
        LOG_ERR("SDL surface failed: " << SDL_GetError());
        goto cleanup;
    }
    init_GL();
    SDL_ShowCursor(SDL_DISABLE);

    if (Mix_OpenAudio(AUDIO_RATE, AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_BUFFERS) != 0) {
        LOG_ERR("mixer not open: " << Mix_GetError());
        goto cleanup;
    } else {
        LOG_INFO("mixer open");
        shoot_snd = Mix_LoadWAV(SHOOT_SND);
        if (NULL == shoot_snd) 
            LOG_ERR("failed to load: " << SHOOT_SND << ": " << Mix_GetError());
        reload_snd = Mix_LoadWAV(RELOAD_SND);
        if (NULL == reload_snd)
            LOG_ERR("failed to load: " << RELOAD_SND << ": " << Mix_GetError());
    }

    if (load_all_textures() != 0) {
        LOG_ERR("error loading textures");
        goto cleanup;
    }

    player_x = surface->w / 2.0; /* - player_img->w / 2.0;*/
    player_y = surface->h / 2.0; /* - player_img->h / 2.0; */
    do_run = true;

    last_tick = SDL_GetTicks();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, VIDEO_W, VIDEO_H, 0, 0, 1);
    while (do_run) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_KEYDOWN:
                    LOG_INFO("key down, key: " << ev.key.keysym.sym);
                    switch (ev.key.keysym.sym) {
                        case KEY_UP:
                            LOG_INFO("up");
                            player_moving_y -= 1;
                            break;
                        case KEY_DOWN:
                            LOG_INFO("down");
                            player_moving_y += 1;
                            break;
                        case KEY_LEFT:
                            LOG_INFO("left");
                            player_moving_x += -1;
                            break;
                        case KEY_RIGHT:
                            LOG_INFO("right");
                            player_moving_x += 1;
                            break;
                        default:
                            LOG_INFO("other");
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    LOG_INFO("key up, key: " << ev.key.keysym.sym);
                    switch (ev.key.keysym.sym) {
                        case KEY_UP:
                            LOG_INFO("up");
                            player_moving_y += 1;
                            break;
                        case KEY_DOWN:
                            LOG_INFO("down");
                            player_moving_y -= 1;
                            break;
                        case KEY_LEFT:
                            LOG_INFO("left");
                            player_moving_x -= -1;
                            break;
                        case KEY_RIGHT:
                            LOG_INFO("right");
                            player_moving_x -= 1;
                            break;
                        default:
                            LOG_INFO("other");
                            break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    LOG_INFO("mouse motion, x: " << ev.motion.x << " y: " << ev.motion.y);
                    pointer_x = ev.motion.x;
                    pointer_y = ev.motion.y;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    if (SDL_MOUSEBUTTONDOWN == ev.type)
                        LOG_INFO("mouse button down");
                    else
                        LOG_INFO("mouse button up");
                    switch (ev.button.button) {
                        case SDL_BUTTON_LEFT:
                            LOG_INFO("left");
                            if (SDL_MOUSEBUTTONDOWN == ev.type && NULL != shoot_snd) {
                                if (-1 == Mix_PlayChannel(-1, shoot_snd, 0))
                                    LOG_ERR("cannot play shoot sound: " << Mix_GetError());
                            }
                            break;
                        case SDL_BUTTON_RIGHT:
                            LOG_INFO("right");
                            break;
                        case SDL_BUTTON_MIDDLE:
                            LOG_INFO("middle");
                            if (SDL_MOUSEBUTTONDOWN == ev.type && NULL != reload_snd) 
                                if (-1 ==Mix_PlayChannel(-1, reload_snd, 0)) {
                                    LOG_ERR("cannot play reload sound: " << Mix_GetError());
                                }
                            break;
                    }
                    break;
                case SDL_QUIT:
                    LOG_INFO("quit request");
                    do_run = false;
                    break;
                default:
                    LOG_INFO("other");
                    break;
            }
        }

        player_x += player_moving_x;
        player_y += player_moving_y;
        if (player_x > VIDEO_W) {
            LOG_INFO("player -> far right end");
            player_x = VIDEO_W;
        } else if (player_x < 0) {
            LOG_INFO("player -> far left end");
            player_x = 0;
        }

        if (player_y > VIDEO_H) {
            LOG_INFO("player -> far bottom end");
            player_y = VIDEO_H;
        } else if (player_y < 0) {
            LOG_INFO("player -> far top end");
            player_y = 0;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        set2D(); 
        {
            double direction_angle = atan2((double) pointer_y - player_y, (double)pointer_x - player_x) * 180.0 / M_PI + 90.0;
            draw_ground();
            draw_player(player_x, player_y, direction_angle);
            draw_arrow(player_x, player_y, direction_angle);
            draw_monster(400, 400, 45);
            draw_cross(pointer_x, pointer_y);
        }
        unset2D();
        /* fix this ^^^^^ */
        SDL_GL_SwapBuffers();

        frames++;
        if (SDL_GetTicks() - last_tick > 5000) {
            uint32_t now = SDL_GetTicks();
            float fps = frames / ((double) (now - last_tick) / 1000.0);
            LOG_STAT("FPS: " << fps);
            last_tick = now;
            frames = 0;
        }
    }
cleanup:
    LOG_INFO("quit");
    if (NULL != player_img)
        SDL_FreeSurface(player_img);
    if (NULL != shoot_snd)
        Mix_FreeChunk(shoot_snd);
    if (NULL != reload_snd)
        Mix_FreeChunk(reload_snd);
    Mix_CloseAudio();
    SDL_Quit();
    return 0;
}
