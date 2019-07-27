















#ifndef ANDROID_GUI_BUFFERITEM_H
#define ANDROID_GUI_BUFFERITEM_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gui/IGraphicBufferConsumer.h>

#include <ui/Rect.h>

#include <utils/Flattenable.h>
#include <utils/StrongPointer.h>

namespace android {

class Fence;
class GraphicBuffer;

class BufferItem : public Flattenable<BufferItem> {
    friend class Flattenable<BufferItem>;
    size_t getPodSize() const;
    size_t getFlattenedSize() const;
    size_t getFdCount() const;
    status_t flatten(void*& buffer, size_t& size, int*& fds, size_t& count) const;
    status_t unflatten(void const*& buffer, size_t& size, int const*& fds, size_t& count);

    public:
    
    enum { INVALID_BUFFER_SLOT = -1 };
    BufferItem();
    operator IGraphicBufferConsumer::BufferItem() const;

    static const char* scalingModeName(uint32_t scalingMode);

    
    
    
    sp<GraphicBuffer> mGraphicBuffer;

    
    sp<Fence> mFence;

    
    Rect mCrop;

    
    
    uint32_t mTransform;

    
    
    uint32_t mScalingMode;

    
    
    
    
    int64_t mTimestamp;

    
    
    bool mIsAutoTimestamp;

    
    uint64_t mFrameNumber;

    
    int mSlot;

    
    
    
    
    
    bool mIsDroppable;

    
    bool mAcquireCalled;

    
    
    bool mTransformToDisplayInverse;
};

} 

#endif
