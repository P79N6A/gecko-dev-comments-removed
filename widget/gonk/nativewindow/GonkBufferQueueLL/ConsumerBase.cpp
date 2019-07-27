















#include <inttypes.h>

#define LOG_TAG "ConsumerBase"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <hardware/hardware.h>

#include <gui/IGraphicBufferAlloc.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ConsumerBase.h>

#include <private/gui/ComposerService.h>

#include <utils/Log.h>
#include <utils/String8.h>
#include <utils/Trace.h>


#define CB_LOGV(x, ...) ALOGV("[%s] "x, mName.string(), ##__VA_ARGS__)
#define CB_LOGD(x, ...) ALOGD("[%s] "x, mName.string(), ##__VA_ARGS__)
#define CB_LOGI(x, ...) ALOGI("[%s] "x, mName.string(), ##__VA_ARGS__)
#define CB_LOGW(x, ...) ALOGW("[%s] "x, mName.string(), ##__VA_ARGS__)
#define CB_LOGE(x, ...) ALOGE("[%s] "x, mName.string(), ##__VA_ARGS__)

namespace android {


static int32_t createProcessUniqueId() {
    static volatile int32_t globalCounter = 0;
    return android_atomic_inc(&globalCounter);
}

ConsumerBase::ConsumerBase(const sp<IGraphicBufferConsumer>& bufferQueue, bool controlledByApp) :
        mAbandoned(false),
        mConsumer(bufferQueue) {
    
    mName = String8::format("unnamed-%d-%d", getpid(), createProcessUniqueId());

    
    
    
    
    wp<ConsumerListener> listener = static_cast<ConsumerListener*>(this);
    sp<IConsumerListener> proxy = new BufferQueue::ProxyConsumerListener(listener);

    status_t err = mConsumer->consumerConnect(proxy, controlledByApp);
    if (err != NO_ERROR) {
        CB_LOGE("ConsumerBase: error connecting to BufferQueue: %s (%d)",
                strerror(-err), err);
    } else {
        mConsumer->setConsumerName(mName);
    }
}

ConsumerBase::~ConsumerBase() {
    CB_LOGV("~ConsumerBase");
    Mutex::Autolock lock(mMutex);

    
    
    
    
    LOG_ALWAYS_FATAL_IF(!mAbandoned, "[%s] ~ConsumerBase was called, but the "
        "consumer is not abandoned!", mName.string());
}

void ConsumerBase::onLastStrongRef(const void* id __attribute__((unused))) {
    abandon();
}

void ConsumerBase::freeBufferLocked(int slotIndex) {
    CB_LOGV("freeBufferLocked: slotIndex=%d", slotIndex);
    mSlots[slotIndex].mGraphicBuffer = 0;
    mSlots[slotIndex].mFence = Fence::NO_FENCE;
    mSlots[slotIndex].mFrameNumber = 0;
}

void ConsumerBase::onFrameAvailable() {
    CB_LOGV("onFrameAvailable");

    sp<FrameAvailableListener> listener;
    { 
        Mutex::Autolock lock(mMutex);
        listener = mFrameAvailableListener.promote();
    }

    if (listener != NULL) {
        CB_LOGV("actually calling onFrameAvailable");
        listener->onFrameAvailable();
    }
}

void ConsumerBase::onBuffersReleased() {
    Mutex::Autolock lock(mMutex);

    CB_LOGV("onBuffersReleased");

    if (mAbandoned) {
        
        return;
    }

    uint64_t mask = 0;
    mConsumer->getReleasedBuffers(&mask);
    for (int i = 0; i < BufferQueue::NUM_BUFFER_SLOTS; i++) {
        if (mask & (1ULL << i)) {
            freeBufferLocked(i);
        }
    }
}

void ConsumerBase::onSidebandStreamChanged() {
}

void ConsumerBase::abandon() {
    CB_LOGV("abandon");
    Mutex::Autolock lock(mMutex);

    if (!mAbandoned) {
        abandonLocked();
        mAbandoned = true;
    }
}

void ConsumerBase::abandonLocked() {
	CB_LOGV("abandonLocked");
    for (int i =0; i < BufferQueue::NUM_BUFFER_SLOTS; i++) {
        freeBufferLocked(i);
    }
    
    mConsumer->consumerDisconnect();
    mConsumer.clear();
}

void ConsumerBase::setFrameAvailableListener(
        const wp<FrameAvailableListener>& listener) {
    CB_LOGV("setFrameAvailableListener");
    Mutex::Autolock lock(mMutex);
    mFrameAvailableListener = listener;
}

void ConsumerBase::dump(String8& result) const {
    dump(result, "");
}

void ConsumerBase::dump(String8& result, const char* prefix) const {
    Mutex::Autolock _l(mMutex);
    dumpLocked(result, prefix);
}

void ConsumerBase::dumpLocked(String8& result, const char* prefix) const {
    result.appendFormat("%smAbandoned=%d\n", prefix, int(mAbandoned));

    if (!mAbandoned) {
        mConsumer->dump(result, prefix);
    }
}

status_t ConsumerBase::acquireBufferLocked(BufferQueue::BufferItem *item,
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

    CB_LOGV("acquireBufferLocked: -> slot=%d/%" PRIu64,
            item->mBuf, item->mFrameNumber);

    return OK;
}

status_t ConsumerBase::addReleaseFence(int slot,
        const sp<GraphicBuffer> graphicBuffer, const sp<Fence>& fence) {
    Mutex::Autolock lock(mMutex);
    return addReleaseFenceLocked(slot, graphicBuffer, fence);
}

status_t ConsumerBase::addReleaseFenceLocked(int slot,
        const sp<GraphicBuffer> graphicBuffer, const sp<Fence>& fence) {
    CB_LOGV("addReleaseFenceLocked: slot=%d", slot);

    
    
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
            CB_LOGE("failed to merge release fences");
            
            
            mSlots[slot].mFence = fence;
            return BAD_VALUE;
        }
        mSlots[slot].mFence = mergedFence;
    }

    return OK;
}

status_t ConsumerBase::releaseBufferLocked(
        int slot, const sp<GraphicBuffer> graphicBuffer,
        EGLDisplay display, EGLSyncKHR eglFence) {
    
    
    
    if (!stillTracking(slot, graphicBuffer)) {
        return OK;
    }

    CB_LOGV("releaseBufferLocked: slot=%d/%" PRIu64,
            slot, mSlots[slot].mFrameNumber);
    status_t err = mConsumer->releaseBuffer(slot, mSlots[slot].mFrameNumber,
            display, eglFence, mSlots[slot].mFence);
    if (err == IGraphicBufferConsumer::STALE_BUFFER_SLOT) {
        freeBufferLocked(slot);
    }

    mSlots[slot].mFence = Fence::NO_FENCE;

    return err;
}

bool ConsumerBase::stillTracking(int slot,
        const sp<GraphicBuffer> graphicBuffer) {
    if (slot < 0 || slot >= BufferQueue::NUM_BUFFER_SLOTS) {
        return false;
    }
    return (mSlots[slot].mGraphicBuffer != NULL &&
            mSlots[slot].mGraphicBuffer->handle == graphicBuffer->handle);
}

} 
