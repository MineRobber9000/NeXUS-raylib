#include "pcm.h"
#include <string.h>

NeXUS_PCM pcm = { 0 };

void EnsurePCMStream() {
    if (!IsAudioStreamValid(vm.pcm_stream)) {
        vm.pcm_stream = LoadAudioStream(11025, 8, 1);
        SetAudioStreamCallback(vm.pcm_stream, pcm_callback);
    }
}

bool pcm_queue(uint8_t *samples, size_t len) {
    EnsurePCMStream();
    NeXUS_PCMChunk *chunk = (NeXUS_PCMChunk *)MemAlloc(sizeof(NeXUS_PCMChunk));
    chunk->len = len;
    chunk->samples = samples;
    chunk->playPtr = samples;
    if (pcm.head==NULL) {
        pcm.head = pcm.tail = chunk;
    } else {
        pcm.tail->next = chunk;
        pcm.tail = chunk;
    }
    if (!IsAudioStreamPlaying(vm.pcm_stream)) pcm_start();
    return true;
}
bool pcm_ready() {
    EnsurePCMStream();
    return true;
}
void pcm_start() {
    EnsurePCMStream();
    if (!IsAudioStreamPlaying(vm.pcm_stream)) PlayAudioStream(vm.pcm_stream);
}
void pcm_stop() {
    EnsurePCMStream();
    if (IsAudioStreamPlaying(vm.pcm_stream)) PauseAudioStream(vm.pcm_stream);
}
void pcm_clear() {
    EnsurePCMStream();
    StopAudioStream(vm.pcm_stream);
    NeXUS_PCMChunk *walker = pcm.head;
    pcm.head = pcm.tail = NULL;
    while (walker != NULL) {
        NeXUS_PCMChunk *next = walker->next;
        MemFree(walker->samples);
        MemFree(walker);
        walker = next;
    }
}

void pcm_callback(void *_framesOut, unsigned int framesWanted) {
    uint8_t *framesOut = (uint8_t *)_framesOut;
    // default to silence
    memset(framesOut, 128, framesWanted);
    // if we don't have any audio to play, that's all she wrote
    // otherwise, queue data
    NeXUS_PCMChunk *chunk = pcm.head;
    while (chunk!=NULL && framesWanted) {
        if (chunk->len == framesWanted) {
            memcpy(framesOut, chunk->playPtr, framesWanted);
            framesWanted = 0; // got all of it
            pcm.head = pcm.head->next;
            MemFree(chunk->samples);
            MemFree(chunk);
        } else if (chunk->len > framesWanted) {
            memcpy(framesOut, chunk->playPtr, framesWanted);
            chunk->playPtr += framesWanted;
            chunk->len -= framesWanted;
            framesWanted = 0; // got all of it
        } else { // chunk->len < framesWanted
            memcpy(framesOut, chunk->playPtr, chunk->len);
            framesOut += chunk->len;
            framesWanted -= chunk->len;
            pcm.head = pcm.head->next;
            MemFree(chunk->samples);
            MemFree(chunk);
            chunk = pcm.head;
        }
    }
}