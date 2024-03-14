#pragma once
#include "cart.h"

typedef struct {
    Cart *cart;
    Font font;
} NeXUS_VM;

extern NeXUS_VM vm;