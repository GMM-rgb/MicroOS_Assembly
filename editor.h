#ifndef MICROOS_EDITOR_H
#define MICROOS_EDITOR_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "filesystem.h"

typedef struct {
    bool is_open;
    char* file_path;  
    char* content;
    int cursor_position;
    int scroll_position;
    SDL_Rect window_rect;
} TextEditor;

TextEditor* editor_create(void);
void editor_destroy(TextEditor* editor);
bool editor_handle_event(TextEditor* editor, SDL_Event* event);
void editor_render(TextEditor* editor, SDL_Renderer* renderer, TTF_Font* font);
void editor_save(TextEditor* editor, FileSystem* fs);

#endif // MICROOS_EDITOR_H
