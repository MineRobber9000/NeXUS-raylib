#include <stdint.h>
#include <string.h>

typedef struct {
	unsigned char *code;
	size_t code_size;
} Cart;

Cart *LoadCart(char * filename);
