#include "raylib.h"
#include "nexus.h"
#include <stddef.h>

typedef struct NeXUS_PCMChunk {
    size_t len;
    uint8_t *samples;
    uint8_t *playPtr; // points within samples
    struct NeXUS_PCMChunk *next;
} NeXUS_PCMChunk;

typedef struct NeXUS_PCM {
    NeXUS_PCMChunk *head;
    NeXUS_PCMChunk *tail;
} NeXUS_PCM;

bool pcm_queue(uint8_t *samples, size_t len);
bool pcm_ready();
void pcm_start();
void pcm_stop();
void pcm_clear();

void pcm_callback(void *framesOut, unsigned int framesWanted);