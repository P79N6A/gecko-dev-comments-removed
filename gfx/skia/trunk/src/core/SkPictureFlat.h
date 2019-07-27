






#ifndef SkPictureFlat_DEFINED
#define SkPictureFlat_DEFINED



#include "SkBitmapHeap.h"
#include "SkChecksum.h"
#include "SkChunkAlloc.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkPaint.h"
#include "SkPicture.h"
#include "SkPtrRecorder.h"
#include "SkTDynamicHash.h"
#include "SkTRefArray.h"

enum DrawType {
    UNUSED,
    CLIP_PATH,
    CLIP_REGION,
    CLIP_RECT,
    CLIP_RRECT,
    CONCAT,
    DRAW_BITMAP,
    DRAW_BITMAP_MATRIX,
    DRAW_BITMAP_NINE,
    DRAW_BITMAP_RECT_TO_RECT,
    DRAW_CLEAR,
    DRAW_DATA,
    DRAW_OVAL,
    DRAW_PAINT,
    DRAW_PATH,
    DRAW_PICTURE,
    DRAW_POINTS,
    DRAW_POS_TEXT,
    DRAW_POS_TEXT_TOP_BOTTOM, 
    DRAW_POS_TEXT_H,
    DRAW_POS_TEXT_H_TOP_BOTTOM, 
    DRAW_RECT,
    DRAW_RRECT,
    DRAW_SPRITE,
    DRAW_TEXT,
    DRAW_TEXT_ON_PATH,
    DRAW_TEXT_TOP_BOTTOM,   
    DRAW_VERTICES,
    RESTORE,
    ROTATE,
    SAVE,
    SAVE_LAYER,
    SCALE,
    SET_MATRIX,
    SKEW,
    TRANSLATE,
    NOOP,
    BEGIN_COMMENT_GROUP,
    COMMENT,
    END_COMMENT_GROUP,

    
    DRAW_DRRECT,
    PUSH_CULL,
    POP_CULL,

    LAST_DRAWTYPE_ENUM = POP_CULL
};


static const int kDRAW_BITMAP_FLAVOR = LAST_DRAWTYPE_ENUM+1;

enum DrawVertexFlags {
    DRAW_VERTICES_HAS_TEXS    = 0x01,
    DRAW_VERTICES_HAS_COLORS  = 0x02,
    DRAW_VERTICES_HAS_INDICES = 0x04,
    DRAW_VERTICES_HAS_XFER    = 0x08,
};





static inline uint32_t ClipParams_pack(SkRegion::Op op, bool doAA) {
    unsigned doAABit = doAA ? 1 : 0;
    return (doAABit << 4) | op;
}

static inline SkRegion::Op ClipParams_unpackRegionOp(uint32_t packed) {
    return (SkRegion::Op)(packed & 0xF);
}

static inline bool ClipParams_unpackDoAA(uint32_t packed) {
    return SkToBool((packed >> 4) & 1);
}



class SkTypefacePlayback {
public:
    SkTypefacePlayback();
    virtual ~SkTypefacePlayback();

    int count() const { return fCount; }

    void reset(const SkRefCntSet*);

    void setCount(int count);
    SkRefCnt* set(int index, SkRefCnt*);

    void setupBuffer(SkReadBuffer& buffer) const {
        buffer.setTypefaceArray((SkTypeface**)fArray, fCount);
    }

protected:
    int fCount;
    SkRefCnt** fArray;
};

class SkFactoryPlayback {
public:
    SkFactoryPlayback(int count) : fCount(count) {
        fArray = SkNEW_ARRAY(SkFlattenable::Factory, count);
    }

    ~SkFactoryPlayback() {
        SkDELETE_ARRAY(fArray);
    }

    SkFlattenable::Factory* base() const { return fArray; }

    void setupBuffer(SkReadBuffer& buffer) const {
        buffer.setFactoryPlayback(fArray, fCount);
    }

private:
    int fCount;
    SkFlattenable::Factory* fArray;
};


























class SkFlatData;

class SkFlatController : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkFlatController)

    SkFlatController(uint32_t writeBufferFlags = 0);
    virtual ~SkFlatController();
    




    virtual void* allocThrow(size_t bytes) = 0;

    



    virtual void unalloc(void* ptr) = 0;

    





    SkBitmapHeap* getBitmapHeap() { return fBitmapHeap; }

    




    SkRefCntSet* getTypefaceSet() { return fTypefaceSet; }

    




    SkTypefacePlayback* getTypefacePlayback() { return fTypefacePlayback; }

    



    SkNamedFactorySet* getNamedFactorySet() { return fFactorySet; }

    


    uint32_t getWriteBufferFlags() { return fWriteBufferFlags; }

