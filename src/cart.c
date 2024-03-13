#include "raylib.h"
#include "cart.h"
#include "eightbitcolor.h"
#include "riff.h"
#include <string.h>

FourCC _CODE = {'C','O','D','E'};
FourCC _GRPH = {'G','R','P','H'};
FourCC _BIN = {'B', 'I', 'N', ' '};

void CartChunkWalker(Cart *cart, RIFF_Chunk *chunk)
{
    if (riff_is_container(chunk->type)) {
        RIFF_ChunkListItem *walker = chunk->contains.chunks;
        while (walker!=NULL) {
            CartChunkWalker(cart,walker->chunk);
            walker = walker->next;
        }
    } else {
        if (riff_fourcc_equals(chunk->type,_CODE)) {
            if (cart->code_size==0) {
                cart->code = MemAlloc(chunk->size);
                memcpy(cart->code,chunk->contains.data,chunk->size);
                cart->code_size = chunk->size;
            } else {
                cart->code = MemRealloc(cart->code, cart->code_size+chunk->size);
                memcpy(cart->code+cart->code_size,chunk->contains.data,chunk->size);
                cart->code_size += chunk->size;
            }
        }
        if (riff_fourcc_equals(chunk->type,_GRPH)) {
            uint32_t id = ((uint32_t*)chunk->contains.data)[0];
            uint32_t width = ((uint32_t*)chunk->contains.data)[1];
            uint32_t height = ((uint32_t*)chunk->contains.data)[2];
            if ((width*height)>(chunk->size-12)) {
                TraceLog(LOG_WARNING, "CART: Truncated graphics chunk; will read all the pixels I can");
            }
            Image img = GenImageColor(width, height, (Color){255,0,255,255});
            int x = 0;
            int y = 0;
            for (int i=12;i<chunk->size;++i) {
                ImageDrawPixel(&img, x, y, eightbitcolor_LUT[chunk->contains.data[i]&0xFF]);
                if ((++x)==width) {
                    x = 0;
                    ++y;
                }
            }
            Cart_GraphicsPage *grph = MemAlloc(sizeof(Cart_GraphicsPage));
            grph->id = id;
            grph->width = width;
            grph->height = height;
            grph->img = img;
            grph->next = cart->graphics;
            cart->graphics = grph;
        }
        if (riff_fourcc_equals(chunk->type,_BIN)) {
            uint32_t id = ((uint32_t*)chunk->contains.data)[0];
            Cart_Blob *blob = MemAlloc(sizeof(Cart_Blob));
            blob->id = id;
            blob->size = chunk->size - 4;
            blob->data = MemAlloc(blob->size);
            memcpy(blob->data,chunk->contains.data+4,blob->size);
            blob->next = cart->blobs;
            cart->blobs = blob;
        }
    }
}

Cart *LoadCart(char * filename)
{
    Cart *ret = MemAlloc(sizeof(Cart));
    int len;
    unsigned char *data = LoadFileData(filename,&len);
    TraceLog(LOG_INFO,"CART: Loaded cart file, %d bytes",len);
    size_t offset = 0;
    RIFF_Chunk *chunk = riff_parse_chunk_from_data(data,(size_t)len,&offset);
    if (chunk==NULL) {
        TraceLog(LOG_ERROR, "CART: Error loading cart: %s", riff_get_error());
        ret->code = "function doframe() cls(7) print('error loading cart') end";
        ret->code_size = strlen(ret->code);
    } else {
        CartChunkWalker(ret,chunk);
    }
    riff_free_chunk(chunk);
    UnloadFileData(data);
    return ret;
}

void FreeGraphics(Cart_GraphicsPage *page) {
    if (page->next) FreeGraphics(page->next);
    UnloadImage(page->img);
    MemFree(page);
}

void FreeBlobs(Cart_Blob *blob) {
    if (blob->next) FreeBlobs(blob->next);
    MemFree(blob->data);
}

void FreeCart(Cart *cart) {
    if (cart->code) MemFree(cart->code);
    if (cart->graphics) FreeGraphics(cart->graphics);
    if (cart->blobs) FreeBlobs(cart->blobs);
    MemFree(cart);
}