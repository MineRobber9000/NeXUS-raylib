#include "raylib.h"
#include <stdint.h>
#include <string.h>

struct Cart_GraphicsPage {
	uint32_t id;
	uint32_t width;
	uint32_t height;
	Image img;
	struct Cart_GraphicsPage *next;
};

typedef struct Cart_GraphicsPage Cart_GraphicsPage;

typedef struct {
	unsigned char *code;
	size_t code_size;
	Cart_GraphicsPage *graphics;
} Cart;

Cart *LoadCart(char * filename);