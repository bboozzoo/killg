#include <signal.h>
#include "logger.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>

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
uint32_t player_x = 0;
uint32_t player_y = 0;
uint32_t player_moving_x = 0;
uint32_t player_moving_y = 0;
uint32_t last_tick = 0;

void sig_handle(int signal) {
    do_run = false;
}

void draw_img(SDL_Surface * img, SDL_Surface * on_what, uint32_t x, uint32_t y) {
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    SDL_BlitSurface(img, NULL, on_what, &rect);
}

int main (int argc, char * argv[]) {
    SDL_Surface * surface = NULL;

    signal(SIGINT, sig_handle);
    signal(SIGTERM, sig_handle);

    LOG_INFO("ala ma kota");
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0) {
        LOG_ERR("SDL init failed: " << SDL_GetError());
        return 0;
    }

    surface = SDL_SetVideoMode(VIDEO_W, VIDEO_H, VIDEO_BPP, SDL_HWSURFACE|SDL_DOUBLEBUF);
    if (NULL != surface) {
        LOG_INFO("surface ready");
    } else {
        LOG_ERR("SDL surface failed: " << SDL_GetError());
        goto cleanup;
    }

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
        LOG_ERR("mixer open");
        shoot_snd = Mix_LoadWAV(SHOOT_SND);
        if (NULL == shoot_snd) 
            LOG_ERR("failed to load: " << SHOOT_SND << ": " << Mix_GetError());
        reload_snd = Mix_LoadWAV(RELOAD_SND);
        if (NULL == reload_snd)
            LOG_ERR("failed to load: " << RELOAD_SND << ": " << Mix_GetError());
    }

    player_x = surface->w / 2.0 - player_img->w / 2.0;
    player_y = surface->h / 2.0 - player_img->h / 2.0;
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));
    draw_img(player_img, surface, player_x, player_y);
    do_run = true;

    last_tick = SDL_GetTicks();
    while (do_run) {
        SDL_Event ev;
        //LOG_INFO("poll event");
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if (SDL_KEYDOWN == ev.type) {
                        LOG_INFO("key down, key: " << ev.key.keysym.sym);
                        switch (ev.key.keysym.sym) {
                            case KEY_UP:
                                LOG_INFO("up");
                                player_moving_y = 1;
                                break;
                            case KEY_DOWN:
                                LOG_INFO("down");
                                player_moving_y = -1;
                                break;
                            case KEY_LEFT:
                                LOG_INFO("left");
                                player_moving_x = -1;
                                break;
                            case KEY_RIGHT:
                                LOG_INFO("right");
                                player_moving_x = 1;
                                break;
                            default:
                                LOG_INFO("other");
                                break;
                        }
                    } else {
                        LOG_INFO("key up, key: " << ev.key.keysym.sym);
                        if (KEY_DOWN == ev.key.keysym.sym || KEY_UP == ev.key.keysym.sym)
                            player_moving_y = 0;
                        else if (KEY_LEFT == ev.key.keysym.sym || KEY_RIGHT == ev.key.keysym.sym)
                            player_moving_x = 0;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    LOG_INFO("mouse motion, x: " << ev.motion.x << " y: " << ev.motion.y);
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
        if (SDL_GetTicks() - last_tick > 500) {
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 255, 255));
            player_x += player_moving_x;
            player_y -= player_moving_y;
            draw_img(player_img, surface, player_x, player_y);
            SDL_Flip(surface);
        }
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
