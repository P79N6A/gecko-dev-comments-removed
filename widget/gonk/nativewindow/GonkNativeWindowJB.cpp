

















#define LOG_TAG "GonkNativeWindow"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Log.h>

#include <gui/BufferItemConsumer.h>

#define BI_LOGV(x, ...) ALOGV("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGD(x, ...) ALOGD("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGI(x, ...) ALOGI("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGW(x, ...) ALOGW("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGE(x, ...) ALOGE("[%s] "x, mName.string(), ##__VA_ARGS__)

namespace android {

GonkNativeWindow::BufferItemConsumer(uint32_t consumerUsage,
        int bufferCount, bool synchronousMode) :
    ConsumerBase(new BufferQueue(true) )
{
    mBufferQueue->setConsumerUsageBits(consumerUsage);
    mBufferQueue->setSynchronousMode(synchronousMode);
    mBufferQueue->setMaxAcquiredBufferCount(bufferCount);
}

GonkNativeWindow::~BufferItemConsumer() {
}

void GonkNativeWindow::setName(const String8& name) {
    Mutex::Autolock _l(mMutex);
    mName = name;
    mBufferQueue->setConsumerName(name);
}

status_t GonkNativeWindow::acquireBuffer(BufferItem *item, bool waitForFence) {
    status_t err;

    if (!item) return BAD_VALUE;

    Mutex::Autolock _l(mMutex);

    err = acquireBufferLocked(item);
    if (err != OK) {
        if (err != NO_BUFFER_AVAILABLE) {
            BI_LOGE("Error acquiring buffer: %s (%d)", strerror(err), err);
        }
        return err;
    }

    if (waitForFence) {
        err = item->mFence->waitForever("GonkNativeWindow::acquireBuffer");
        if (err != OK) {
            BI_LOGE("Failed to wait for fence of acquired buffer: %s (%d)",
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

    err = addReleaseFenceLocked(item.mBuf, releaseFence);

    err = releaseBufferLocked(item.mBuf, EGL_NO_DISPLAY,
            EGL_NO_SYNC_KHR);
    if (err != OK) {
        BI_LOGE("Failed to release buffer: %s (%d)",
                strerror(-err), err);
    }
    return err;
}

status_t GonkNativeWindow::setDefaultBufferSize(uint32_t w, uint32_t h) {
    Mutex::Autolock _l(mMutex);
    return mBufferQueue->setDefaultBufferSize(w, h);
}

status_t GonkNativeWindow::setDefaultBufferFormat(uint32_t defaultFormat) {
    Mutex::Autolock _l(mMutex);
    return mBufferQueue->setDefaultBufferFormat(defaultFormat);
}

} 
