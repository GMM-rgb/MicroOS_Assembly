#ifndef MICROOS_H
#define MICROOS_H

#include <SDL.h>
#include <stdbool.h>

#define MENU_ITEM_COUNT 4
#define MENU_ANIMATION_DURATION 500

typedef struct {
    float x;
    float y;
    float target_x;
    float target_y;
    float velocity;
    bool is_visible;
} MenuItemAnimation;

typedef struct {
    float current_height;
    float target_height;
    float start_height;
    Uint32 animation_start;
    bool is_animating;
} MenuAnimation;

// New settings UI state structure
typedef struct {
    bool display_section_expanded;
    bool sound_section_expanded;
    bool network_section_expanded;
    int scroll_position;
    float animation_progress;
} SettingsUIState;

void draw_rounded_rect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);
void draw_rounded_rect_with_shadow(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);

// Add declaration for calculate_settings_content_height
int calculate_settings_content_height(void);

#endif // MICROOS_H
