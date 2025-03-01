#include "terminal.h"
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
        term->lines[term->line_count] = strdup(line);
        term->line_count++;
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
    } else if (strcmp(command, "ls") == 0) {
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
        }
    } else if (strcmp(command, "cd") == 0) {
        char* dir = strtok(NULL, " ");
        if (dir && fs_change_dir(term->fs, dir))
            terminal_add_line(term, "Directory changed");
        else
            terminal_add_line(term, "Error: Invalid directory");
    } else if (strcmp(command, "pwd") == 0) {
        terminal_add_line(term, fs_get_current_path(term->fs));
    }
    // Further commands can be added here

    term->current_command[0] = '\0';
    term->cursor_position = 0;
}

void terminal_render(Terminal* term, SDL_Renderer* renderer, TTF_Font* font, SDL_Rect content_area) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &content_area);

    SDL_Color text_color = {0, 255, 0, 255};
    int line_height = 20;
    int visible_lines = content_area.h / line_height;
    int start_line = term->line_count > visible_lines ? term->line_count - visible_lines : 0;

    for (int i = start_line; i < term->line_count; i++) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, term->lines[i], text_color);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect position = {
                content_area.x + 5,
                content_area.y + (i - start_line) * line_height,
                surface->w,
                surface->h
            };
            SDL_RenderCopy(renderer, texture, NULL, &position);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
    }

    char prompt[MAX_COMMAND_LENGTH + 3];
    snprintf(prompt, sizeof(prompt), "> %s", term->current_command);
    SDL_Surface* surface = TTF_RenderText_Solid(font, prompt, text_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect position = {
            content_area.x + 5,
            content_area.y + (term->line_count - start_line) * line_height,
            surface->w,
            surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &position);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
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
    } else {
        if (term->cursor_position < MAX_COMMAND_LENGTH - 1) {
            term->current_command[term->cursor_position] = event->keysym.sym;
            term->cursor_position++;
            term->current_command[term->cursor_position] = '\0';
        }
    }
}
