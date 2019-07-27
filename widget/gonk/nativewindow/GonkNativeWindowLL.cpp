
















#define LOG_TAG "BufferItemConsumer"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Log.h>

#include <gui/BufferItemConsumer.h>

#define BI_LOGV(x, ...) ALOGV("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGD(x, ...) ALOGD("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGI(x, ...) ALOGI("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGW(x, ...) ALOGW("[%s] "x, mName.string(), ##__VA_ARGS__)
#define BI_LOGE(x, ...) ALOGE("[%s] "x, mName.string(), ##__VA_ARGS__)

namespace android {

BufferItemConsumer::BufferItemConsumer(
        const sp<IGraphicBufferConsumer>& consumer, uint32_t consumerUsage,
        int bufferCount, bool controlledByApp) :
    ConsumerBase(consumer, controlledByApp)
{
    status_t err = mConsumer->setConsumerUsageBits(consumerUsage);
    LOG_ALWAYS_FATAL_IF(err != OK,
            "Failed to set consumer usage bits to %#x", consumerUsage);
    if (bufferCount != DEFAULT_MAX_BUFFERS) {
        err = mConsumer->setMaxAcquiredBufferCount(bufferCount);
        LOG_ALWAYS_FATAL_IF(err != OK,
                "Failed to set max acquired buffer count to %d", bufferCount);
    }
}

BufferItemConsumer::~BufferItemConsumer() {
}

void BufferItemConsumer::setName(const String8& name) {
    Mutex::Autolock _l(mMutex);
    mName = name;
    mConsumer->setConsumerName(name);
}

status_t BufferItemConsumer::acquireBuffer(BufferItem *item,
        nsecs_t presentWhen, bool waitForFence) {
    status_t err;

    if (!item) return BAD_VALUE;

    Mutex::Autolock _l(mMutex);

    err = acquireBufferLocked(item, presentWhen);
    if (err != OK) {
        if (err != NO_BUFFER_AVAILABLE) {
            BI_LOGE("Error acquiring buffer: %s (%d)", strerror(err), err);
        }
        return err;
    }

    if (waitForFence) {
        err = item->mFence->waitForever("BufferItemConsumer::acquireBuffer");
        if (err != OK) {
            BI_LOGE("Failed to wait for fence of acquired buffer: %s (%d)",
                    strerror(-err), err);
            return err;
        }
    }

    item->mGraphicBuffer = mSlots[item->mBuf].mGraphicBuffer;

    return OK;
}

status_t BufferItemConsumer::releaseBuffer(const BufferItem &item,
        const sp<Fence>& releaseFence) {
    status_t err;

    Mutex::Autolock _l(mMutex);

    err = addReleaseFenceLocked(item.mBuf, item.mGraphicBuffer, releaseFence);

    err = releaseBufferLocked(item.mBuf, item.mGraphicBuffer, EGL_NO_DISPLAY,
            EGL_NO_SYNC_KHR);
    if (err != OK) {
        BI_LOGE("Failed to release buffer: %s (%d)",
                strerror(-err), err);
    }
    return err;
}

status_t BufferItemConsumer::setDefaultBufferSize(uint32_t w, uint32_t h) {
    Mutex::Autolock _l(mMutex);
    return mConsumer->setDefaultBufferSize(w, h);
}

status_t BufferItemConsumer::setDefaultBufferFormat(uint32_t defaultFormat) {
    Mutex::Autolock _l(mMutex);
    return mConsumer->setDefaultBufferFormat(defaultFormat);
}

} 
