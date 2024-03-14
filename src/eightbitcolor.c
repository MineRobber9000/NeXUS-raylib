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

uint8_t eightbitcolor_nearest(Color c) {
    double r, g, b;
    r = c.r/255.0f;
    g = c.g/255.0f;
    b = c.b/255.0f;
    uint8_t rb, gb, bb;
    rb = (uint8_t)round(r*7);
    gb = (uint8_t)round(g*7);
    bb = (uint8_t)round(b*3);
    if (rb<0) rb=0;
    if (rb>7) rb=7;
    if (gb<0) gb=0;
    if (gb>7) gb=7;
    if (bb<0) bb=0;
    if (bb>3) bb=3;
    return (uint8_t)((bb<<6)|(gb<<3)|rb);
}