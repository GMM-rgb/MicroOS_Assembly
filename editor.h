#ifndef MICROOS_EDITOR_H
#define MICROOS_EDITOR_H

#include <SDL.h>
#include <SDL_ttf.h>
#include "filesystem.h"

#define MAX_DOCUMENT_LINES 1000
#define MAX_LINE_LENGTH 256
#define MAX_UNDO_STEPS 50

typedef struct {
    char* text;
    int length;
} DocumentLine;

typedef enum {
    FONT_NORMAL,
    FONT_BOLD,
    FONT_ITALIC,
    FONT_UNDERLINE
} FontStyle;

typedef struct {
    bool is_open;
    char* file_path;
    DocumentLine lines[MAX_DOCUMENT_LINES];
    int line_count;
    int cursor_line;
    int cursor_col;
    int scroll_x;
    int scroll_y;
    int selection_start_line;
    int selection_start_col;
    int selection_end_line;
    int selection_end_col;
    SDL_Rect window_rect;
    SDL_Color text_color;
    FontStyle current_style;
    float font_size;
    bool show_toolbar;
    bool show_ruler;
    bool word_wrap;
    char* clipboard;
    
    // Undo/Redo support
    char* undo_buffer[MAX_UNDO_STEPS];
    int undo_count;
    int undo_position;
    
    // File management
    bool has_changes;
    time_t last_save;
} TextEditor;

TextEditor* editor_create(void);
void editor_destroy(TextEditor* editor);
bool editor_handle_event(TextEditor* editor, SDL_Event* event);
void editor_render(TextEditor* editor, SDL_Renderer* renderer, TTF_Font* font);
bool editor_save(TextEditor* editor, FileSystem* fs);
bool editor_load(TextEditor* editor, FileSystem* fs, const char* path);
void editor_insert_text(TextEditor* editor, const char* text);
void editor_delete_selection(TextEditor* editor);
void editor_copy(TextEditor* editor);
void editor_paste(TextEditor* editor);
void editor_undo(TextEditor* editor);
void editor_redo(TextEditor* editor);

#endif // MICROOS_EDITOR_H
