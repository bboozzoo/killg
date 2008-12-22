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
#include <SDL/SDL_image.h>
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

GLuint ground_texture = 0;
SDL_Surface * surface = NULL;

void sig_handle(int signal) {
    do_run = false;
}

void draw_ground(void) {
    glPushMatrix();
    glLoadIdentity();
    glBindTexture(GL_TEXTURE_2D, ground_texture);
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
void draw_ground_notex(void) {
    glPushMatrix();
    glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3ub(0, 100, 0);
    glVertex3d(0, 0, 0);
    glColor3ub(0, 100, 0);
    glVertex3d(VIDEO_W, 0, 0);
    glColor3ub(0, 100, 0);
    glVertex3d(VIDEO_W, VIDEO_H, 0);
    glColor3ub(0, 100, 0);
    glVertex3d(0, VIDEO_H, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
}

int load_ground_texture(void) {
/*
    SDL_Surface * ground_img = IMG_Load(GROUND_IMG);
    SDL_Surface * temp_img = NULL;
    SDL_PixelFormat * pixel_format = surface->format; 
    LOG_INFO("bytes per pixel: " << pixel_format->BytesPerPixel);
    if (ground_img == NULL) {
        LOG_ERR("failed to load ground texture: " << IMG_GetError());
        return -1;
    }

    temp_img = SDL_CreateRGBSurface(SDL_SWSURFACE, ground_img->w, ground_img->h, 32, 
                                              pixel_format->Bmask, pixel_format->Gmask, pixel_format->Rmask, pixel_format->Amask );

    SDL_SetAlpha(ground_img, 0, 0);
    SDL_BlitSurface(ground_img, NULL, temp_img, NULL);

    glGenTextures(1, &ground_texture);
    glBindTexture(GL_TEXTURE_2D, ground_texture);
    LOG_INFO("ground texture: " << ground_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, temp_img->w, temp_img->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp_img->pixels );

    SDL_FreeSurface(temp_img);
*/
/*    glTexImage2D(GL_TEXTURE_2D, 0, 3, ground_img->w, ground_img->h, 0, GL_RGB, GL_UNSIGNED_BYTE, ground_img->pixels);*/
/*    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
*/
 /*   SDL_FreeSurface(ground_img); */
    ground_texture = SOIL_load_OGL_texture(GROUND_IMG, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
    if (0 == ground_texture) {
        LOG_ERR("failed to load texture file: " << SOIL_last_result());
        return -1;
    }   
    return 0;
}

void draw_player(uint32_t x, uint32_t y, double direction_angle) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    glRotated(direction_angle, 0, 0, 1);
    glBegin(GL_TRIANGLES);
    glColor3ub(255, 0, 0);
    glVertex2d(-10, 5);
    glColor3ub(0, 255, 0);
    glVertex2d(10, 5);
    glColor3ub(0, 0, 255);
    glVertex2d(0, -10);
    glEnd();
    glPopMatrix();
}

void draw_arrow(uint32_t x, uint32_t y, double direction_angle) {
    /*double direction_angle = atan2(point_to_y - y, point_to_x - x) * 180.0 / M_PI + 90.0;*/

    /*LOG_INFO("pointer x: " << point_to_x << " y: " << point_to_y << " player x: " << x << " y: " << y);*/
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    glRotated(direction_angle, 0, 0, 1);
    glTranslatef(0, -50, 0);
    glBegin(GL_POLYGON);
    glColor3ub(0, 0, 0);
    glVertex2d(0, - 10);
    glVertex2d(- 10, 0);
    glVertex2d(- 5, 0);
    glVertex2d(- 5, 10);
    glVertex2d(5, 10);
    glVertex2d(5, 0);
    glVertex2d(10, 0);
/*    LOG_INFO("center x: " << center_x << " y: " << center_y << " p2 x: " << point_to_x << " p2 y: " << point_to_y << " atan: " << atan((double) (point_to_y - center_y) / (double) (point_to_x / center_x))); */
    /*glRotated(atan((double) (point_to_y - center_y) / (double) (point_to_x / center_x)), 0, 0, 1);*/
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
}

void set2D(void) {
/*    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, VIDEO_W, VIDEO_H, 0, 0, 1); */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void unset2D(void) {
/*    glMatrixMode(GL_PROJECTION);
    glPopMatrix(); */
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

int main (int argc, char * argv[]) {
    uint32_t video_flags = 0x0;
    const SDL_VideoInfo * video_info = NULL;

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

    /*
    player_img = IMG_Load(PLAYER_IMG);
    if (NULL != player_img) {
        LOG_INFO("player image ready, width: " << player_img->w << " height: " << player_img->h);

    } else {
        LOG_ERR("player image loading error: " << IMG_GetError());
        goto cleanup;
    }
*/
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

    if (load_ground_texture() != 0) {
        LOG_ERR("error loading ground texture");
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
        }
        unset2D();
        /* fix this ^^^^^ */
        SDL_GL_SwapBuffers();

        frames++;
        if (SDL_GetTicks() - last_tick > 5000) {
            uint32_t now = SDL_GetTicks();
            float fps = frames / ((double) (now - last_tick) / 1000.0);
            LOG_INFO("FPS: " << fps);
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
