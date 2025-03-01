#include "fileui.h"    // Assuming both UI headers share the same fileui.h
#include "editor.h"
#include <stdlib.h>
#include <string.h>

static const SDL_Color TOOLBAR_COLOR = {240, 240, 240, 255};
static const SDL_Color RULER_COLOR = {220, 220, 220, 255};
static const SDL_Color BUTTON_HOVER_COLOR = {200, 200, 200, 255};
static const SDL_Color SELECTION_COLOR = {51, 153, 255, 128};

// Create and initialize a TextEditor instance with empty content.
TextEditor* editor_create(void) {
    TextEditor* editor = malloc(sizeof(TextEditor));
    editor->is_open = false;
    editor->file_path = NULL;
    editor->line_count = 1;
    editor->lines[0].text = strdup("");
    editor->lines[0].length = 0;
    editor->cursor_line = 0;
    editor->cursor_col = 0;
    editor->scroll_x = 0;
    editor->scroll_y = 0;
    editor->selection_start_line = -1;
    editor->window_rect = (SDL_Rect){0, 0, 320, 320}; // Full screen
    editor->text_color = (SDL_Color){0, 0, 0, 255};
    editor->current_style = FONT_NORMAL;
    editor->font_size = 14.0f;
    editor->show_toolbar = true;
    editor->show_ruler = true;
    editor->word_wrap = true;
    editor->clipboard = NULL;
    editor->undo_count = 0;
    editor->undo_position = -1;
    editor->has_changes = false;
    return editor;
}

// Clean up the TextEditor instance.
void editor_destroy(TextEditor* editor) {
    if(editor->file_path)
        free(editor->file_path);
    for (int i = 0; i < editor->line_count; i++) {
        free(editor->lines[i].text);
    }
    free(editor);
}

static void editor_save_undo_state(TextEditor* editor) {
    // Save current state to undo buffer
    char* state = malloc(MAX_LINE_LENGTH * editor->line_count);
    int pos = 0;
    for (int i = 0; i < editor->line_count; i++) {
        strcpy(state + pos, editor->lines[i].text);
        pos += editor->lines[i].length + 1;
    }
    
    // Remove any redo states
    while (editor->undo_count > editor->undo_position + 1) {
        free(editor->undo_buffer[--editor->undo_count]);
    }
    
    // Add new state
    editor->undo_buffer[editor->undo_count++] = state;
    editor->undo_position++;
    
    // Remove oldest state if buffer is full
    if (editor->undo_count > MAX_UNDO_STEPS) {
        free(editor->undo_buffer[0]);
        memmove(editor->undo_buffer, editor->undo_buffer + 1, 
                (MAX_UNDO_STEPS - 1) * sizeof(char*));
        editor->undo_count--;
        editor->undo_position--;
    }
}

// Render the TextEditor window with a header and close button.
void editor_render(TextEditor* editor, SDL_Renderer* renderer, TTF_Font* font) {
    if (!editor->is_open) return;

    // Draw editor background
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &editor->window_rect);

    // Draw toolbar if enabled
    if (editor->show_toolbar) {
        SDL_Rect toolbar = {
            editor->window_rect.x,
            editor->window_rect.y,
            editor->window_rect.w,
            30
        };
        SDL_SetRenderDrawColor(renderer, TOOLBAR_COLOR.r, TOOLBAR_COLOR.g, 
                             TOOLBAR_COLOR.b, TOOLBAR_COLOR.a);
        SDL_RenderFillRect(renderer, &toolbar);

        // Draw toolbar buttons
        const char* buttons[] = {"Save", "Bold", "Italic", "Underline"};
        for (int i = 0; i < 4; i++) {
            SDL_Rect btn = {toolbar.x + 5 + i * 60, toolbar.y + 5, 55, 20};
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderFillRect(renderer, &btn);
            
            SDL_Surface* surface = TTF_RenderText_Solid(font, buttons[i], 
                                                      (SDL_Color){0, 0, 0, 255});
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect text_pos = {
                    btn.x + (btn.w - surface->w) / 2,
                    btn.y + (btn.h - surface->h) / 2,
                    surface->w,
                    surface->h
                };
                SDL_RenderCopy(renderer, texture, NULL, &text_pos);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
        }
    }

    // Draw ruler if enabled
    if (editor->show_ruler) {
        SDL_Rect ruler = {
            editor->window_rect.x,
            editor->window_rect.y + (editor->show_toolbar ? 30 : 0),
            editor->window_rect.w,
            20
        };
        SDL_SetRenderDrawColor(renderer, RULER_COLOR.r, RULER_COLOR.g, 
                             RULER_COLOR.b, RULER_COLOR.a);
        SDL_RenderFillRect(renderer, &ruler);

        // Draw ruler markings
        for (int i = 0; i < editor->window_rect.w / 50; i++) {
            char num[4];
            snprintf(num, sizeof(num), "%d", i * 5);
            SDL_Surface* surface = TTF_RenderText_Solid(font, num, 
                                                      (SDL_Color){128, 128, 128, 255});
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect pos = {
                    ruler.x + i * 50 + 5,
                    ruler.y + 2,
                    surface->w,
                    surface->h
                };
                SDL_RenderCopy(renderer, texture, NULL, &pos);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
        }
    }

    // Calculate content area
    SDL_Rect content = {
        editor->window_rect.x,
        editor->window_rect.y + (editor->show_toolbar ? 30 : 0) + (editor->show_ruler ? 20 : 0),
        editor->window_rect.w,
        editor->window_rect.h - (editor->show_toolbar ? 30 : 0) - (editor->show_ruler ? 20 : 0)
    };

    // Draw text content
    int y = content.y - editor->scroll_y;
    for (int i = 0; i < editor->line_count; i++) {
        if (y + editor->font_size > content.y && y < content.y + content.h) {
            // Draw selection if this line is selected
            if (editor->selection_start_line >= 0 && 
                i >= editor->selection_start_line && 
                i <= editor->selection_end_line) {
                SDL_Rect sel = {
                    content.x,
                    y,
                    editor->window_rect.w,
                    editor->font_size + 2
                };
                SDL_SetRenderDrawColor(renderer, SELECTION_COLOR.r, SELECTION_COLOR.g,
                                     SELECTION_COLOR.b, SELECTION_COLOR.a);
                SDL_RenderFillRect(renderer, &sel);
            }

            // Draw text
            SDL_Surface* surface = TTF_RenderText_Solid(font, editor->lines[i].text,
                                                      editor->text_color);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_Rect pos = {
                    content.x + 5 - editor->scroll_x,
                    y,
                    surface->w,
                    surface->h
                };
                SDL_RenderCopy(renderer, texture, NULL, &pos);
                SDL_DestroyTexture(texture);
                SDL_FreeSurface(surface);
            }
        }
        y += editor->font_size + 2;
    }

    // Draw cursor
    if (SDL_GetTicks() % 1000 < 500) {  // Blinking cursor
        int cursor_x = content.x + 5 + editor->cursor_col * 8 - editor->scroll_x;
        int cursor_y = content.y + editor->cursor_line * (editor->font_size + 2) - editor->scroll_y;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawLine(renderer, cursor_x, cursor_y, cursor_x, cursor_y + editor->font_size);
    }

    // Draw close button
    SDL_Rect closeBtn = {editor->window_rect.x + editor->window_rect.w - 25, editor->window_rect.y, 25, 25};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeBtn);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, closeBtn.x + 5, closeBtn.y + 5, closeBtn.x + 20, closeBtn.y + 20);
    SDL_RenderDrawLine(renderer, closeBtn.x + 20, closeBtn.y + 5, closeBtn.x + 5, closeBtn.y + 20);
}

