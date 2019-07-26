
















#define LOG_TAG "GonkNativeWindowClient"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#include <android/native_window.h>

#include <binder/Parcel.h>

#include <utils/Log.h>
#include <utils/Trace.h>

#include <ui/Fence.h>

#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/GLConsumer.h>
#include <gui/Surface.h>

#include <private/gui/ComposerService.h>

namespace android {

GonkNativeWindowClient::GonkNativeWindowClient(
        const sp<IGraphicBufferProducer>& bufferProducer)
    : mGraphicBufferProducer(bufferProducer)
{
    
    ANativeWindow::setSwapInterval  = hook_setSwapInterval;
    ANativeWindow::dequeueBuffer    = hook_dequeueBuffer;
    ANativeWindow::cancelBuffer     = hook_cancelBuffer;
    ANativeWindow::queueBuffer      = hook_queueBuffer;
    ANativeWindow::query            = hook_query;
    ANativeWindow::perform          = hook_perform;

    ANativeWindow::dequeueBuffer_DEPRECATED = hook_dequeueBuffer_DEPRECATED;
    ANativeWindow::cancelBuffer_DEPRECATED  = hook_cancelBuffer_DEPRECATED;
    ANativeWindow::lockBuffer_DEPRECATED    = hook_lockBuffer_DEPRECATED;
    ANativeWindow::queueBuffer_DEPRECATED   = hook_queueBuffer_DEPRECATED;

    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;

    mReqWidth = 0;
    mReqHeight = 0;
    mReqFormat = 0;
    mReqUsage = 0;
    mTimestamp = NATIVE_WINDOW_TIMESTAMP_AUTO;
    mCrop.clear();
    mScalingMode = NATIVE_WINDOW_SCALING_MODE_FREEZE;
    mTransform = 0;
    mDefaultWidth = 0;
    mDefaultHeight = 0;
    mUserWidth = 0;
    mUserHeight = 0;
    mTransformHint = 0;
    mConsumerRunningBehind = false;
    mConnectedToCpu = false;
}

GonkNativeWindowClient::~GonkNativeWindowClient() {
    if (mConnectedToCpu) {
        GonkNativeWindowClient::disconnect(NATIVE_WINDOW_API_CPU);
    }
}

sp<IGraphicBufferProducer> GonkNativeWindowClient::getIGraphicBufferProducer() const {
    return mGraphicBufferProducer;
}

int GonkNativeWindowClient::hook_setSwapInterval(ANativeWindow* window, int interval) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->setSwapInterval(interval);
}

int GonkNativeWindowClient::hook_dequeueBuffer(ANativeWindow* window,
        ANativeWindowBuffer** buffer, int* fenceFd) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->dequeueBuffer(buffer, fenceFd);
}

int GonkNativeWindowClient::hook_cancelBuffer(ANativeWindow* window,
        ANativeWindowBuffer* buffer, int fenceFd) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->cancelBuffer(buffer, fenceFd);
}

int GonkNativeWindowClient::hook_queueBuffer(ANativeWindow* window,
        ANativeWindowBuffer* buffer, int fenceFd) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->queueBuffer(buffer, fenceFd);
}

int GonkNativeWindowClient::hook_dequeueBuffer_DEPRECATED(ANativeWindow* window,
        ANativeWindowBuffer** buffer) {
    GonkNativeWindowClient* c = getSelf(window);
    ANativeWindowBuffer* buf;
    int fenceFd = -1;
    int result = c->dequeueBuffer(&buf, &fenceFd);
    sp<Fence> fence(new Fence(fenceFd));
    int waitResult = fence->waitForever("dequeueBuffer_DEPRECATED");
    if (waitResult != OK) {
        ALOGE("dequeueBuffer_DEPRECATED: Fence::wait returned an error: %d",
                waitResult);
        c->cancelBuffer(buf, -1);
        return waitResult;
    }
    *buffer = buf;
    return result;
}

int GonkNativeWindowClient::hook_cancelBuffer_DEPRECATED(ANativeWindow* window,
        ANativeWindowBuffer* buffer) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->cancelBuffer(buffer, -1);
}

int GonkNativeWindowClient::hook_lockBuffer_DEPRECATED(ANativeWindow* window,
        ANativeWindowBuffer* buffer) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->lockBuffer_DEPRECATED(buffer);
}

