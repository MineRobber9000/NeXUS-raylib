#include "raylib.h"
#include "cart.h"
#include "riff.h"
#include <string.h>

FourCC _CODE = {'C','O','D','E'};

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
    return ret;
}
