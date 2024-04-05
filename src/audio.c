#include "audio.h"

AudioQueueItem *audio_head = NULL;
AudioQueueItem *audio_tail = NULL;

void audio_queue(int8_t *data, uint32_t size) {
    AudioQueueItem *local_head = NULL;
    AudioQueueItem *local_tail = NULL;
    if (audio_tail->size<128) {
        uint8_t diff = 128 - audio_tail->size;
        memcpy((audio_tail->data+audio_tail->size),data,diff);
        data+=diff;
        size-=diff;
    }
    while (size>=128) {
        AudioQueueItem *item = MemAlloc(sizeof(AudioQueueItem));
        item->data = MemAlloc(128);
        item->size = 128;
        for (int i=0;i<128;++i) item->data[i]=data[i]+128;
        size-=128;
        data+=128;
        if (local_tail==NULL) {
            local_head = item;
            local_tail = item;
        } else {
            local_tail->next = item;
            local_tail = item;
        }
    }
    if (size!=0) {
        AudioQueueItem *last = MemAlloc(sizeof(AudioQueueItem));
        last->data = MemAlloc(128);
        last->size = size;
        for (int i=0;i<size;++i) last->data[i]=data[i]+128;
        if (local_tail==NULL) {
            local_head = last;
            local_tail = last;
        } else {
            local_tail->next = last;
            local_tail = last;
        }        
    }
    if (audio_head) {
        audio_tail->next = local_head;
    } else {
        audio_head = local_head;
    }
    audio_tail = local_tail;
}

void audio_callback(void *buffer, unsigned int frames) {
    uint8_t *data = (uint8_t*)buffer;

    // fill with 128 regardless
    memset(data, 128, frames);
    // if we don't have any queued audio, we're done
    if (audio_head==NULL) {
        return;
    }
    // if we do, copy it in
    memcpy(data, audio_head->data, audio_head->size);
    AudioQueueItem *tmp = audio_head->next;
    MemFree(audio_head->data);
    MemFree(audio_head);
    audio_head = tmp;
    if (!audio_head) audio_tail=NULL;
}