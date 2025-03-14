#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "filesystem.h"
#include "terminal.h"
#include "fileui.h"
#include "editor.h"  // Ensure this include is present
#include "settings.h"
#include "microos.h"
#include "settings_menu.h"  // Add new settings menu header
#include "bios.h"       // New header for bios_restart
#include "drivers.h"  // Ensure the drivers header is included near the top

// OS State
typedef enum
{
    OS_STATE_BOOT,
    OS_STATE_DESKTOP,
    OS_STATE_APP1,   // Terminal
    OS_STATE_APP2,   // Settings
    OS_STATE_APP3,   // Document Editor
    OS_STATE_MENU
} OSState;

// Application structure
typedef struct
{
    char name[32];
    SDL_Rect icon;
    SDL_Color color;
    FileSystem* fs;  // Add this field
    Terminal* terminal;
    FileUI* fileui;
    TextEditor* editor;
    SystemSettings settings;
} Application;

// Function declarations for assembly routines
extern void initialize_display_memory(unsigned char *display_memory);
extern void update_display(unsigned char *display_memory);

// Function declarations
void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color);
void draw_rounded_rect_with_shadow(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);
void render_display_memory(SDL_Renderer *renderer, unsigned char *display_memory);
void draw_taskbar(SDL_Renderer *renderer, TTF_Font *font);
void draw_terminal_icon(SDL_Renderer* renderer, SDL_Rect icon_rect);
void draw_document_icon(SDL_Renderer* renderer, SDL_Rect icon_rect);
void draw_desktop(SDL_Renderer *renderer, TTF_Font *font, Application *apps, int appCount);
void draw_boot_sequence(SDL_Renderer *renderer, TTF_Font *font, int progress, unsigned char *display_memory);
void draw_app(SDL_Renderer *renderer, TTF_Font *font, const char *appName, SDL_Color appColor, unsigned char *display_memory, FileSystem* fs, Application* apps);
void draw_start_menu(SDL_Renderer* renderer, TTF_Font* font);
void draw_settings_page(SDL_Renderer* renderer, TTF_Font* font, SDL_Rect content_area, Application* app, const char* page);
void init_menu_animations();
float ease_out_elastic(float t);

// Add these global variables after the OSState declaration
MenuAnimation menu_anim = {0, 180, 0, 0, false};
MenuItemAnimation menu_items[MENU_ITEM_COUNT] = {0};

// New global variables for settings page and sidebar state
char currentSettingsPage[16] = "";
bool showSettingsSidebar = false;

// Add new global variable for settings UI state
SettingsUIState settings_ui_state = {0};

// New global flag (set according to command–line, default false)
bool running_on_raw_hardware = false;

// Prototype for new external media detection function.
bool fs_detect_external_media(FileSystem* fs);

// Function to calculate the total height of the settings content
int calculate_settings_content_height(void) {
    int height = 60;  // Initial offset
    height += 45;  // Display section header height
    if (settings_ui_state.display_section_expanded) {
        height += 205;  // Display section content height
    }
    height += 45;  // Sound section header height
    if (settings_ui_state.sound_section_expanded) {
        height += 205;  // Sound section content height
    }
    height += 45;  // Network section header height
    if (settings_ui_state.network_section_expanded) {
        height += 205;  // Network section content height
    }
    return height;
}

void init_menu_animations() {
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        menu_items[i].x = -100;  // Start offscreen
        menu_items[i].y = 110 + i * 40;
        menu_items[i].target_x = 10;
        menu_items[i].target_y = 110 + i * 40;
        menu_items[i].velocity = 0;
        menu_items[i].is_visible = false;
    }
}

float ease_out_elastic(float t) {
    const float c4 = (2.0f * 3.14159f) / 3.0f;
    return t == 0 ? 0 : t == 1 ? 1
        : powf(2, -10 * t) * sinf((t * 10 - 0.75f) * c4) + 1;
}

void draw_rounded_rect_with_shadow(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color) {
    // Draw shadow
    SDL_Rect shadowRect = {rect.x + 2, rect.y + 2, rect.w, rect.h};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 64);
    draw_rounded_rect(renderer, shadowRect, radius, (SDL_Color){0, 0, 0, 64});
    
    // Draw main rectangle
    draw_rounded_rect(renderer, rect, radius, color);
}

