















#ifndef ANDROID_GUI_IGRAPHIC_BUFFER_ALLOC_H
#define ANDROID_GUI_IGRAPHIC_BUFFER_ALLOC_H

#include <stdint.h>
#include <sys/types.h>

#include <binder/IInterface.h>
#include <ui/PixelFormat.h>
#include <utils/RefBase.h>

namespace android {


class GraphicBuffer;

class IGraphicBufferAlloc : public IInterface
{
public:
    DECLARE_META_INTERFACE(GraphicBufferAlloc);

    

    virtual sp<GraphicBuffer> createGraphicBuffer(uint32_t w, uint32_t h,
            PixelFormat format, uint32_t usage, status_t* error) = 0;

    virtual void acquireBufferReferenceSlot(int32_t slot) {}
    virtual void releaseBufferReferenceSlot(int32_t slot) {}

};



class BnGraphicBufferAlloc : public BnInterface<IGraphicBufferAlloc>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};



}; 

#endif 
