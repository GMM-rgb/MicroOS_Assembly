#include "settings_menu.h"
#include "microos.h"  // Include the header file for draw_rounded_rect_with_shadow
#include "settings.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <math.h>

// Draws a left-side settings sidebar showing resolution and UI scale.
void draw_settings_menu(SDL_Renderer* renderer, TTF_Font* font, SystemSettings* settings, SDL_Rect window) {
    // Define sidebar dimensions (adjust width and height to a fraction of window)
    SDL_Rect sidebar = {0, 0, window.w / 2, window.h};
    // Draw background with rounded corners
    draw_rounded_rect_with_shadow(renderer, sidebar, 8, (SDL_Color){60, 60, 80, 255});
    
    // Draw header text
    SDL_Surface* headerSurf = TTF_RenderText_Solid(font, "Settings", (SDL_Color){255,255,255,255});
    if(headerSurf) {
        SDL_Texture* headerTex = SDL_CreateTextureFromSurface(renderer, headerSurf);
        SDL_Rect headerRect = {10, 10, headerSurf->w, headerSurf->h};
        SDL_RenderCopy(renderer, headerTex, NULL, &headerRect);
        SDL_DestroyTexture(headerTex);
        SDL_FreeSurface(headerSurf);
    }

    // Draw close button
    SDL_Rect closeButtonRect = {sidebar.w - 30, 10, 20, 20};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeButtonRect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, closeButtonRect.x + 5, closeButtonRect.y + 5, closeButtonRect.x + 15, closeButtonRect.y + 15);
    SDL_RenderDrawLine(renderer, closeButtonRect.x + 15, closeButtonRect.y + 5, closeButtonRect.x + 5, closeButtonRect.y + 15);
    
    // Draw resolution selector
    int y = 50;
    const char* resOptions[] = {"144p", "360p", "480p", "720p", "1080p"};
    SDL_Surface* label = TTF_RenderText_Solid(font, "Resolution:", (SDL_Color){200,200,200,255});
    if(label){
        SDL_Texture* labelTex = SDL_CreateTextureFromSurface(renderer, label);
        SDL_Rect labelRect = {10, y, label->w, label->h};
        SDL_RenderCopy(renderer, labelTex, NULL, &labelRect);
        SDL_DestroyTexture(labelTex);
        SDL_FreeSurface(label);
    }
    y += 25;
    for(int i = 0; i < 5; i++){
        SDL_Rect btn = {10, y, 120, 25};
        SDL_Color btnColor = (settings->resolution == i) ? (SDL_Color){180,180,220,255} : (SDL_Color){100,100,120,255};
        draw_rounded_rect_with_shadow(renderer, btn, 5, btnColor);
        
        SDL_Surface* optSurf = TTF_RenderText_Solid(font, resOptions[i], (SDL_Color){255,255,255,255});
        if(optSurf){
            SDL_Texture* optTex = SDL_CreateTextureFromSurface(renderer, optSurf);
            SDL_Rect optRect = {btn.x + 10, btn.y + (btn.h - optSurf->h)/2, optSurf->w, optSurf->h};
            SDL_RenderCopy(renderer, optTex, NULL, &optRect);
            SDL_DestroyTexture(optTex);
            SDL_FreeSurface(optSurf);
        }
        y += 35;
    }
    
    // Draw UI Scale slider
    SDL_Surface* scaleLabel = TTF_RenderText_Solid(font, "UI Scale:", (SDL_Color){200,200,200,255});
    if(scaleLabel){
        SDL_Texture* scaleTex = SDL_CreateTextureFromSurface(renderer, scaleLabel);
        SDL_Rect scaleRect = {10, y, scaleLabel->w, scaleLabel->h};
        SDL_RenderCopy(renderer, scaleTex, NULL, &scaleRect);
        SDL_DestroyTexture(scaleTex);
        SDL_FreeSurface(scaleLabel);
    }
    y += 25;
    SDL_Rect slider = {10, y, 100, 8};
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderFillRect(renderer, &slider);
    float knobPos = (settings->ui_scale - 1.0f) * 100.0f; // assuming ui_scale between 1.0 and 2.0
    SDL_Rect knob = {10 + (int)knobPos, y - 4, 16, 16};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &knob);
    
    // End of sidebar. You can add more options as required.
}
