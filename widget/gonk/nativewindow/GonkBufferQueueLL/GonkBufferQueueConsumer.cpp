
















#include <inttypes.h>

#define LOG_TAG "GonkBufferQueueConsumer"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#include "GonkBufferItem.h"
#include "GonkBufferQueueConsumer.h"
#include "GonkBufferQueueCore.h"
#include <gui/IConsumerListener.h>
#include <gui/IProducerListener.h>

namespace android {

GonkBufferQueueConsumer::GonkBufferQueueConsumer(const sp<GonkBufferQueueCore>& core) :
    mCore(core),
    mSlots(core->mSlots),
    mConsumerName() {}

GonkBufferQueueConsumer::~GonkBufferQueueConsumer() {}

status_t GonkBufferQueueConsumer::acquireBuffer(BufferItem* outBuffer,
        nsecs_t expectedPresent) {
    ATRACE_CALL();
    Mutex::Autolock lock(mCore->mMutex);

    
    
    
    
    int numAcquiredBuffers = 0;
    for (int s = 0; s < GonkBufferQueueDefs::NUM_BUFFER_SLOTS; ++s) {
        if (mSlots[s].mBufferState == GonkBufferSlot::ACQUIRED) {
            ++numAcquiredBuffers;
        }
    }
    if (numAcquiredBuffers >= mCore->mMaxAcquiredBufferCount + 1) {
        ALOGE("acquireBuffer: max acquired buffer count reached: %d (max %d)",
                numAcquiredBuffers, mCore->mMaxAcquiredBufferCount);
        return INVALID_OPERATION;
    }

    
    
    
    if (mCore->mQueue.empty()) {
        return NO_BUFFER_AVAILABLE;
    }

    GonkBufferQueueCore::Fifo::iterator front(mCore->mQueue.begin());

    
    
    
    if (expectedPresent != 0) {
        const int MAX_REASONABLE_NSEC = 1000000000ULL; 

        
        
        
        
        
        
        
        
        
        
        
        
        

        
        
        
        
        while (mCore->mQueue.size() > 1 && !mCore->mQueue[0].mIsAutoTimestamp) {
            
            
            
            
            
            
            
            
            
            const BufferItem& bufferItem(mCore->mQueue[1]);
            nsecs_t desiredPresent = bufferItem.mTimestamp;
            if (desiredPresent < expectedPresent - MAX_REASONABLE_NSEC ||
                    desiredPresent > expectedPresent) {
                
                
                
                ALOGV("acquireBuffer: nodrop desire=%" PRId64 " expect=%"
                        PRId64 " (%" PRId64 ") now=%" PRId64,
                        desiredPresent, expectedPresent,
                        desiredPresent - expectedPresent,
                        systemTime(CLOCK_MONOTONIC));
                break;
            }

            ALOGV("acquireBuffer: drop desire=%" PRId64 " expect=%" PRId64
                    " size=%zu",
                    desiredPresent, expectedPresent, mCore->mQueue.size());
            if (mCore->stillTracking(front)) {
                
                mSlots[front->mSlot].mBufferState = GonkBufferSlot::FREE;
            }
            mCore->mQueue.erase(front);
            front = mCore->mQueue.begin();
        }

        
        nsecs_t desiredPresent = front->mTimestamp;
        if (desiredPresent > expectedPresent &&
                desiredPresent < expectedPresent + MAX_REASONABLE_NSEC) {
            ALOGV("acquireBuffer: defer desire=%" PRId64 " expect=%" PRId64
                    " (%" PRId64 ") now=%" PRId64,
                    desiredPresent, expectedPresent,
                    desiredPresent - expectedPresent,
                    systemTime(CLOCK_MONOTONIC));
            return PRESENT_LATER;
        }

        ALOGV("acquireBuffer: accept desire=%" PRId64 " expect=%" PRId64 " "
                "(%" PRId64 ") now=%" PRId64, desiredPresent, expectedPresent,
                desiredPresent - expectedPresent,
                systemTime(CLOCK_MONOTONIC));
    }

    int slot = front->mSlot;
    
    outBuffer->mGraphicBuffer = mSlots[slot].mGraphicBuffer;
    outBuffer->mFrameNumber = mSlots[slot].mFrameNumber;
    outBuffer->mBuf = slot;
    outBuffer->mFence = mSlots[slot].mFence;

    ATRACE_BUFFER_INDEX(slot);

    ALOGV("acquireBuffer: acquiring { slot=%d/%" PRIu64 " buffer=%p }",
            slot, front->mFrameNumber, front->mGraphicBuffer->handle);
    
    if (mCore->stillTracking(front)) {
        mSlots[slot].mAcquireCalled = true;
        mSlots[slot].mNeedsCleanupOnRelease = false;
        mSlots[slot].mBufferState = GonkBufferSlot::ACQUIRED;
        mSlots[slot].mFence = Fence::NO_FENCE;
    }

    
    
    
    
    
    

    mCore->mQueue.erase(front);

    
    
    
    mCore->mDequeueCondition.broadcast();

    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::detachBuffer(int slot) {
    ATRACE_CALL();
    ATRACE_BUFFER_INDEX(slot);
    ALOGV("detachBuffer(C): slot %d", slot);
    Mutex::Autolock lock(mCore->mMutex);

    if (mCore->mIsAbandoned) {
        ALOGE("detachBuffer(C): GonkBufferQueue has been abandoned");
        return NO_INIT;
    }

    if (slot < 0 || slot >= GonkBufferQueueDefs::NUM_BUFFER_SLOTS) {
        ALOGE("detachBuffer(C): slot index %d out of range [0, %d)",
                slot, GonkBufferQueueDefs::NUM_BUFFER_SLOTS);
        return BAD_VALUE;
    } else if (mSlots[slot].mBufferState != GonkBufferSlot::ACQUIRED) {
        ALOGE("detachBuffer(C): slot %d is not owned by the consumer "
                "(state = %d)", slot, mSlots[slot].mBufferState);
        return BAD_VALUE;
    }

    mCore->freeBufferLocked(slot);
    mCore->mDequeueCondition.broadcast();

    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::attachBuffer(int* outSlot,
        const sp<android::GraphicBuffer>& buffer) {
    ATRACE_CALL();

    if (outSlot == NULL) {
        ALOGE("attachBuffer(P): outSlot must not be NULL");
        return BAD_VALUE;
    } else if (buffer == NULL) {
        ALOGE("attachBuffer(P): cannot attach NULL buffer");
        return BAD_VALUE;
    }

    Mutex::Autolock lock(mCore->mMutex);

    
    
    int numAcquiredBuffers = 0;
    int found = GonkBufferQueueCore::INVALID_BUFFER_SLOT;
    for (int s = 0; s < GonkBufferQueueDefs::NUM_BUFFER_SLOTS; ++s) {
        if (mSlots[s].mBufferState == GonkBufferSlot::ACQUIRED) {
            ++numAcquiredBuffers;
        } else if (mSlots[s].mBufferState == GonkBufferSlot::FREE) {
            if (found == GonkBufferQueueCore::INVALID_BUFFER_SLOT ||
                    mSlots[s].mFrameNumber < mSlots[found].mFrameNumber) {
                found = s;
            }
        }
    }

    if (numAcquiredBuffers >= mCore->mMaxAcquiredBufferCount + 1) {
        ALOGE("attachBuffer(P): max acquired buffer count reached: %d "
                "(max %d)", numAcquiredBuffers,
                mCore->mMaxAcquiredBufferCount);
        return INVALID_OPERATION;
    }
    if (found == GonkBufferQueueCore::INVALID_BUFFER_SLOT) {
        ALOGE("attachBuffer(P): could not find free buffer slot");
        return NO_MEMORY;
    }

    *outSlot = found;
    ATRACE_BUFFER_INDEX(*outSlot);
    ALOGV("attachBuffer(C): returning slot %d", *outSlot);

    mSlots[*outSlot].mGraphicBuffer = buffer;
    mSlots[*outSlot].mBufferState = GonkBufferSlot::ACQUIRED;
    mSlots[*outSlot].mAttachedByConsumer = true;
    mSlots[*outSlot].mNeedsCleanupOnRelease = false;
    mSlots[*outSlot].mFence = Fence::NO_FENCE;
    mSlots[*outSlot].mFrameNumber = 0;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    mSlots[*outSlot].mAcquireCalled = false;

    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::releaseBuffer(int slot, uint64_t frameNumber,
        const sp<Fence>& releaseFence) {
    ATRACE_CALL();

    if (slot < 0 || slot >= GonkBufferQueueDefs::NUM_BUFFER_SLOTS ||
            releaseFence == NULL) {
        return BAD_VALUE;
    }

    sp<IProducerListener> listener;
    { 
        Mutex::Autolock lock(mCore->mMutex);

        
        
        
        
        

        
        GonkBufferQueueCore::Fifo::iterator current(mCore->mQueue.begin());
        while (current != mCore->mQueue.end()) {
            if (current->mSlot == slot) {
                ALOGE("releaseBuffer: buffer slot %d pending release is "
                        "currently queued", slot);
                return BAD_VALUE;
            }
            ++current;
        }

        if (mSlots[slot].mBufferState == GonkBufferSlot::ACQUIRED) {
            mSlots[slot].mFence = releaseFence;
            mSlots[slot].mBufferState = GonkBufferSlot::FREE;
            listener = mCore->mConnectedProducerListener;
            ALOGV("releaseBuffer: releasing slot %d", slot);
        } else if (mSlots[slot].mNeedsCleanupOnRelease) {
            ALOGV("releaseBuffer: releasing a stale buffer slot %d "
                    "(state = %d)", slot, mSlots[slot].mBufferState);
            mSlots[slot].mNeedsCleanupOnRelease = false;
            return STALE_BUFFER_SLOT;
        } else {
            ALOGV("releaseBuffer: attempted to release buffer slot %d "
                    "but its state was %d", slot, mSlots[slot].mBufferState);
            return BAD_VALUE;
        }

        mCore->mDequeueCondition.broadcast();
    } 

    
    if (listener != NULL) {
        listener->onBufferReleased();
    }

    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::connect(
        const sp<IConsumerListener>& consumerListener, bool controlledByApp) {
    ATRACE_CALL();

    if (consumerListener == NULL) {
        ALOGE("connect(C): consumerListener may not be NULL");
        return BAD_VALUE;
    }

    ALOGV("connect(C): controlledByApp=%s",
            controlledByApp ? "true" : "false");

    Mutex::Autolock lock(mCore->mMutex);

    if (mCore->mIsAbandoned) {
        ALOGE("connect(C): GonkBufferQueue has been abandoned");
        return NO_INIT;
    }

    mCore->mConsumerListener = consumerListener;
    mCore->mConsumerControlledByApp = controlledByApp;

    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::disconnect() {
    ATRACE_CALL();

    ALOGV("disconnect(C)");

    Mutex::Autolock lock(mCore->mMutex);

    if (mCore->mConsumerListener == NULL) {
        ALOGE("disconnect(C): no consumer is connected");
        return BAD_VALUE;
    }

    mCore->mIsAbandoned = true;
    mCore->mConsumerListener = NULL;
    mCore->mQueue.clear();
    mCore->freeAllBuffersLocked();
    mCore->mDequeueCondition.broadcast();
    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::getReleasedBuffers(uint64_t *outSlotMask) {
    ATRACE_CALL();

    if (outSlotMask == NULL) {
        ALOGE("getReleasedBuffers: outSlotMask may not be NULL");
        return BAD_VALUE;
    }

    Mutex::Autolock lock(mCore->mMutex);

    if (mCore->mIsAbandoned) {
        ALOGE("getReleasedBuffers: GonkBufferQueue has been abandoned");
        return NO_INIT;
    }

    uint64_t mask = 0;
    for (int s = 0; s < GonkBufferQueueDefs::NUM_BUFFER_SLOTS; ++s) {
        if (!mSlots[s].mAcquireCalled) {
            mask |= (1ULL << s);
        }
    }

    
    
    
    GonkBufferQueueCore::Fifo::iterator current(mCore->mQueue.begin());
    while (current != mCore->mQueue.end()) {
        if (current->mAcquireCalled) {
            mask &= ~(1ULL << current->mSlot);
        }
        ++current;
    }

    ALOGV("getReleasedBuffers: returning mask %#" PRIx64, mask);
    *outSlotMask = mask;
    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::setDefaultBufferSize(uint32_t width,
        uint32_t height) {
    ATRACE_CALL();

    if (width == 0 || height == 0) {
        ALOGV("setDefaultBufferSize: dimensions cannot be 0 (width=%u "
                "height=%u)", width, height);
        return BAD_VALUE;
    }

    ALOGV("setDefaultBufferSize: width=%u height=%u", width, height);

    Mutex::Autolock lock(mCore->mMutex);
    mCore->mDefaultWidth = width;
    mCore->mDefaultHeight = height;
    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::setDefaultMaxBufferCount(int bufferCount) {
    ATRACE_CALL();
    Mutex::Autolock lock(mCore->mMutex);
    return mCore->setDefaultMaxBufferCountLocked(bufferCount);
}

status_t GonkBufferQueueConsumer::disableAsyncBuffer() {
    ATRACE_CALL();

    Mutex::Autolock lock(mCore->mMutex);

    if (mCore->mConsumerListener != NULL) {
        ALOGE("disableAsyncBuffer: consumer already connected");
        return INVALID_OPERATION;
    }

    ALOGV("disableAsyncBuffer");
    mCore->mUseAsyncBuffer = false;
    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::setMaxAcquiredBufferCount(
        int maxAcquiredBuffers) {
    ATRACE_CALL();

    if (maxAcquiredBuffers < 1 ||
            maxAcquiredBuffers > GonkBufferQueueCore::MAX_MAX_ACQUIRED_BUFFERS) {
        ALOGE("setMaxAcquiredBufferCount: invalid count %d",
                maxAcquiredBuffers);
        return BAD_VALUE;
    }

    Mutex::Autolock lock(mCore->mMutex);

    if (mCore->mConnectedApi != GonkBufferQueueCore::NO_CONNECTED_API) {
        ALOGE("setMaxAcquiredBufferCount: producer is already connected");
        return INVALID_OPERATION;
    }

    ALOGV("setMaxAcquiredBufferCount: %d", maxAcquiredBuffers);
    mCore->mMaxAcquiredBufferCount = maxAcquiredBuffers;
    return NO_ERROR;
}

void GonkBufferQueueConsumer::setConsumerName(const String8& name) {
    ATRACE_CALL();
    ALOGV("setConsumerName: '%s'", name.string());
    Mutex::Autolock lock(mCore->mMutex);
    mCore->mConsumerName = name;
    mConsumerName = name;
}

status_t GonkBufferQueueConsumer::setDefaultBufferFormat(uint32_t defaultFormat) {
    ATRACE_CALL();
    ALOGV("setDefaultBufferFormat: %u", defaultFormat);
    Mutex::Autolock lock(mCore->mMutex);
    mCore->mDefaultBufferFormat = defaultFormat;
    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::setConsumerUsageBits(uint32_t usage) {
    ATRACE_CALL();
    ALOGV("setConsumerUsageBits: %#x", usage);
    Mutex::Autolock lock(mCore->mMutex);
    mCore->mConsumerUsageBits = usage;
    return NO_ERROR;
}

status_t GonkBufferQueueConsumer::setTransformHint(uint32_t hint) {
    ATRACE_CALL();
    ALOGV("setTransformHint: %#x", hint);
    Mutex::Autolock lock(mCore->mMutex);
    mCore->mTransformHint = hint;
    return NO_ERROR;
}

sp<NativeHandle> GonkBufferQueueConsumer::getSidebandStream() const {
    return mCore->mSidebandStream;
}

void GonkBufferQueueConsumer::dumpToString(String8& result, const char* prefix) const {
    mCore->dump(result, prefix);
}

already_AddRefed<GonkBufferSlot::TextureClient>
GonkBufferQueueConsumer::getTextureClientFromBuffer(ANativeWindowBuffer* buffer)
{
    Mutex::Autolock _l(mCore->mMutex);
    if (buffer == NULL) {
        ALOGE("getSlotFromBufferLocked: encountered NULL buffer");
        return nullptr;
    }

    for (int i = 0; i < GonkBufferQueueDefs::NUM_BUFFER_SLOTS; i++) {
        if (mSlots[i].mGraphicBuffer != NULL && mSlots[i].mGraphicBuffer->handle == buffer->handle) {
            RefPtr<TextureClient> client(mSlots[i].mTextureClient);
            return client.forget();
        }
    }
    ALOGE("getSlotFromBufferLocked: unknown buffer: %p", buffer->handle);
    return nullptr;
}

int
GonkBufferQueueConsumer::getSlotFromTextureClientLocked(GonkBufferSlot::TextureClient* client) const
{
    if (client == NULL) {
        ALOGE("getSlotFromBufferLocked: encountered NULL buffer");
        return BAD_VALUE;
    }

    for (int i = 0; i < GonkBufferQueueDefs::NUM_BUFFER_SLOTS; i++) {
        if (mSlots[i].mTextureClient == client) {
            return i;
        }
    }
    ALOGE("getSlotFromBufferLocked: unknown TextureClient: %p", client);
    return BAD_VALUE;
}
} 
