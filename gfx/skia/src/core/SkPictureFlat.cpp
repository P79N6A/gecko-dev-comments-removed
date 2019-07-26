






#include "SkPictureFlat.h"

#include "SkChecksum.h"
#include "SkColorFilter.h"
#include "SkDrawLooper.h"
#include "SkMaskFilter.h"
#include "SkRasterizer.h"
#include "SkShader.h"
#include "SkTypeface.h"
#include "SkXfermode.h"

SK_DEFINE_INST_COUNT(SkFlatController)



SkTypefacePlayback::SkTypefacePlayback() : fCount(0), fArray(NULL) {}

SkTypefacePlayback::~SkTypefacePlayback() {
    this->reset(NULL);
}

void SkTypefacePlayback::reset(const SkRefCntSet* rec) {
    for (int i = 0; i < fCount; i++) {
        SkASSERT(fArray[i]);
        fArray[i]->unref();
    }
    SkDELETE_ARRAY(fArray);

    if (rec!= NULL && rec->count() > 0) {
        fCount = rec->count();
        fArray = SkNEW_ARRAY(SkRefCnt*, fCount);
        rec->copyToArray(fArray);
        for (int i = 0; i < fCount; i++) {
            fArray[i]->ref();
        }
    } else {
        fCount = 0;
        fArray = NULL;
    }
}

void SkTypefacePlayback::setCount(int count) {
    this->reset(NULL);

    fCount = count;
    fArray = SkNEW_ARRAY(SkRefCnt*, count);
    sk_bzero(fArray, count * sizeof(SkRefCnt*));
}

SkRefCnt* SkTypefacePlayback::set(int index, SkRefCnt* obj) {
    SkASSERT((unsigned)index < (unsigned)fCount);
    SkRefCnt_SafeAssign(fArray[index], obj);
    return obj;
}



SkFlatController::SkFlatController()
: fBitmapHeap(NULL)
, fTypefaceSet(NULL)
, fTypefacePlayback(NULL)
, fFactorySet(NULL)
, fWriteBufferFlags(0) {}

SkFlatController::~SkFlatController() {
    SkSafeUnref(fBitmapHeap);
    SkSafeUnref(fTypefaceSet);
    SkSafeUnref(fFactorySet);
}

void SkFlatController::setBitmapHeap(SkBitmapHeap* heap) {
    SkRefCnt_SafeAssign(fBitmapHeap, heap);
}

void SkFlatController::setTypefaceSet(SkRefCntSet *set) {
    SkRefCnt_SafeAssign(fTypefaceSet, set);
}

void SkFlatController::setTypefacePlayback(SkTypefacePlayback* playback) {
    fTypefacePlayback = playback;
}

SkNamedFactorySet* SkFlatController::setNamedFactorySet(SkNamedFactorySet* set) {
    SkRefCnt_SafeAssign(fFactorySet, set);
    return set;
}



SkFlatData* SkFlatData::Create(SkFlatController* controller, const void* obj,
        int index, void (*flattenProc)(SkOrderedWriteBuffer&, const void*)) {
    
    
    intptr_t storage[256];
    SkOrderedWriteBuffer buffer(256, storage, sizeof(storage));

    buffer.setBitmapHeap(controller->getBitmapHeap());
    buffer.setTypefaceRecorder(controller->getTypefaceSet());
    buffer.setNamedFactoryRecorder(controller->getNamedFactorySet());
    buffer.setFlags(controller->getWriteBufferFlags());

    flattenProc(buffer, obj);
    uint32_t size = buffer.size();
    SkASSERT(SkIsAlign4(size));

    





    size_t allocSize = sizeof(SkFlatData) + size + sizeof(uint32_t);
    SkFlatData* result = (SkFlatData*) controller->allocThrow(allocSize);

    result->fIndex = index;
    result->fFlatSize = size;

    
    buffer.writeToMemory(result->data());
    result->fChecksum = SkChecksum::Compute(result->data32(), size);
    result->setSentinelAsCandidate();
    return result;
}

void SkFlatData::unflatten(void* result,
        void (*unflattenProc)(SkOrderedReadBuffer&, void*),
        SkBitmapHeap* bitmapHeap,
        SkTypefacePlayback* facePlayback) const {

    SkOrderedReadBuffer buffer(this->data(), fFlatSize);

    if (bitmapHeap) {
        buffer.setBitmapStorage(bitmapHeap);
    }
    if (facePlayback) {
        facePlayback->setupBuffer(buffer);
    }

    unflattenProc(buffer, result);
    SkASSERT(fFlatSize == (int32_t)buffer.offset());
}