// Add a function to visualize the display memory pattern
void render_display_memory(SDL_Renderer *renderer, unsigned char *display_memory)
{
    int cell_size = 10;  // Size of each display memory cell when rendered
    int display_x = 25;  // X position for display visualization in the terminal app
    int display_y = 100; // Y position for display visualization in the terminal app

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            unsigned char value = display_memory[y * 8 + x];
            SDL_Rect cell = {
                display_x + x * cell_size,
                display_y + y * cell_size,
                cell_size,
                cell_size};

            SDL_SetRenderDrawColor(renderer, value, value / 2, 255 - value, 255);
            SDL_RenderFillRect(renderer, &cell);
        }
    }
}

void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw the central rectangle
    SDL_Rect centralRect = {rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_RenderFillRect(renderer, &centralRect);

    // Draw the left and right rectangles
    SDL_Rect leftRect = {rect.x, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &leftRect);

    SDL_Rect rightRect = {rect.x + rect.w - radius, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &rightRect);

    // Draw the top and bottom rectangles
    SDL_Rect topRect = {rect.x + radius, rect.y, rect.w - 2 * radius, radius};
    SDL_RenderFillRect(renderer, &topRect);

    SDL_Rect bottomRect = {rect.x + radius, rect.y + rect.h - radius, rect.w - 2 * radius, radius};
    SDL_RenderFillRect(renderer, &bottomRect);

    // Draw the four corners using filled circles
    for (int w = 0; w < radius * 3; w++)
    {
        for (int h = 0; h < radius * 3; h++)
        {
            int dx = radius - w; // Horizontal offset
            int dy = radius - h; // Vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius))
            {
                SDL_RenderDrawPoint(renderer, rect.x + dx + radius, rect.y + dy + radius);
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - dx - radius, rect.y + dy + radius);
                SDL_RenderDrawPoint(renderer, rect.x + dx + radius, rect.y + rect.h - dy - radius);
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - dx - radius, rect.y + rect.h - dy - radius);
            }
        }
    }
}

void draw_taskbar(SDL_Renderer *renderer, TTF_Font *font)
{
    // Draw taskbar background
    SDL_Rect taskbarRect = {0, 280, 320, 40};
    SDL_Color taskbarColor = {50, 50, 70, 255};
    SDL_SetRenderDrawColor(renderer, taskbarColor.r, taskbarColor.g, taskbarColor.b, taskbarColor.a);
    SDL_RenderFillRect(renderer, &taskbarRect);

    // Draw start button
    SDL_Rect startButtonRect = {5, 285, 50, 30};
    SDL_Color startButtonColor = {0, 120, 215, 255};
    draw_rounded_rect(renderer, startButtonRect, 5, startButtonColor);

    // Render "Start" text
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Start", textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect textRect = {startButtonRect.x + 5, startButtonRect.y + 5, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Draw clock
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timeString[9];
    strftime(timeString, sizeof(timeString), "%H:%M:%S", timeinfo);

    textSurface = TTF_RenderText_Solid(font, timeString, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect clockRect = {260, 290, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &clockRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }
}

void draw_terminal_icon(SDL_Renderer* renderer, SDL_Rect icon_rect) {
    // Draw terminal icon background (black rectangle)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &icon_rect);
    
    // Calculate prompt symbol position (centered)
    int prompt_width = 20;  // Width of the prompt symbol
    int prompt_height = 20; // Height of the prompt symbol
    
    int x = icon_rect.x + (icon_rect.w - prompt_width) / 2;
    int y = icon_rect.y + (icon_rect.h - prompt_height) / 2;
    
    // Draw the ">" symbol in green
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    
    // Draw the ">" symbol using lines
    SDL_RenderDrawLine(renderer, x, y, x + prompt_width/2, y + prompt_height/2);
    SDL_RenderDrawLine(renderer, x + prompt_width/2, y + prompt_height/2, x, y + prompt_height);
    
    // Draw the "..." (three dots)
    int dot_size = 3;
    int dot_spacing = 5;
    int dots_start_x = x + prompt_width/2 + 5;
    int dots_y = y + prompt_height/2;
    
    for (int i = 0; i < 3; i++) {
        SDL_Rect dot = {
            dots_start_x + i * dot_spacing,
            dots_y - dot_size/2,
            dot_size,
            dot_size
        };
        SDL_RenderFillRect(renderer, &dot);
    }
}

// Add document editor icon drawing function
void draw_document_icon(SDL_Renderer* renderer, SDL_Rect icon_rect) {
    // Draw document icon background (white rectangle)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &icon_rect);
    
    // Draw document outline
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &icon_rect);
    
    // Draw text lines
    int line_spacing = 6;
    int start_y = icon_rect.y + 10;
    int line_width = icon_rect.w * 0.7;
    
    for (int i = 0; i < 5; i++) {
        SDL_Rect line = {
            icon_rect.x + (icon_rect.w - line_width) / 2,
            start_y + i * line_spacing,
            line_width,
            2
        };
        SDL_RenderFillRect(renderer, &line);
    }
    
    // Draw folded corner
    SDL_Point points[3] = {
        {icon_rect.x + icon_rect.w - 10, icon_rect.y},
        {icon_rect.x + icon_rect.w, icon_rect.y},
        {icon_rect.x + icon_rect.w, icon_rect.y + 10}
    };
    SDL_RenderDrawLines(renderer, points, 3);
}

