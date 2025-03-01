#ifndef MICROOS_SETTINGS_H
#define MICROOS_SETTINGS_H

#include <SDL.h>  // Include SDL header for SDL_Color and SDL_Window

typedef enum {
    THEME_DEFAULT,
    THEME_CYAN,
    THEME_GRAYSCALE
} ColorTheme;

typedef enum {
    RES_144P,   // 256x144
    RES_360P,   // 640x360
    RES_480P,   // 854x480
    RES_720P,   // 1280x720
    RES_1080P   // 1920x1080
} Resolution;

typedef struct {
    ColorTheme theme;
    Resolution resolution;
    float ui_scale;  // 1.0 = 100%, 2.0 = 200%
    SDL_Color bg_color;
    SDL_Color accent_color;
    SDL_Color text_color;
    int window_width;
    int window_height;
} SystemSettings;

void settings_init(SystemSettings* settings);
void settings_apply_theme(SystemSettings* settings);
void settings_apply_resolution(SystemSettings* settings, SDL_Window* window);
void settings_save(SystemSettings* settings);
void settings_load(SystemSettings* settings);

#endif // MICROOS_SETTINGS_H
