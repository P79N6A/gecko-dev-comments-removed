






#include "SkDataPixelRef.h"
#include "SkData.h"
#include "SkFlattenableBuffers.h"

SkDataPixelRef::SkDataPixelRef(SkData* data) : fData(data) {
    fData->ref();
    this->setPreLocked(const_cast<void*>(fData->data()), NULL);
}

SkDataPixelRef::~SkDataPixelRef() {
    fData->unref();
}

void* SkDataPixelRef::onLockPixels(SkColorTable** ct) {
    *ct = NULL;
    return const_cast<void*>(fData->data());
}

void SkDataPixelRef::onUnlockPixels() {
    
}

void SkDataPixelRef::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fData);
}

SkDataPixelRef::SkDataPixelRef(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer, NULL) {
    fData = (SkData*)buffer.readFlattenable();
    this->setPreLocked(const_cast<void*>(fData->data()), NULL);
}

SK_DEFINE_FLATTENABLE_REGISTRAR(SkDataPixelRef)
