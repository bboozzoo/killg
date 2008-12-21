#include <signal.h>
#include <cmath>
#include "logger.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <GL/gl.h>

#define VIDEO_W 640
#define VIDEO_H 480
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


/* static stuff */
volatile bool do_run = false;
Mix_Chunk * shoot_snd = NULL;
Mix_Chunk * reload_snd = NULL;
SDL_Surface * player_img = NULL;
int32_t player_x = 0;
int32_t player_y = 0;
int32_t player_moving_x = 0;
int32_t player_moving_y = 0;
uint32_t pointer_x = 0;
uint32_t pointer_y = 0;
uint32_t last_tick = 0;
double center_x = VIDEO_W / 2;
double center_y = VIDEO_H / 2;

void sig_handle(int signal) {
    do_run = false;
}

void draw_img(SDL_Surface * img, SDL_Surface * on_what, uint32_t x, uint32_t y) {
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    SDL_BlitSurface(img, NULL, on_what, &rect);
}

void draw_player(uint32_t x, uint32_t y) {
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x, y, 0);
    glBegin(GL_QUADS);
    glColor3ub(255, 0, 0);
    glVertex2d(-10, -10);
    glColor3ub(0, 255, 0);
    glVertex2d(10, -10);
    glColor3ub(0, 0, 255);
    glVertex2d(10, 10);
    glColor3ub(0, 255, 0);
    glVertex2d(-10, 10);
    glEnd();
    glPopMatrix();
}

void draw_arrow(uint32_t x, uint32_t y, double point_to_x, double point_to_y) {
    double direction_angle = atan2(point_to_y - y, point_to_x - x) * 180.0 / M_PI + 90.0;

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
    /*glEnable(GL_DEPTH_TEST);*/
    /*glDepthFunc(GL_LEQUAL);*/
    /*glHint .. any? */
}

void set2D(void) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, VIDEO_W, VIDEO_H, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void unset2D(void) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

int main (int argc, char * argv[]) {
    SDL_Surface * surface = NULL;
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

    player_img = IMG_Load(PLAYER_IMG);
    if (NULL != player_img) {
        LOG_INFO("player image ready, width: " << player_img->w << " height: " << player_img->h);

    } else {
        LOG_ERR("player image loading error: " << IMG_GetError());
        goto cleanup;
    }

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

    player_x = surface->w / 2.0 - player_img->w / 2.0;
    player_y = surface->h / 2.0 - player_img->h / 2.0;
    /*SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));
    draw_img(player_img, surface, player_x, player_y);*/
    do_run = true;

    last_tick = SDL_GetTicks();
    while (do_run) {
        SDL_Event ev;
        //LOG_INFO("poll event");
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_KEYDOWN:
                    LOG_INFO("key down, key: " << ev.key.keysym.sym);
                    switch (ev.key.keysym.sym) {
                        case KEY_UP:
                            LOG_INFO("up");
                            player_moving_y += 1;
                            break;
                        case KEY_DOWN:
                            LOG_INFO("down");
                            player_moving_y += -1;
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
                            player_moving_y -= 1;
                            break;
                        case KEY_DOWN:
                            LOG_INFO("down");
                            player_moving_y -= -1;
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
        /*if (SDL_GetTicks() - last_tick > 50) {
            last_tick = SDL_GetTicks(); */
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            /*SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));*/
            player_x += player_moving_x;
            player_y -= player_moving_y;
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

            set2D();
            /*draw_img(player_img, surface, player_x, player_y);*/
            draw_player(player_x, player_y);
            draw_arrow(player_x, player_y, pointer_x, pointer_y);
            unset2D();
            /* fix this ^^^^^ */
            SDL_GL_SwapBuffers();
       /* }*/
        //LOG_INFO("poll finished");
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
