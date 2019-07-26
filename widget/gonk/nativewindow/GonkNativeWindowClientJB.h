
















#ifndef NATIVEWINDOW_GONKNATIVEWINDOWCLIENT_JB_H
#define NATIVEWINDOW_GONKNATIVEWINDOWCLIENT_JB_H

#if ANDROID_VERSION == 17
#include <gui/ISurfaceTexture.h>
#else
#include <gui/IGraphicBufferProducer.h>
#endif

#include <ui/ANativeObjectBase.h>
#include <ui/Region.h>

#include <utils/RefBase.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>

#include "GonkBufferQueue.h"

struct ANativeWindow_Buffer;

namespace android {














class GonkNativeWindowClient
    : public ANativeObjectBase<ANativeWindow, GonkNativeWindowClient, RefBase>
{
public:

    











    GonkNativeWindowClient(const sp<IGraphicBufferProducer>& bufferProducer);

    



#if ANDROID_VERSION == 17
    sp<IGraphicBufferProducer> getISurfaceTexture() const;
#else
    sp<IGraphicBufferProducer> getIGraphicBufferProducer() const;
#endif

    

#if ANDROID_VERSION >= 18
    static bool isValid(const sp<GonkNativeWindowClient>& surface) {
        return surface != NULL && surface->getIGraphicBufferProducer() != NULL;
    }
#endif

protected:
    virtual ~GonkNativeWindowClient();

private:
    
    GonkNativeWindowClient& operator = (const GonkNativeWindowClient& rhs);
    GonkNativeWindowClient(const GonkNativeWindowClient& rhs);

    
    static int hook_cancelBuffer(ANativeWindow* window,
            ANativeWindowBuffer* buffer, int fenceFd);
    static int hook_dequeueBuffer(ANativeWindow* window,
            ANativeWindowBuffer** buffer, int* fenceFd);
    static int hook_perform(ANativeWindow* window, int operation, ...);
    static int hook_query(const ANativeWindow* window, int what, int* value);
    static int hook_queueBuffer(ANativeWindow* window,
            ANativeWindowBuffer* buffer, int fenceFd);
    static int hook_setSwapInterval(ANativeWindow* window, int interval);

    static int hook_cancelBuffer_DEPRECATED(ANativeWindow* window,
            ANativeWindowBuffer* buffer);
    static int hook_dequeueBuffer_DEPRECATED(ANativeWindow* window,
            ANativeWindowBuffer** buffer);
    static int hook_lockBuffer_DEPRECATED(ANativeWindow* window,
            ANativeWindowBuffer* buffer);
    static int hook_queueBuffer_DEPRECATED(ANativeWindow* window,
            ANativeWindowBuffer* buffer);

    int dispatchConnect(va_list args);
    int dispatchDisconnect(va_list args);
    int dispatchSetBufferCount(va_list args);
    int dispatchSetBuffersGeometry(va_list args);
    int dispatchSetBuffersDimensions(va_list args);
    int dispatchSetBuffersUserDimensions(va_list args);
    int dispatchSetBuffersFormat(va_list args);
    int dispatchSetScalingMode(va_list args);
    int dispatchSetBuffersTransform(va_list args);
    int dispatchSetBuffersTimestamp(va_list args);
    int dispatchSetCrop(va_list args);
    int dispatchSetPostTransformCrop(va_list args);
    int dispatchSetUsage(va_list args);
    int dispatchLock(va_list args);
    int dispatchUnlockAndPost(va_list args);

protected:
    virtual int dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd);
    virtual int cancelBuffer(ANativeWindowBuffer* buffer, int fenceFd);
    virtual int queueBuffer(ANativeWindowBuffer* buffer, int fenceFd);
    virtual int perform(int operation, va_list args);
    virtual int query(int what, int* value) const;
    virtual int setSwapInterval(int interval);

    virtual int lockBuffer_DEPRECATED(ANativeWindowBuffer* buffer);

    virtual int connect(int api);
    virtual int disconnect(int api);
    virtual int setBufferCount(int bufferCount);
    virtual int setBuffersDimensions(int w, int h);
    virtual int setBuffersUserDimensions(int w, int h);
    virtual int setBuffersFormat(int format);
    virtual int setScalingMode(int mode);
    virtual int setBuffersTransform(int transform);
    virtual int setBuffersTimestamp(int64_t timestamp);
    virtual int setCrop(Rect const* rect);
    virtual int setUsage(uint32_t reqUsage);

public:
    virtual int lock(ANativeWindow_Buffer* outBuffer, ARect* inOutDirtyBounds);
    virtual int unlockAndPost();

protected:
    enum { NUM_BUFFER_SLOTS = GonkBufferQueue::NUM_BUFFER_SLOTS };
    enum { DEFAULT_FORMAT = PIXEL_FORMAT_RGBA_8888 };

private:
    void freeAllBuffers();
    int getSlotFromBufferLocked(android_native_buffer_t* buffer) const;

    struct BufferSlot {
        sp<GraphicBuffer> buffer;
        Region dirtyRegion;
    };

    
    
    
    sp<IGraphicBufferProducer> mBufferProducer;

    
    
    
    
    
    
    BufferSlot mSlots[NUM_BUFFER_SLOTS];

    
    
    uint32_t mReqWidth;

    
    
    uint32_t mReqHeight;

    
    
    uint32_t mReqFormat;

    
    
    uint32_t mReqUsage;

    
    
    
    int64_t mTimestamp;

    
    
    Rect mCrop;

    
    
    int mScalingMode;

    
    
    uint32_t mTransform;

     
     
     uint32_t mDefaultWidth;

     
     
     uint32_t mDefaultHeight;

     
     
     
     uint32_t mUserWidth;

     
     
     
     uint32_t mUserHeight;

    
    
    uint32_t mTransformHint;

    
    
    mutable bool mConsumerRunningBehind;

    
    
    
    mutable Mutex mMutex;

    
    sp<GraphicBuffer>           mLockedBuffer;
    sp<GraphicBuffer>           mPostedBuffer;
    bool                        mConnectedToCpu;

    
    Region mDirtyRegion;
};

}; 

#endif
