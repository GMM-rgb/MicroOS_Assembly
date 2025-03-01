#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

// Function declarations for assembly routines
extern void initialize_display_memory(unsigned char *display_memory);
extern void update_display(unsigned char *display_memory);

// OS State
typedef enum
{
    OS_STATE_BOOT,
    OS_STATE_DESKTOP,
    OS_STATE_APP1,
    OS_STATE_APP2,
    OS_STATE_MENU
} OSState;

// Application structure
typedef struct
{
    char name[32];
    SDL_Rect icon;
    SDL_Color color;
} Application;

// Add a function to visualize the display memory pattern
void render_display_memory(SDL_Renderer *renderer, unsigned char *display_memory)
{
    int cell_size = 10;  // Size of each display memory cell when rendered
    int display_x = 25;  // X position for display visualization in the terminal app
    int display_y = 100; // Y position for display visualization in the terminal app

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            unsigned char value = display_memory[y * 8 + x];
            SDL_Rect cell = {
                display_x + x * cell_size,
                display_y + y * cell_size,
                cell_size,
                cell_size};

            SDL_SetRenderDrawColor(renderer, value, value / 2, 255 - value, 255);
            SDL_RenderFillRect(renderer, &cell);
        }
    }
}

void draw_rounded_rect(SDL_Renderer *renderer, SDL_Rect rect, int radius, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    // Draw the central rectangle
    SDL_Rect centralRect = {rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_RenderFillRect(renderer, &centralRect);

    // Draw the left and right rectangles
    SDL_Rect leftRect = {rect.x, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &leftRect);

    SDL_Rect rightRect = {rect.x + rect.w - radius, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &rightRect);

    // Draw the top and bottom rectangles
    SDL_Rect topRect = {rect.x + radius, rect.y, rect.w - 2 * radius, radius};
    SDL_RenderFillRect(renderer, &topRect);

    SDL_Rect bottomRect = {rect.x + radius, rect.y + rect.h - radius, rect.w - 2 * radius, radius};
    SDL_RenderFillRect(renderer, &bottomRect);

    // Draw the four corners using filled circles
    for (int w = 0; w < radius * 3; w++)
    {
        for (int h = 0; h < radius * 3; h++)
        {
            int dx = radius - w; // Horizontal offset
            int dy = radius - h; // Vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius))
            {
                SDL_RenderDrawPoint(renderer, rect.x + dx + radius, rect.y + dy + radius);
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - dx - radius, rect.y + dy + radius);
                SDL_RenderDrawPoint(renderer, rect.x + dx + radius, rect.y + rect.h - dy - radius);
                SDL_RenderDrawPoint(renderer, rect.x + rect.w - dx - radius, rect.y + rect.h - dy - radius);
            }
        }
    }
}