protected:
    


    void setBitmapHeap(SkBitmapHeap*);

    



    void setTypefaceSet(SkRefCntSet*);

    




    void setTypefacePlayback(SkTypefacePlayback*);

    




    SkNamedFactorySet* setNamedFactorySet(SkNamedFactorySet*);

private:
    SkBitmapHeap*       fBitmapHeap;
    SkRefCntSet*        fTypefaceSet;
    SkTypefacePlayback* fTypefacePlayback;
    SkNamedFactorySet*  fFactorySet;
    const uint32_t      fWriteBufferFlags;

    typedef SkRefCnt INHERITED;
};

class SkFlatData {
public:
    
    template <typename Traits, typename T>
    static SkFlatData* Create(SkFlatController* controller, const T& obj, int index) {
        
        uint32_t storage[64];
        SkWriteBuffer buffer(storage, sizeof(storage), controller->getWriteBufferFlags());

        buffer.setBitmapHeap(controller->getBitmapHeap());
        buffer.setTypefaceRecorder(controller->getTypefaceSet());
        buffer.setNamedFactoryRecorder(controller->getNamedFactorySet());

        Traits::Flatten(buffer, obj);
        size_t size = buffer.bytesWritten();
        SkASSERT(SkIsAlign4(size));

        
        size_t allocSize = sizeof(SkFlatData) + size;
        SkFlatData* result = (SkFlatData*) controller->allocThrow(allocSize);

        
        buffer.writeToMemory(result->data());
        
        result->stampHeader(index, SkToS32(size));
        return result;
    }

    
    template <typename Traits, typename T>
    void unflatten(T* result,
                   SkBitmapHeap* bitmapHeap = NULL,
                   SkTypefacePlayback* facePlayback = NULL) const {
        SkReadBuffer buffer(this->data(), fFlatSize);

        if (bitmapHeap) {
            buffer.setBitmapStorage(bitmapHeap);
        }
        if (facePlayback) {
            facePlayback->setupBuffer(buffer);
        }

        Traits::Unflatten(buffer, result);
        SkASSERT(fFlatSize == (int32_t)buffer.offset());
    }

    
    bool operator==(const SkFlatData& that) const {
        if (this->checksum() != that.checksum() || this->flatSize() != that.flatSize()) {
            return false;
        }
        return memcmp(this->data(), that.data(), this->flatSize()) == 0;
    }

    int index() const { return fIndex; }
    const uint8_t* data() const { return (const uint8_t*)this + sizeof(*this); }
    size_t flatSize() const { return fFlatSize; }
    uint32_t checksum() const { return fChecksum; }

    
    bool isTopBotWritten() const {
        return !SkScalarIsNaN(fTopBot[0]);
    }

    
    
    SkScalar* writableTopBot() const {
        SkASSERT(!this->isTopBotWritten());
        return fTopBot;
    }

    
    const SkScalar* topBot() const {
        SkASSERT(this->isTopBotWritten());
        return fTopBot;
    }

private:
    struct HashTraits {
        static const SkFlatData& GetKey(const SkFlatData& flat) { return flat; }
        static uint32_t Hash(const SkFlatData& flat) { return flat.checksum(); }
    };

    void setIndex(int index) { fIndex = index; }
    uint8_t* data() { return (uint8_t*)this + sizeof(*this); }

    
    void stampHeader(int index, int32_t size) {
        SkASSERT(SkIsAlign4(size));
        fIndex     = index;
        fFlatSize  = size;
        fTopBot[0] = SK_ScalarNaN;  
        fChecksum  = SkChecksum::Compute((uint32_t*)this->data(), size);
    }

    int fIndex;
    int32_t fFlatSize;
    uint32_t fChecksum;
    mutable SkScalar fTopBot[2];  
    

    template <typename T, typename Traits> friend class SkFlatDictionary;
};

template <typename T, typename Traits>
class SkFlatDictionary {
public:
    explicit SkFlatDictionary(SkFlatController* controller)
    : fController(SkRef(controller))
    , fScratch(controller->getWriteBufferFlags())
    , fReady(false) {
        this->reset();
    }

    



    void reset() {
        fIndexedData.rewind();
    }

    int count() const {
        SkASSERT(fHash.count() == fIndexedData.count());
        return fHash.count();
    }

    
    const SkFlatData* operator[](int index) {
        return fIndexedData[index];
    }

    





    int find(const T& element) {
        return this->findAndReturnFlat(element)->index();
    }

    






    const SkFlatData* findAndReplace(const T& element,
                                     const SkFlatData* toReplace,
                                     bool* added,
                                     bool* replaced) {
        SkASSERT(added != NULL && replaced != NULL);

        const int oldCount = this->count();
        SkFlatData* flat = this->findAndReturnMutableFlat(element);
        *added = this->count() > oldCount;

        
        if (!*added || toReplace == NULL) {
            *replaced = false;
            return flat;
        }

        
        const SkFlatData* found = fHash.find(*toReplace);
        if (found == NULL) {
            *replaced = false;
            return flat;
        }

        
        
        SkASSERT(flat->index() == this->count());
        flat->setIndex(found->index());
        fIndexedData.removeShuffle(found->index()-1);
        SkASSERT(flat == fIndexedData[found->index()-1]);

        
        fHash.remove(*found);
        fController->unalloc((void*)found);
        SkASSERT(this->count() == oldCount);

        *replaced = true;
        return flat;
    }

    



