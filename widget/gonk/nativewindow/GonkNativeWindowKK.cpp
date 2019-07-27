

















#define LOG_TAG "GonkNativeWindow"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Log.h>

#include "GonkNativeWindowKK.h"
#include "GrallocImages.h"

using namespace mozilla;
using namespace mozilla::layers;

namespace android {

GonkNativeWindow::GonkNativeWindow(int bufferCount) :
    GonkConsumerBase(new GonkBufferQueue(true), false),
    mNewFrameCallback(nullptr)
{
    mConsumer->setMaxAcquiredBufferCount(bufferCount);
}

GonkNativeWindow::GonkNativeWindow(const sp<GonkBufferQueue>& bq,
        uint32_t consumerUsage, int bufferCount, bool controlledByApp) :
    GonkConsumerBase(bq, controlledByApp)
{
    mConsumer->setConsumerUsageBits(consumerUsage);
    mConsumer->setMaxAcquiredBufferCount(bufferCount);
}

GonkNativeWindow::~GonkNativeWindow() {
}

void GonkNativeWindow::setName(const String8& name) {
    Mutex::Autolock _l(mMutex);
    mName = name;
    mConsumer->setConsumerName(name);
}

status_t GonkNativeWindow::acquireBuffer(BufferItem *item,
        nsecs_t presentWhen, bool waitForFence) {
    status_t err;

    if (!item) return BAD_VALUE;

    Mutex::Autolock _l(mMutex);

    err = acquireBufferLocked(item, presentWhen);
    if (err != OK) {
        if (err != NO_BUFFER_AVAILABLE) {
            ALOGE("Error acquiring buffer: %s (%d)", strerror(err), err);
        }
        return err;
    }

    if (waitForFence) {
        err = item->mFence->waitForever("GonkNativeWindow::acquireBuffer");
        if (err != OK) {
            ALOGE("Failed to wait for fence of acquired buffer: %s (%d)",
                    strerror(-err), err);
            return err;
        }
    }

    item->mGraphicBuffer = mSlots[item->mBuf].mGraphicBuffer;

    return OK;
}

status_t GonkNativeWindow::releaseBuffer(const BufferItem &item,
        const sp<Fence>& releaseFence) {
    status_t err;

    Mutex::Autolock _l(mMutex);

    err = addReleaseFenceLocked(item.mBuf, item.mGraphicBuffer, releaseFence);

    err = releaseBufferLocked(item.mBuf, item.mGraphicBuffer);
    if (err != OK) {
        ALOGE("Failed to release buffer: %s (%d)",
                strerror(-err), err);
    }
    return err;
}

status_t GonkNativeWindow::setDefaultBufferSize(uint32_t w, uint32_t h) {
    Mutex::Autolock _l(mMutex);
    return mConsumer->setDefaultBufferSize(w, h);
}

status_t GonkNativeWindow::setDefaultBufferFormat(uint32_t defaultFormat) {
    Mutex::Autolock _l(mMutex);
    return mConsumer->setDefaultBufferFormat(defaultFormat);
}

already_AddRefed<TextureClient>
GonkNativeWindow::getCurrentBuffer() {
    Mutex::Autolock _l(mMutex);
    BufferItem item;

    
    
    status_t err = acquireBufferLocked(&item, 0); 
    if (err != NO_ERROR) {
        return NULL;
    }

    RefPtr<TextureClient> textureClient =
      mConsumer->getTextureClientFromBuffer(item.mGraphicBuffer.get());
    if (!textureClient) {
        return NULL;
    }
    textureClient->SetRecycleCallback(GonkNativeWindow::RecycleCallback, this);
    return textureClient.forget();
}

 void
GonkNativeWindow::RecycleCallback(TextureClient* client, void* closure) {
  GonkNativeWindow* nativeWindow =
    static_cast<GonkNativeWindow*>(closure);

  MOZ_ASSERT(client && !client->IsDead());
  client->ClearRecycleCallback();
  nativeWindow->returnBuffer(client);
}

void GonkNativeWindow::returnBuffer(TextureClient* client) {
    ALOGD("GonkNativeWindow::returnBuffer");
    Mutex::Autolock lock(mMutex);

    int index =  mConsumer->getSlotFromTextureClientLocked(client);
    if (index < 0) {
    }

    FenceHandle handle = client->GetAndResetReleaseFenceHandle();
    nsRefPtr<FenceHandle::FdObj> fdObj = handle.GetAndResetFdObj();
    sp<Fence> fence = new Fence(fdObj->GetAndResetFd());

    addReleaseFenceLocked(index,
                          mSlots[index].mGraphicBuffer,
                          fence);

    releaseBufferLocked(index, mSlots[index].mGraphicBuffer);
}

already_AddRefed<TextureClient>
GonkNativeWindow::getTextureClientFromBuffer(ANativeWindowBuffer* buffer) {
    Mutex::Autolock lock(mMutex);
    return mConsumer->getTextureClientFromBuffer(buffer);
}

void GonkNativeWindow::setNewFrameCallback(
        GonkNativeWindowNewFrameCallback* callback) {
    ALOGD("setNewFrameCallback");
    Mutex::Autolock lock(mMutex);
    mNewFrameCallback = callback;
}

void GonkNativeWindow::onFrameAvailable() {
    GonkConsumerBase::onFrameAvailable();

    if (mNewFrameCallback) {
      mNewFrameCallback->OnNewFrame();
    }
}

} 