void draw_taskbar(SDL_Renderer *renderer, TTF_Font *font)
{
    // Draw taskbar background
    SDL_Rect taskbarRect = {0, 280, 320, 40};
    SDL_Color taskbarColor = {50, 50, 70, 255};
    SDL_SetRenderDrawColor(renderer, taskbarColor.r, taskbarColor.g, taskbarColor.b, taskbarColor.a);
    SDL_RenderFillRect(renderer, &taskbarRect);

    // Draw start button
    SDL_Rect startButtonRect = {5, 285, 50, 30};
    SDL_Color startButtonColor = {0, 120, 215, 255};
    draw_rounded_rect(renderer, startButtonRect, 5, startButtonColor);

    // Render "Start" text
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Start", textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect textRect = {startButtonRect.x + 5, startButtonRect.y + 5, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Draw clock
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timeString[9];
    strftime(timeString, sizeof(timeString), "%H:%M:%S", timeinfo);

    textSurface = TTF_RenderText_Solid(font, timeString, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect clockRect = {260, 290, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &clockRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }
}

void draw_desktop(SDL_Renderer *renderer, TTF_Font *font, Application *apps, int appCount)
{
    // Draw desktop background
    SDL_SetRenderDrawColor(renderer, 0, 100, 170, 255);
    SDL_Rect desktopRect = {0, 0, 320, 280};
    SDL_RenderFillRect(renderer, &desktopRect);

    // Draw application icons
    SDL_Color textColor = {255, 255, 255, 255};
    for (int i = 0; i < appCount; i++)
    {
        // Draw icon background
        draw_rounded_rect(renderer, apps[i].icon, 5, apps[i].color);

        // Draw icon text
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, apps[i].name, textColor);
        if (textSurface != NULL)
        {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != NULL)
            {
                SDL_Rect textRect = {
                    apps[i].icon.x + (apps[i].icon.w - textSurface->w) / 2,
                    apps[i].icon.y + apps[i].icon.h + 5,
                    textSurface->w,
                    textSurface->h};
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        else
        {
            fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        }
    }
}

void draw_boot_sequence(SDL_Renderer *renderer, TTF_Font *font, int progress, unsigned char *display_memory)
{
    // Background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // OS Name
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "MicroOS 1.0", textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect textRect = {(320 - textSurface->w) / 2, 50, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Progress bar border
    SDL_Rect progressBorderRect = {60, 100, 200, 20};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &progressBorderRect);

    // Progress bar fill
    SDL_Rect progressFillRect = {62, 102, (int)(196.0f * (progress / 100.0f)), 16};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &progressFillRect);

    // Progress text
    char progressText[10];
    sprintf(progressText, "%d%%", progress);
    textSurface = TTF_RenderText_Solid(font, progressText, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect progressTextRect = {(320 - textSurface->w) / 2, 130, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &progressTextRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Display the memory visualization during boot as a sort of "hardware check"
    render_display_memory(renderer, display_memory);

    // Add some loading text
    const char *loadingText = "Checking hardware...";
    textSurface = TTF_RenderText_Solid(font, loadingText, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect loadingTextRect = {(320 - textSurface->w) / 2, 200, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &loadingTextRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }
}

void draw_app(SDL_Renderer *renderer, TTF_Font *font, const char *appName, SDL_Color appColor, unsigned char *display_memory)
{
    // App window background
    SDL_Rect windowRect = {20, 20, 280, 240};
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_RenderFillRect(renderer, &windowRect);

    // App title bar
    SDL_Rect titleBarRect = {20, 20, 280, 25};
    SDL_SetRenderDrawColor(renderer, appColor.r, appColor.g, appColor.b, appColor.a);
    SDL_RenderFillRect(renderer, &titleBarRect);

    // App title
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, appName, textColor);
    if (textSurface != NULL)
    {
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture != NULL)
        {
            SDL_Rect textRect = {25, 22, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_DestroyTexture(textTexture);
        }
        SDL_FreeSurface(textSurface);
    }
    else
    {
        fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
    }

    // Close button
    SDL_Rect closeButtonRect = {275, 20, 25, 25};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &closeButtonRect);

    // X on close button
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, 280, 25, 295, 40);
    SDL_RenderDrawLine(renderer, 295, 25, 280, 40);

    // App content area
    SDL_Rect contentRect = {25, 50, 270, 205};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &contentRect);

    // Display content based on app
    if (strcmp(appName, "Terminal") == 0)
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &contentRect);

        const char *lines[] = {
            "MicroOS Terminal v1.0",
            "> _"};

        for (int i = 0; i < 2; i++)
        {
            SDL_Color terminalText = {0, 255, 0, 255};
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, lines[i], terminalText);
            if (textSurface != NULL)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL)
                {
                    SDL_Rect lineRect = {30, 60 + i * 25, textSurface->w, textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, NULL, &lineRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            else
            {
                fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
            }
        }

        // Render the display memory visualization in the terminal
        render_display_memory(renderer, display_memory);

        // Add a label for the display visualization
        SDL_Color terminalText = {0, 255, 0, 255};
        textSurface = TTF_RenderText_Solid(font, "Display Memory:", terminalText);
        if (textSurface != NULL)
        {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != NULL)
            {
                SDL_Rect labelRect = {30, 85, textSurface->w, textSurface->h};
                SDL_RenderCopy(renderer, textTexture, NULL, &labelRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        else
        {
            fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        }
    }
    else if (strcmp(appName, "Settings") == 0)
    {
        const char *options[] = {
            "Display",
            "Sound",
            "Network",
            "About"};

        for (int i = 0; i < 4; i++)
        {
            SDL_Rect optionRect = {35, 60 + i * 40, 250, 30};
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
            SDL_RenderFillRect(renderer, &optionRect);

            SDL_Color optionText = {0, 0, 0, 255};
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, options[i], optionText);
            if (textSurface != NULL)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL)
                {
                    SDL_Rect textRect = {45, 65 + i * 40, textSurface->w, textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            else
            {
                fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
            }
        }
    }
}

void draw_start_menu(SDL_Renderer *renderer, TTF_Font *font)
{
    // Menu background
    SDL_Rect menuRect = {5, 100, 150, 180};
    SDL_Color menuColor = {50, 50, 70, 255};
    draw_rounded_rect(renderer, menuRect, 5, menuColor);

    // Menu items
    const char *menuItems[] = {
        "Terminal",
        "Settings",
        "File Explorer",
        "Shutdown"};

    SDL_Color textColor = {255, 255, 255, 255};
    for (int i = 0; i < 4; i++)
    {
        SDL_Rect itemRect = {10, 110 + i * 40, 140, 35};
        SDL_Color itemColor = {70, 70, 90, 255};
        draw_rounded_rect(renderer, itemRect, 5, itemColor);

        SDL_Surface *textSurface = TTF_RenderText_Solid(font, menuItems[i], textColor);
        if (textSurface != NULL)
        {
            SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            if (textTexture != NULL)
            {
                SDL_Rect textRect = {20, 120 + i * 40, textSurface->w, textSurface->h};
                SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                SDL_DestroyTexture(textTexture);
            }
            SDL_FreeSurface(textSurface);
        }
        else
        {
            fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
        }
    }
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("MicroOS", 100, 100, 320, 320, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        SDL_DestroyWindow(window);
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 14);
    if (font == NULL)
    {
        // Try alternative font paths
        font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 14);
        if (font == NULL)
        {
            font = TTF_OpenFont("/Library/Fonts/Arial.ttf", 14);
            if (font == NULL)
            {
                fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
                fprintf(stderr, "Unable to load any fonts. Exiting...\n");
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                TTF_Quit();
                SDL_Quit();
                return 1;
            }
        }
    }

    unsigned char display_memory[64];
    // Initialize display_memory with zeros as a safety measure
    memset(display_memory, 0, sizeof(display_memory));
    initialize_display_memory(display_memory);

    // Define applications
    Application apps[3];
    strcpy(apps[0].name, "Terminal");
    apps[0].icon = (SDL_Rect){40, 40, 50, 50};
    apps[0].color = (SDL_Color){0, 0, 0, 255};

    strcpy(apps[1].name, "Settings");
    apps[1].icon = (SDL_Rect){140, 40, 50, 50};
    apps[1].color = (SDL_Color){100, 100, 200, 255};

    strcpy(apps[2].name, "Files");
    apps[2].icon = (SDL_Rect){240, 40, 50, 50};
    apps[2].color = (SDL_Color){50, 150, 50, 255};

    OSState currentState = OS_STATE_BOOT;
    int bootProgress = 0;
    bool showMenu = false;
    char notification[64] = "";
    time_t notificationTime = 0;
    SDL_Color notificationBgColor = {0, 0, 0, 200};
    SDL_Color currentAppColor = {0, 0, 0, 255};
    char currentAppName[32] = "";

    bool running = true;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();
    int frameCount = 0;

    printf("MicroOS starting...\n");
    printf("Press any key to exit the application\n");

    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int x = e.button.x;
                int y = e.button.y;

                if (currentState == OS_STATE_DESKTOP)
                {
                    // Check if start button was clicked
                    if (x >= 5 && x <= 55 && y >= 285 && y <= 315)
                    {
                        showMenu = !showMenu;
                    }

                    // Check if any app icon was clicked
                    for (int i = 0; i < 3; i++)
                    {
                        if (x >= apps[i].icon.x && x <= apps[i].icon.x + apps[i].icon.w &&
                            y >= apps[i].icon.y && y <= apps[i].icon.y + apps[i].icon.h)
                        {
                            if (i == 0)
                            {
                                currentState = OS_STATE_APP1;
                                strcpy(currentAppName, "Terminal");
                                currentAppColor = apps[i].color;
                            }
                            else if (i == 1)
                            {
                                currentState = OS_STATE_APP2;
                                strcpy(currentAppName, "Settings");
                                currentAppColor = apps[i].color;
                            }
                            showMenu = false;
                            snprintf(notification, sizeof(notification), "Opening %s...", apps[i].name);
                            notificationTime = time(NULL);
                        }
                    }

                    // Check menu items if menu is shown
                    if (showMenu)
                    {
                        for (int i = 0; i < 4; i++)
                        {
                            if (x >= 10 && x <= 150 && y >= 110 + i * 40 && y <= 145 + i * 40)
                            {
                                if (i == 0)
                                {
                                    currentState = OS_STATE_APP1;
                                    strcpy(currentAppName, "Terminal");
                                    currentAppColor = apps[0].color;
                                }
                                else if (i == 1)
                                {
                                    currentState = OS_STATE_APP2;
                                    strcpy(currentAppName, "Settings");
                                    currentAppColor = apps[1].color;
                                }
                                else if (i == 3)
                                {
                                    // Shutdown
                                    currentState = OS_STATE_BOOT;
                                    bootProgress = 0;
                                }
                                showMenu = false;
                            }
                        }
                    }
                }
                else if (currentState == OS_STATE_APP1 || currentState == OS_STATE_APP2)
                {
                    // Check if close button was clicked
                    if (x >= 275 && x <= 300 && y >= 20 && y <= 45)
                    {
                        currentState = OS_STATE_DESKTOP;
                        snprintf(notification, sizeof(notification), "Closed %s", currentAppName);
                        notificationTime = time(NULL);
                    }
                }
            }
        }

        // Update logic
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 16)
        { // ~60 FPS
            // Call assembly function to update display memory
            update_display(display_memory);

            // Handle boot sequence
            if (currentState == OS_STATE_BOOT)
            {
                bootProgress += 1;
                if (bootProgress >= 100)
                {
                    currentState = OS_STATE_DESKTOP;
                    snprintf(notification, sizeof(notification), "MicroOS Started!");
                    notificationTime = time(NULL);
                }
            }

            lastTime = currentTime;
            frameCount++;
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render based on current state
        switch (currentState)
        {
        case OS_STATE_BOOT:
            draw_boot_sequence(renderer, font, bootProgress, display_memory);
            break;

        case OS_STATE_DESKTOP:
            draw_desktop(renderer, font, apps, 3);
            draw_taskbar(renderer, font);
            if (showMenu)
            {
                draw_start_menu(renderer, font);
            }
            break;

        case OS_STATE_APP1:
        case OS_STATE_APP2:
            draw_desktop(renderer, font, apps, 3);
            draw_taskbar(renderer, font);
            draw_app(renderer, font, currentAppName, currentAppColor, display_memory);
            break;

        default:
            break;
        }

        // Show notification if needed
        if (time(NULL) - notificationTime < 3)
        {
            // Draw notification background
            SDL_Rect notifRect = {50, 150, 220, 40};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, notificationBgColor.r, notificationBgColor.g,
                                   notificationBgColor.b, notificationBgColor.a);
            draw_rounded_rect(renderer, notifRect, 8, notificationBgColor);

            // Draw notification text
            SDL_Color notifTextColor = {255, 255, 255, 255};
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, notification, notifTextColor);
            if (textSurface != NULL)
            {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture != NULL)
                {
                    SDL_Rect textRect = {
                        (320 - textSurface->w) / 2,
                        (notifRect.y + (notifRect.h - textSurface->h) / 2),
                        textSurface->w,
                        textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            else
            {
                fprintf(stderr, "TTF_RenderText_Solid Error: %s\n", TTF_GetError());
            }
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_RenderPresent(renderer);

        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    printf("MicroOS exited.\n");
    return 0;
}
