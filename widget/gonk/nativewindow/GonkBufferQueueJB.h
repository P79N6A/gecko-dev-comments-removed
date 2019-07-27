
















#ifndef NATIVEWINDOW_GONKBUFFERQUEUE_JB_H
#define NATIVEWINDOW_GONKBUFFERQUEUE_JB_H

#include <gui/IGraphicBufferAlloc.h>
#if ANDROID_VERSION == 17
#include <gui/ISurfaceTexture.h>
#else
#include <gui/IGraphicBufferProducer.h>
#endif

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/layers/TextureClient.h"

#if ANDROID_VERSION == 17
#define IGraphicBufferProducer ISurfaceTexture
#endif

namespace android {


#if ANDROID_VERSION == 17
class GonkBufferQueue : public BnSurfaceTexture {
#else
class GonkBufferQueue : public BnGraphicBufferProducer {
#endif
    typedef mozilla::layers::TextureClient TextureClient;

public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { NUM_BUFFER_SLOTS = 32 };
    enum { NO_CONNECTED_API = 0 };
    enum { INVALID_BUFFER_SLOT = -1 };
    enum { STALE_BUFFER_SLOT = 1, NO_BUFFER_AVAILABLE };

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = NUM_BUFFER_SLOTS - 2 };

    
    
    
    
    
    struct ConsumerListener : public virtual RefBase {
        
        
        
        
        
        
        
        
        virtual void onFrameAvailable() = 0;

        
        
        
        
        
        
        
        virtual void onBuffersReleased() = 0;
    };

    
    
    
    
    
    
    
    
    
    class ProxyConsumerListener : public GonkBufferQueue::ConsumerListener {
    public:

        ProxyConsumerListener(const wp<GonkBufferQueue::ConsumerListener>& consumerListener);
        virtual ~ProxyConsumerListener();
        virtual void onFrameAvailable();
        virtual void onBuffersReleased();

    private:

        
        
        wp<GonkBufferQueue::ConsumerListener> mConsumerListener;
    };


    
    
    
    
    GonkBufferQueue(bool allowSynchronousMode = true,
            const sp<IGraphicBufferAlloc>& allocator = NULL);
    virtual ~GonkBufferQueue();

    
    
    virtual int query(int what, int* value);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t setBufferCount(int bufferCount);

    
    
    
    
    
    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
#if ANDROID_VERSION == 17
    virtual status_t dequeueBuffer(int *buf, sp<Fence>& fence,
            uint32_t width, uint32_t height, uint32_t format, uint32_t usage) {
        return dequeueBuffer(buf, &fence, width, height, format, usage);
    }
#endif

    virtual status_t dequeueBuffer(int *buf, sp<Fence>* fence,
            uint32_t width, uint32_t height, uint32_t format, uint32_t usage);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t queueBuffer(int buf,
            const QueueBufferInput& input, QueueBufferOutput* output);

    
    
    
    
    
#if ANDROID_VERSION == 17
    virtual void cancelBuffer(int buf, sp<Fence> fence);
#else
    virtual void cancelBuffer(int buf, const sp<Fence>& fence);
#endif

    
    
    
    
    
    
    
    
    virtual status_t setSynchronousMode(bool enabled);

    
    
    
    
    
    
    
    
    
    virtual status_t connect(int api, QueueBufferOutput* output);

    
    
    
    
    
    
    
    
    virtual status_t disconnect(int api);

    
    virtual void dump(String8& result) const;
    virtual void dump(String8& result, const char* prefix, char* buffer, size_t SIZE) const;

    
    struct BufferItem {

        BufferItem()
         :
           mTransform(0),
           mScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
           mTimestamp(0),
           mFrameNumber(0),
           mBuf(INVALID_BUFFER_SLOT) {
             mCrop.makeInvalid();
        }
        
        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        Rect mCrop;

        
        uint32_t mTransform;

        
        uint32_t mScalingMode;

        
        
