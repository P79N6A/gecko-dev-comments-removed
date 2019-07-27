
















#ifndef NATIVEWINDOW_BUFFERQUEUECOREDEFS_H
#define NATIVEWINDOW_BUFFERQUEUECOREDEFS_H

#include "GonkBufferSlot.h"

namespace android {
    class GonkBufferQueueCore;

    namespace GonkBufferQueueDefs {
        
        
        
        enum { NUM_BUFFER_SLOTS = 64 };

        typedef GonkBufferSlot SlotsType[NUM_BUFFER_SLOTS];
    } 
} 

#endif
