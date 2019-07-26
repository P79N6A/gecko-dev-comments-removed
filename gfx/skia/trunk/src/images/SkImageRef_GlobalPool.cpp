






#include "SkImageRef_GlobalPool.h"
#include "SkImageRefPool.h"
#include "SkThread.h"

SK_DECLARE_STATIC_MUTEX(gGlobalPoolMutex);





static SkImageRefPool* GetGlobalPool() {
    static SkImageRefPool* gPool;
    if (NULL == gPool) {
        gPool = SkNEW(SkImageRefPool);
        
    }
    return gPool;
}

SkImageRef_GlobalPool::SkImageRef_GlobalPool(const SkImageInfo& info,
                                             SkStreamRewindable* stream,
                                             int sampleSize)
        : SkImageRef(info, stream, sampleSize, &gGlobalPoolMutex) {
    SkASSERT(&gGlobalPoolMutex == this->mutex());
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    GetGlobalPool()->addToHead(this);
}

SkImageRef_GlobalPool::~SkImageRef_GlobalPool() {
    SkASSERT(&gGlobalPoolMutex == this->mutex());
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    GetGlobalPool()->detach(this);
}






bool SkImageRef_GlobalPool::onDecode(SkImageDecoder* codec, SkStreamRewindable* stream,
                                     SkBitmap* bitmap, SkBitmap::Config config,
                                     SkImageDecoder::Mode mode) {
    if (!this->INHERITED::onDecode(codec, stream, bitmap, config, mode)) {
        return false;
    }
    if (mode == SkImageDecoder::kDecodePixels_Mode) {
        
        GetGlobalPool()->justAddedPixels(this);
    }
    return true;
}

void SkImageRef_GlobalPool::onUnlockPixels() {
    this->INHERITED::onUnlockPixels();

    
    GetGlobalPool()->canLosePixels(this);
}

SkImageRef_GlobalPool::SkImageRef_GlobalPool(SkReadBuffer& buffer)
        : INHERITED(buffer, &gGlobalPoolMutex) {
    SkASSERT(&gGlobalPoolMutex == this->mutex());
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    GetGlobalPool()->addToHead(this);
}




size_t SkImageRef_GlobalPool::GetRAMBudget() {
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    return GetGlobalPool()->getRAMBudget();
}

void SkImageRef_GlobalPool::SetRAMBudget(size_t size) {
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    GetGlobalPool()->setRAMBudget(size);
}

size_t SkImageRef_GlobalPool::GetRAMUsed() {
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    return GetGlobalPool()->getRAMUsed();
}

void SkImageRef_GlobalPool::SetRAMUsed(size_t usage) {
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    GetGlobalPool()->setRAMUsed(usage);
}

void SkImageRef_GlobalPool::DumpPool() {
    SkAutoMutexAcquire ac(gGlobalPoolMutex);
    GetGlobalPool()->dump();
}