void draw_desktop(SDL_Renderer *renderer, TTF_Font *font, Application *apps, int appCount)
{
    // Draw desktop background
    SDL_SetRenderDrawColor(renderer, 0, 100, 170, 255);
    SDL_Rect desktopRect = {0, 0, 320, 280};
    SDL_RenderFillRect(renderer, &desktopRect);

    // Draw application icons
    SDL_Color textColor = {255, 255, 255, 255};
    for (int i = 0; i < appCount; i++)
    {
        // Draw icon background
        draw_rounded_rect(renderer, apps[i].icon, 5, apps[i].color);

        // For Terminal app (index 0), draw the special terminal icon
        if (i == 0) {  // Terminal is the first app
            draw_terminal_icon(renderer, apps[i].icon);
        } else if (i == 2) {  // Document Editor
            draw_document_icon(renderer, apps[i].icon);
        }

        // Draw icon text
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, apps[i].name, textColor);
        if (textSurface != NULL)
        {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != NULL)
            {
                SDL_Rect textRect = {
                    apps[i].icon.x + (apps[i].icon.w - textSurface->w) / 2,
                    apps[i].icon.y + apps[i].icon.h + 5,
                    textSurface->w,
                    textSurface->h};
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        else
        {
            fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        }
    }
}

void draw_boot_sequence(SDL_Renderer *renderer, TTF_Font *font, int progress, unsigned char *display_memory)
{
    // Background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // OS Name
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "MicroOS 1.0", textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect textRect = {(320 - textSurface->w) / 2, 50, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Progress bar border
    SDL_Rect progressBorderRect = {60, 100, 200, 20};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &progressBorderRect);

    // Progress bar fill
    SDL_Rect progressFillRect = {62, 102, (int)(196.0f * (progress / 100.0f)), 16};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &progressFillRect);

    // Progress text
    char progressText[10];
    sprintf(progressText, "%d%%", progress);
    textSurface = TTF_RenderText_Solid(font, progressText, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect progressTextRect = {(320 - textSurface->w) / 2, 130, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &progressTextRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Display the memory visualization during boot as a sort of "hardware check"
    render_display_memory(renderer, display_memory);

    // Add some loading text
    const char *loadingText = "Checking hardware...";
    textSurface = TTF_RenderText_Solid(font, loadingText, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect loadingTextRect = {(320 - textSurface->w) / 2, 200, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &loadingTextRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }
}

// Update function declaration to include apps parameter
void draw_app(SDL_Renderer *renderer, TTF_Font *font, const char *appName, SDL_Color appColor, 
             unsigned char *display_memory, FileSystem* fs, Application* apps) {
    // App window background
    SDL_Rect windowRect = {0, 0, 320, 320}; // Full screen
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderFillRect(renderer, &windowRect);

    // App title bar
    SDL_Rect titleBarRect = {0, 0, 320, 25};
    SDL_SetRenderDrawColor(renderer, appColor.r, appColor.g, appColor.b, appColor.a);
    SDL_RenderFillRect(renderer, &titleBarRect);

    // App title
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, appName, textColor);  // Add textColor argument
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect textRect = {5, 2, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Close button
    SDL_Rect closeButtonRect = {295, 0, 25, 25};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeButtonRect);

    // X on close button
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, 300, 5, 315, 20);
    SDL_RenderDrawLine(renderer, 315, 5, 300, 20);

    // App content area
    SDL_Rect contentRect = {0, 25, 320, 295};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &contentRect);

    // Display content based on app
    if (strcmp(appName, "Terminal") == 0)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &contentRect);

        // Render terminal content
        terminal_render(apps[0].terminal, renderer, font, contentRect);
    }
    else if (strcmp(appName, "Settings") == 0)
    {
        if (showSettingsSidebar) {
            SDL_Rect winRect = {0, 0, 320, 320};
            draw_settings_menu(renderer, font, &apps[1].settings, winRect);
        } else {
            const char *options[] = {
                "Display",
                "Sound",
                "Network",
                "About"};

            for (int i = 0; i < 4; i++)
            {
                SDL_Rect optionRect = {35, 60 + i * 40, 250, 30};
                SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
                SDL_RenderFillRect(renderer, &optionRect);

                SDL_Color optionText = {0, 0, 0, 255};
                SDL_Surface *textSurface = TTF_RenderText_Solid(font, options[i], optionText);
                if (textSurface != NULL)
                {
                    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture != NULL)
                    {
                        SDL_Rect textRect = {45, 65 + i * 40, textSurface->w, textSurface->h};
                        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                        SDL_DestroyTexture(textTexture);
                    }
                    SDL_FreeSurface(textSurface);
                }
                else
                {
                    fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
                }
            }
        }
    }
    else if (strcmp(appName, "Files") == 0) {
        fileui_render(apps[2].fileui, renderer, font);
        if (apps[2].editor->is_open) {
            editor_render(apps[2].editor, renderer, font);
        }
    } else if (strcmp(appName, "Document Editor") == 0) {
        editor_render(apps[2].editor, renderer, font);
    }
}

