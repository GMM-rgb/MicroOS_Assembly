#include "fileui.h"
#include "editor.h"  // Add this include so TextEditor is known
#include <stdlib.h>
#include <string.h>

FileUI* fileui_create(FileSystem* fs) {
    FileUI* ui = malloc(sizeof(FileUI));
    ui->is_open = false;
    ui->is_minimized = false;
    ui->window_rect = (SDL_Rect){20, -220, 280, 200}; // starts hidden (animated from top)
    ui->original_rect = (SDL_Rect){20, 20, 280, 200};
    ui->current_directory = fs->root;
    ui->selected_file = NULL;
    ui->scroll_position = 0;
    ui->is_animating = false;
    ui->animation_progress = 0.0f;
    ui->minimize_start = (SDL_Point){20,20};
    ui->minimize_end = (SDL_Point){240,280}; // target position when minimized
    ui->fs = fs;  // Store FileSystem pointer
    return ui;
}

void fileui_destroy(FileUI* ui) {
    free(ui);
}

void fileui_render(FileUI* ui, SDL_Renderer* renderer, TTF_Font* font) {
    if (!ui->is_open || ui->is_minimized) return;

    // Draw window background
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderFillRect(renderer, &ui->window_rect);

    // Draw title bar
    SDL_Rect titlebar = {ui->window_rect.x, ui->window_rect.y, ui->window_rect.w, 25};
    SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
    SDL_RenderFillRect(renderer, &titlebar);

    // Draw buttons
    SDL_Rect minimizeBtn = {titlebar.x + titlebar.w - 50, titlebar.y, 25, 25};
    SDL_Rect closeBtn = {titlebar.x + titlebar.w - 25, titlebar.y, 25, 25};
    SDL_SetRenderDrawColor(renderer, 200, 200, 0, 255); // Yellow for minimize
    SDL_RenderFillRect(renderer, &minimizeBtn);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for close
    SDL_RenderFillRect(renderer, &closeBtn);

    // Draw toolbar
    SDL_Rect toolbar = {ui->window_rect.x, ui->window_rect.y + 25, ui->window_rect.w, 30};
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderFillRect(renderer, &toolbar);

    // Draw content area
    SDL_Rect content = {
        ui->window_rect.x + 5,
        ui->window_rect.y + 60,
        ui->window_rect.w - 10,
        ui->window_rect.h - 65
    };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &content);

    // Draw files and folders
    FileNode** files;
    int count;
    fs_list_directory(ui->fs, fs_get_current_path(ui->fs), &files, &count);

    SDL_Color folderColor = {0, 0, 255, 255};
    SDL_Color fileColor = {0, 0, 0, 255};
    int y_offset = content.y - ui->scroll_position;

    for (int i = 0; i < count; i++) {
        SDL_Color color = files[i]->is_directory ? folderColor : fileColor;
        SDL_Surface* surface = TTF_RenderText_Solid(font, files[i]->name, color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect pos = {content.x + 5, y_offset + i * 20, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &pos);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }
}

bool fileui_handle_event(FileUI* ui, SDL_Event* event, TextEditor* editor) {
    // For now, simply return false (not handled)
    // You can add button click handling, drag to animate, etc.
    return false;
}

void fileui_animate(FileUI* ui) {
    if (!ui->is_animating) return;

    ui->animation_progress += 0.1f;
    if (ui->animation_progress > 1.0f) {
        ui->animation_progress = 1.0f;
        ui->is_animating = false;
    }

    if (ui->is_minimized) {
        ui->window_rect.x = ui->minimize_start.x + (ui->minimize_end.x - ui->minimize_start.x) * ui->animation_progress;
        ui->window_rect.y = ui->minimize_start.y + (ui->minimize_end.y - ui->minimize_start.y) * ui->animation_progress;
        ui->window_rect.w = ui->original_rect.w * (1.0f - ui->animation_progress);
        ui->window_rect.h = ui->original_rect.h * (1.0f - ui->animation_progress);
    } else {
        ui->window_rect.x = ui->minimize_end.x + (ui->minimize_start.x - ui->minimize_end.x) * ui->animation_progress;
        ui->window_rect.y = ui->minimize_end.y + (ui->minimize_start.y - ui->minimize_end.y) * ui->animation_progress;
        ui->window_rect.w = ui->original_rect.w * ui->animation_progress;
        ui->window_rect.h = ui->original_rect.h * ui->animation_progress;
    }
}

// ... Add the TextEditor implementation ...
