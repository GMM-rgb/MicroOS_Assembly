#include "terminal.h"
#include "editor.h"  // Include the editor header file
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

Terminal* terminal_create(FileSystem* fs) {
    Terminal* term = malloc(sizeof(Terminal));
    term->line_count = 0;
    term->cursor_position = 0;
    term->scroll_position = 0;
    term->fs = fs;
    term->history_count = 0;
    term->history_position = -1;
    term->visible_lines = 0;  // Will be set in render
    term->max_chars_per_line = 0;  // Will be set in render
    term->current_command[0] = '\0';
    terminal_add_line(term, "MicroOS Terminal v1.0");
    terminal_add_line(term, "Type 'help' for available commands");
    return term;
}

void terminal_destroy(Terminal* term) {
    for (int i = 0; i < term->line_count; ++i) {
        free(term->lines[i]);
    }
    free(term);
}

void terminal_add_line(Terminal* term, const char* line) {
    if (term->line_count < MAX_TERMINAL_LINES) {
        // Word wrap long lines
        if (term->max_chars_per_line > 0 && strlen(line) > term->max_chars_per_line) {
            char buffer[MAX_COMMAND_LENGTH];
            int pos = 0;
            while (pos < strlen(line)) {
                int chars_to_copy = term->max_chars_per_line;
                if (strlen(line + pos) > chars_to_copy) {
                    // Look for last space within the limit
                    int last_space = chars_to_copy;
                    while (last_space > 0 && line[pos + last_space] != ' ') {
                        last_space--;
                    }
                    if (last_space > 0) {
                        chars_to_copy = last_space;
                    }
                } else {
                    chars_to_copy = strlen(line + pos);
                }
                
                strncpy(buffer, line + pos, chars_to_copy);
                buffer[chars_to_copy] = '\0';
                term->lines[term->line_count++] = strdup(buffer);
                pos += chars_to_copy;
                if (line[pos] == ' ') pos++; // Skip space
            }
        } else {
            term->lines[term->line_count++] = strdup(line);
        }
        
        // Auto-scroll to bottom when new line is added
        if (term->line_count > term->visible_lines) {
            term->scroll_position = term->line_count - term->visible_lines;
        }
    }
}

void terminal_execute_command(Terminal* term) {
    if (strlen(term->current_command) > 0) {
        strcpy(term->command_history[term->history_count], term->current_command);
        term->history_count = (term->history_count + 1) % MAX_COMMAND_HISTORY;
    }

    char cmd[MAX_COMMAND_LENGTH];
    snprintf(cmd, sizeof(cmd), "> %s", term->current_command);
    terminal_add_line(term, cmd);

    char* command = strtok(term->current_command, " ");
    if (!command) return;

    if (strcmp(command, "help") == 0) {
        terminal_add_line(term, "Available commands:");
        terminal_add_line(term, "  ls            - List files in current directory");
        terminal_add_line(term, "  cd <dir>      - Change directory");
        terminal_add_line(term, "  cat <file>    - Display file contents");
        terminal_add_line(term, "  miVo <file>   - Edit file");
        terminal_add_line(term, "  mkdir <dir>   - Create directory");
        terminal_add_line(term, "  touch <file>  - Create empty file");
        terminal_add_line(term, "  clear         - Clear terminal");
        terminal_add_line(term, "  pwd           - Print working directory");
        terminal_add_line(term, "  dir           - List files and directories in current directory");
        terminal_add_line(term, "  nedir <dir>   - Create a new directory");
        terminal_add_line(term, "  reboot        - Reboots / Restarts the system");
    } else if (strcmp(command, "view") == 0) {
        char* file = strtok(NULL, " ");
        if (file) {
            terminal_open_file_view(term, file);
        } else {
            terminal_add_line(term, "Error: No file specified.");
        }
    } else if (strcmp(command, "edit") == 0) {
        char* file = strtok(NULL, " ");
        if (file) {
            terminal_open_file_edit(term, file);
        } else {
            terminal_add_line(term, "Error: No file specified.");
        }
    } else if (strcmp(command, "ls") == 0 || strcmp(command, "dir") == 0) {
        FileNode** files;
        int count;
        fs_list_directory(term->fs, fs_get_current_path(term->fs), &files, &count);
        for (int i = 0; i < count; i++) {
            char info[256];
            snprintf(info, sizeof(info), "%s  %s  %s", 
                    files[i]->name,
                    fs_format_size(files[i]->size),
                    fs_format_time(files[i]->modified));
            terminal_add_line(term, info);
            if (files[i]->is_directory) {
                FileNode** sub_files;
                int sub_count;
                fs_list_directory(term->fs, files[i]->name, &sub_files, &sub_count);
                for (int j = 0; j < sub_count; j++) {
                    snprintf(info, sizeof(info), "  %s  %s  %s", 
                            sub_files[j]->name,
                            fs_format_size(sub_files[j]->size),
                            fs_format_time(sub_files[j]->modified));
                    terminal_add_line(term, info);
                }
            }
        }
    } else if (strcmp(command, "cd") == 0) {
        char* dir = strtok(NULL, " ");
        if (dir && fs_change_dir(term->fs, dir))
            terminal_add_line(term, "Directory changed");
        else
            terminal_add_line(term, "Error: Invalid directory");
    } else if (strcmp(command, "pwd") == 0) {
        terminal_add_line(term, fs_get_current_path(term->fs));
    } else if (strcmp(command, "nedir") == 0) {
        char* dir = strtok(NULL, " ");
        if (dir && fs_create_file(term->fs, dir, true))
            terminal_add_line(term, "Directory created");
        else
            terminal_add_line(term, "Error: Could not create directory");
    }
    // Further commands can be added here

    term->current_command[0] = '\0';
    term->cursor_position = 0;
}

