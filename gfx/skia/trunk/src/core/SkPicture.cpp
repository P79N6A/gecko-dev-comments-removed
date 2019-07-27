








#include "SkPictureFlat.h"
#include "SkPictureData.h"
#include "SkPicturePlayback.h"
#include "SkPictureRecord.h"
#include "SkPictureRecorder.h"
#include "SkPictureStateTree.h"

#include "SkBBHFactory.h"
#include "SkBitmapDevice.h"
#include "SkCanvas.h"
#include "SkChunkAlloc.h"
#include "SkDrawPictureCallback.h"
#include "SkPaintPriv.h"
#include "SkPicture.h"
#include "SkRecordAnalysis.h"
#include "SkRegion.h"
#include "SkStream.h"
#include "SkTDArray.h"
#include "SkTSearch.h"
#include "SkTime.h"

#include "SkReader32.h"
#include "SkWriter32.h"
#include "SkRTree.h"
#include "SkBBoxHierarchyRecord.h"

#if SK_SUPPORT_GPU
#include "GrContext.h"
#endif

#include "SkRecord.h"
#include "SkRecordDraw.h"
#include "SkRecorder.h"

template <typename T> int SafeCount(const T* obj) {
    return obj ? obj->count() : 0;
}

#define DUMP_BUFFER_SIZE 65536

#ifdef SK_DEBUG









#endif

#if defined SK_DEBUG_TRACE || defined SK_DEBUG_DUMP
const char* DrawTypeToString(DrawType drawType) {
    switch (drawType) {
        case UNUSED: SkDebugf("DrawType UNUSED\n"); SkASSERT(0); break;
        case CLIP_PATH: return "CLIP_PATH";
        case CLIP_REGION: return "CLIP_REGION";
        case CLIP_RECT: return "CLIP_RECT";
        case CLIP_RRECT: return "CLIP_RRECT";
        case CONCAT: return "CONCAT";
        case DRAW_BITMAP: return "DRAW_BITMAP";
        case DRAW_BITMAP_MATRIX: return "DRAW_BITMAP_MATRIX";
        case DRAW_BITMAP_NINE: return "DRAW_BITMAP_NINE";
        case DRAW_BITMAP_RECT_TO_RECT: return "DRAW_BITMAP_RECT_TO_RECT";
        case DRAW_CLEAR: return "DRAW_CLEAR";
        case DRAW_DATA: return "DRAW_DATA";
        case DRAW_OVAL: return "DRAW_OVAL";
        case DRAW_PAINT: return "DRAW_PAINT";
        case DRAW_PATH: return "DRAW_PATH";
        case DRAW_PICTURE: return "DRAW_PICTURE";
        case DRAW_POINTS: return "DRAW_POINTS";
        case DRAW_POS_TEXT: return "DRAW_POS_TEXT";
        case DRAW_POS_TEXT_TOP_BOTTOM: return "DRAW_POS_TEXT_TOP_BOTTOM";
        case DRAW_POS_TEXT_H: return "DRAW_POS_TEXT_H";
        case DRAW_POS_TEXT_H_TOP_BOTTOM: return "DRAW_POS_TEXT_H_TOP_BOTTOM";
        case DRAW_RECT: return "DRAW_RECT";
        case DRAW_RRECT: return "DRAW_RRECT";
        case DRAW_SPRITE: return "DRAW_SPRITE";
        case DRAW_TEXT: return "DRAW_TEXT";
        case DRAW_TEXT_ON_PATH: return "DRAW_TEXT_ON_PATH";
        case DRAW_TEXT_TOP_BOTTOM: return "DRAW_TEXT_TOP_BOTTOM";
        case DRAW_VERTICES: return "DRAW_VERTICES";
        case RESTORE: return "RESTORE";
        case ROTATE: return "ROTATE";
        case SAVE: return "SAVE";
        case SAVE_LAYER: return "SAVE_LAYER";
        case SCALE: return "SCALE";
        case SET_MATRIX: return "SET_MATRIX";
        case SKEW: return "SKEW";
        case TRANSLATE: return "TRANSLATE";
        case NOOP: return "NOOP";
        default:
            SkDebugf("DrawType error 0x%08x\n", drawType);
            SkASSERT(0);
            break;
    }
    SkASSERT(0);
    return NULL;
}
#endif



#ifdef SK_SUPPORT_LEGACY_DEFAULT_PICTURE_CTOR

SkPicture::SkPicture()
    : fWidth(0)
    , fHeight(0)
    , fRecordWillPlayBackBitmaps(false) {
    this->needsNewGenID();
}
#endif


SkPicture::SkPicture(int width, int height,
                     const SkPictureRecord& record,
                     bool deepCopyOps)
    : fWidth(width)
    , fHeight(height)
    , fRecordWillPlayBackBitmaps(false) {
    this->needsNewGenID();

    SkPictInfo info;
    this->createHeader(&info);
    fData.reset(SkNEW_ARGS(SkPictureData, (record, info, deepCopyOps)));
}



static SkPicture* backport(const SkRecord& src, int width, int height) {
    SkPictureRecorder recorder;
    SkRecordDraw(src, recorder.beginRecording(width, height));
    return recorder.endRecording();
}


