















#ifndef ANDROID_SF_GRAPHIC_BUFFER_ALLOC_H
#define ANDROID_SF_GRAPHIC_BUFFER_ALLOC_H

#include <stdint.h>
#include <sys/types.h>

#include <gui/IGraphicBufferAlloc.h>
#include <ui/PixelFormat.h>
#include <utils/Errors.h>

namespace android {


class GraphicBuffer;

class GraphicBufferAlloc : public BnGraphicBufferAlloc {
public:
    GraphicBufferAlloc();
    virtual ~GraphicBufferAlloc();
    virtual sp<GraphicBuffer> createGraphicBuffer(uint32_t w, uint32_t h,
        PixelFormat format, uint32_t usage, status_t* error);
};



}; 

#endif 