    SkTRefArray<T>* unflattenToArray() const {
        const int count = this->count();
        if (count == 0) {
            return NULL;
        }
        SkTRefArray<T>* array = SkTRefArray<T>::Create(count);
        for (int i = 0; i < count; i++) {
            this->unflatten(&array->writableAt(i), fIndexedData[i]);
        }
        return array;
    }

    



    T* unflatten(int index) const {
        
        const SkFlatData* element = fIndexedData[index-1];
        SkASSERT(index == element->index());

        T* dst = new T;
        this->unflatten(dst, element);
        return dst;
    }

    



    const SkFlatData* findAndReturnFlat(const T& element) {
        return this->findAndReturnMutableFlat(element);
    }

private:
    
    
    void lazyInit() {
        if (fReady) {
            return;
        }

        
        SkASSERT(fController->getBitmapHeap() != NULL);
        fScratch.setBitmapHeap(fController->getBitmapHeap());
        fScratch.setTypefaceRecorder(fController->getTypefaceSet());
        fScratch.setNamedFactoryRecorder(fController->getNamedFactorySet());
        fReady = true;
    }

    
    SkFlatData* findAndReturnMutableFlat(const T& element) {
        
        const SkFlatData& scratch = this->resetScratch(element, this->count()+1);

        SkFlatData* candidate = fHash.find(scratch);
        if (candidate != NULL) {
            return candidate;
        }

        SkFlatData* detached = this->detachScratch();
        fHash.add(detached);
        *fIndexedData.append() = detached;
        SkASSERT(fIndexedData.top()->index() == this->count());
        return detached;
    }

    
    const SkFlatData& resetScratch(const T& element, int index) {
        this->lazyInit();

        
        fScratch.reset();
        fScratch.reserve(sizeof(SkFlatData));
        Traits::Flatten(fScratch, element);
        const size_t dataSize = fScratch.bytesWritten() - sizeof(SkFlatData);

        
        SkFlatData* scratch = (SkFlatData*)fScratch.getWriter32()->contiguousArray();
        SkASSERT(scratch != NULL);
        scratch->stampHeader(index, SkToS32(dataSize));
        return *scratch;
    }

    
    SkFlatData* detachScratch() {
        
        
        
        SkFlatData* detached = (SkFlatData*)fController->allocThrow(fScratch.bytesWritten());

        
        SkFlatData* scratch = (SkFlatData*)fScratch.getWriter32()->contiguousArray();
        SkASSERT(scratch != NULL);
        memcpy(detached, scratch, fScratch.bytesWritten());

        
        return detached;
    }

    void unflatten(T* dst, const SkFlatData* element) const {
        element->unflatten<Traits>(dst,
                                   fController->getBitmapHeap(),
                                   fController->getTypefacePlayback());
    }

    
    SkAutoTUnref<SkFlatController> fController;
    SkWriteBuffer fScratch;
    bool fReady;

    
    SkTDArray<const SkFlatData*> fIndexedData;

    
    SkTDynamicHash<SkFlatData, SkFlatData, SkFlatData::HashTraits> fHash;
};

typedef SkFlatDictionary<SkPaint, SkPaint::FlatteningTraits> SkPaintDictionary;

class SkChunkFlatController : public SkFlatController {
public:
    SkChunkFlatController(size_t minSize)
    : fHeap(minSize)
    , fTypefaceSet(SkNEW(SkRefCntSet))
    , fLastAllocated(NULL) {
        this->setTypefaceSet(fTypefaceSet);
        this->setTypefacePlayback(&fTypefacePlayback);
    }

    virtual void* allocThrow(size_t bytes) SK_OVERRIDE {
        fLastAllocated = fHeap.allocThrow(bytes);
        return fLastAllocated;
    }

    virtual void unalloc(void* ptr) SK_OVERRIDE {
        
        
        if (ptr == fLastAllocated) (void)fHeap.unalloc(ptr);
    }

    void setupPlaybacks() const {
        fTypefacePlayback.reset(fTypefaceSet.get());
    }

    void setBitmapStorage(SkBitmapHeap* heap) {
        this->setBitmapHeap(heap);
    }

private:
    SkChunkAlloc               fHeap;
    SkAutoTUnref<SkRefCntSet>  fTypefaceSet;
    void*                      fLastAllocated;
    mutable SkTypefacePlayback fTypefacePlayback;
};

#endif