SkPicture::~SkPicture() {
    this->callDeletionListeners();
}

#ifdef SK_SUPPORT_LEGACY_PICTURE_CLONE

SkPicture* SkPicture::clone() const {

    SkAutoTDelete<SkPictureData> newData;

    if (fData.get()) {
        SkPictCopyInfo copyInfo;

        int paintCount = SafeCount(fData->fPaints);

        




        copyInfo.paintData.setCount(paintCount);

        



        if (fData->fBitmapHeap.get() == NULL) {
            
            SkBitmapHeap* heap = SkNEW(SkBitmapHeap);
            copyInfo.controller.setBitmapStorage(heap);
            heap->unref();
        } else {
            copyInfo.controller.setBitmapStorage(fData->fBitmapHeap);
        }

        SkDEBUGCODE(int heapSize = SafeCount(fData->fBitmapHeap.get());)
        for (int i = 0; i < paintCount; i++) {
            if (NeedsDeepCopy(fData->fPaints->at(i))) {
                copyInfo.paintData[i] =
                    SkFlatData::Create<SkPaint::FlatteningTraits>(&copyInfo.controller,
                    fData->fPaints->at(i), 0);

            } else {
                
                copyInfo.paintData[i] = NULL;
            }
        }
        SkASSERT(SafeCount(fData->fBitmapHeap.get()) == heapSize);

        
        copyInfo.controller.setupPlaybacks();

        newData.reset(SkNEW_ARGS(SkPictureData, (*fData, &copyInfo)));
    }

    SkPicture* clone = SkNEW_ARGS(SkPicture, (newData.detach(), fWidth, fHeight));
    clone->fRecordWillPlayBackBitmaps = fRecordWillPlayBackBitmaps;
    clone->fUniqueID = this->uniqueID(); 

    return clone;
}
#endif


void SkPicture::EXPERIMENTAL_addAccelData(const SkPicture::AccelData* data) const {
    fAccelData.reset(SkRef(data));
}


const SkPicture::AccelData* SkPicture::EXPERIMENTAL_getAccelData(
        SkPicture::AccelData::Key key) const {
    if (NULL != fAccelData.get() && fAccelData->getKey() == key) {
        return fAccelData.get();
    }
    return NULL;
}


SkPicture::AccelData::Domain SkPicture::AccelData::GenerateDomain() {
    static int32_t gNextID = 0;

    int32_t id = sk_atomic_inc(&gNextID);
    if (id >= 1 << (8 * sizeof(Domain))) {
        SK_CRASH();
    }

    return static_cast<Domain>(id);
}



uint32_t SkPicture::OperationList::offset(int index) const {
    SkASSERT(index < fOps.count());
    return ((SkPictureStateTree::Draw*)fOps[index])->fOffset;
}

const SkMatrix& SkPicture::OperationList::matrix(int index) const {
    SkASSERT(index < fOps.count());
    return *((SkPictureStateTree::Draw*)fOps[index])->fMatrix;
}


const SkPicture::OperationList* SkPicture::EXPERIMENTAL_getActiveOps(const SkIRect& queryRect) const {
    SkASSERT(NULL != fData.get());
    if (NULL != fData.get()) {
        return fData->getActiveOps(queryRect);
    }
    return NULL;
}


void SkPicture::draw(SkCanvas* canvas, SkDrawPictureCallback* callback) const {
    SkASSERT(NULL != canvas);
    SkASSERT(NULL != fData.get() || NULL != fRecord.get());

    if (NULL != fData.get()) {
        SkPicturePlayback playback(this);
        playback.draw(canvas, callback);
    }
    if (NULL != fRecord.get()) {
        SkRecordDraw(*fRecord, canvas, callback);
    }
}



#include "SkStream.h"

static const char kMagic[] = { 's', 'k', 'i', 'a', 'p', 'i', 'c', 't' };


bool SkPicture::IsValidPictInfo(const SkPictInfo& info) {
    if (0 != memcmp(info.fMagic, kMagic, sizeof(kMagic))) {
        return false;
    }

    if (info.fVersion < MIN_PICTURE_VERSION ||
        info.fVersion > CURRENT_PICTURE_VERSION) {
        return false;
    }

    return true;
}


bool SkPicture::InternalOnly_StreamIsSKP(SkStream* stream, SkPictInfo* pInfo) {
    if (NULL == stream) {
        return false;
    }

    
    SkPictInfo info;
    SkASSERT(sizeof(kMagic) == sizeof(info.fMagic));
    if (!stream->read(&info, sizeof(info)) || !IsValidPictInfo(info)) {
        return false;
    }

    if (pInfo != NULL) {
        *pInfo = info;
    }
    return true;
}


bool SkPicture::InternalOnly_BufferIsSKP(SkReadBuffer& buffer, SkPictInfo* pInfo) {
    
    SkPictInfo info;
    SkASSERT(sizeof(kMagic) == sizeof(info.fMagic));
    if (!buffer.readByteArray(&info, sizeof(info)) || !IsValidPictInfo(info)) {
        return false;
    }

    if (pInfo != NULL) {
        *pInfo = info;
    }
    return true;
}