void terminal_render(Terminal* term, SDL_Renderer* renderer, TTF_Font* font, SDL_Rect content_area) {
    // Calculate visible lines and max chars based on content area
    // Reserve 2 lines: one for cwd and one for the prompt
    term->visible_lines = (content_area.h - 2 * CHAR_HEIGHT) / CHAR_HEIGHT; 
    term->max_chars_per_line = (content_area.w - 10) / CHAR_WIDTH; // Account for margins

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &content_area);

    SDL_Color text_color = {0, 255, 0, 255};
    
    // Calculate the range of lines to display for history (excluding cwd and prompt)
    int start_line = term->scroll_position;
    int end_line = start_line + term->visible_lines;
    if (end_line > term->line_count) end_line = term->line_count;

    // Render terminal history lines
    for (int i = start_line; i < end_line; i++) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, term->lines[i], text_color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect position = {
                content_area.x + 5,
                content_area.y + (i - start_line) * CHAR_HEIGHT,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &position);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }

    // Render current working directory above prompt
    char cwd_buffer[MAX_PATH];
    strcpy(cwd_buffer, fs_get_current_path(term->fs));
    char cwd_display[MAX_PATH];
    int max_chars = term->max_chars_per_line - 5; // allow space for "cwd: "
    if ((int)strlen(cwd_buffer) > max_chars) {
        strncpy(cwd_display, cwd_buffer, max_chars-3);
        cwd_display[max_chars-3] = '\0';
        strcat(cwd_display, "...");
    } else {
        strcpy(cwd_display, cwd_buffer);
    }
    char cwd_text[MAX_PATH];
    snprintf(cwd_text, sizeof(cwd_text), "cwd: %s", cwd_display);
    SDL_Surface* cwdSurface = TTF_RenderText_Solid(font, cwd_text, text_color);
    if (cwdSurface) {
        SDL_Texture* cwdTexture = SDL_CreateTextureFromSurface(renderer, cwdSurface);
        SDL_Rect cwdRect = {
            content_area.x + 5,
            content_area.y + content_area.h - 2 * CHAR_HEIGHT,
            cwdSurface->w,
            cwdSurface->h
        };
        SDL_RenderCopy(renderer, cwdTexture, NULL, &cwdRect);
        SDL_DestroyTexture(cwdTexture);
        SDL_FreeSurface(cwdSurface);
    }
    
    // Render command line in fixed position at bottom
    char prompt[MAX_COMMAND_LENGTH + 3];
    snprintf(prompt, sizeof(prompt), "> %s", term->current_command);
    SDL_Surface* surface = TTF_RenderText_Solid(font, prompt, text_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect position = {
            content_area.x + 5,
            content_area.y + content_area.h - CHAR_HEIGHT,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &position);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }

    // Draw scrollbar if needed
    if (term->line_count > term->visible_lines) {
        int scrollbar_height = (content_area.h - 2 * CHAR_HEIGHT) * term->visible_lines / term->line_count;
        int scrollbar_position = (content_area.h - 2 * CHAR_HEIGHT) * term->scroll_position / term->line_count;
        
        SDL_Rect scrollbar = {
            content_area.x + content_area.w - 8,
            content_area.y + scrollbar_position,
            6,
            scrollbar_height
        };
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderFillRect(renderer, &scrollbar);
    }
}

