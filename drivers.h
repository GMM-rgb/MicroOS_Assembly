#ifndef DRIVERS_H
#define DRIVERS_H

#include <stdbool.h>

// Initialize the OS drivers based on mode (raw or simulation)
void init_drivers(bool raw_mode);

#endif // DRIVERS_H
