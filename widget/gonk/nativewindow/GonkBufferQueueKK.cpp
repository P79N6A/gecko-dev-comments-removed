















#define LOG_TAG "BufferQueue"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gui/BufferQueue.h>
#include <gui/IConsumerListener.h>
#include <gui/ISurfaceComposer.h>
#include <private/gui/ComposerService.h>

#include <utils/Log.h>
#include <utils/Trace.h>
#include <utils/CallStack.h>


#define ST_LOGV(x, ...) ALOGV("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define ST_LOGD(x, ...) ALOGD("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define ST_LOGI(x, ...) ALOGI("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define ST_LOGW(x, ...) ALOGW("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define ST_LOGE(x, ...) ALOGE("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)

#define ATRACE_BUFFER_INDEX(index)                                            \
    if (ATRACE_ENABLED()) {                                                   \
        char ___traceBuf[1024];                                               \
        snprintf(___traceBuf, 1024, "%s: %d", mConsumerName.string(),         \
                (index));                                                     \
        android::ScopedTrace ___bufTracer(ATRACE_TAG, ___traceBuf);           \
    }

namespace android {


static int32_t createProcessUniqueId() {
    static volatile int32_t globalCounter = 0;
    return android_atomic_inc(&globalCounter);
}

static const char* scalingModeName(int scalingMode) {
    switch (scalingMode) {
        case NATIVE_WINDOW_SCALING_MODE_FREEZE: return "FREEZE";
        case NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW: return "SCALE_TO_WINDOW";
        case NATIVE_WINDOW_SCALING_MODE_SCALE_CROP: return "SCALE_CROP";
        default: return "Unknown";
    }
}

BufferQueue::BufferQueue(const sp<IGraphicBufferAlloc>& allocator) :
    mDefaultWidth(1),
    mDefaultHeight(1),
    mMaxAcquiredBufferCount(1),
    mDefaultMaxBufferCount(2),
    mOverrideMaxBufferCount(0),
    mConsumerControlledByApp(false),
    mDequeueBufferCannotBlock(false),
    mUseAsyncBuffer(true),
    mConnectedApi(NO_CONNECTED_API),
    mAbandoned(false),
    mFrameCounter(0),
    mBufferHasBeenQueued(false),
    mDefaultBufferFormat(PIXEL_FORMAT_RGBA_8888),
    mConsumerUsageBits(0),
    mTransformHint(0)
{
    
    mConsumerName = String8::format("unnamed-%d-%d", getpid(), createProcessUniqueId());

    ST_LOGV("BufferQueue");
    if (allocator == NULL) {
        sp<ISurfaceComposer> composer(ComposerService::getComposerService());
        mGraphicBufferAlloc = composer->createGraphicBufferAlloc();
        if (mGraphicBufferAlloc == 0) {
            ST_LOGE("createGraphicBufferAlloc() failed in BufferQueue()");
        }
    } else {
        mGraphicBufferAlloc = allocator;
    }
}

BufferQueue::~BufferQueue() {
    ST_LOGV("~BufferQueue");
}

status_t BufferQueue::setDefaultMaxBufferCountLocked(int count) {
    const int minBufferCount = mUseAsyncBuffer ? 2 : 1;
    if (count < minBufferCount || count > NUM_BUFFER_SLOTS)
        return BAD_VALUE;

    mDefaultMaxBufferCount = count;
    mDequeueCondition.broadcast();

    return NO_ERROR;
}

void BufferQueue::setConsumerName(const String8& name) {
    Mutex::Autolock lock(mMutex);
    mConsumerName = name;
}

status_t BufferQueue::setDefaultBufferFormat(uint32_t defaultFormat) {
    Mutex::Autolock lock(mMutex);
    mDefaultBufferFormat = defaultFormat;
    return NO_ERROR;
}

status_t BufferQueue::setConsumerUsageBits(uint32_t usage) {
    Mutex::Autolock lock(mMutex);
    mConsumerUsageBits = usage;
    return NO_ERROR;
}

status_t BufferQueue::setTransformHint(uint32_t hint) {
    ST_LOGV("setTransformHint: %02x", hint);
    Mutex::Autolock lock(mMutex);
    mTransformHint = hint;
    return NO_ERROR;
}

status_t BufferQueue::setBufferCount(int bufferCount) {
    ST_LOGV("setBufferCount: count=%d", bufferCount);

    sp<IConsumerListener> listener;
    {
        Mutex::Autolock lock(mMutex);

        if (mAbandoned) {
            ST_LOGE("setBufferCount: BufferQueue has been abandoned!");
            return NO_INIT;
        }
        if (bufferCount > NUM_BUFFER_SLOTS) {
            ST_LOGE("setBufferCount: bufferCount too large (max %d)",
                    NUM_BUFFER_SLOTS);
            return BAD_VALUE;
        }

        
        for (int i=0 ; i<NUM_BUFFER_SLOTS; i++) {
            if (mSlots[i].mBufferState == BufferSlot::DEQUEUED) {
                ST_LOGE("setBufferCount: client owns some buffers");
                return -EINVAL;
            }
        }

        if (bufferCount == 0) {
            mOverrideMaxBufferCount = 0;
            mDequeueCondition.broadcast();
            return NO_ERROR;
        }

        
        const int minBufferSlots = getMinMaxBufferCountLocked(false);
        if (bufferCount < minBufferSlots) {
            ST_LOGE("setBufferCount: requested buffer count (%d) is less than "
                    "minimum (%d)", bufferCount, minBufferSlots);
            return BAD_VALUE;
        }

        
        
        
        freeAllBuffersLocked();
        mOverrideMaxBufferCount = bufferCount;
        mDequeueCondition.broadcast();
        listener = mConsumerListener;
    } 

    if (listener != NULL) {
        listener->onBuffersReleased();
    }

    return NO_ERROR;
}

int BufferQueue::query(int what, int* outValue)
{
    ATRACE_CALL();
    Mutex::Autolock lock(mMutex);

    if (mAbandoned) {
        ST_LOGE("query: BufferQueue has been abandoned!");
        return NO_INIT;
    }

    int value;
    switch (what) {
    case NATIVE_WINDOW_WIDTH:
        value = mDefaultWidth;
        break;
    case NATIVE_WINDOW_HEIGHT:
        value = mDefaultHeight;
        break;
    case NATIVE_WINDOW_FORMAT:
        value = mDefaultBufferFormat;
        break;
    case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
        value = getMinUndequeuedBufferCount(false);
        break;
    case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
        value = (mQueue.size() >= 2);
        break;
    case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
        value = mConsumerUsageBits;
        break;
    default:
        return BAD_VALUE;
    }
    outValue[0] = value;
    return NO_ERROR;
}

status_t BufferQueue::requestBuffer(int slot, sp<GraphicBuffer>* buf) {
    ATRACE_CALL();
    ST_LOGV("requestBuffer: slot=%d", slot);
    Mutex::Autolock lock(mMutex);
    if (mAbandoned) {
        ST_LOGE("requestBuffer: BufferQueue has been abandoned!");
        return NO_INIT;
    }
    if (slot < 0 || slot >= NUM_BUFFER_SLOTS) {
        ST_LOGE("requestBuffer: slot index out of range [0, %d]: %d",
                NUM_BUFFER_SLOTS, slot);
        return BAD_VALUE;
    } else if (mSlots[slot].mBufferState != BufferSlot::DEQUEUED) {
        ST_LOGE("requestBuffer: slot %d is not owned by the client (state=%d)",
                slot, mSlots[slot].mBufferState);
        return BAD_VALUE;
    }
    mSlots[slot].mRequestBufferCalled = true;
    *buf = mSlots[slot].mGraphicBuffer;
    return NO_ERROR;
}

status_t BufferQueue::dequeueBuffer(int *outBuf, sp<Fence>* outFence, bool async,
        uint32_t w, uint32_t h, uint32_t format, uint32_t usage) {
    ATRACE_CALL();
    ST_LOGV("dequeueBuffer: w=%d h=%d fmt=%#x usage=%#x", w, h, format, usage);

    if ((w && !h) || (!w && h)) {
        ST_LOGE("dequeueBuffer: invalid size: w=%u, h=%u", w, h);
        return BAD_VALUE;
    }

    status_t returnFlags(OK);
    EGLDisplay dpy = EGL_NO_DISPLAY;
    EGLSyncKHR eglFence = EGL_NO_SYNC_KHR;

    { 
        Mutex::Autolock lock(mMutex);

        if (format == 0) {
            format = mDefaultBufferFormat;
        }
        
        usage |= mConsumerUsageBits;

        int found = -1;
        bool tryAgain = true;
        while (tryAgain) {
            if (mAbandoned) {
                ST_LOGE("dequeueBuffer: BufferQueue has been abandoned!");
                return NO_INIT;
            }

            const int maxBufferCount = getMaxBufferCountLocked(async);
            if (async && mOverrideMaxBufferCount) {
                
                
                
                if (mOverrideMaxBufferCount < maxBufferCount) {
                    ST_LOGE("dequeueBuffer: async mode is invalid with buffercount override");
                    return BAD_VALUE;
                }
            }

            
            
            for (int i = maxBufferCount; i < NUM_BUFFER_SLOTS; i++) {
                assert(mSlots[i].mBufferState == BufferSlot::FREE);
                if (mSlots[i].mGraphicBuffer != NULL) {
                    freeBufferLocked(i);
                    returnFlags |= IGraphicBufferProducer::RELEASE_ALL_BUFFERS;
                }
            }

            
            found = INVALID_BUFFER_SLOT;
            int dequeuedCount = 0;
            int acquiredCount = 0;
            for (int i = 0; i < maxBufferCount; i++) {
                const int state = mSlots[i].mBufferState;
                switch (state) {
                    case BufferSlot::DEQUEUED:
                        dequeuedCount++;
                        break;
                    case BufferSlot::ACQUIRED:
                        acquiredCount++;
                        break;
                    case BufferSlot::FREE:
                        




                        if ((found < 0) ||
                                mSlots[i].mFrameNumber < mSlots[found].mFrameNumber) {
                            found = i;
                        }
                        break;
                }
            }

            
            
            if (!mOverrideMaxBufferCount && dequeuedCount) {
                ST_LOGE("dequeueBuffer: can't dequeue multiple buffers without "
                        "setting the buffer count");
                return -EINVAL;
            }

            
            
            
            if (mBufferHasBeenQueued) {
                
                
                const int newUndequeuedCount = maxBufferCount - (dequeuedCount+1);
                const int minUndequeuedCount = getMinUndequeuedBufferCount(async);
                if (newUndequeuedCount < minUndequeuedCount) {
                    ST_LOGE("dequeueBuffer: min undequeued buffer count (%d) "
                            "exceeded (dequeued=%d undequeudCount=%d)",
                            minUndequeuedCount, dequeuedCount,
                            newUndequeuedCount);
                    return -EBUSY;
                }
            }

            
            
            tryAgain = found == INVALID_BUFFER_SLOT;
            if (tryAgain) {
                
                
                
                
                
                
                if (mDequeueBufferCannotBlock && (acquiredCount <= mMaxAcquiredBufferCount)) {
                    ST_LOGE("dequeueBuffer: would block! returning an error instead.");
                    return WOULD_BLOCK;
                }
                mDequeueCondition.wait(mMutex);
            }
        }


        if (found == INVALID_BUFFER_SLOT) {
            
            ST_LOGE("dequeueBuffer: no available buffer slots");
            return -EBUSY;
        }

        const int buf = found;
        *outBuf = found;

        ATRACE_BUFFER_INDEX(buf);

        const bool useDefaultSize = !w && !h;
        if (useDefaultSize) {
            
            w = mDefaultWidth;
            h = mDefaultHeight;
        }

        mSlots[buf].mBufferState = BufferSlot::DEQUEUED;

        const sp<GraphicBuffer>& buffer(mSlots[buf].mGraphicBuffer);
        if ((buffer == NULL) ||
            (uint32_t(buffer->width)  != w) ||
            (uint32_t(buffer->height) != h) ||
            (uint32_t(buffer->format) != format) ||
            ((uint32_t(buffer->usage) & usage) != usage))
        {
            mSlots[buf].mAcquireCalled = false;
            mSlots[buf].mGraphicBuffer = NULL;
            mSlots[buf].mRequestBufferCalled = false;
            mSlots[buf].mEglFence = EGL_NO_SYNC_KHR;
            mSlots[buf].mFence = Fence::NO_FENCE;
            mSlots[buf].mEglDisplay = EGL_NO_DISPLAY;

            returnFlags |= IGraphicBufferProducer::BUFFER_NEEDS_REALLOCATION;
        }


        if (CC_UNLIKELY(mSlots[buf].mFence == NULL)) {
            ST_LOGE("dequeueBuffer: about to return a NULL fence from mSlot. "
                    "buf=%d, w=%d, h=%d, format=%d",
                    buf, buffer->width, buffer->height, buffer->format);
        }

        dpy = mSlots[buf].mEglDisplay;
        eglFence = mSlots[buf].mEglFence;
        *outFence = mSlots[buf].mFence;
        mSlots[buf].mEglFence = EGL_NO_SYNC_KHR;
        mSlots[buf].mFence = Fence::NO_FENCE;
    }  

    if (returnFlags & IGraphicBufferProducer::BUFFER_NEEDS_REALLOCATION) {
        status_t error;
        sp<GraphicBuffer> graphicBuffer(
                mGraphicBufferAlloc->createGraphicBuffer(w, h, format, usage, &error));
        if (graphicBuffer == 0) {
            ST_LOGE("dequeueBuffer: SurfaceComposer::createGraphicBuffer failed");
            return error;
        }

        { 
            Mutex::Autolock lock(mMutex);

            if (mAbandoned) {
                ST_LOGE("dequeueBuffer: BufferQueue has been abandoned!");
                return NO_INIT;
            }

            mSlots[*outBuf].mFrameNumber = ~0;
            mSlots[*outBuf].mGraphicBuffer = graphicBuffer;
        }
    }

    if (eglFence != EGL_NO_SYNC_KHR) {
        EGLint result = eglClientWaitSyncKHR(dpy, eglFence, 0, 1000000000);
        
        
        
        if (result == EGL_FALSE) {
            ST_LOGE("dequeueBuffer: error waiting for fence: %#x", eglGetError());
        } else if (result == EGL_TIMEOUT_EXPIRED_KHR) {
            ST_LOGE("dequeueBuffer: timeout waiting for fence");
        }
        eglDestroySyncKHR(dpy, eglFence);
    }

    ST_LOGV("dequeueBuffer: returning slot=%d/%llu buf=%p flags=%#x", *outBuf,
            mSlots[*outBuf].mFrameNumber,
            mSlots[*outBuf].mGraphicBuffer->handle, returnFlags);

    return returnFlags;
}

status_t BufferQueue::queueBuffer(int buf,
        const QueueBufferInput& input, QueueBufferOutput* output) {
    ATRACE_CALL();
    ATRACE_BUFFER_INDEX(buf);

    Rect crop;
    uint32_t transform;
    int scalingMode;
    int64_t timestamp;
    bool isAutoTimestamp;
    bool async;
    sp<Fence> fence;

    input.deflate(&timestamp, &isAutoTimestamp, &crop, &scalingMode, &transform,
            &async, &fence);

    if (fence == NULL) {
        ST_LOGE("queueBuffer: fence is NULL");
        return BAD_VALUE;
    }

    switch (scalingMode) {
        case NATIVE_WINDOW_SCALING_MODE_FREEZE:
        case NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW:
        case NATIVE_WINDOW_SCALING_MODE_SCALE_CROP:
        case NATIVE_WINDOW_SCALING_MODE_NO_SCALE_CROP:
            break;
        default:
            ST_LOGE("unknown scaling mode: %d", scalingMode);
            return -EINVAL;
    }

    sp<IConsumerListener> listener;

    { 
        Mutex::Autolock lock(mMutex);

        if (mAbandoned) {
            ST_LOGE("queueBuffer: BufferQueue has been abandoned!");
            return NO_INIT;
        }

        const int maxBufferCount = getMaxBufferCountLocked(async);
        if (async && mOverrideMaxBufferCount) {
            
            
            
            if (mOverrideMaxBufferCount < maxBufferCount) {
                ST_LOGE("queueBuffer: async mode is invalid with buffercount override");
                return BAD_VALUE;
            }
        }
        if (buf < 0 || buf >= maxBufferCount) {
            ST_LOGE("queueBuffer: slot index out of range [0, %d]: %d",
                    maxBufferCount, buf);
            return -EINVAL;
        } else if (mSlots[buf].mBufferState != BufferSlot::DEQUEUED) {
            ST_LOGE("queueBuffer: slot %d is not owned by the client "
                    "(state=%d)", buf, mSlots[buf].mBufferState);
            return -EINVAL;
        } else if (!mSlots[buf].mRequestBufferCalled) {
            ST_LOGE("queueBuffer: slot %d was enqueued without requesting a "
                    "buffer", buf);
            return -EINVAL;
        }

        ST_LOGV("queueBuffer: slot=%d/%llu time=%#llx crop=[%d,%d,%d,%d] "
                "tr=%#x scale=%s",
                buf, mFrameCounter + 1, timestamp,
                crop.left, crop.top, crop.right, crop.bottom,
                transform, scalingModeName(scalingMode));

        const sp<GraphicBuffer>& graphicBuffer(mSlots[buf].mGraphicBuffer);
        Rect bufferRect(graphicBuffer->getWidth(), graphicBuffer->getHeight());
        Rect croppedCrop;
        crop.intersect(bufferRect, &croppedCrop);
        if (croppedCrop != crop) {
            ST_LOGE("queueBuffer: crop rect is not contained within the "
                    "buffer in slot %d", buf);
            return -EINVAL;
        }

        mSlots[buf].mFence = fence;
        mSlots[buf].mBufferState = BufferSlot::QUEUED;
        mFrameCounter++;
        mSlots[buf].mFrameNumber = mFrameCounter;

        BufferItem item;
        item.mAcquireCalled = mSlots[buf].mAcquireCalled;
        item.mGraphicBuffer = mSlots[buf].mGraphicBuffer;
        item.mCrop = crop;
        item.mTransform = transform & ~NATIVE_WINDOW_TRANSFORM_INVERSE_DISPLAY;
        item.mTransformToDisplayInverse = bool(transform & NATIVE_WINDOW_TRANSFORM_INVERSE_DISPLAY);
        item.mScalingMode = scalingMode;
        item.mTimestamp = timestamp;
        item.mIsAutoTimestamp = isAutoTimestamp;
        item.mFrameNumber = mFrameCounter;
        item.mBuf = buf;
        item.mFence = fence;
        item.mIsDroppable = mDequeueBufferCannotBlock || async;

        if (mQueue.empty()) {
            
            
            mQueue.push_back(item);
            listener = mConsumerListener;
        } else {
            
            
            Fifo::iterator front(mQueue.begin());
            if (front->mIsDroppable) {
                
                if (stillTracking(front)) {
                    mSlots[front->mBuf].mBufferState = BufferSlot::FREE;
                    
                    
                    mSlots[front->mBuf].mFrameNumber = 0;
                }
                
                *front = item;
            } else {
                mQueue.push_back(item);
                listener = mConsumerListener;
            }
        }

        mBufferHasBeenQueued = true;
        mDequeueCondition.broadcast();

        output->inflate(mDefaultWidth, mDefaultHeight, mTransformHint,
                mQueue.size());

        ATRACE_INT(mConsumerName.string(), mQueue.size());
    } 

    
    if (listener != 0) {
        listener->onFrameAvailable();
    }
    return NO_ERROR;
}

void BufferQueue::cancelBuffer(int buf, const sp<Fence>& fence) {
    ATRACE_CALL();
    ST_LOGV("cancelBuffer: slot=%d", buf);
    Mutex::Autolock lock(mMutex);

    if (mAbandoned) {
        ST_LOGW("cancelBuffer: BufferQueue has been abandoned!");
        return;
    }

    if (buf < 0 || buf >= NUM_BUFFER_SLOTS) {
        ST_LOGE("cancelBuffer: slot index out of range [0, %d]: %d",
                NUM_BUFFER_SLOTS, buf);
        return;
    } else if (mSlots[buf].mBufferState != BufferSlot::DEQUEUED) {
        ST_LOGE("cancelBuffer: slot %d is not owned by the client (state=%d)",
                buf, mSlots[buf].mBufferState);
        return;
    } else if (fence == NULL) {
        ST_LOGE("cancelBuffer: fence is NULL");
        return;
    }
    mSlots[buf].mBufferState = BufferSlot::FREE;
    mSlots[buf].mFrameNumber = 0;
    mSlots[buf].mFence = fence;
    mDequeueCondition.broadcast();
}


status_t BufferQueue::connect(const sp<IBinder>& token,
        int api, bool producerControlledByApp, QueueBufferOutput* output) {
    ATRACE_CALL();
    ST_LOGV("connect: api=%d producerControlledByApp=%s", api,
            producerControlledByApp ? "true" : "false");
    Mutex::Autolock lock(mMutex);

retry:
    if (mAbandoned) {
        ST_LOGE("connect: BufferQueue has been abandoned!");
        return NO_INIT;
    }

    if (mConsumerListener == NULL) {
        ST_LOGE("connect: BufferQueue has no consumer!");
        return NO_INIT;
    }

    if (mConnectedApi != NO_CONNECTED_API) {
        ST_LOGE("connect: already connected (cur=%d, req=%d)",
                mConnectedApi, api);
        return -EINVAL;
    }

    
    
    
    
    int maxBufferCount = getMaxBufferCountLocked(false);    
    if (mQueue.size() > (size_t) maxBufferCount) {
        
        ST_LOGV("queue size is %d, waiting", mQueue.size());
        mDequeueCondition.wait(mMutex);
        goto retry;
    }

    int err = NO_ERROR;
    switch (api) {
        case NATIVE_WINDOW_API_EGL:
        case NATIVE_WINDOW_API_CPU:
        case NATIVE_WINDOW_API_MEDIA:
        case NATIVE_WINDOW_API_CAMERA:
            mConnectedApi = api;
            output->inflate(mDefaultWidth, mDefaultHeight, mTransformHint, mQueue.size());

            
            
            if (token != NULL && token->remoteBinder() != NULL) {
                status_t err = token->linkToDeath(static_cast<IBinder::DeathRecipient*>(this));
                if (err == NO_ERROR) {
                    mConnectedProducerToken = token;
                } else {
                    ALOGE("linkToDeath failed: %s (%d)", strerror(-err), err);
                }
            }
            break;
        default:
            err = -EINVAL;
            break;
    }

    mBufferHasBeenQueued = false;
    mDequeueBufferCannotBlock = mConsumerControlledByApp && producerControlledByApp;

    return err;
}

void BufferQueue::binderDied(const wp<IBinder>& who) {
    
    
    
    
    
    int api = mConnectedApi;
    this->disconnect(api);
}

status_t BufferQueue::disconnect(int api) {
    ATRACE_CALL();
    ST_LOGV("disconnect: api=%d", api);

    int err = NO_ERROR;
    sp<IConsumerListener> listener;

    { 
        Mutex::Autolock lock(mMutex);

        if (mAbandoned) {
            
            
            return NO_ERROR;
        }

        switch (api) {
            case NATIVE_WINDOW_API_EGL:
            case NATIVE_WINDOW_API_CPU:
            case NATIVE_WINDOW_API_MEDIA:
            case NATIVE_WINDOW_API_CAMERA:
                if (mConnectedApi == api) {
                    freeAllBuffersLocked();
                    
                    sp<IBinder> token = mConnectedProducerToken;
                    if (token != NULL) {
                        
                        
                        token->unlinkToDeath(static_cast<IBinder::DeathRecipient*>(this));
                    }
                    mConnectedProducerToken = NULL;
                    mConnectedApi = NO_CONNECTED_API;
                    mDequeueCondition.broadcast();
                    listener = mConsumerListener;
                } else {
                    ST_LOGE("disconnect: connected to another api (cur=%d, req=%d)",
                            mConnectedApi, api);
                    err = -EINVAL;
                }
                break;
            default:
                ST_LOGE("disconnect: unknown API %d", api);
                err = -EINVAL;
                break;
        }
    }

    if (listener != NULL) {
        listener->onBuffersReleased();
    }

    return err;
}

void BufferQueue::dump(String8& result, const char* prefix) const {
    Mutex::Autolock _l(mMutex);

    String8 fifo;
    int fifoSize = 0;
    Fifo::const_iterator i(mQueue.begin());
    while (i != mQueue.end()) {
        fifo.appendFormat("%02d:%p crop=[%d,%d,%d,%d], "
                "xform=0x%02x, time=%#llx, scale=%s\n",
                i->mBuf, i->mGraphicBuffer.get(),
                i->mCrop.left, i->mCrop.top, i->mCrop.right,
                i->mCrop.bottom, i->mTransform, i->mTimestamp,
                scalingModeName(i->mScalingMode)
                );
        i++;
        fifoSize++;
    }


    result.appendFormat(
            "%s-BufferQueue mMaxAcquiredBufferCount=%d, mDequeueBufferCannotBlock=%d, default-size=[%dx%d], "
            "default-format=%d, transform-hint=%02x, FIFO(%d)={%s}\n",
            prefix, mMaxAcquiredBufferCount, mDequeueBufferCannotBlock, mDefaultWidth,
            mDefaultHeight, mDefaultBufferFormat, mTransformHint,
            fifoSize, fifo.string());

    struct {
        const char * operator()(int state) const {
            switch (state) {
                case BufferSlot::DEQUEUED: return "DEQUEUED";
                case BufferSlot::QUEUED: return "QUEUED";
                case BufferSlot::FREE: return "FREE";
                case BufferSlot::ACQUIRED: return "ACQUIRED";
                default: return "Unknown";
            }
        }
    } stateName;

    
    int maxBufferCount = 0;
    for (int i=NUM_BUFFER_SLOTS-1 ; i>=0 ; i--) {
        const BufferSlot& slot(mSlots[i]);
        if ((slot.mBufferState != BufferSlot::FREE) || (slot.mGraphicBuffer != NULL)) {
            maxBufferCount = i+1;
            break;
        }
    }

    for (int i=0 ; i<maxBufferCount ; i++) {
        const BufferSlot& slot(mSlots[i]);
        const sp<GraphicBuffer>& buf(slot.mGraphicBuffer);
        result.appendFormat(
            "%s%s[%02d:%p] state=%-8s",
                prefix, (slot.mBufferState == BufferSlot::ACQUIRED)?">":" ", i, buf.get(),
                stateName(slot.mBufferState)
        );

        if (buf != NULL) {
            result.appendFormat(
                    ", %p [%4ux%4u:%4u,%3X]",
                    buf->handle, buf->width, buf->height, buf->stride,
                    buf->format);
        }
        result.append("\n");
    }
}

void BufferQueue::freeBufferLocked(int slot) {
    ST_LOGV("freeBufferLocked: slot=%d", slot);
    mSlots[slot].mGraphicBuffer = 0;
    if (mSlots[slot].mBufferState == BufferSlot::ACQUIRED) {
        mSlots[slot].mNeedsCleanupOnRelease = true;
    }
    mSlots[slot].mBufferState = BufferSlot::FREE;
    mSlots[slot].mFrameNumber = 0;
    mSlots[slot].mAcquireCalled = false;

    
    if (mSlots[slot].mEglFence != EGL_NO_SYNC_KHR) {
        eglDestroySyncKHR(mSlots[slot].mEglDisplay, mSlots[slot].mEglFence);
        mSlots[slot].mEglFence = EGL_NO_SYNC_KHR;
    }
    mSlots[slot].mFence = Fence::NO_FENCE;
}

void BufferQueue::freeAllBuffersLocked() {
    mBufferHasBeenQueued = false;
    for (int i = 0; i < NUM_BUFFER_SLOTS; i++) {
        freeBufferLocked(i);
    }
}

status_t BufferQueue::acquireBuffer(BufferItem *buffer, nsecs_t expectedPresent) {
    ATRACE_CALL();
    Mutex::Autolock _l(mMutex);

    
    
    
    
    int numAcquiredBuffers = 0;
    for (int i = 0; i < NUM_BUFFER_SLOTS; i++) {
        if (mSlots[i].mBufferState == BufferSlot::ACQUIRED) {
            numAcquiredBuffers++;
        }
    }
    if (numAcquiredBuffers >= mMaxAcquiredBufferCount+1) {
        ST_LOGE("acquireBuffer: max acquired buffer count reached: %d (max=%d)",
                numAcquiredBuffers, mMaxAcquiredBufferCount);
        return INVALID_OPERATION;
    }

    
    
    
    if (mQueue.empty()) {
        return NO_BUFFER_AVAILABLE;
    }

    Fifo::iterator front(mQueue.begin());

    
    
    
    if (expectedPresent != 0) {
        const int MAX_REASONABLE_NSEC = 1000000000ULL;  

        
        
        
        
        
        
        
        
        
        
        
        
        

        
        
        
        
        while (mQueue.size() > 1 && !mQueue[0].mIsAutoTimestamp) {
            
            
            
            
            
            
            
            
            
            
            
            
            const BufferItem& bi(mQueue[1]);
            nsecs_t desiredPresent = bi.mTimestamp;
            if (desiredPresent < expectedPresent - MAX_REASONABLE_NSEC ||
                    desiredPresent > expectedPresent) {
                
                
                
                ST_LOGV("pts nodrop: des=%lld expect=%lld (%lld) now=%lld",
                        desiredPresent, expectedPresent, desiredPresent - expectedPresent,
                        systemTime(CLOCK_MONOTONIC));
                break;
            }
            ST_LOGV("pts drop: queue1des=%lld expect=%lld size=%d",
                    desiredPresent, expectedPresent, mQueue.size());
            if (stillTracking(front)) {
                
                mSlots[front->mBuf].mBufferState = BufferSlot::FREE;
            }
            mQueue.erase(front);
            front = mQueue.begin();
        }

        
        nsecs_t desiredPresent = front->mTimestamp;
        if (desiredPresent > expectedPresent &&
                desiredPresent < expectedPresent + MAX_REASONABLE_NSEC) {
            ST_LOGV("pts defer: des=%lld expect=%lld (%lld) now=%lld",
                    desiredPresent, expectedPresent, desiredPresent - expectedPresent,
                    systemTime(CLOCK_MONOTONIC));
            return PRESENT_LATER;
        }

        ST_LOGV("pts accept: des=%lld expect=%lld (%lld) now=%lld",
                desiredPresent, expectedPresent, desiredPresent - expectedPresent,
                systemTime(CLOCK_MONOTONIC));
    }

    int buf = front->mBuf;
    *buffer = *front;
    ATRACE_BUFFER_INDEX(buf);

    ST_LOGV("acquireBuffer: acquiring { slot=%d/%llu, buffer=%p }",
            front->mBuf, front->mFrameNumber,
            front->mGraphicBuffer->handle);
    
    if (stillTracking(front)) {
        mSlots[buf].mAcquireCalled = true;
        mSlots[buf].mNeedsCleanupOnRelease = false;
        mSlots[buf].mBufferState = BufferSlot::ACQUIRED;
        mSlots[buf].mFence = Fence::NO_FENCE;
    }

    
    
    
    if (buffer->mAcquireCalled) {
        buffer->mGraphicBuffer = NULL;
    }

    mQueue.erase(front);
    mDequeueCondition.broadcast();

    ATRACE_INT(mConsumerName.string(), mQueue.size());

    return NO_ERROR;
}

status_t BufferQueue::releaseBuffer(
        int buf, uint64_t frameNumber, EGLDisplay display,
        EGLSyncKHR eglFence, const sp<Fence>& fence) {
    ATRACE_CALL();
    ATRACE_BUFFER_INDEX(buf);

    if (buf == INVALID_BUFFER_SLOT || fence == NULL) {
        return BAD_VALUE;
    }

    Mutex::Autolock _l(mMutex);

    
    
    if (frameNumber != mSlots[buf].mFrameNumber) {
        return STALE_BUFFER_SLOT;
    }


    
    
    Fifo::iterator front(mQueue.begin());
    Fifo::const_iterator const end(mQueue.end());
    while (front != end) {
        if (front->mBuf == buf) {
            LOG_ALWAYS_FATAL("[%s] received new buffer(#%lld) on slot #%d that has not yet been "
                    "acquired", mConsumerName.string(), frameNumber, buf);
            break; 
        }
        front++;
    }

    
    if (mSlots[buf].mBufferState == BufferSlot::ACQUIRED) {
        mSlots[buf].mEglDisplay = display;
        mSlots[buf].mEglFence = eglFence;
        mSlots[buf].mFence = fence;
        mSlots[buf].mBufferState = BufferSlot::FREE;
    } else if (mSlots[buf].mNeedsCleanupOnRelease) {
        ST_LOGV("releasing a stale buf %d its state was %d", buf, mSlots[buf].mBufferState);
        mSlots[buf].mNeedsCleanupOnRelease = false;
        return STALE_BUFFER_SLOT;
    } else {
        ST_LOGE("attempted to release buf %d but its state was %d", buf, mSlots[buf].mBufferState);
        return -EINVAL;
    }

    mDequeueCondition.broadcast();
    return NO_ERROR;
}

status_t BufferQueue::consumerConnect(const sp<IConsumerListener>& consumerListener,
        bool controlledByApp) {
    ST_LOGV("consumerConnect controlledByApp=%s",
            controlledByApp ? "true" : "false");
    Mutex::Autolock lock(mMutex);

    if (mAbandoned) {
        ST_LOGE("consumerConnect: BufferQueue has been abandoned!");
        return NO_INIT;
    }
    if (consumerListener == NULL) {
        ST_LOGE("consumerConnect: consumerListener may not be NULL");
        return BAD_VALUE;
    }

    mConsumerListener = consumerListener;
    mConsumerControlledByApp = controlledByApp;

    return NO_ERROR;
}

status_t BufferQueue::consumerDisconnect() {
    ST_LOGV("consumerDisconnect");
    Mutex::Autolock lock(mMutex);

    if (mConsumerListener == NULL) {
        ST_LOGE("consumerDisconnect: No consumer is connected!");
        return -EINVAL;
    }

    mAbandoned = true;
    mConsumerListener = NULL;
    mQueue.clear();
    freeAllBuffersLocked();
    mDequeueCondition.broadcast();
    return NO_ERROR;
}

status_t BufferQueue::getReleasedBuffers(uint32_t* slotMask) {
    ST_LOGV("getReleasedBuffers");
    Mutex::Autolock lock(mMutex);

    if (mAbandoned) {
        ST_LOGE("getReleasedBuffers: BufferQueue has been abandoned!");
        return NO_INIT;
    }

    uint32_t mask = 0;
    for (int i = 0; i < NUM_BUFFER_SLOTS; i++) {
        if (!mSlots[i].mAcquireCalled) {
            mask |= 1 << i;
        }
    }

    
    
    
    Fifo::iterator front(mQueue.begin());
    while (front != mQueue.end()) {
        if (front->mAcquireCalled)
            mask &= ~(1 << front->mBuf);
        front++;
    }

    *slotMask = mask;

    ST_LOGV("getReleasedBuffers: returning mask %#x", mask);
    return NO_ERROR;
}

status_t BufferQueue::setDefaultBufferSize(uint32_t w, uint32_t h) {
    ST_LOGV("setDefaultBufferSize: w=%d, h=%d", w, h);
    if (!w || !h) {
        ST_LOGE("setDefaultBufferSize: dimensions cannot be 0 (w=%d, h=%d)",
                w, h);
        return BAD_VALUE;
    }

    Mutex::Autolock lock(mMutex);
    mDefaultWidth = w;
    mDefaultHeight = h;
    return NO_ERROR;
}

status_t BufferQueue::setDefaultMaxBufferCount(int bufferCount) {
    ATRACE_CALL();
    Mutex::Autolock lock(mMutex);
    return setDefaultMaxBufferCountLocked(bufferCount);
}

status_t BufferQueue::disableAsyncBuffer() {
    ATRACE_CALL();
    Mutex::Autolock lock(mMutex);
    if (mConsumerListener != NULL) {
        ST_LOGE("disableAsyncBuffer: consumer already connected!");
        return INVALID_OPERATION;
    }
    mUseAsyncBuffer = false;
    return NO_ERROR;
}

status_t BufferQueue::setMaxAcquiredBufferCount(int maxAcquiredBuffers) {
    ATRACE_CALL();
    Mutex::Autolock lock(mMutex);
    if (maxAcquiredBuffers < 1 || maxAcquiredBuffers > MAX_MAX_ACQUIRED_BUFFERS) {
        ST_LOGE("setMaxAcquiredBufferCount: invalid count specified: %d",
                maxAcquiredBuffers);
        return BAD_VALUE;
    }
    if (mConnectedApi != NO_CONNECTED_API) {
        return INVALID_OPERATION;
    }
    mMaxAcquiredBufferCount = maxAcquiredBuffers;
    return NO_ERROR;
}

int BufferQueue::getMinUndequeuedBufferCount(bool async) const {
    
    
    if (!mUseAsyncBuffer)
        return mMaxAcquiredBufferCount;

    
    
    if (mDequeueBufferCannotBlock || async)
        return mMaxAcquiredBufferCount+1;

    return mMaxAcquiredBufferCount;
}

int BufferQueue::getMinMaxBufferCountLocked(bool async) const {
    return getMinUndequeuedBufferCount(async) + 1;
}

int BufferQueue::getMaxBufferCountLocked(bool async) const {
    int minMaxBufferCount = getMinMaxBufferCountLocked(async);

    int maxBufferCount = mDefaultMaxBufferCount;
    if (maxBufferCount < minMaxBufferCount) {
        maxBufferCount = minMaxBufferCount;
    }
    if (mOverrideMaxBufferCount != 0) {
        assert(mOverrideMaxBufferCount >= minMaxBufferCount);
        maxBufferCount = mOverrideMaxBufferCount;
    }

    
    
    
    
    for (int i = maxBufferCount; i < NUM_BUFFER_SLOTS; i++) {
        BufferSlot::BufferState state = mSlots[i].mBufferState;
        if (state == BufferSlot::QUEUED || state == BufferSlot::DEQUEUED) {
            maxBufferCount = i + 1;
        }
    }

    return maxBufferCount;
}

bool BufferQueue::stillTracking(const BufferItem *item) const {
    const BufferSlot &slot = mSlots[item->mBuf];

    ST_LOGV("stillTracking?: item: { slot=%d/%llu, buffer=%p }, "
            "slot: { slot=%d/%llu, buffer=%p }",
            item->mBuf, item->mFrameNumber,
            (item->mGraphicBuffer.get() ? item->mGraphicBuffer->handle : 0),
            item->mBuf, slot.mFrameNumber,
            (slot.mGraphicBuffer.get() ? slot.mGraphicBuffer->handle : 0));

    
    
    return (slot.mGraphicBuffer != NULL &&
            item->mGraphicBuffer->handle == slot.mGraphicBuffer->handle);
}

BufferQueue::ProxyConsumerListener::ProxyConsumerListener(
        const wp<ConsumerListener>& consumerListener):
        mConsumerListener(consumerListener) {}

BufferQueue::ProxyConsumerListener::~ProxyConsumerListener() {}

void BufferQueue::ProxyConsumerListener::onFrameAvailable() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onFrameAvailable();
    }
}

void BufferQueue::ProxyConsumerListener::onBuffersReleased() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onBuffersReleased();
    }
}

}; 
