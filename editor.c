#include "fileui.h"    // Assuming both UI headers share the same fileui.h
#include "editor.h"
#include <stdlib.h>
#include <string.h>

// Create and initialize a TextEditor instance with empty content.
TextEditor* editor_create(void) {
    TextEditor* editor = malloc(sizeof(TextEditor));
    editor->is_open = false;
    editor->file_path = NULL;
    editor->content = strdup(""); // empty content
    editor->cursor_position = 0;
    editor->scroll_position = 0;
    editor->window_rect = (SDL_Rect){40, 40, 240, 200}; // default editor window
    return editor;
}

// Clean up the TextEditor instance.
void editor_destroy(TextEditor* editor) {
    if(editor->file_path)
        free(editor->file_path);
    if(editor->content)
        free(editor->content);
    free(editor);
}

// Render the TextEditor window with a header and close button.
void editor_render(TextEditor* editor, SDL_Renderer* renderer, TTF_Font* font) {
    if (!editor->is_open)
        return;
        
    // Draw editor window background
    SDL_SetRenderDrawColor(renderer, 255, 255, 240, 255);
    SDL_RenderFillRect(renderer, &editor->window_rect);
    
    // Draw header bar
    SDL_Rect header = {editor->window_rect.x, editor->window_rect.y, editor->window_rect.w, 25};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &header);
    
    // Draw close button (X) at top right of header
    SDL_Rect closeBtn = {editor->window_rect.x + editor->window_rect.w - 25, editor->window_rect.y, 25, 25};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeBtn);
    
    // Render editor content (stub)
    SDL_Surface* surface = TTF_RenderText_Solid(font, editor->content, (SDL_Color){0,0,0,255});
    if(surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect contentRect = {
            editor->window_rect.x + 5, 
            editor->window_rect.y + 30, 
            surface->w, surface->h
        };
        SDL_RenderCopy(renderer, texture, NULL, &contentRect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
}

// Handle events for the TextEditor window (return true if event handled)
bool editor_handle_event(TextEditor* editor, SDL_Event* event) {
    // Minimal stub: if close button (for example, mouse click in header's close region) is pressed, close editor.
    // (You can add text input handling later.)
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;
        SDL_Rect closeBtn = {editor->window_rect.x + editor->window_rect.w - 25, editor->window_rect.y, 25, 25};
        if(x >= closeBtn.x && x <= closeBtn.x + closeBtn.w &&
           y >= closeBtn.y && y <= closeBtn.y + closeBtn.h) {
               editor->is_open = false;
               return true;
           }
    }
    return false;
}

// Stub function to "save" the editor content to the filesystem.
void editor_save(TextEditor* editor, FileSystem* fs) {
    // For now, just print content.
    printf("Saving file: %s\nContent:\n%s\n", editor->file_path ? editor->file_path : "(unnamed)", editor->content);
}