int GonkNativeWindowClient::hook_queueBuffer_DEPRECATED(ANativeWindow* window,
        ANativeWindowBuffer* buffer) {
    GonkNativeWindowClient* c = getSelf(window);
    return c->queueBuffer(buffer, -1);
}

int GonkNativeWindowClient::hook_query(const ANativeWindow* window,
                                int what, int* value) {
    const GonkNativeWindowClient* c = getSelf(window);
    return c->query(what, value);
}

int GonkNativeWindowClient::hook_perform(ANativeWindow* window, int operation, ...) {
    va_list args;
    va_start(args, operation);
    GonkNativeWindowClient* c = getSelf(window);
    return c->perform(operation, args);
}

int GonkNativeWindowClient::setSwapInterval(int interval) {
    ATRACE_CALL();
    
    
    
    

    if (interval < minSwapInterval)
        interval = minSwapInterval;

    if (interval > maxSwapInterval)
        interval = maxSwapInterval;

    status_t res = mGraphicBufferProducer->setSynchronousMode(interval ? true : false);

    return res;
}

int GonkNativeWindowClient::dequeueBuffer(android_native_buffer_t** buffer,
        int* fenceFd) {
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::dequeueBuffer");
    Mutex::Autolock lock(mMutex);
    int buf = -1;
    int reqW = mReqWidth ? mReqWidth : mUserWidth;
    int reqH = mReqHeight ? mReqHeight : mUserHeight;
    sp<Fence> fence;
    status_t result = mGraphicBufferProducer->dequeueBuffer(&buf, &fence,
            reqW, reqH, mReqFormat, mReqUsage);
    if (result < 0) {
        ALOGV("dequeueBuffer: IGraphicBufferProducer::dequeueBuffer(%d, %d, %d, %d)"
             "failed: %d", mReqWidth, mReqHeight, mReqFormat, mReqUsage,
             result);
        return result;
    }
    sp<GraphicBuffer>& gbuf(mSlots[buf].buffer);
    if (result & IGraphicBufferProducer::RELEASE_ALL_BUFFERS) {
        freeAllBuffers();
    }

    if ((result & IGraphicBufferProducer::BUFFER_NEEDS_REALLOCATION) || gbuf == 0) {
        result = mGraphicBufferProducer->requestBuffer(buf, &gbuf);
        if (result != NO_ERROR) {
            ALOGE("dequeueBuffer: IGraphicBufferProducer::requestBuffer failed: %d",
                    result);
            return result;
        }
    }

    if (fence->isValid()) {
        *fenceFd = fence->dup();
        if (*fenceFd == -1) {
            ALOGE("dequeueBuffer: error duping fence: %d", errno);
            
            
            
        }
    } else {
        *fenceFd = -1;
    }

    *buffer = gbuf.get();
    return OK;
}

int GonkNativeWindowClient::cancelBuffer(android_native_buffer_t* buffer,
        int fenceFd) {
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::cancelBuffer");
    Mutex::Autolock lock(mMutex);
    int i = getSlotFromBufferLocked(buffer);
    if (i < 0) {
        return i;
    }
    sp<Fence> fence(fenceFd >= 0 ? new Fence(fenceFd) : Fence::NO_FENCE);
    mGraphicBufferProducer->cancelBuffer(i, fence);
    return OK;
}

int GonkNativeWindowClient::getSlotFromBufferLocked(
        android_native_buffer_t* buffer) const {
    bool dumpedState = false;
    for (int i = 0; i < NUM_BUFFER_SLOTS; i++) {
        if (mSlots[i].buffer != NULL &&
                mSlots[i].buffer->handle == buffer->handle) {
            return i;
        }
    }
    ALOGE("getSlotFromBufferLocked: unknown buffer: %p", buffer->handle);
    return BAD_VALUE;
}

int GonkNativeWindowClient::lockBuffer_DEPRECATED(android_native_buffer_t* buffer) {
    ALOGV("GonkNativeWindowClient::lockBuffer");
    Mutex::Autolock lock(mMutex);
    return OK;
}

