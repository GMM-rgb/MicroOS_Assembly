#ifndef MICROOS_SETTINGS_MENU_H
#define MICROOS_SETTINGS_MENU_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "microos.h"
#include "settings.h" // For SystemSettings

// Draws a small sidebar from the left with resolution and UI scale options.
void draw_settings_menu(SDL_Renderer* renderer, TTF_Font* font, SystemSettings* settings, SDL_Rect window);

#endif // MICROOS_SETTINGS_MENU_H
