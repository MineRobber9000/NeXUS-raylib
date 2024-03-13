#include "eightbitcolor.h"

Color eightbitcolor_LUT[256] = { { 0 } };

void eightbitcolor_init_color(uint8_t col) {
    double r,g,b;
    r = (col&7)/7.0f;
    g = ((col>>3)&7)/7.0f;
    b = ((col>>6)&3)/3.0f;
    Color *c = &eightbitcolor_LUT[col];
    c->r = (int)round(r*255);
    c->g = (int)round(g*255);
    c->b = (int)round(b*255);
    c->a = 255; // alpha is always 255
}

void eightbitcolor_init() {
    for (int i=0;i<256;++i) eightbitcolor_init_color(i);
}