int GonkNativeWindowClient::queueBuffer(android_native_buffer_t* buffer, int fenceFd) {
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::queueBuffer");
    Mutex::Autolock lock(mMutex);
    int64_t timestamp;
    if (mTimestamp == NATIVE_WINDOW_TIMESTAMP_AUTO) {
        timestamp = systemTime(SYSTEM_TIME_MONOTONIC);
        ALOGV("GonkNativeWindowClient::queueBuffer making up timestamp: %.2f ms",
             timestamp / 1000000.f);
    } else {
        timestamp = mTimestamp;
    }
    int i = getSlotFromBufferLocked(buffer);
    if (i < 0) {
        return i;
    }


    
    Rect crop;
    mCrop.intersect(Rect(buffer->width, buffer->height), &crop);

    sp<Fence> fence(fenceFd >= 0 ? new Fence(fenceFd) : Fence::NO_FENCE);
    IGraphicBufferProducer::QueueBufferOutput output;
    IGraphicBufferProducer::QueueBufferInput input(timestamp, crop, mScalingMode,
            mTransform, fence);
    status_t err = mGraphicBufferProducer->queueBuffer(i, input, &output);
    if (err != OK)  {
        ALOGE("queueBuffer: error queuing buffer to SurfaceTexture, %d", err);
    }
    uint32_t numPendingBuffers = 0;
    output.deflate(&mDefaultWidth, &mDefaultHeight, &mTransformHint,
            &numPendingBuffers);

    mConsumerRunningBehind = (numPendingBuffers >= 2);

    return err;
}

int GonkNativeWindowClient::query(int what, int* value) const {
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::query");
    { 
        Mutex::Autolock lock(mMutex);
        switch (what) {
            case NATIVE_WINDOW_FORMAT:
                if (mReqFormat) {
                    *value = mReqFormat;
                    return NO_ERROR;
                }
                break;
            case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER: {
                sp<ISurfaceComposer> composer(
                        ComposerService::getComposerService());
                if (composer->authenticateSurfaceTexture(mGraphicBufferProducer)) {
                    *value = 1;
                } else {
                    *value = 0;
                }
                return NO_ERROR;
            }
            case NATIVE_WINDOW_CONCRETE_TYPE:
                *value = NATIVE_WINDOW_SURFACE;
                return NO_ERROR;
            case NATIVE_WINDOW_DEFAULT_WIDTH:
                *value = mUserWidth ? mUserWidth : mDefaultWidth;
                return NO_ERROR;
            case NATIVE_WINDOW_DEFAULT_HEIGHT:
                *value = mUserHeight ? mUserHeight : mDefaultHeight;
                return NO_ERROR;
            case NATIVE_WINDOW_TRANSFORM_HINT:
                *value = mTransformHint;
                return NO_ERROR;
            case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND: {
                status_t err = NO_ERROR;
                if (!mConsumerRunningBehind) {
                    *value = 0;
                } else {
                    err = mGraphicBufferProducer->query(what, value);
                    if (err == NO_ERROR) {
                        mConsumerRunningBehind = *value;
                    }
                }
                return err;
            }
        }
    }
    return mGraphicBufferProducer->query(what, value);
}

int GonkNativeWindowClient::perform(int operation, va_list args)
{
    int res = NO_ERROR;
    switch (operation) {
    case NATIVE_WINDOW_CONNECT:
        
        break;
    case NATIVE_WINDOW_DISCONNECT:
        
        break;
    case NATIVE_WINDOW_SET_USAGE:
        res = dispatchSetUsage(args);
        break;
    case NATIVE_WINDOW_SET_CROP:
        res = dispatchSetCrop(args);
        break;
    case NATIVE_WINDOW_SET_BUFFER_COUNT:
        res = dispatchSetBufferCount(args);
        break;
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        res = dispatchSetBuffersGeometry(args);
        break;
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
        res = dispatchSetBuffersTransform(args);
        break;
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
        res = dispatchSetBuffersTimestamp(args);
        break;
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
        res = dispatchSetBuffersDimensions(args);
        break;
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS:
        res = dispatchSetBuffersUserDimensions(args);
        break;
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
        res = dispatchSetBuffersFormat(args);
        break;
    case NATIVE_WINDOW_LOCK:
        res = dispatchLock(args);
        break;
    case NATIVE_WINDOW_UNLOCK_AND_POST:
        res = dispatchUnlockAndPost(args);
        break;
    case NATIVE_WINDOW_SET_SCALING_MODE:
        res = dispatchSetScalingMode(args);
        break;
    case NATIVE_WINDOW_API_CONNECT:
        res = dispatchConnect(args);
        break;
    case NATIVE_WINDOW_API_DISCONNECT:
        res = dispatchDisconnect(args);
        break;
    default:
        res = NAME_NOT_FOUND;
        break;
    }
    return res;
}

int GonkNativeWindowClient::dispatchConnect(va_list args) {
    int api = va_arg(args, int);
    return connect(api);
}

