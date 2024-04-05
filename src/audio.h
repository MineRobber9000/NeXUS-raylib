#ifndef NEXUS_AUDIO
#define NEXUS_AUDIO
#include <stdint.h>
#include <string.h>

struct AudioQueueItem {
    uint8_t *data;
    uint8_t size; // max 128; if the item at the back of the list is <128 we'll add to this
    struct AudioQueueItem *next;
};

typedef struct AudioQueueItem AudioQueueItem;

extern AudioQueueItem *audio_head;
extern AudioQueueItem *audio_tail;

void audio_queue(int8_t *data, uint32_t size);
void audio_callback(void *buffer, unsigned int frames);
#endif // NEXUS_AUDIO