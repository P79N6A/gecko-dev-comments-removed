
















#define LOG_TAG "GonkConsumerBase"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#define EGL_EGLEXT_PROTOTYPES

#include <hardware/hardware.h>

#include <gui/IGraphicBufferAlloc.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include "GonkConsumerBaseKK.h"

namespace android {


static int32_t createProcessUniqueId() {
    static volatile int32_t globalCounter = 0;
    return android_atomic_inc(&globalCounter);
}

GonkConsumerBase::GonkConsumerBase(const sp<GonkBufferQueue>& bufferQueue, bool controlledByApp) :
        mAbandoned(false),
        mConsumer(bufferQueue) {
    
    mName = String8::format("unnamed-%d-%d", getpid(), createProcessUniqueId());

    
    
    
    
    wp<ConsumerListener> listener = static_cast<ConsumerListener*>(this);
    sp<IConsumerListener> proxy = new GonkBufferQueue::ProxyConsumerListener(listener);

    status_t err = mConsumer->consumerConnect(proxy, controlledByApp);
    if (err != NO_ERROR) {
        ALOGE("GonkConsumerBase: error connecting to GonkBufferQueue: %s (%d)",
                strerror(-err), err);
    } else {
        mConsumer->setConsumerName(mName);
    }
}

GonkConsumerBase::~GonkConsumerBase() {
    ALOGV("~GonkConsumerBase");
    Mutex::Autolock lock(mMutex);

    
    
    
    
    LOG_ALWAYS_FATAL_IF(!mAbandoned, "[%s] ~GonkConsumerBase was called, but the "
        "consumer is not abandoned!", mName.string());
}

void GonkConsumerBase::onLastStrongRef(const void* id) {
    abandon();
}

void GonkConsumerBase::freeBufferLocked(int slotIndex) {
    ALOGV("freeBufferLocked: slotIndex=%d", slotIndex);
    mSlots[slotIndex].mGraphicBuffer = 0;
    mSlots[slotIndex].mFence = Fence::NO_FENCE;
    mSlots[slotIndex].mFrameNumber = 0;
}


sp<GonkBufferQueue> GonkConsumerBase::getBufferQueue() const {
    Mutex::Autolock lock(mMutex);
    return mConsumer;
}

void GonkConsumerBase::onFrameAvailable() {
    ALOGV("onFrameAvailable");

    sp<FrameAvailableListener> listener;
    { 
        Mutex::Autolock lock(mMutex);
        listener = mFrameAvailableListener.promote();
    }

    if (listener != NULL) {
        ALOGV("actually calling onFrameAvailable");
        listener->onFrameAvailable();
    }
}

void GonkConsumerBase::onBuffersReleased() {
    Mutex::Autolock lock(mMutex);

    ALOGV("onBuffersReleased");

    if (mAbandoned) {
        
        return;
    }

    uint32_t mask = 0;
    mConsumer->getReleasedBuffers(&mask);
    for (int i = 0; i < GonkBufferQueue::NUM_BUFFER_SLOTS; i++) {
        if (mask & (1 << i)) {
            freeBufferLocked(i);
        }
    }
}

void GonkConsumerBase::abandon() {
    ALOGV("abandon");
    Mutex::Autolock lock(mMutex);

    if (!mAbandoned) {
        abandonLocked();
        mAbandoned = true;
    }
}

void GonkConsumerBase::abandonLocked() {
	ALOGV("abandonLocked");
    for (int i =0; i < GonkBufferQueue::NUM_BUFFER_SLOTS; i++) {
        freeBufferLocked(i);
    }
    
    mConsumer->consumerDisconnect();
    mConsumer.clear();
}

void GonkConsumerBase::setFrameAvailableListener(
        const wp<FrameAvailableListener>& listener) {
    ALOGV("setFrameAvailableListener");
    Mutex::Autolock lock(mMutex);
    mFrameAvailableListener = listener;
}

void GonkConsumerBase::dump(String8& result) const {
    dump(result, "");
}

void GonkConsumerBase::dump(String8& result, const char* prefix) const {
    Mutex::Autolock _l(mMutex);
    dumpLocked(result, prefix);
}

void GonkConsumerBase::dumpLocked(String8& result, const char* prefix) const {
    result.appendFormat("%smAbandoned=%d\n", prefix, int(mAbandoned));

    if (!mAbandoned) {
        mConsumer->dump(result, prefix);
    }
}

status_t GonkConsumerBase::acquireBufferLocked(IGonkGraphicBufferConsumer::BufferItem *item,
        nsecs_t presentWhen) {
    status_t err = mConsumer->acquireBuffer(item, presentWhen);
    if (err != NO_ERROR) {
        return err;
    }

    if (item->mGraphicBuffer != NULL) {
        mSlots[item->mBuf].mGraphicBuffer = item->mGraphicBuffer;
    }

    mSlots[item->mBuf].mFrameNumber = item->mFrameNumber;
    mSlots[item->mBuf].mFence = item->mFence;

    ALOGV("acquireBufferLocked: -> slot=%d", item->mBuf);

    return OK;
}

status_t GonkConsumerBase::addReleaseFence(int slot,
        const sp<GraphicBuffer> graphicBuffer, const sp<Fence>& fence) {
    Mutex::Autolock lock(mMutex);
    return addReleaseFenceLocked(slot, graphicBuffer, fence);
}

status_t GonkConsumerBase::addReleaseFenceLocked(int slot,
        const sp<GraphicBuffer> graphicBuffer, const sp<Fence>& fence) {
    ALOGV("addReleaseFenceLocked: slot=%d", slot);

    
    
    if (!stillTracking(slot, graphicBuffer)) {
        return OK;
    }

    if (!mSlots[slot].mFence.get()) {
        mSlots[slot].mFence = fence;
    } else {
        sp<Fence> mergedFence = Fence::merge(
                String8::format("%.28s:%d", mName.string(), slot),
                mSlots[slot].mFence, fence);
        if (!mergedFence.get()) {
            ALOGE("failed to merge release fences");
            
            
            mSlots[slot].mFence = fence;
            return BAD_VALUE;
        }
        mSlots[slot].mFence = mergedFence;
    }

    return OK;
}

status_t GonkConsumerBase::releaseBufferLocked(int slot, const sp<GraphicBuffer> graphicBuffer) {
    
    
    
    if (!stillTracking(slot, graphicBuffer)) {
        return OK;
    }

    ALOGV("releaseBufferLocked: slot=%d/%llu",
            slot, mSlots[slot].mFrameNumber);
    status_t err = mConsumer->releaseBuffer(slot, mSlots[slot].mFrameNumber, mSlots[slot].mFence);
    if (err == GonkBufferQueue::STALE_BUFFER_SLOT) {
        freeBufferLocked(slot);
    }

    mSlots[slot].mFence = Fence::NO_FENCE;

    return err;
}

bool GonkConsumerBase::stillTracking(int slot,
        const sp<GraphicBuffer> graphicBuffer) {
    if (slot < 0 || slot >= GonkBufferQueue::NUM_BUFFER_SLOTS) {
        return false;
    }
    return (mSlots[slot].mGraphicBuffer != NULL &&
            mSlots[slot].mGraphicBuffer->handle == graphicBuffer->handle);
}

} 
