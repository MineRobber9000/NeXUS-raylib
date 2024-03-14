#pragma once
#include "cart.h"

typedef struct {
    Cart *cart;
    RenderTexture2D framebuffer;
    Image screen;
    int screen_dirty;
    Font font;
} NeXUS_VM;

extern NeXUS_VM vm;

#define HAS_SCREEN() ((vm.screen.data!=NULL)&&(!vm.screen_dirty))
#define NO_SCREEN() ((vm.screen.data==NULL)||(vm.screen_dirty))