void draw_start_menu(SDL_Renderer* renderer, TTF_Font* font) {
    Uint32 current_time = SDL_GetTicks();
    
    // Update menu height animation
    if (menu_anim.is_animating) {
        float progress = (current_time - menu_anim.animation_start) / (float)MENU_ANIMATION_DURATION;
        if (progress > 1.0f) {
            progress = 1.0f;
            menu_anim.is_animating = false;
        }
        float eased = ease_out_elastic(progress);
        menu_anim.current_height = menu_anim.start_height + 
            (menu_anim.target_height - menu_anim.start_height) * eased;
    }

    // Menu background with current animated height
    SDL_Rect menuRect = {5, 100, 150, (int)menu_anim.current_height};
    SDL_Color menuColor = {50, 50, 70, 255};
    draw_rounded_rect_with_shadow(renderer, menuRect, 10, menuColor);

    // Menu items
    const char* menuItems[] = {"Terminal", "Settings", "File Explorer", "Restart"};
    SDL_Color textColor = {255, 255, 255, 255};
    
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        // Update item position with spring animation
        if (menu_items[i].is_visible) {
            const float spring_constant = 0.3f;
            const float damping = 0.7f;
            
            float dx = menu_items[i].target_x - menu_items[i].x;
            float dy = menu_items[i].target_y - menu_items[i].y;
            
            menu_items[i].velocity += dx * spring_constant;
            menu_items[i].velocity *= damping;
            menu_items[i].x += menu_items[i].velocity;
            
            // Draw menu item with glow effect
            SDL_Rect itemRect = {
                (int)menu_items[i].x, 
                (int)menu_items[i].y, 
                140, 35
            };
            
            // Glow effect
            SDL_Color glowColor = {80, 80, 100, 128};
            SDL_Rect glowRect = {itemRect.x - 2, itemRect.y - 2, itemRect.w + 4, itemRect.h + 4};
            draw_rounded_rect(renderer, glowRect, 8, glowColor);
            
            // Main button
            SDL_Color itemColor = {70, 70, 90, 255};
            draw_rounded_rect_with_shadow(renderer, itemRect, 5, itemColor);

            // Text
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, menuItems[i], textColor);
            if (textSurface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_Rect textRect = {
                    itemRect.x + 10,
                    itemRect.y + (itemRect.h - textSurface->h) / 2,
                    textSurface->w,
                    textSurface->h
                };
                SDL_RenderCopy(renderer, texture, NULL, &textRect);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(textSurface);
            }
        }
    }
}