int GonkNativeWindowClient::dispatchDisconnect(va_list args) {
    int api = va_arg(args, int);
    return disconnect(api);
}

int GonkNativeWindowClient::dispatchSetUsage(va_list args) {
    int usage = va_arg(args, int);
    return setUsage(usage);
}

int GonkNativeWindowClient::dispatchSetCrop(va_list args) {
    android_native_rect_t const* rect = va_arg(args, android_native_rect_t*);
    return setCrop(reinterpret_cast<Rect const*>(rect));
}

int GonkNativeWindowClient::dispatchSetBufferCount(va_list args) {
    size_t bufferCount = va_arg(args, size_t);
    return setBufferCount(bufferCount);
}

int GonkNativeWindowClient::dispatchSetBuffersGeometry(va_list args) {
    int w = va_arg(args, int);
    int h = va_arg(args, int);
    int f = va_arg(args, int);
    int err = setBuffersDimensions(w, h);
    if (err != 0) {
        return err;
    }
    return setBuffersFormat(f);
}

int GonkNativeWindowClient::dispatchSetBuffersDimensions(va_list args) {
    int w = va_arg(args, int);
    int h = va_arg(args, int);
    return setBuffersDimensions(w, h);
}

int GonkNativeWindowClient::dispatchSetBuffersUserDimensions(va_list args) {
    int w = va_arg(args, int);
    int h = va_arg(args, int);
    return setBuffersUserDimensions(w, h);
}

int GonkNativeWindowClient::dispatchSetBuffersFormat(va_list args) {
    int f = va_arg(args, int);
    return setBuffersFormat(f);
}

int GonkNativeWindowClient::dispatchSetScalingMode(va_list args) {
    int m = va_arg(args, int);
    return setScalingMode(m);
}

int GonkNativeWindowClient::dispatchSetBuffersTransform(va_list args) {
    int transform = va_arg(args, int);
    return setBuffersTransform(transform);
}

int GonkNativeWindowClient::dispatchSetBuffersTimestamp(va_list args) {
    int64_t timestamp = va_arg(args, int64_t);
    return setBuffersTimestamp(timestamp);
}

int GonkNativeWindowClient::dispatchLock(va_list args) {
    ANativeWindow_Buffer* outBuffer = va_arg(args, ANativeWindow_Buffer*);
    ARect* inOutDirtyBounds = va_arg(args, ARect*);
    return lock(outBuffer, inOutDirtyBounds);
}

int GonkNativeWindowClient::dispatchUnlockAndPost(va_list args) {
    return unlockAndPost();
}


int GonkNativeWindowClient::connect(int api) {
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::connect");
    Mutex::Autolock lock(mMutex);
    IGraphicBufferProducer::QueueBufferOutput output;
    int err = mGraphicBufferProducer->connect(api, &output);
    if (err == NO_ERROR) {
        uint32_t numPendingBuffers = 0;
        output.deflate(&mDefaultWidth, &mDefaultHeight, &mTransformHint,
                &numPendingBuffers);
        mConsumerRunningBehind = (numPendingBuffers >= 2);
    }
    if (!err && api == NATIVE_WINDOW_API_CPU) {
        mConnectedToCpu = true;
    }
    return err;
}

int GonkNativeWindowClient::disconnect(int api) {
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::disconnect");
    Mutex::Autolock lock(mMutex);
    freeAllBuffers();
    int err = mGraphicBufferProducer->disconnect(api);
    if (!err) {
        mReqFormat = 0;
        mReqWidth = 0;
        mReqHeight = 0;
        mReqUsage = 0;
        mCrop.clear();
        mScalingMode = NATIVE_WINDOW_SCALING_MODE_FREEZE;
        mTransform = 0;
        if (api == NATIVE_WINDOW_API_CPU) {
            mConnectedToCpu = false;
        }
    }
    return err;
}

int GonkNativeWindowClient::setUsage(uint32_t reqUsage)
{
    ALOGV("GonkNativeWindowClient::setUsage");
    Mutex::Autolock lock(mMutex);
    mReqUsage = reqUsage;
    return OK;
}

int GonkNativeWindowClient::setCrop(Rect const* rect)
{
    ATRACE_CALL();

    Rect realRect;
    if (rect == NULL || rect->isEmpty()) {
        realRect.clear();
    } else {
        realRect = *rect;
    }

    ALOGV("GonkNativeWindowClient::setCrop rect=[%d %d %d %d]",
            realRect.left, realRect.top, realRect.right, realRect.bottom);

    Mutex::Autolock lock(mMutex);
    mCrop = realRect;
    return NO_ERROR;
}

