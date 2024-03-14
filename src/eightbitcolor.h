#include "raylib.h"
#include <math.h>
#include <stdint.h>

extern Color eightbitcolor_LUT[256];

void eightbitcolor_init();
uint8_t eightbitcolor_nearest(Color c);