void draw_settings_page(SDL_Renderer* renderer, TTF_Font* font, SDL_Rect content_area, 
                       Application* app, const char* page) {
    if (strcmp(page, "Display") == 0) {
        int y = content_area.y + 10;
        SDL_Color textColor = {0, 0, 0, 255};

        // Resolution selector
        const char* resolutions[] = {"144p", "360p", "480p", "720p", "1080p"};
        SDL_Surface* surface = TTF_RenderText_Solid(font, "Resolution:", textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {content_area.x + 10, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
        y += 30;

        for (int i = 0; i < 5; i++) {
            SDL_Rect button = {content_area.x + 10, y, 100, 25};
            SDL_SetRenderDrawColor(renderer, 
                app->settings.resolution == i ? 200 : 230,
                app->settings.resolution == i ? 200 : 230,
                app->settings.resolution == i ? 200 : 230, 255);
            SDL_RenderFillRect(renderer, &button);

            surface = TTF_RenderText_Solid(font, resolutions[i], textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect textRect = {button.x + 5, button.y + 5, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &textRect);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
            y += 35;
        }

        // Theme selector
        y += 20;
        surface = TTF_RenderText_Solid(font, "Color Theme:", textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {content_area.x + 10, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
        y += 30;

        const char* themes[] = {"Default", "Cyan", "Grayscale"};
        for (int i = 0; i < 3; i++) {
            SDL_Rect button = {content_area.x + 10, y, 100, 25};
            SDL_SetRenderDrawColor(renderer, 
                app->settings.theme == i ? 200 : 230,
                app->settings.theme == i ? 200 : 230,
                app->settings.theme == i ? 200 : 230, 255);
            SDL_RenderFillRect(renderer, &button);

            surface = TTF_RenderText_Solid(font, themes[i], textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect textRect = {button.x + 5, button.y + 5, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &textRect);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
            y += 35;
        }

        // UI Scale slider
        y += 20;
        surface = TTF_RenderText_Solid(font, "UI Scale:", textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {content_area.x + 10, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
        y += 30;

        // Draw slider
        SDL_Rect slider = {content_area.x + 10, y, 200, 10};
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderFillRect(renderer, &slider);

        float knobPos = (app->settings.ui_scale - 1.0f) * 200.0f;
        SDL_Rect knob = {content_area.x + 10 + (int)knobPos, y - 5, 20, 20};
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &knob);

        char scaleText[32];
        snprintf(scaleText, sizeof(scaleText), "%.0f%%", app->settings.ui_scale * 100);
        surface = TTF_RenderText_Solid(font, scaleText, textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect textRect = {content_area.x + 220, y - 5, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &textRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }
}

void reset_temporary_data(Application* apps) {
    // Reset terminal
    if (apps[0].terminal) {
        terminal_reset(apps[0].terminal);
    }
    
    // Reset document editor
    if (apps[2].editor) {
        editor_reset(apps[2].editor);
    }
    
    // Reset settings UI state
    settings_ui_state = (SettingsUIState){0};
    showSettingsSidebar = false;
}

void draw_settings_section(SDL_Renderer* renderer, TTF_Font* font, SDL_Rect* rect, 
                         const char* title, bool expanded, SDL_Color* color) {
    // Draw section background
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    SDL_RenderFillRect(renderer, rect);

    // Draw title
    SDL_Color text_color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, title, text_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect text_rect = {
            rect->x + 10,
            rect->y + (rect->h - surface->h) / 2,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &text_rect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }

    // Draw expand/collapse arrow
    int arrow_size = 10;
    SDL_Point arrow[3];
    if (expanded) {
        // Down arrow
        arrow[0] = (SDL_Point){rect->x + rect->w - 20, rect->y + rect->h/2 - arrow_size/2};
        arrow[1] = (SDL_Point){rect->x + rect->w - 20 + arrow_size, rect->y + rect->h/2 - arrow_size/2};
        arrow[2] = (SDL_Point){rect->x + rect->w - 20 + arrow_size/2, rect->y + rect->h/2 + arrow_size/2};
    } else {
        // Right arrow
        arrow[0] = (SDL_Point){rect->x + rect->w - 20, rect->y + rect->h/2 - arrow_size/2};
        arrow[1] = (SDL_Point){rect->x + rect->w - 20 + arrow_size, rect->y + rect->h/2};
        arrow[2] = (SDL_Point){rect->x + rect->w - 20, rect->y + rect->h/2 + arrow_size/2};
    }
    SDL_RenderDrawLines(renderer, arrow, 3);
}

// Update the settings app drawing function:
void draw_settings_app(SDL_Renderer* renderer, TTF_Font* font, Application* app) {
    SDL_Rect content = {0, 25, 320, 295};
    int y_offset = 60 - settings_ui_state.scroll_position;
    
    // Display section
    SDL_Rect display_section = {35, y_offset, 250, 40};
    draw_settings_section(renderer, font, &display_section, "Display", 
                         settings_ui_state.display_section_expanded, 
                         &(SDL_Color){100, 100, 200, 255});

    y_offset += 45;  // Base spacing between sections

    // If display section is expanded, draw its content
    if (settings_ui_state.display_section_expanded) {
        SDL_Rect settings_content = {35, y_offset, 250, 200};  // Adjust height as needed
        draw_settings_page(renderer, font, settings_content, app, "Display");
        y_offset += 205;  // Adjust based on content height
    }

    // Sound section
    SDL_Rect sound_section = {35, y_offset, 250, 40};
    draw_settings_section(renderer, font, &sound_section, "Sound",
                         settings_ui_state.sound_section_expanded,
                         &(SDL_Color){100, 200, 100, 255});
    
    y_offset += 45;

    // Network section
    SDL_Rect network_section = {35, y_offset, 250, 40};
    draw_settings_section(renderer, font, &network_section, "Network",
                         settings_ui_state.network_section_expanded,
                         &(SDL_Color){200, 100, 100, 255});
}

int main(int argc, char *argv[])
{
    // Check command line arguments; if "raw" is passed then assume real hardware.
    if (argc > 1 && strcmp(argv[1], "raw") == 0) {
        running_on_raw_hardware = true;
    }

    // Initialize drivers based on mode
    init_drivers(running_on_raw_hardware);
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Create a resizable window.
    SDL_Window *window = SDL_CreateWindow("MicroOS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                            640, 480,   // Default size
                                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    
    if (renderer) {
        // Enable anti-aliasing and better scaling quality
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");  // Use best quality scaling
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetHint(SDL_HINT_RENDER_LINE_METHOD, "3");  // Use anti-aliased lines
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14);
    if (font == NULL)
    {
        // Try alternative font paths
        font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 14);
        if (font == NULL)
        {
            font = TTF_OpenFont("/Library/Fonts/Arial.ttf", 14);
            if (font == NULL)
            {
                // Try loading font in common MacOS locations
                font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttf", 14);
                if (font == NULL) {
                    font = TTF_OpenFont("/System/Library/Fonts/SFNS.ttf", 14);
                    if (font == NULL) {
                        font = TTF_OpenFont("/System/Library/Fonts/SFNSText.ttf", 14);
                        if (font == NULL) {
                            font = TTF_OpenFont("/System/Library/Fonts/AppleSDGothicNeo.ttc", 14);
                            if (font == NULL) {
                                fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
                                fprintf(stderr, "Unable to load any fonts. Exiting...\n");
                                SDL_DestroyRenderer(renderer);
                                SDL_DestroyWindow(window);
                                TTF_Quit();
                                SDL_Quit();
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }

    unsigned char display_memory[64];
    // Initialize display_memory with zeros as a safety measure
    memset(display_memory, 0, sizeof(display_memory));
    initialize_display_memory(display_memory);

    // Define applications
    Application apps[3];
    strcpy(apps[0].name, "Terminal");
    apps[0].icon = (SDL_Rect){40, 40, 50, 50};
    apps[0].color = (SDL_Color){0, 0, 0, 255};

    strcpy(apps[1].name, "Settings");
    apps[1].icon = (SDL_Rect){140, 40, 50, 50};
    apps[1].color = (SDL_Color){100, 100, 200, 255};

    strcpy(apps[2].name, "Document Editor");
    apps[2].icon = (SDL_Rect){240, 40, 50, 50};
    apps[2].color = (SDL_Color){255, 255, 255, 255};

    FileSystem* fs = fs_init();
    apps[2].fs = fs;  // Assign to Files app

    // After initializing apps array:
    apps[0].terminal = terminal_create(fs);
    apps[2].fileui = fileui_create(fs);
    apps[2].editor = editor_create();

    OSState currentState = OS_STATE_BOOT;
    int bootProgress = 0;
    bool showMenu = false;
    char notification[64] = "";
    time_t notificationTime = 0;
    SDL_Color notificationBgColor = {0, 0, 0, 200};
    SDL_Color currentAppColor = {0, 0, 0, 255};
    char currentAppName[32] = "";

    bool running = true;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    int frameCount = 0;

    printf("MicroOS starting...\n");
    printf("Press any key to exit the application\n");

    init_menu_animations();

    // New global flag for settings sidebar
    bool showSettingsMenu = false;

    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int x = e.button.x;
                int y = e.button.y;
                int w, h;
                SDL_GetWindowSize(window, &w, &h);

                if (currentState == OS_STATE_DESKTOP)
                {
                    // Check if start button was clicked
                    if (x >= 5 && x <= 55 && y >= 285 && y <= 315)
                    {
                        showMenu = !showMenu;
                        if (showMenu) {
                            // Start menu animation
                            menu_anim.current_height = 0;
                            menu_anim.start_height = 0;
                            menu_anim.target_height = 180;
                            menu_anim.animation_start = SDL_GetTicks();
                            menu_anim.is_animating = true;
                            
                            // Reset and start item animations
                            for (int i = 0; i < MENU_ITEM_COUNT; i++) {
                                menu_items[i].x = -100;
                                menu_items[i].velocity = 0;
                                menu_items[i].is_visible = true;
                            }
                        }
                    }

                    // Check if any app icon was clicked
                    for (int i = 0; i < 3; i++)
                    {
                        if (x >= apps[i].icon.x && x <= apps[i].icon.x + apps[i].icon.w &&
                            y >= apps[i].icon.y && y <= apps[i].icon.y + apps[i].icon.h)
                        {
                            if (i == 0)
                            {
                                currentState = OS_STATE_APP1;
                                strcpy(currentAppName, "Terminal");
                                currentAppColor = apps[i].color;
                            }
                            else if (i == 1)
                            {
                                currentState = OS_STATE_APP2;
                                strcpy(currentAppName, "Settings");
                                currentAppColor = apps[i].color;
                                showSettingsSidebar = false;
                            } else if (i == 2) {
                                currentState = OS_STATE_APP3;
                                strcpy(currentAppName, "Document Editor");
                                currentAppColor = apps[i].color;
                                apps[2].editor->is_open = true;
                            }
                            showMenu = false;
                            snprintf(notification, sizeof(notification), "Opening %s...", apps[i].name);
                            notificationTime = time(NULL);
                        }
                    }

                    // Check menu items if menu is shown
                    if (showMenu)
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            if (x >= 10 && x <= 150 && y >= 110 + i * 40 && y <= 145 + i * 40)
                            {
                                if (i == 0)
                                {
                                    currentState = OS_STATE_APP1;
                                    strcpy(currentAppName, "Terminal");
                                    currentAppColor = apps[0].color;
                                }
                                else if (i == 1)
                                {
                                    currentState = OS_STATE_APP2;
                                    strcpy(currentAppName, "Settings");
                                    currentAppColor = apps[1].color;
                                }
                                else if (i == 3)
                                {
                                    // If running on real hardware, reboot via BIOS
                                    if (running_on_raw_hardware) {
                                        bios_restart();
                                        // Should not return.
                                    } else {
                                        // In simulation, clear unsaved data and reboot.
                                        reset_temporary_data(apps);
                                        currentState = OS_STATE_BOOT;
                                        bootProgress = 0;
                                    }
                                }
                                showMenu = false;
                            }
                        }
                    }
                }
                else if (currentState == OS_STATE_APP1 || currentState == OS_STATE_APP2 || currentState == OS_STATE_APP3)
                {
                    // Close button as drawn in draw_app: x from 295 to 320, y from 0 to 25.
                    if (x >= 295 && x <= 320 && y >= 0 && y <= 25) {
                        currentState = OS_STATE_DESKTOP;
                        snprintf(notification, sizeof(notification), "Closed %s", currentAppName);
                        notificationTime = time(NULL);
                    }
                }
            }
            else if (e.type == SDL_KEYDOWN && currentState == OS_STATE_APP1) {
                terminal_handle_keypress(apps[0].terminal, &e.key);
            }
            else if (e.type == SDL_MOUSEWHEEL && currentState == OS_STATE_APP1) {
                terminal_handle_mouse(apps[0].terminal, &e.wheel);
            }
            if (currentState == OS_STATE_APP2) {
                if (fileui_handle_event(apps[2].fileui, &e, apps[2].editor)) {
                    // Event was handled by file UI
                    continue;
                }
                if (editor_handle_event(apps[2].editor, &e, apps[2].fs)) {
                    // Event was handled by text editor
                    continue;
                }
            }
            else if (currentState == OS_STATE_APP2 && strcmp(currentAppName, "Settings") == 0) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    int x = e.button.x;
                    int y = e.button.y;
                    int win_w, win_h;
                    SDL_GetWindowSize(window, &win_w, &win_h);
                    
                    // Check for close button in settings sidebar
                    SDL_Rect closeButtonRect = {win_w / 2 - 30, 10, 20, 20};
                    if (x >= 0 && x <= win_w/2) {
                        if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.w &&
                            y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.h) {
                            showSettingsSidebar = false;
                        }
                    }
                    
                    // Check for resolution buttons
                    int res_y = 75;
                    for (int i = 0; i < 5; i++) {
                        SDL_Rect button = {10, res_y, 120, 25};
                        if (x >= 0 && x <= win_w/2) {
                            if (x >= button.x && x <= button.x + button.w &&
                                y >= button.y && y <= button.y + button.h) {
                                apps[0].settings.resolution = i;
                                settings_apply_resolution(&apps[0].settings, window);
                                break;
                            }
                        }
                        res_y += 35;
                    }
                }
                // Handle UI scale slider dragging
                else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    int sliderY = 320;
                    if (e.button.y >= sliderY - 5 && e.button.y <= sliderY + 15) {
                        float scale = 1.0f + (e.button.x - 45) / 200.0f;
                        if (scale >= 1.0f && scale <= 2.0f) {
                            apps[0].settings.ui_scale = scale;
                            settings_apply_resolution(&apps[0].settings, window);
                        }
                    }
                }
            }
            if (currentState == OS_STATE_APP3) {
                if (editor_handle_event(apps[2].editor, &e, apps[2].fs)) {
                    if (!apps[2].editor->is_open) {
                        currentState = OS_STATE_DESKTOP;
                    }
                    continue;
                }
            }
            // Handle text input for document editor
            if (e.type == SDL_TEXTINPUT && currentState == OS_STATE_APP3) {
                editor_insert_text(apps[2].editor, e.text.text);
            }
            // Handle window resize event
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                SDL_RenderSetLogicalSize(renderer, w, h);
            }
            // In the main event loop, update the settings click handling:
            if (currentState == OS_STATE_APP2 && e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.button.x;
                int y = e.button.y;
                int real_y = y + settings_ui_state.scroll_position;
                
                // Check display section header
                SDL_Rect display_section = {35, 60, 250, 40};
                if (x >= display_section.x && x <= display_section.x + display_section.w &&
                    real_y >= display_section.y && real_y <= display_section.y + display_section.h) {
                    settings_ui_state.display_section_expanded = !settings_ui_state.display_section_expanded;
                }
                
                // Similar checks for sound and network sections...
            } else if (currentState == OS_STATE_APP2 && e.type == SDL_MOUSEWHEEL) {
                settings_ui_state.scroll_position -= e.wheel.y * 20;  // Adjust scroll speed
                if (settings_ui_state.scroll_position < 0) {
                    settings_ui_state.scroll_position = 0;
                }
                
                // Calculate max scroll based on content height
                int max_scroll = calculate_settings_content_height() - 295;  // 295 is visible area height
                if (settings_ui_state.scroll_position > max_scroll) {
                    settings_ui_state.scroll_position = max_scroll;
                }
            }
        }

        // Update logic
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 16)
        { // ~60 FPS
            // Call assembly function to update display memory
            update_display(display_memory);

            // Handle boot sequence
            if (currentState == OS_STATE_BOOT)
            {
                bootProgress += 1;
                if (bootProgress >= 100)
                {
                    currentState = OS_STATE_DESKTOP;
                    snprintf(notification, sizeof(notification), "MicroOS Started!");
                    notificationTime = time(NULL);
                }
            }

            lastTime = currentTime;
            frameCount++;
        }

        // Update external media detection once per frame
        if (fs_detect_external_media(fs)) {
            snprintf(notification, sizeof(notification), "External media connected!");
            notificationTime = time(NULL);
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Update logical size each frame to auto adjust on window resize:
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_RenderSetLogicalSize(renderer, w, h);

        // Render based on current state
        switch (currentState)
        {
        case OS_STATE_BOOT:
            draw_boot_sequence(renderer, font, bootProgress, display_memory);
            break;

        case OS_STATE_DESKTOP:
            draw_desktop(renderer, font, apps, 3);
            draw_taskbar(renderer, font);
            if (showMenu)
            {
                draw_start_menu(renderer, font);
            }
            // NEW: if settings sidebar is active, draw it on top
            if (showSettingsMenu) {
                SDL_Rect winRect = {0, 0, w, h};
                draw_settings_menu(renderer, font, &apps[1].settings, winRect);
            }
            break;

        case OS_STATE_APP1:
        case OS_STATE_APP2:
        case OS_STATE_APP3:
            draw_desktop(renderer, font, apps, 3);
            draw_taskbar(renderer, font);
            draw_app(renderer, font, currentAppName, currentAppColor, display_memory, apps[2].fs, apps);
            break;

        default:
            break;
        }

        // Show notification if needed
        if (time(NULL) - notificationTime < 3)
        {
            // Draw notification background
            SDL_Rect notifRect = {50, 150, 220, 40};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, notificationBgColor.r, notificationBgColor.g,
                                   notificationBgColor.b, notificationBgColor.a);
            draw_rounded_rect(renderer, notifRect, 8, notificationBgColor);

            // Draw notification text
            SDL_Color notifTextColor = {255, 255, 255, 255};
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, notification, notifTextColor);
            if (textSurface != NULL)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL)
                {
                    SDL_Rect textRect = {
                        (320 - textSurface->w) / 2,
                        (notifRect.y + (notifRect.h - textSurface->h) / 2),
                        textSurface->w,
                        textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            else
            {
                fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
            }
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_RenderPresent(renderer);

        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    fileui_destroy(apps[2].fileui);
    editor_destroy(apps[2].editor);
    TTF_Quit();
    SDL_Quit();

    printf("MicroOS exited.\n");
    return 0;
}
