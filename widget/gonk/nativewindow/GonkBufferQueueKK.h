
















#ifndef NATIVEWINDOW_GONKBUFFERQUEUE_KK_H
#define NATIVEWINDOW_GONKBUFFERQUEUE_KK_H

#include <gui/IConsumerListener.h>
#include <gui/IGraphicBufferAlloc.h>
#include <gui/IGraphicBufferProducer.h>
#include "IGonkGraphicBufferConsumer.h"

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/layers/TextureClient.h"

namespace android {


class GonkBufferQueue : public BnGraphicBufferProducer,
                        public BnGonkGraphicBufferConsumer,
                        private IBinder::DeathRecipient
{
    typedef mozilla::layers::TextureClient TextureClient;

public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { NUM_BUFFER_SLOTS = 32 };
    enum { NO_CONNECTED_API = 0 };
    enum { INVALID_BUFFER_SLOT = -1 };
    enum { STALE_BUFFER_SLOT = 1, NO_BUFFER_AVAILABLE, PRESENT_LATER };

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = NUM_BUFFER_SLOTS - 2 };

    
    typedef ::android::ConsumerListener ConsumerListener;

    
    
    
    
    
    
    
    
    
    class ProxyConsumerListener : public BnConsumerListener {
    public:
        ProxyConsumerListener(const wp<ConsumerListener>& consumerListener);
        virtual ~ProxyConsumerListener();
        virtual void onFrameAvailable();
        virtual void onBuffersReleased();
    private:
        
        
        wp<ConsumerListener> mConsumerListener;
    };


    
    
    
    GonkBufferQueue(bool allowSynchronousMode = true,
            const sp<IGraphicBufferAlloc>& allocator = NULL);
    virtual ~GonkBufferQueue();

    



    virtual void binderDied(const wp<IBinder>& who);

    



    
    
    virtual int query(int what, int* value);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t setBufferCount(int bufferCount);

    
    
    
    
    
    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t dequeueBuffer(int *buf, sp<Fence>* fence, bool async,
            uint32_t width, uint32_t height, uint32_t format, uint32_t usage);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t queueBuffer(int buf,
            const QueueBufferInput& input, QueueBufferOutput* output);

    
    
    
    
    
    virtual void cancelBuffer(int buf, const sp<Fence>& fence);

    
    
    
    
    
    
    
    
    virtual status_t setSynchronousMode(bool enabled);

    
    
    
    
    
    
    
    
    
    virtual status_t connect(const sp<IBinder>& token,
            int api, bool producerControlledByApp, QueueBufferOutput* output);

    
    
    
    
    
    
    
    
    virtual status_t disconnect(int api);

    



    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t acquireBuffer(BufferItem *buffer, nsecs_t presentWhen);

    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t releaseBuffer(int buf, uint64_t frameNumber,
                    const sp<Fence>& releaseFence);

    
    
    
    
    
    
    
    
    virtual status_t consumerConnect(const sp<IConsumerListener>& consumer, bool controlledByApp);

    
    
    
    
    virtual status_t consumerDisconnect();

    
    
    
    
    
    virtual status_t getReleasedBuffers(uint32_t* slotMask);

    
    
    
    virtual status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    
    
    
    virtual status_t setDefaultMaxBufferCount(int bufferCount);

    
    
    
    
    
    virtual status_t disableAsyncBuffer();

    
    
    
    virtual status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers);

    
    virtual void setConsumerName(const String8& name);

    
    
    
    
    virtual status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    
    
    virtual status_t setConsumerUsageBits(uint32_t usage);

    
    
    
    virtual status_t setTransformHint(uint32_t hint);

    
    virtual void dumpToString(String8& result, const char* prefix) const;

     already_AddRefed<TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    int getSlotFromTextureClientLocked(TextureClient* client) const;

private:
    
    
    

    
    
    void freeAllBuffersLocked();

    
    
    
    
    status_t setDefaultMaxBufferCountLocked(int count);

    
    
    
    int getMinUndequeuedBufferCount(bool async) const;

    
    
    
    int getMinMaxBufferCountLocked(bool async) const;

    
    
    
    
    
    
    
    
    
    
    
    
    int getMaxBufferCountLocked(bool async) const;

    
    
    bool stillTracking(const BufferItem *item) const;

    struct BufferSlot {

        BufferSlot()
        : mBufferState(BufferSlot::FREE),
          mRequestBufferCalled(false),
          mFrameNumber(0),
          mAcquireCalled(false),
          mNeedsCleanupOnRelease(false) {
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

    
    
    
    sp<IConsumerListener> mConsumerListener;

    
    bool mSynchronousMode;

    
    
    bool mConsumerControlledByApp;

    
    
    
    bool mDequeueBufferCannotBlock;

    
    
    bool mUseAsyncBuffer;

    
    
    
    int mConnectedApi;

    
    mutable Condition mDequeueCondition;

    
    typedef Vector<BufferItem> Fifo;
    Fifo mQueue;

    
    
    
    
    
    
    bool mAbandoned;

    
    
    String8 mConsumerName;

    
    
    
    mutable Mutex mMutex;

    
    
    uint64_t mFrameCounter;

    
    
    
    bool mBufferHasBeenQueued;

    
    
    uint32_t mDefaultBufferFormat;

    
    uint32_t mConsumerUsageBits;

    
    uint32_t mTransformHint;

    
    sp<IBinder> mConnectedProducerToken;
};


}; 

#endif