int GonkNativeWindowClient::setBufferCount(int bufferCount)
{
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::setBufferCount");
    Mutex::Autolock lock(mMutex);

    status_t err = mGraphicBufferProducer->setBufferCount(bufferCount);
    ALOGE_IF(err, "IGraphicBufferProducer::setBufferCount(%d) returned %s",
            bufferCount, strerror(-err));

    if (err == NO_ERROR) {
        freeAllBuffers();
    }

    return err;
}

int GonkNativeWindowClient::setBuffersDimensions(int w, int h)
{
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::setBuffersDimensions");

    if (w<0 || h<0)
        return BAD_VALUE;

    if ((w && !h) || (!w && h))
        return BAD_VALUE;

    Mutex::Autolock lock(mMutex);
    mReqWidth = w;
    mReqHeight = h;
    return NO_ERROR;
}

int GonkNativeWindowClient::setBuffersUserDimensions(int w, int h)
{
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::setBuffersUserDimensions");

    if (w<0 || h<0)
        return BAD_VALUE;

    if ((w && !h) || (!w && h))
        return BAD_VALUE;

    Mutex::Autolock lock(mMutex);
    mUserWidth = w;
    mUserHeight = h;
    return NO_ERROR;
}

int GonkNativeWindowClient::setBuffersFormat(int format)
{
    ALOGV("GonkNativeWindowClient::setBuffersFormat");

    if (format<0)
        return BAD_VALUE;

    Mutex::Autolock lock(mMutex);
    mReqFormat = format;
    return NO_ERROR;
}

int GonkNativeWindowClient::setScalingMode(int mode)
{
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::setScalingMode(%d)", mode);

    switch (mode) {
        case NATIVE_WINDOW_SCALING_MODE_FREEZE:
        case NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW:
        case NATIVE_WINDOW_SCALING_MODE_SCALE_CROP:
            break;
        default:
            ALOGE("unknown scaling mode: %d", mode);
            return BAD_VALUE;
    }

    Mutex::Autolock lock(mMutex);
    mScalingMode = mode;
    return NO_ERROR;
}

int GonkNativeWindowClient::setBuffersTransform(int transform)
{
    ATRACE_CALL();
    ALOGV("GonkNativeWindowClient::setBuffersTransform");
    Mutex::Autolock lock(mMutex);
    mTransform = transform;
    return NO_ERROR;
}

int GonkNativeWindowClient::setBuffersTimestamp(int64_t timestamp)
{
    ALOGV("GonkNativeWindowClient::setBuffersTimestamp");
    Mutex::Autolock lock(mMutex);
    mTimestamp = timestamp;
    return NO_ERROR;
}

void GonkNativeWindowClient::freeAllBuffers() {
    for (int i = 0; i < NUM_BUFFER_SLOTS; i++) {
        mSlots[i].buffer = 0;
    }
}




static status_t copyBlt(
        const sp<GraphicBuffer>& dst,
        const sp<GraphicBuffer>& src,
        const Region& reg)
{
    
    
    status_t err;
    uint8_t const * src_bits = NULL;
    err = src->lock(GRALLOC_USAGE_SW_READ_OFTEN, reg.bounds(), (void**)&src_bits);
    ALOGE_IF(err, "error locking src buffer %s", strerror(-err));

    uint8_t* dst_bits = NULL;
    err = dst->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, reg.bounds(), (void**)&dst_bits);
    ALOGE_IF(err, "error locking dst buffer %s", strerror(-err));

    Region::const_iterator head(reg.begin());
    Region::const_iterator tail(reg.end());
    if (head != tail && src_bits && dst_bits) {
        const size_t bpp = bytesPerPixel(src->format);
        const size_t dbpr = dst->stride * bpp;
        const size_t sbpr = src->stride * bpp;

        while (head != tail) {
            const Rect& r(*head++);
            ssize_t h = r.height();
            if (h <= 0) continue;
            size_t size = r.width() * bpp;
            uint8_t const * s = src_bits + (r.left + src->stride * r.top) * bpp;
            uint8_t       * d = dst_bits + (r.left + dst->stride * r.top) * bpp;
            if (dbpr==sbpr && size==sbpr) {
                size *= h;
                h = 1;
            }
            do {
                memcpy(d, s, size);
                d += dbpr;
                s += sbpr;
            } while (--h > 0);
        }
    }

    if (src_bits)
        src->unlock();

    if (dst_bits)
        dst->unlock();

    return err;
}



