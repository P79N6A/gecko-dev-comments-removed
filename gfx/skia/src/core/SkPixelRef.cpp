






#include "SkPixelRef.h"
#include "SkFlattenableBuffers.h"
#include "SkThread.h"

SK_DEFINE_INST_COUNT(SkPixelRef)


#define PIXELREF_MUTEX_RING_COUNT       32

#ifdef PIXELREF_MUTEX_RING_COUNT
    static int32_t gPixelRefMutexRingIndex;
    static SK_DECLARE_MUTEX_ARRAY(gPixelRefMutexRing, PIXELREF_MUTEX_RING_COUNT);
#else
    SK_DECLARE_STATIC_MUTEX(gPixelRefMutex);
#endif

static SkBaseMutex* get_default_mutex() {
#ifdef PIXELREF_MUTEX_RING_COUNT
    
    
    int index = sk_atomic_inc(&gPixelRefMutexRingIndex);
    return &gPixelRefMutexRing[index & (PIXELREF_MUTEX_RING_COUNT - 1)];
#else
    return &gPixelRefMutex;
#endif
}



int32_t SkNextPixelRefGenerationID();

int32_t SkNextPixelRefGenerationID() {
    static int32_t  gPixelRefGenerationID;
    
    
    int32_t genID;
    do {
        genID = sk_atomic_inc(&gPixelRefGenerationID) + 1;
    } while (0 == genID);
    return genID;
}



void SkPixelRef::setMutex(SkBaseMutex* mutex) {
    if (NULL == mutex) {
        mutex = get_default_mutex();
    }
    fMutex = mutex;
}


#define SKPIXELREF_PRELOCKED_LOCKCOUNT     123456789

SkPixelRef::SkPixelRef(SkBaseMutex* mutex) : fPreLocked(false) {
    this->setMutex(mutex);
    fPixels = NULL;
    fColorTable = NULL; 
    fLockCount = 0;
    fGenerationID = 0;  
    fIsImmutable = false;
    fPreLocked = false;
}

SkPixelRef::SkPixelRef(SkFlattenableReadBuffer& buffer, SkBaseMutex* mutex)
        : INHERITED(buffer) {
    this->setMutex(mutex);
    fPixels = NULL;
    fColorTable = NULL; 
    fLockCount = 0;
    fIsImmutable = buffer.readBool();
    fGenerationID = buffer.readUInt();
    fPreLocked = false;
}

void SkPixelRef::setPreLocked(void* pixels, SkColorTable* ctable) {
    
    
    fPixels = pixels;
    fColorTable = ctable;
    fLockCount = SKPIXELREF_PRELOCKED_LOCKCOUNT;
    fPreLocked = true;
}

void SkPixelRef::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeBool(fIsImmutable);
    
    
    
    
    
    if (buffer.isCrossProcess()) {
        buffer.writeUInt(0);
    } else {
        buffer.writeUInt(fGenerationID);
    }
}

void SkPixelRef::lockPixels() {
    SkASSERT(!fPreLocked || SKPIXELREF_PRELOCKED_LOCKCOUNT == fLockCount);

    if (!fPreLocked) {
        SkAutoMutexAcquire  ac(*fMutex);

        if (1 == ++fLockCount) {
            fPixels = this->onLockPixels(&fColorTable);
        }
    }
}

void SkPixelRef::unlockPixels() {
    SkASSERT(!fPreLocked || SKPIXELREF_PRELOCKED_LOCKCOUNT == fLockCount);

    if (!fPreLocked) {
        SkAutoMutexAcquire  ac(*fMutex);

        SkASSERT(fLockCount > 0);
        if (0 == --fLockCount) {
            this->onUnlockPixels();
            fPixels = NULL;
            fColorTable = NULL;
        }
    }
}

bool SkPixelRef::lockPixelsAreWritable() const {
    return this->onLockPixelsAreWritable();
}

bool SkPixelRef::onLockPixelsAreWritable() const {
    return true;
}

uint32_t SkPixelRef::getGenerationID() const {
    if (0 == fGenerationID) {
        fGenerationID = SkNextPixelRefGenerationID();
    }
    return fGenerationID;
}

void SkPixelRef::notifyPixelsChanged() {
#ifdef SK_DEBUG
    if (fIsImmutable) {
        SkDebugf("========== notifyPixelsChanged called on immutable pixelref");
    }
#endif
    
    fGenerationID = 0;
}

void SkPixelRef::setImmutable() {
    fIsImmutable = true;
}

bool SkPixelRef::readPixels(SkBitmap* dst, const SkIRect* subset) {
    return this->onReadPixels(dst, subset);
}

bool SkPixelRef::onReadPixels(SkBitmap* dst, const SkIRect* subset) {
    return false;
}



#ifdef SK_BUILD_FOR_ANDROID
void SkPixelRef::globalRef(void* data) {
    this->ref();
}

void SkPixelRef::globalUnref() {
    this->unref();
}
#endif
