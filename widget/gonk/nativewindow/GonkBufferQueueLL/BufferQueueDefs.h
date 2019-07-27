















#ifndef ANDROID_GUI_BUFFERQUEUECOREDEFS_H
#define ANDROID_GUI_BUFFERQUEUECOREDEFS_H

#include <gui/BufferSlot.h>

namespace android {
    class BufferQueueCore;

    namespace BufferQueueDefs {
        
        
        
        enum { NUM_BUFFER_SLOTS = 64 };

        typedef BufferSlot SlotsType[NUM_BUFFER_SLOTS];
    } 
} 

#endif
