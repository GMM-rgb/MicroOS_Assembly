#ifndef MICROOS_TERMINAL_H
#define MICROOS_TERMINAL_H

#include "filesystem.h"
#include <SDL.h>
#include <SDL_ttf.h>

#define MAX_COMMAND_HISTORY 100
#define MAX_COMMAND_LENGTH 256
#define MAX_TERMINAL_LINES 1000

typedef struct {
    char* lines[MAX_TERMINAL_LINES];
    int line_count;
    char current_command[MAX_COMMAND_LENGTH];
    int cursor_position;
    int scroll_position;
    FileSystem* fs;
    char command_history[MAX_COMMAND_HISTORY][MAX_COMMAND_LENGTH];
    int history_count;
    int history_position;
} Terminal;

Terminal* terminal_create(FileSystem* fs);
void terminal_destroy(Terminal* term);  // declaration only, remove inline definition
void terminal_handle_keypress(Terminal* term, SDL_KeyboardEvent* event);
void terminal_execute_command(Terminal* term);
void terminal_add_line(Terminal* term, const char* line);
void terminal_render(Terminal* term, SDL_Renderer* renderer, TTF_Font* font, SDL_Rect content_area);

#endif // MICROOS_TERMINAL_H