        int64_t mTimestamp;

        
        uint64_t mFrameNumber;

        
        int mBuf;

        
        sp<Fence> mFence;
    };

    

    
    
    
    
    
    
    
    status_t acquireBuffer(BufferItem *buffer);

    
    
    
    
    
    
    
    
    
    
    
    
    status_t releaseBuffer(int buf, const sp<Fence>& releaseFence);

    
    
    
    
    
    
    status_t consumerConnect(const sp<ConsumerListener>& consumer);

    
    
    
    
    status_t consumerDisconnect();

    
    
    
    
    
    status_t getReleasedBuffers(uint32_t* slotMask);

    
    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    
    
    
    status_t setDefaultMaxBufferCount(int bufferCount);

    
    
    
    status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers);

    
    
    bool isSynchronousMode() const;

    
    void setConsumerName(const String8& name);

    
    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    
    
    status_t setConsumerUsageBits(uint32_t usage);

    
    
    
    status_t setTransformHint(uint32_t hint);

    mozilla::TemporaryRef<TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    int getSlotFromTextureClientLocked(TextureClient* client) const;

private:
    
    
    

    
    
    
    void freeAllBuffersLocked();

    
    
    
    
    status_t setDefaultMaxBufferCountLocked(int count);

    
    
    int getMinMaxBufferCountLocked() const;

    
    
    int getMinUndequeuedBufferCountLocked() const;

    
    
    
    
    
    
    
    
    
    
    
    int getMaxBufferCountLocked() const;

    struct BufferSlot {

        BufferSlot()
        : mBufferState(BufferSlot::FREE),
          mRequestBufferCalled(false),
          mTransform(0),
          mScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
          mTimestamp(0),
          mFrameNumber(0),
          mAcquireCalled(false),
          mNeedsCleanupOnRelease(false) {
            mCrop.makeInvalid();
        }

        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        mozilla::RefPtr<TextureClient> mTextureClient;

        
        
        enum BufferState {
            
            
            
            
            
            
            
            FREE = 0,

            
            
            
            
            
            
            
            DEQUEUED = 1,

            
            
            
            
            
            
            
            
            
            QUEUED = 2,

            
            
            
            
            
            
            ACQUIRED = 3
        };

        
        BufferState mBufferState;

        
        
        
        bool mRequestBufferCalled;

        
        Rect mCrop;

        
        
        uint32_t mTransform;

        
        
        uint32_t mScalingMode;

        
        
        int64_t mTimestamp;

        
        
        
        uint64_t mFrameNumber;

        
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        sp<Fence> mFence;

        
        bool mAcquireCalled;

        
        
        
        bool mNeedsCleanupOnRelease;
    };

    
    
    
    
    
    
    BufferSlot mSlots[NUM_BUFFER_SLOTS];

    
    
    uint32_t mDefaultWidth;

    
    
    uint32_t mDefaultHeight;

    
    
    
    
    
    
    
    int mMaxAcquiredBufferCount;

    
    
    
    
    int mDefaultMaxBufferCount;

    
    
    
    
    
    int mOverrideMaxBufferCount;

    
    
    sp<IGraphicBufferAlloc> mGraphicBufferAlloc;

    
    
    
    sp<ConsumerListener> mConsumerListener;

    
    bool mSynchronousMode;

    
    
    const bool mAllowSynchronousMode;

    
    
    
    int mConnectedApi;

    
    mutable Condition mDequeueCondition;

    
    typedef Vector<int> Fifo;
    Fifo mQueue;

    
    
    
    
    
    
    bool mAbandoned;

    
    
    String8 mConsumerName;

    
    
    
    mutable Mutex mMutex;

    
    
    uint64_t mFrameCounter;

    
    
    
    bool mBufferHasBeenQueued;

    
    
    uint32_t mDefaultBufferFormat;

    
    uint32_t mConsumerUsageBits;

    
    uint32_t mTransformHint;

};


}; 

#endif 