// Add a helper function to delete a word
static void editor_delete_word(TextEditor* editor) {
    DocumentLine* line = &editor->lines[editor->cursor_line];
    if (editor->cursor_col == 0) return;

    int start = editor->cursor_col - 1;
    while (start > 0 && line->text[start] == ' ') start--;
    while (start > 0 && line->text[start] != ' ') start--;

    if (line->text[start] == ' ') start++;

    int length = editor->cursor_col - start;
    memmove(line->text + start, line->text + editor->cursor_col, line->length - editor->cursor_col + 1);
    line->length -= length;
    editor->cursor_col = start;
    editor->has_changes = true;
}

// Handle events for the TextEditor window (return true if event handled)
bool editor_handle_event(TextEditor* editor, SDL_Event* event, FileSystem* fs) {
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;
        SDL_Rect closeBtn = {editor->window_rect.x + editor->window_rect.w - 25, editor->window_rect.y, 25, 25};
        if(x >= closeBtn.x && x <= closeBtn.x + closeBtn.w &&
           y >= closeBtn.y && y <= closeBtn.y + closeBtn.h) {
               if (editor->has_changes) {
                   // Show save confirmation dialog
                   // For simplicity, we'll just print to console
                   printf("Do you want to save changes? (y/n)\n");
                   char response;
                   scanf(" %c", &response);
                   if (response == 'y' || response == 'Y') {
                       // Save file
                       printf("Enter file path to save: ");
                       char path[256];
                       scanf("%s", path);
                       editor->file_path = strdup(path);
                       // Pass the FileSystem pointer to editor_save
                       editor_save(editor, fs);
                   }
               }
               editor->is_open = false;
               return true;
           }
    } else if (event->type == SDL_KEYDOWN) {
        if (event->key.keysym.sym == SDLK_BACKSPACE) {
            if (SDL_GetModState() & KMOD_CTRL) {
                editor_delete_word(editor);
            } else {
                DocumentLine* line = &editor->lines[editor->cursor_line];
                if (editor->cursor_col > 0) {
                    memmove(line->text + editor->cursor_col - 1, line->text + editor->cursor_col, line->length - editor->cursor_col + 1);
                    line->length--;
                    editor->cursor_col--;
                    editor->has_changes = true;
                }
            }
            return true;
        }
    }
    return false;
}

// Save the editor content to the filesystem.
bool editor_save(TextEditor* editor, FileSystem* fs) {
    if (!editor->file_path) return false;

    // Concatenate all lines into a single string
    char* content = malloc(MAX_DOCUMENT_LINES * MAX_LINE_LENGTH);
    content[0] = '\0';
    for (int i = 0; i < editor->line_count; i++) {
        strcat(content, editor->lines[i].text);
        strcat(content, "\n");
    }

    bool result = fs_write_file(fs, editor->file_path, content);
    free(content);
    return result;
}

void editor_insert_text(TextEditor* editor, const char* text) {
    if (!editor->is_open) return;

    // Save undo state
    editor_save_undo_state(editor);

    // Insert text at the current cursor position
    DocumentLine* line = &editor->lines[editor->cursor_line];
    int text_len = strlen(text);
    int new_length = line->length + text_len;

    if (new_length >= MAX_LINE_LENGTH) {
        text_len = MAX_LINE_LENGTH - line->length - 1;
    }

    memmove(line->text + editor->cursor_col + text_len, line->text + editor->cursor_col, line->length - editor->cursor_col + 1);
    memcpy(line->text + editor->cursor_col, text, text_len);
    line->length += text_len;
    editor->cursor_col += text_len;

    editor->has_changes = true;
}
