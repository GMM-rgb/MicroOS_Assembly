#ifndef MICROOS_FILEUI_H
#define MICROOS_FILEUI_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "filesystem.h"
#include "editor.h"  // Include editor.h to use TextEditor

typedef struct {
    bool is_open;
    bool is_minimized;
    SDL_Rect window_rect;
    SDL_Rect original_rect;
    FileNode* current_directory;
    char* selected_file;
    int scroll_position;
    bool is_animating;
    float animation_progress;
    SDL_Point minimize_start;
    SDL_Point minimize_end;
    FileSystem* fs;  // FileSystem pointer
} FileUI;

FileUI* fileui_create(FileSystem* fs);
void fileui_destroy(FileUI* ui);
void fileui_render(FileUI* ui, SDL_Renderer* renderer, TTF_Font* font);
bool fileui_handle_event(FileUI* ui, SDL_Event* event, TextEditor* editor);
void fileui_animate(FileUI* ui);

#endif // MICROOS_FILEUI_H
