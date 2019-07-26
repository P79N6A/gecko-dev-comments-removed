
















#define LOG_TAG "GonkConsumerBase"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#define EGL_EGLEXT_PROTOTYPES

#include <hardware/hardware.h>

#include <gui/IGraphicBufferAlloc.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include "GonkConsumerBase.h"


#define CB_LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define CB_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define CB_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define CB_LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define CB_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace android {


static int32_t createProcessUniqueId() {
    static volatile int32_t globalCounter = 0;
    return android_atomic_inc(&globalCounter);
}

GonkConsumerBase::GonkConsumerBase(const sp<GonkBufferQueue>& bufferQueue) :
        mAbandoned(false),
        mBufferQueue(bufferQueue) {
    
    mName = String8::format("unnamed-%d-%d", getpid(), createProcessUniqueId());

    
    
    
    
    wp<GonkBufferQueue::ConsumerListener> listener;
    sp<GonkBufferQueue::ConsumerListener> proxy;
    listener = static_cast<GonkBufferQueue::ConsumerListener*>(this);
    proxy = new GonkBufferQueue::ProxyConsumerListener(listener);

    status_t err = mBufferQueue->consumerConnect(proxy);
    if (err != NO_ERROR) {
        CB_LOGE("GonkConsumerBase: error connecting to GonkBufferQueue: %s (%d)",
                strerror(-err), err);
    } else {
        mBufferQueue->setConsumerName(mName);
    }
}

GonkConsumerBase::~GonkConsumerBase() {
    CB_LOGV("~GonkConsumerBase");
    Mutex::Autolock lock(mMutex);

    
    
    
    
    LOG_ALWAYS_FATAL_IF(!mAbandoned, "[%s] ~GonkConsumerBase was called, but the "
        "consumer is not abandoned!", mName.string());
}

void GonkConsumerBase::onLastStrongRef(const void* id) {
    abandon();
}

void GonkConsumerBase::freeBufferLocked(int slotIndex) {
    CB_LOGV("freeBufferLocked: slotIndex=%d", slotIndex);
    mSlots[slotIndex].mGraphicBuffer = 0;
    mSlots[slotIndex].mFence = Fence::NO_FENCE;
}


sp<GonkBufferQueue> GonkConsumerBase::getBufferQueue() const {
    Mutex::Autolock lock(mMutex);
    return mBufferQueue;
}

void GonkConsumerBase::onFrameAvailable() {
    CB_LOGV("onFrameAvailable");

    sp<FrameAvailableListener> listener;
    { 
        Mutex::Autolock lock(mMutex);
#if ANDROID_VERSION == 17
        listener = mFrameAvailableListener;
#else
        listener = mFrameAvailableListener.promote();
#endif
    }

    if (listener != NULL) {
        CB_LOGV("actually calling onFrameAvailable");
        listener->onFrameAvailable();
    }
}

void GonkConsumerBase::onBuffersReleased() {
    Mutex::Autolock lock(mMutex);

    CB_LOGV("onBuffersReleased");

    if (mAbandoned) {
        
        return;
    }

    uint32_t mask = 0;
    mBufferQueue->getReleasedBuffers(&mask);
    for (int i = 0; i < GonkBufferQueue::NUM_BUFFER_SLOTS; i++) {
        if (mask & (1 << i)) {
            freeBufferLocked(i);
        }
    }
}

void GonkConsumerBase::abandon() {
    CB_LOGV("abandon");
    Mutex::Autolock lock(mMutex);

    if (!mAbandoned) {
        abandonLocked();
        mAbandoned = true;
    }
}

void GonkConsumerBase::abandonLocked() {
	CB_LOGV("abandonLocked");
    for (int i =0; i < GonkBufferQueue::NUM_BUFFER_SLOTS; i++) {
        freeBufferLocked(i);
    }
    
    mBufferQueue->consumerDisconnect();
    mBufferQueue.clear();
}

void GonkConsumerBase::setFrameAvailableListener(
#if ANDROID_VERSION == 17
        const sp<FrameAvailableListener>& listener) {
#else
        const wp<FrameAvailableListener>& listener) {
#endif
    CB_LOGV("setFrameAvailableListener");
    Mutex::Autolock lock(mMutex);
    mFrameAvailableListener = listener;
}

void GonkConsumerBase::dump(String8& result) const {
    char buffer[1024];
    dump(result, "", buffer, 1024);
}

void GonkConsumerBase::dump(String8& result, const char* prefix,
        char* buffer, size_t size) const {
    Mutex::Autolock _l(mMutex);
    dumpLocked(result, prefix, buffer, size);
}

void GonkConsumerBase::dumpLocked(String8& result, const char* prefix,
        char* buffer, size_t SIZE) const {
    snprintf(buffer, SIZE, "%smAbandoned=%d\n", prefix, int(mAbandoned));
    result.append(buffer);

    if (!mAbandoned) {
        mBufferQueue->dump(result, prefix, buffer, SIZE);
    }
}

status_t GonkConsumerBase::acquireBufferLocked(GonkBufferQueue::BufferItem *item) {
    status_t err = mBufferQueue->acquireBuffer(item);
    if (err != NO_ERROR) {
        return err;
    }

    if (item->mGraphicBuffer != NULL) {
        mSlots[item->mBuf].mGraphicBuffer = item->mGraphicBuffer;
    }

    mSlots[item->mBuf].mFence = item->mFence;

    CB_LOGV("acquireBufferLocked: -> slot=%d", item->mBuf);

    return OK;
}

status_t GonkConsumerBase::addReleaseFence(int slot, const sp<Fence>& fence) {
    Mutex::Autolock lock(mMutex);
    return addReleaseFenceLocked(slot, fence);
}

status_t GonkConsumerBase::addReleaseFenceLocked(int slot, const sp<Fence>& fence) {
    CB_LOGV("addReleaseFenceLocked: slot=%d", slot);

    if (!mSlots[slot].mFence.get()) {
        mSlots[slot].mFence = fence;
    } else {
        sp<Fence> mergedFence = Fence::merge(
                String8::format("%.28s:%d", mName.string(), slot),
                mSlots[slot].mFence, fence);
        if (!mergedFence.get()) {
            CB_LOGE("failed to merge release fences");
            
            
            mSlots[slot].mFence = fence;
            return BAD_VALUE;
        }
        mSlots[slot].mFence = mergedFence;
    }

    return OK;
}

status_t GonkConsumerBase::releaseBufferLocked(int slot) {
    CB_LOGV("releaseBufferLocked: slot=%d", slot);
    status_t err = mBufferQueue->releaseBuffer(slot, mSlots[slot].mFence);
    if (err == GonkBufferQueue::STALE_BUFFER_SLOT) {
        freeBufferLocked(slot);
    }

    mSlots[slot].mFence = Fence::NO_FENCE;

    return err;
}

} 
