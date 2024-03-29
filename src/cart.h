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

struct Cart_Blob {
	uint32_t id;
	uint32_t size;
	uint8_t *data;
	struct Cart_Blob *next;
};

typedef struct Cart_Blob Cart_Blob;

struct Cart_Sprites {
	uint32_t id;
	Image img;
	Texture2D texture;
	struct Cart_Sprites *next;
};

typedef struct Cart_Sprites Cart_Sprites;

typedef struct {
	unsigned char *code;
	size_t code_size;
	Cart_GraphicsPage *graphics;
	Cart_Blob *blobs;
	Cart_Sprites *sprites;
} Cart;

Cart *LoadCart(char * filename);
void FreeCart(Cart *cart);