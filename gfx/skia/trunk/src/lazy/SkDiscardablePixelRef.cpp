






#include "SkDiscardablePixelRef.h"
#include "SkDiscardableMemory.h"
#include "SkImageGenerator.h"

SkDiscardablePixelRef::SkDiscardablePixelRef(const SkImageInfo& info,
                                             SkImageGenerator* generator,
                                             size_t rowBytes,
                                             SkDiscardableMemory::Factory* fact)
    : INHERITED(info)
    , fGenerator(generator)
    , fDMFactory(fact)
    , fRowBytes(rowBytes)
    , fDiscardableMemory(NULL)
{
    SkASSERT(fGenerator != NULL);
    SkASSERT(fRowBytes > 0);
    
    
    this->setImmutable();
    SkSafeRef(fDMFactory);
}

SkDiscardablePixelRef::~SkDiscardablePixelRef() {
    if (this->isLocked()) {
        fDiscardableMemory->unlock();
    }
    SkDELETE(fDiscardableMemory);
    SkSafeUnref(fDMFactory);
    SkDELETE(fGenerator);
}

bool SkDiscardablePixelRef::onNewLockPixels(LockRec* rec) {
    if (fDiscardableMemory != NULL) {
        if (fDiscardableMemory->lock()) {
            rec->fPixels = fDiscardableMemory->data();
            rec->fColorTable = fCTable.get();
            rec->fRowBytes = fRowBytes;
            return true;
        }
        SkDELETE(fDiscardableMemory);
        fDiscardableMemory = NULL;
    }

    const size_t size = this->info().getSafeSize(fRowBytes);

    if (fDMFactory != NULL) {
        fDiscardableMemory = fDMFactory->create(size);
    } else {
        fDiscardableMemory = SkDiscardableMemory::Create(size);
    }
    if (NULL == fDiscardableMemory) {
        return false;  
    }

    void* pixels = fDiscardableMemory->data();
    const SkImageInfo& info = this->info();
    SkPMColor colors[256];
    int colorCount = 0;

#ifdef SK_SUPPORT_LEGACY_IMAGEGENERATORAPI
    if (!fGenerator->getPixels(info, pixels, fRowBytes)) {
#else
    if (!fGenerator->getPixels(info, pixels, fRowBytes, colors, &colorCount)) {
#endif
        fDiscardableMemory->unlock();
        SkDELETE(fDiscardableMemory);
        fDiscardableMemory = NULL;
        return false;
    }

    
    
    
    
    
    if (colorCount > 0) {
        fCTable.reset(SkNEW_ARGS(SkColorTable, (colors, colorCount)));
    } else {
        fCTable.reset(NULL);
    }

    rec->fPixels = pixels;
    rec->fColorTable = fCTable.get();
    rec->fRowBytes = fRowBytes;
    return true;
}

void SkDiscardablePixelRef::onUnlockPixels() {
    fDiscardableMemory->unlock();
}

bool SkInstallDiscardablePixelRef(SkImageGenerator* generator, SkBitmap* dst,
                                  SkDiscardableMemory::Factory* factory) {
    SkImageInfo info;
    SkAutoTDelete<SkImageGenerator> autoGenerator(generator);
    if ((NULL == autoGenerator.get())
        || (!autoGenerator->getInfo(&info))
        || (!dst->setInfo(info))) {
        return false;
    }
    SkASSERT(dst->colorType() != kUnknown_SkColorType);
    if (dst->empty()) {  
        return dst->allocPixels();
    }
    SkAutoTUnref<SkDiscardablePixelRef> ref(
        SkNEW_ARGS(SkDiscardablePixelRef,
                   (info, autoGenerator.detach(), dst->rowBytes(), factory)));
    dst->setPixelRef(ref);
    return true;
}


bool SkInstallDiscardablePixelRef(SkImageGenerator* generator, SkBitmap* dst) {
    return SkInstallDiscardablePixelRef(generator, dst, NULL);
}
