






#ifndef SkPictureFlat_DEFINED
#define SkPictureFlat_DEFINED



#include "SkChunkAlloc.h"
#include "SkBitmap.h"
#include "SkBitmapHeap.h"
#include "SkOrderedReadBuffer.h"
#include "SkOrderedWriteBuffer.h"
#include "SkPicture.h"
#include "SkPtrRecorder.h"
#include "SkMatrix.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkRegion.h"
#include "SkTRefArray.h"
#include "SkTSearch.h"

enum DrawType {
    UNUSED,
    CLIP_PATH,
    CLIP_REGION,
    CLIP_RECT,
    CONCAT,
    DRAW_BITMAP,
    DRAW_BITMAP_MATRIX,
    DRAW_BITMAP_NINE,
    DRAW_BITMAP_RECT,
    DRAW_CLEAR,
    DRAW_DATA,
    DRAW_PAINT,
    DRAW_PATH,
    DRAW_PICTURE,
    DRAW_POINTS,
    DRAW_POS_TEXT,
    DRAW_POS_TEXT_TOP_BOTTOM, 
    DRAW_POS_TEXT_H,
    DRAW_POS_TEXT_H_TOP_BOTTOM, 
    DRAW_RECT,
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

    LAST_DRAWTYPE_ENUM = TRANSLATE
};

enum DrawVertexFlags {
    DRAW_VERTICES_HAS_TEXS    = 0x01,
    DRAW_VERTICES_HAS_COLORS  = 0x02,
    DRAW_VERTICES_HAS_INDICES = 0x04
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