SkPicture::SkPicture(SkPictureData* data, int width, int height)
    : fData(data)
    , fWidth(width)
    , fHeight(height)
    , fRecordWillPlayBackBitmaps(false) {
    this->needsNewGenID();
}


SkPicture* SkPicture::CreateFromStream(SkStream* stream, InstallPixelRefProc proc) {
    SkPictInfo info;

    if (!InternalOnly_StreamIsSKP(stream, &info)) {
        return NULL;
    }

    
    if (stream->readBool()) {
        SkPictureData* data = SkPictureData::CreateFromStream(stream, info, proc);
        if (NULL == data) {
            return NULL;
        }

        return SkNEW_ARGS(SkPicture, (data, info.fWidth, info.fHeight));
    }

    return NULL;
}


SkPicture* SkPicture::CreateFromBuffer(SkReadBuffer& buffer) {
    SkPictInfo info;

    if (!InternalOnly_BufferIsSKP(buffer, &info)) {
        return NULL;
    }

    
    if (buffer.readBool()) {
        SkPictureData* data = SkPictureData::CreateFromBuffer(buffer, info);
        if (NULL == data) {
            return NULL;
        }

        return SkNEW_ARGS(SkPicture, (data, info.fWidth, info.fHeight));
    }

    return NULL;
}


void SkPicture::createHeader(SkPictInfo* info) const {
    
    SkASSERT(sizeof(kMagic) == 8);
    SkASSERT(sizeof(kMagic) == sizeof(info->fMagic));
    memcpy(info->fMagic, kMagic, sizeof(kMagic));

    
    info->fVersion = CURRENT_PICTURE_VERSION;
    info->fWidth = fWidth;
    info->fHeight = fHeight;
    info->fFlags = SkPictInfo::kCrossProcess_Flag;
    
    info->fFlags |= SkPictInfo::kScalarIsFloat_Flag;

    if (8 == sizeof(void*)) {
        info->fFlags |= SkPictInfo::kPtrIs64Bit_Flag;
    }
}


void SkPicture::serialize(SkWStream* stream, EncodeBitmap encoder) const {
    const SkPictureData* data = fData.get();

    
    SkAutoTDelete<SkPicture> oldFormat;
    if (NULL == data && NULL != fRecord.get()) {
        oldFormat.reset(backport(*fRecord, fWidth, fHeight));
        data = oldFormat->fData.get();
        SkASSERT(NULL != data);
    }

    SkPictInfo info;
    this->createHeader(&info);
    stream->write(&info, sizeof(info));

    if (NULL != data) {
        stream->writeBool(true);
        data->serialize(stream, encoder);
    } else {
        stream->writeBool(false);
    }
}


void SkPicture::flatten(SkWriteBuffer& buffer) const {
    const SkPictureData* data = fData.get();

    
    SkAutoTDelete<SkPicture> oldFormat;
    if (NULL == data && NULL != fRecord.get()) {
        oldFormat.reset(backport(*fRecord, fWidth, fHeight));
        data = oldFormat->fData.get();
        SkASSERT(NULL != data);
    }

    SkPictInfo info;
    this->createHeader(&info);
    buffer.writeByteArray(&info, sizeof(info));

    if (NULL != data) {
        buffer.writeBool(true);
        data->flatten(buffer);
    } else {
        buffer.writeBool(false);
    }
}

#if SK_SUPPORT_GPU

bool SkPicture::suitableForGpuRasterization(GrContext* context, const char **reason) const {
    if (NULL == fData.get()) {
        if (NULL != reason) {
            *reason = "Missing internal data.";
        }
        return false;
    }

    return fData->suitableForGpuRasterization(context, reason);
}
#endif


bool SkPicture::willPlayBackBitmaps() const {
    if (fRecord.get()) {
        return fRecordWillPlayBackBitmaps;
    }
    if (!fData.get()) {
        return false;
    }
    return fData->containsBitmaps();
}


static int32_t next_picture_generation_id() {
    static int32_t  gPictureGenerationID = 0;
    
    
    int32_t genID;
    do {
        genID = sk_atomic_inc(&gPictureGenerationID) + 1;
    } while (SK_InvalidGenID == genID);
    return genID;
}


uint32_t SkPicture::uniqueID() const {
    if (SK_InvalidGenID == fUniqueID) {
        fUniqueID = next_picture_generation_id();
    }
    return fUniqueID;
}


SkPicture::SkPicture(int width, int height, SkRecord* record)
    : fWidth(width)
    , fHeight(height)
    , fRecord(record)
    , fRecordWillPlayBackBitmaps(SkRecordWillPlaybackBitmaps(*record)) {
    this->needsNewGenID();
}





void SkPicture::addDeletionListener(DeletionListener* listener) const {
    SkASSERT(NULL != listener);

    *fDeletionListeners.append() = SkRef(listener);
}

void SkPicture::callDeletionListeners() {
    for (int i = 0; i < fDeletionListeners.count(); ++i) {
        fDeletionListeners[i]->onDeletion(this->uniqueID());
    }

    fDeletionListeners.unrefAll();
}