void terminal_handle_keypress(Terminal* term, SDL_KeyboardEvent* event) {
    if (event->keysym.sym == SDLK_RETURN) {
        terminal_execute_command(term);
    } else if (event->keysym.sym == SDLK_BACKSPACE) {
        if (term->cursor_position > 0) {
            term->cursor_position--;
            term->current_command[term->cursor_position] = '\0';
        }
    } else if (event->keysym.sym == SDLK_UP) {
        if (term->history_position < term->history_count - 1) {
            term->history_position++;
            strcpy(term->current_command, term->command_history[term->history_position]);
            term->cursor_position = strlen(term->current_command);
        }
    } else if (event->keysym.sym == SDLK_DOWN) {
        if (term->history_position > 0) {
            term->history_position--;
            strcpy(term->current_command, term->command_history[term->history_position]);
            term->cursor_position = strlen(term->current_command);
        } else {
            term->current_command[0] = '\0';
            term->cursor_position = 0;
        }
    } else if (event->keysym.sym == SDLK_LEFT) {
        if (term->cursor_position > 0) term->cursor_position--;
    } else if (event->keysym.sym == SDLK_RIGHT) {
        if (term->cursor_position < strlen(term->current_command)) term->cursor_position++;
    } else if (event->keysym.sym == SDLK_PAGEUP) {
        if (term->scroll_position > 0) {
            term->scroll_position -= term->visible_lines;
            if (term->scroll_position < 0) term->scroll_position = 0;
        }
    } else if (event->keysym.sym == SDLK_PAGEDOWN) {
        int max_scroll = term->line_count - term->visible_lines;
        if (term->scroll_position < max_scroll) {
            term->scroll_position += term->visible_lines;
            if (term->scroll_position > max_scroll) term->scroll_position = max_scroll;
        }
    } else {
        if (term->cursor_position < MAX_COMMAND_LENGTH - 1) {
            term->current_command[term->cursor_position] = event->keysym.sym;
            term->cursor_position++;
            term->current_command[term->cursor_position] = '\0';
        }
    }
}

// Add new mouse wheel handling function
void terminal_handle_mouse(Terminal* term, SDL_MouseWheelEvent* event) {
    // Scroll up
    if (event->y > 0) {
        if (term->scroll_position > 0) {
            term->scroll_position--;
        }
    }
    // Scroll down
    else if (event->y < 0) {
        int max_scroll = term->line_count - term->visible_lines;
        if (term->scroll_position < max_scroll) {
            term->scroll_position++;
        }
    }
}

void terminal_open_file_view(Terminal* term, const char* path) {
    char* content = fs_read_file(term->fs, path);
    if (content) {
        terminal_add_line(term, "Viewing file:");
        char* line = strtok(content, "\n");
        while (line) {
            terminal_add_line(term, line);
            line = strtok(NULL, "\n");
        }
    } else {
        terminal_add_line(term, "Error: File not found or cannot be read.");
    }
}

void terminal_open_file_edit(Terminal* term, const char* path) {
    TextEditor* editor = editor_create();
    if (editor_load(editor, term->fs, path)) {
        editor->is_open = true;
        // Assuming SDL_Renderer* renderer and TTF_Font* font are available
        // You need to pass these parameters to this function or make them accessible
        // editor_render(editor, renderer, font);
    } else {
        terminal_add_line(term, "Error: File not found or cannot be read.");
        editor_destroy(editor);
    }
}

void terminal_reset(Terminal* term) {
    // Clear all lines except the first two (welcome messages)
    for (int i = 2; i < term->line_count; i++) {
        free(term->lines[i]);
    }
    term->line_count = 2;
    term->cursor_position = 0;
    term->scroll_position = 0;
    term->current_command[0] = '\0';
    term->history_count = 0;
    term->history_position = -1;
}
