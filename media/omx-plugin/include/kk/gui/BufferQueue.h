















#ifndef ANDROID_GUI_BUFFERQUEUE_H
#define ANDROID_GUI_BUFFERQUEUE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gui/IGraphicBufferAlloc.h>
#include <gui/IGraphicBufferProducer.h>

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

namespace android {


class BufferQueue : public BnGraphicBufferProducer {
public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { NUM_BUFFER_SLOTS =
#ifdef GFX_BUF_EXT
    64 }; 
#else
    32 };
#endif
    enum { NO_CONNECTED_API = 0 };
    enum { INVALID_BUFFER_SLOT = -1 };
    enum { STALE_BUFFER_SLOT = 1, NO_BUFFER_AVAILABLE };

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = NUM_BUFFER_SLOTS - 2 };

    
    
    
    
    
    struct ConsumerListener : public virtual RefBase {
        
        
        
        
        
        
        
        
        virtual void onFrameAvailable() = 0;

        
        
        
        
        
        
        
        virtual void onBuffersReleased() = 0;
    };

    
    
    
    
    
    
    
    
    
    class ProxyConsumerListener : public BufferQueue::ConsumerListener {
    public:

        ProxyConsumerListener(const wp<BufferQueue::ConsumerListener>& consumerListener);
        virtual ~ProxyConsumerListener();
        virtual void onFrameAvailable();
        virtual void onBuffersReleased();

    private:

        
        
        wp<BufferQueue::ConsumerListener> mConsumerListener;
    };


    
    
    
    
    BufferQueue(bool allowSynchronousMode = true,
            const sp<IGraphicBufferAlloc>& allocator = NULL);
    virtual ~BufferQueue();

    
    
    virtual int query(int what, int* value);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t setBufferCount(int bufferCount);

    
    
    
    
    
    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t dequeueBuffer(int *buf, sp<Fence>* fence,
            uint32_t width, uint32_t height, uint32_t format, uint32_t usage);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t queueBuffer(int buf,
            const QueueBufferInput& input, QueueBufferOutput* output);

    
    
    
    
    
    virtual void cancelBuffer(int buf, const sp<Fence>& fence);

    
    
    
    
    
    
    
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
           mTrickMode(false),
           mBuf(INVALID_BUFFER_SLOT),
           mVideoSessionID(0) {
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

        
        bool mTrickMode;

        
        uint32_t mVideoSessionID;
    };

    

    
    
    
    
    
    
    
    status_t acquireBuffer(BufferItem *buffer);

    
    
    
    
    
    
    
    
    
    
    
    
    status_t releaseBuffer(int buf, EGLDisplay display, EGLSyncKHR fence,
            const sp<Fence>& releaseFence);

    
    
    
    
    
    
    status_t consumerConnect(const sp<ConsumerListener>& consumer);

    
    
    
    
    status_t consumerDisconnect();

    
    
    
    
    
    status_t getReleasedBuffers(uint64_t* slotMask);

    
    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    
    
    
    status_t setDefaultMaxBufferCount(int bufferCount);

    
    
    
    status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers);

    
    
    bool isSynchronousMode() const;

    
    void setConsumerName(const String8& name);

    
    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    
    
    status_t setConsumerUsageBits(uint32_t usage);

    
    
    
    status_t setTransformHint(uint32_t hint);

private:
    
    
    void freeBufferLocked(int index);

    
    
    void freeAllBuffersLocked();

    
    
    void freeAllBuffersExceptHeadLocked();

    
    
    
    
    status_t drainQueueLocked();

    
    
    
    status_t drainQueueAndFreeBuffersLocked();

    
    
    
    
    status_t setDefaultMaxBufferCountLocked(int count);

    
    
    int getMinMaxBufferCountLocked() const;

    
    
    int getMinUndequeuedBufferCountLocked() const;

    
    
    
    
    
    
    
    
    
    
    
    int getMaxBufferCountLocked() const;

    struct BufferSlot {

        BufferSlot()
        : mEglDisplay(EGL_NO_DISPLAY),
          mBufferState(BufferSlot::FREE),
          mRequestBufferCalled(false),
          mTransform(0),
          mScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
          mTimestamp(0),
          mFrameNumber(0),
          mEglFence(EGL_NO_SYNC_KHR),
          mAcquireCalled(false),
          mTrickMode(false),
          mNeedsCleanupOnRelease(false),
          mVideoSessionID(0) {
            mCrop.makeInvalid();
        }

        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        EGLDisplay mEglDisplay;

        
        
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

        
        
        
        
        
        EGLSyncKHR mEglFence;

        
        
        
        
        
        
        
        
        
        sp<Fence> mFence;

        
        bool mAcquireCalled;

        
        
        
        bool mNeedsCleanupOnRelease;

        
        bool mTrickMode;

        
        uint32_t mVideoSessionID;
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
