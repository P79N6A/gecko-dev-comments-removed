
















#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <cutils/log.h>

#include <utils/String8.h>

#include <ui/Rect.h>

#include <EGL/egl.h>

#include <hardware/hardware.h>
#if ANDROID_VERSION == 17
#include <gui/SurfaceTextureClient.h>
#endif
#include <ui/GraphicBuffer.h>

#include "FramebufferSurface.h"
#include "GraphicBufferAlloc.h"

#ifndef NUM_FRAMEBUFFER_SURFACE_BUFFERS
#define NUM_FRAMEBUFFER_SURFACE_BUFFERS (2)
#endif


namespace android {






FramebufferSurface::FramebufferSurface(int disp,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t format,
                                       const sp<StreamConsumer>& sc)
    : DisplaySurface(sc)
    , mDisplayType(disp)
    , mCurrentBufferSlot(-1)
    , mCurrentBuffer(0)
{
    mName = "FramebufferSurface";

#if ANDROID_VERSION >= 19
    sp<IGraphicBufferConsumer> consumer = mConsumer;
#else
    sp<BufferQueue> consumer = mBufferQueue;
    consumer->setSynchronousMode(true);
#endif
    consumer->setConsumerName(mName);
    consumer->setConsumerUsageBits(GRALLOC_USAGE_HW_FB |
                                   GRALLOC_USAGE_HW_RENDER |
                                   GRALLOC_USAGE_HW_COMPOSER);
    consumer->setDefaultBufferFormat(format);
    consumer->setDefaultBufferSize(width, height);
    consumer->setDefaultMaxBufferCount(NUM_FRAMEBUFFER_SURFACE_BUFFERS);
}

status_t FramebufferSurface::beginFrame(bool ) {
    return NO_ERROR;
}

status_t FramebufferSurface::prepareFrame(CompositionType ) {
    return NO_ERROR;
}

status_t FramebufferSurface::advanceFrame() {
    
    
    
    return NO_ERROR;
}

status_t FramebufferSurface::nextBuffer(sp<GraphicBuffer>& outBuffer, sp<Fence>& outFence) {
    Mutex::Autolock lock(mMutex);

    BufferQueue::BufferItem item;
#if ANDROID_VERSION >= 19
    status_t err = acquireBufferLocked(&item, 0);
#else
    status_t err = acquireBufferLocked(&item);
#endif
    if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
        outBuffer = mCurrentBuffer;
        return NO_ERROR;
    } else if (err != NO_ERROR) {
        ALOGE("error acquiring buffer: %s (%d)", strerror(-err), err);
        return err;
    }

    
    
    
    
    
    
    
    
    if (mCurrentBufferSlot != BufferQueue::INVALID_BUFFER_SLOT &&
        item.mBuf != mCurrentBufferSlot) {
        
#if ANDROID_VERSION >= 19
        err = releaseBufferLocked(mCurrentBufferSlot, mCurrentBuffer,
                EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
#else
        err = releaseBufferLocked(mCurrentBufferSlot, EGL_NO_DISPLAY,
                EGL_NO_SYNC_KHR);
#endif
        if (err != NO_ERROR && err != StreamConsumer::STALE_BUFFER_SLOT) {
            ALOGE("error releasing buffer: %s (%d)", strerror(-err), err);
            return err;
        }
    }
    mCurrentBufferSlot = item.mBuf;
    mCurrentBuffer = mSlots[mCurrentBufferSlot].mGraphicBuffer;
    outFence = item.mFence;
    outBuffer = mCurrentBuffer;
    return NO_ERROR;
}


#if ANDROID_VERSION >= 22
void FramebufferSurface::onFrameAvailable(const ::android::BufferItem &item) {
#else
void FramebufferSurface::onFrameAvailable() {
#endif
    sp<GraphicBuffer> buf;
    sp<Fence> acquireFence;
    status_t err = nextBuffer(buf, acquireFence);
    if (err != NO_ERROR) {
        ALOGE("error latching nnext FramebufferSurface buffer: %s (%d)",
                strerror(-err), err);
        return;
    }
    if (acquireFence.get() && acquireFence->isValid())
        mPrevFBAcquireFence = new Fence(acquireFence->dup());
    else
        mPrevFBAcquireFence = Fence::NO_FENCE;

    lastHandle = buf->handle;
}

void FramebufferSurface::freeBufferLocked(int slotIndex) {
    ConsumerBase::freeBufferLocked(slotIndex);
    if (slotIndex == mCurrentBufferSlot) {
        mCurrentBufferSlot = BufferQueue::INVALID_BUFFER_SLOT;
    }
}

status_t FramebufferSurface::setReleaseFenceFd(int fenceFd) {
    status_t err = NO_ERROR;
    if (fenceFd >= 0) {
        sp<Fence> fence(new Fence(fenceFd));
        if (mCurrentBufferSlot != BufferQueue::INVALID_BUFFER_SLOT) {
#if ANDROID_VERSION >= 19
            status_t err = addReleaseFence(mCurrentBufferSlot, mCurrentBuffer,  fence);
#else
            status_t err = addReleaseFence(mCurrentBufferSlot, fence);
#endif
            ALOGE_IF(err, "setReleaseFenceFd: failed to add the fence: %s (%d)",
                    strerror(-err), err);
        }
    }
    return err;
}

int FramebufferSurface::GetPrevDispAcquireFd() {
    if (mPrevFBAcquireFence.get() && mPrevFBAcquireFence->isValid()) {
        return mPrevFBAcquireFence->dup();
    }
    return -1;
}

void FramebufferSurface::onFrameCommitted() {
  
}

status_t FramebufferSurface::compositionComplete()
{
    
    
    return NO_ERROR;
}

void FramebufferSurface::dump(String8& result) const {
    ConsumerBase::dump(result);
}


}; 