    void setupBuffer(SkOrderedReadBuffer& buffer) const {
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

    void setupBuffer(SkOrderedReadBuffer& buffer) const {
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

    SkFlatController();
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

    


    void setWriteBufferFlags(uint32_t flags) { fWriteBufferFlags = flags; }

private:
    SkBitmapHeap*       fBitmapHeap;
    SkRefCntSet*        fTypefaceSet;
    SkTypefacePlayback* fTypefacePlayback;
    SkNamedFactorySet*  fFactorySet;
    uint32_t            fWriteBufferFlags;

    typedef SkRefCnt INHERITED;
};

class SkFlatData {
public:
    















    static int Compare(const SkFlatData* a, const SkFlatData* b) {
        const uint32_t* stop = a->dataStop();
        const uint32_t* a_ptr = a->dataToCompare() - 1;
        const uint32_t* b_ptr = b->dataToCompare() - 1;
        
        while (*++a_ptr == *++b_ptr) {}

        if (a_ptr == stop) {    
            SkASSERT(b->dataStop() == b_ptr);
            return 0;
        }
        SkASSERT(a_ptr < a->dataStop());
        SkASSERT(b_ptr < b->dataStop());
        return (*a_ptr < *b_ptr) ? -1 : 1;
    }

    int index() const { return fIndex; }
    const void* data() const { return (const char*)this + sizeof(*this); }
    void* data() { return (char*)this + sizeof(*this); }
    
    uint32_t* data32() { return (uint32_t*)this->data(); }
    
    size_t flatSize() const { return fFlatSize; }

    void setSentinelInCache() {
        this->setSentinel(kInCache_Sentinel);
    }
    void setSentinelAsCandidate() {
        this->setSentinel(kCandidate_Sentinel);
    }

    uint32_t checksum() const { return fChecksum; }

#ifdef SK_DEBUG_SIZE
    
    
    size_t size() const {
        return sizeof(SkFlatData) + fFlatSize;
    }
#endif

    static SkFlatData* Create(SkFlatController* controller, const void* obj, int index,
                              void (*flattenProc)(SkOrderedWriteBuffer&, const void*));

    void unflatten(void* result,
                   void (*unflattenProc)(SkOrderedReadBuffer&, void*),
                   SkBitmapHeap* bitmapHeap = NULL,
                   SkTypefacePlayback* facePlayback = NULL) const;

    
    
    void setIndex(int index) { fIndex = index; }

    
    friend bool operator==(const SkFlatData& a, const SkFlatData& b) {
        size_t N = (const char*)a.dataStop() - (const char*)a.dataToCompare();
        return !memcmp(a.dataToCompare(), b.dataToCompare(), N);
    }

private:
    int fIndex;

    
    
    uint32_t fChecksum;
    int32_t  fFlatSize;  
    
    

    const uint32_t* dataToCompare() const {
        return (const uint32_t*)&fChecksum;
    }
    const uint32_t* dataStop() const {
        SkASSERT(SkIsAlign4(fFlatSize));
        return (const uint32_t*)((const char*)this->data() + fFlatSize);
    }

    enum {
        kInCache_Sentinel = 0,
        kCandidate_Sentinel = ~0U,
    };
    void setSentinel(uint32_t value) {
        SkASSERT(SkIsAlign4(fFlatSize));
        this->data32()[fFlatSize >> 2] = value;
    }
};

template <class T>
class SkFlatDictionary {
public:
    SkFlatDictionary(SkFlatController* controller)
    : fController(controller) {
        fFlattenProc = NULL;
        fUnflattenProc = NULL;
        SkASSERT(controller);
        fController->ref();
        
        fNextIndex = 1;
        sk_bzero(fHash, sizeof(fHash));
    }

    virtual ~SkFlatDictionary() {
        fController->unref();
    }

    int count() const { return fData.count(); }

    const SkFlatData*  operator[](int index) const {
        SkASSERT(index >= 0 && index < fData.count());
        return fData[index];
    }

    



    void reset() {
        fData.reset();
        fNextIndex = 1;
        sk_bzero(fHash, sizeof(fHash));
    }

    






    const SkFlatData* findAndReplace(const T& element,
                                     const SkFlatData* toReplace, bool* added,
                                     bool* replaced) {
        SkASSERT(added != NULL && replaced != NULL);
        int oldCount = fData.count();
        const SkFlatData* flat = this->findAndReturnFlat(element);
        *added = fData.count() == oldCount + 1;
        *replaced = false;
        if (*added && toReplace != NULL) {
            
            int indexToReplace = fData.find(toReplace);
            if (indexToReplace >= 0) {
                
                
                
                const_cast<SkFlatData*>(flat)->setIndex(toReplace->index());
                fNextIndex--;
                
                fData.remove(indexToReplace);
                
                int oldHash = ChecksumToHashIndex(toReplace->checksum());
                if (fHash[oldHash] == toReplace) {
                    fHash[oldHash] = NULL;
                }
                
                fController->unalloc((void*)toReplace);
                *replaced = true;
            }
        }
        return flat;
    }

    











    int find(const T& element) {
        return this->findAndReturnFlat(element)->index();
    }

    




    int unflattenDictionary(T*& array) const {
        int elementCount = fData.count();
        if (elementCount > 0) {
            array = SkNEW_ARRAY(T, elementCount);
            this->unflattenIntoArray(array);
        }
        return elementCount;
    }

    



    SkTRefArray<T>* unflattenToArray() const {
        int count = fData.count();
        SkTRefArray<T>* array = NULL;
        if (count > 0) {
            array = SkTRefArray<T>::Create(count);
            this->unflattenIntoArray(&array->writableAt(0));
        }
        return array;
    }

protected:
    void (*fFlattenProc)(SkOrderedWriteBuffer&, const void*);
    void (*fUnflattenProc)(SkOrderedReadBuffer&, void*);

private:
    void unflattenIntoArray(T* array) const {
        const int count = fData.count();
        const SkFlatData** iter = fData.begin();
        for (int i = 0; i < count; ++i) {
            const SkFlatData* element = iter[i];
            int index = element->index() - 1;
            SkASSERT((unsigned)index < (unsigned)count);
            element->unflatten(&array[index], fUnflattenProc,
                               fController->getBitmapHeap(),
                               fController->getTypefacePlayback());
        }
    }


    SkFlatController * const     fController;
    int                          fNextIndex;
    SkTDArray<const SkFlatData*> fData;

    const SkFlatData* findAndReturnFlat(const T& element) {
        SkFlatData* flat = SkFlatData::Create(fController, &element, fNextIndex, fFlattenProc);

        int hashIndex = ChecksumToHashIndex(flat->checksum());
        const SkFlatData* candidate = fHash[hashIndex];
        if (candidate && !SkFlatData::Compare(flat, candidate)) {
            fController->unalloc(flat);
            return candidate;
        }

        int index = SkTSearch<SkFlatData>((const SkFlatData**) fData.begin(),
                                          fData.count(), flat, sizeof(flat),
                                          &SkFlatData::Compare);
        if (index >= 0) {
            fController->unalloc(flat);
            fHash[hashIndex] = fData[index];
            return fData[index];
        }

        index = ~index;
        *fData.insert(index) = flat;
        SkASSERT(fData.count() == fNextIndex);
        fNextIndex++;
        flat->setSentinelInCache();
        fHash[hashIndex] = flat;
        return flat;
    }


    enum {
        
        
        
        
        HASH_BITS   = 7,
        HASH_MASK   = (1 << HASH_BITS) - 1,
        HASH_COUNT  = 1 << HASH_BITS
    };
    const SkFlatData* fHash[HASH_COUNT];

    static int ChecksumToHashIndex(uint32_t checksum) {
        int n = checksum;
        if (HASH_BITS < 32) {
            n ^= n >> 16;
        }
        if (HASH_BITS < 16) {
            n ^= n >> 8;
        }
        if (HASH_BITS < 8) {
            n ^= n >> 4;
        }
        return n & HASH_MASK;
    }
};





template <class T>
static void SkFlattenObjectProc(SkOrderedWriteBuffer& buffer, const void* obj) {
    ((T*)obj)->flatten(buffer);
}

template <class T>
static void SkUnflattenObjectProc(SkOrderedReadBuffer& buffer, void* obj) {
    ((T*)obj)->unflatten(buffer);
}

class SkChunkFlatController : public SkFlatController {
public:
    SkChunkFlatController(size_t minSize)
    : fHeap(minSize)
    , fTypefaceSet(SkNEW(SkRefCntSet)) {
        this->setTypefaceSet(fTypefaceSet);
        this->setTypefacePlayback(&fTypefacePlayback);
    }

    ~SkChunkFlatController() {
        fTypefaceSet->unref();
    }

    virtual void* allocThrow(size_t bytes) SK_OVERRIDE {
        return fHeap.allocThrow(bytes);
    }

    virtual void unalloc(void* ptr) SK_OVERRIDE {
        (void) fHeap.unalloc(ptr);
    }

    void setupPlaybacks() const {
        fTypefacePlayback.reset(fTypefaceSet);
    }

    void setBitmapStorage(SkBitmapHeap* heap) {
        this->setBitmapHeap(heap);
    }

private:
    SkChunkAlloc               fHeap;
    SkRefCntSet*               fTypefaceSet;
    mutable SkTypefacePlayback fTypefacePlayback;
};

class SkBitmapDictionary : public SkFlatDictionary<SkBitmap> {
public:
    SkBitmapDictionary(SkFlatController* controller)
    : SkFlatDictionary<SkBitmap>(controller) {
        fFlattenProc = &SkFlattenObjectProc<SkBitmap>;
        fUnflattenProc = &SkUnflattenObjectProc<SkBitmap>;
    }
};

class SkMatrixDictionary : public SkFlatDictionary<SkMatrix> {
 public:
    SkMatrixDictionary(SkFlatController* controller)
    : SkFlatDictionary<SkMatrix>(controller) {
        fFlattenProc = &flattenMatrix;
        fUnflattenProc = &unflattenMatrix;
    }

    static void flattenMatrix(SkOrderedWriteBuffer& buffer, const void* obj) {
        buffer.getWriter32()->writeMatrix(*(SkMatrix*)obj);
    }

    static void unflattenMatrix(SkOrderedReadBuffer& buffer, void* obj) {
        buffer.getReader32()->readMatrix((SkMatrix*)obj);
    }
};

class SkPaintDictionary : public SkFlatDictionary<SkPaint> {
 public:
    SkPaintDictionary(SkFlatController* controller)
    : SkFlatDictionary<SkPaint>(controller) {
        fFlattenProc = &SkFlattenObjectProc<SkPaint>;
        fUnflattenProc = &SkUnflattenObjectProc<SkPaint>;
    }
};

class SkRegionDictionary : public SkFlatDictionary<SkRegion> {
 public:
    SkRegionDictionary(SkFlatController* controller)
    : SkFlatDictionary<SkRegion>(controller) {
        fFlattenProc = &flattenRegion;
        fUnflattenProc = &unflattenRegion;
    }

    static void flattenRegion(SkOrderedWriteBuffer& buffer, const void* obj) {
        buffer.getWriter32()->writeRegion(*(SkRegion*)obj);
    }

    static void unflattenRegion(SkOrderedReadBuffer& buffer, void* obj) {
        buffer.getReader32()->readRegion((SkRegion*)obj);
    }
};

#endif