status_t GonkNativeWindowClient::lock(
        ANativeWindow_Buffer* outBuffer, ARect* inOutDirtyBounds)
{
    if (mLockedBuffer != 0) {
        ALOGE("GonkNativeWindowClient::lock failed, already locked");
        return INVALID_OPERATION;
    }

    if (!mConnectedToCpu) {
        int err = GonkNativeWindowClient::connect(NATIVE_WINDOW_API_CPU);
        if (err) {
            return err;
        }
        
        setUsage(GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);
    }

    ANativeWindowBuffer* out;
    int fenceFd = -1;
    status_t err = dequeueBuffer(&out, &fenceFd);
    ALOGE_IF(err, "dequeueBuffer failed (%s)", strerror(-err));
    if (err == NO_ERROR) {
        sp<GraphicBuffer> backBuffer(GraphicBuffer::getSelf(out));
        sp<Fence> fence(new Fence(fenceFd));

        err = fence->waitForever("GonkNativeWindowClient::lock");
        if (err != OK) {
            ALOGE("Fence::wait failed (%s)", strerror(-err));
            cancelBuffer(out, fenceFd);
            return err;
        }

        const Rect bounds(backBuffer->width, backBuffer->height);

        Region newDirtyRegion;
        if (inOutDirtyBounds) {
            newDirtyRegion.set(static_cast<Rect const&>(*inOutDirtyBounds));
            newDirtyRegion.andSelf(bounds);
        } else {
            newDirtyRegion.set(bounds);
        }

        
        const sp<GraphicBuffer>& frontBuffer(mPostedBuffer);
        const bool canCopyBack = (frontBuffer != 0 &&
                backBuffer->width  == frontBuffer->width &&
                backBuffer->height == frontBuffer->height &&
                backBuffer->format == frontBuffer->format);

        if (canCopyBack) {
            
            const Region copyback(mDirtyRegion.subtract(newDirtyRegion));
            if (!copyback.isEmpty())
                copyBlt(backBuffer, frontBuffer, copyback);
        } else {
            
            
            newDirtyRegion.set(bounds);
            mDirtyRegion.clear();
            Mutex::Autolock lock(mMutex);
            for (size_t i=0 ; i<NUM_BUFFER_SLOTS ; i++) {
                mSlots[i].dirtyRegion.clear();
            }
        }


        { 
            Mutex::Autolock lock(mMutex);
            int backBufferSlot(getSlotFromBufferLocked(backBuffer.get()));
            if (backBufferSlot >= 0) {
                Region& dirtyRegion(mSlots[backBufferSlot].dirtyRegion);
                mDirtyRegion.subtract(dirtyRegion);
                dirtyRegion = newDirtyRegion;
            }
        }

        mDirtyRegion.orSelf(newDirtyRegion);
        if (inOutDirtyBounds) {
            *inOutDirtyBounds = newDirtyRegion.getBounds();
        }

        void* vaddr;
        status_t res = backBuffer->lock(
                GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN,
                newDirtyRegion.bounds(), &vaddr);

        ALOGW_IF(res, "failed locking buffer (handle = %p)",
                backBuffer->handle);

        if (res != 0) {
            err = INVALID_OPERATION;
        } else {
            mLockedBuffer = backBuffer;
            outBuffer->width  = backBuffer->width;
            outBuffer->height = backBuffer->height;
            outBuffer->stride = backBuffer->stride;
            outBuffer->format = backBuffer->format;
            outBuffer->bits   = vaddr;
        }
    }
    return err;
}

status_t GonkNativeWindowClient::unlockAndPost()
{
    if (mLockedBuffer == 0) {
        ALOGE("GonkNativeWindowClient::unlockAndPost failed, no locked buffer");
        return INVALID_OPERATION;
    }

    status_t err = mLockedBuffer->unlock();
    ALOGE_IF(err, "failed unlocking buffer (%p)", mLockedBuffer->handle);

    err = queueBuffer(mLockedBuffer.get(), -1);
    ALOGE_IF(err, "queueBuffer (handle=%p) failed (%s)",
            mLockedBuffer->handle, strerror(-err));

    mPostedBuffer = mLockedBuffer;
    mLockedBuffer = 0;
    return err;
}

}; 
