#include "settings.h"
#include <stdio.h>

static const SDL_Color THEME_DEFAULT_COLORS[] = {
    {0, 100, 170, 255},  // bg
    {0, 120, 215, 255},  // accent
    {255, 255, 255, 255} // text
};

static const SDL_Color THEME_CYAN_COLORS[] = {
    {0, 150, 170, 255},  // bg
    {0, 200, 215, 255},  // accent
    {255, 255, 255, 255} // text
};

static const SDL_Color THEME_GRAY_COLORS[] = {
    {70, 70, 70, 255},   // bg
    {100, 100, 100, 255},// accent
    {220, 220, 220, 255} // text
};

static const int RESOLUTION_WIDTH[] = {256, 640, 854, 1280, 1920};
static const int RESOLUTION_HEIGHT[] = {144, 360, 480, 720, 1080};

void settings_init(SystemSettings* settings) {
    settings->theme = THEME_DEFAULT;
    settings->resolution = RES_360P;
    settings->ui_scale = 1.0f;
    settings_apply_theme(settings);
    settings->window_width = RESOLUTION_WIDTH[settings->resolution];
    settings->window_height = RESOLUTION_HEIGHT[settings->resolution];
}

void settings_apply_theme(SystemSettings* settings) {
    const SDL_Color* colors;
    switch (settings->theme) {
        case THEME_CYAN:
            colors = THEME_CYAN_COLORS;
            break;
        case THEME_GRAYSCALE:
            colors = THEME_GRAY_COLORS;
            break;
        default:
            colors = THEME_DEFAULT_COLORS;
            break;
    }
    settings->bg_color = colors[0];
    settings->accent_color = colors[1];
    settings->text_color = colors[2];
}

void settings_apply_resolution(SystemSettings* settings, SDL_Window* window) {
    settings->window_width = RESOLUTION_WIDTH[settings->resolution];
    settings->window_height = RESOLUTION_HEIGHT[settings->resolution];
    SDL_SetWindowSize(window, 
        settings->window_width * settings->ui_scale, 
        settings->window_height * settings->ui_scale);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}
