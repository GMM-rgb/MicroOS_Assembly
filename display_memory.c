/**
 * display_memory.c
 *
 * C implementation of the display memory functions
 * Used as a fallback for platforms where the assembly implementation
 * isn't available
 */

#include <stdio.h>

void initialize_display_memory(unsigned char *display_memory)
{
    // Initialize the display memory with a pattern
    for (int i = 0; i < 64; i++)
    {
        display_memory[i] = i * 4; // Same pattern as the assembly version
    }
    printf("C implementation: Display memory initialized\n");
}

void update_display(unsigned char *display_memory)
{
    // Animate the display memory with a simple pattern
    static int tick = 0;

    for (int i = 0; i < 64; i++)
    {
        // Create a simple animated pattern
        display_memory[i] = (display_memory[i] + 1) % 256;

        // Add some variation based on position and time
        if ((i + tick) % 5 == 0)
        {
            display_memory[i] = (display_memory[i] + 10) % 256;
        }
    }

    tick++;
}
