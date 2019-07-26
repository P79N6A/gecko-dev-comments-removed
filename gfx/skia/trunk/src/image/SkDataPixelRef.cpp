






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

size_t SkDataPixelRef::getAllocatedSizeInBytes() const {
    return fData ? fData->size() : 0;
}

void SkDataPixelRef::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeDataAsByteArray(fData);
}

SkDataPixelRef::SkDataPixelRef(SkFlattenableReadBuffer& buffer)
        : INHERITED(buffer, NULL) {
    fData = buffer.readByteArrayAsData();
    this->setPreLocked(const_cast<void*>(fData->data()), NULL);
}
