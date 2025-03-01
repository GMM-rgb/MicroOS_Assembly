#ifndef MICROOS_H
#define MICROOS_H

#include <SDL.h>
#include <stdbool.h>

typedef struct {
    float current_height;
    float target_height;
    float start_height;
    Uint32 animation_start;
    bool is_animating;
} MenuAnimation;

typedef struct {
    float x;
    float y;
    float target_x;
    float target_y;
    float velocity;
    bool is_visible;
} MenuItemAnimation;

#define MENU_ANIMATION_DURATION 300
#define MENU_ITEM_DELAY 100
#define MENU_ITEM_COUNT 4

// Function declarations
void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color);
void draw_rounded_rect_with_shadow(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);

#endif // MICROOS_H
