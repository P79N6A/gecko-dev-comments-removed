








#ifndef SkWriter32_DEFINED
#define SkWriter32_DEFINED

#include "SkTypes.h"

#include "SkScalar.h"
#include "SkPath.h"
#include "SkPoint.h"
#include "SkRect.h"
#include "SkRRect.h"
#include "SkMatrix.h"
#include "SkRegion.h"

class SkStream;
class SkWStream;

class SkWriter32 : SkNoncopyable {
    struct BlockHeader;
public:
    





    SkWriter32(size_t minSize, void* initialStorage, size_t storageSize);

    SkWriter32(size_t minSize)
        : fHead(NULL)
        , fTail(NULL)
        , fMinSize(minSize)
        , fSize(0)
        , fWrittenBeforeLastBlock(0)
        {}

    ~SkWriter32();

    
    uint32_t bytesWritten() const { return fSize; }
    
    uint32_t  size() const { return this->bytesWritten(); }

    void      reset();

    
    uint32_t* reserve(size_t size) {
        SkASSERT(SkAlign4(size) == size);

        Block* block = fTail;
        if (NULL == block || block->available() < size) {
            block = this->doReserve(size);
        }
        fSize += size;
        return block->alloc(size);
    }

    void reset(void* storage, size_t size);

    bool writeBool(bool value) {
        this->writeInt(value);
        return value;
    }

    void writeInt(int32_t value) {
        *(int32_t*)this->reserve(sizeof(value)) = value;
    }

    void write8(int32_t value) {
        *(int32_t*)this->reserve(sizeof(value)) = value & 0xFF;
    }

    void write16(int32_t value) {
        *(int32_t*)this->reserve(sizeof(value)) = value & 0xFFFF;
    }

    void write32(int32_t value) {
        *(int32_t*)this->reserve(sizeof(value)) = value;
    }

    void writePtr(void* ptr) {
        
        
        int32_t* addr = (int32_t*)this->reserve(sizeof(void*));
        if (4 == sizeof(void*)) {
            *(void**)addr = ptr;
        } else {
            memcpy(addr, &ptr, sizeof(void*));
        }
    }

    void writeScalar(SkScalar value) {
        *(SkScalar*)this->reserve(sizeof(value)) = value;
    }

    void writePoint(const SkPoint& pt) {
        *(SkPoint*)this->reserve(sizeof(pt)) = pt;
    }

    void writeRect(const SkRect& rect) {
        *(SkRect*)this->reserve(sizeof(rect)) = rect;
    }

    void writeRRect(const SkRRect& rrect) {
        rrect.writeToMemory(this->reserve(SkRRect::kSizeInMemory));
    }

    void writePath(const SkPath& path) {
        size_t size = path.writeToMemory(NULL);
        SkASSERT(SkAlign4(size) == size);
        path.writeToMemory(this->reserve(size));
    }

    void writeMatrix(const SkMatrix& matrix) {
        size_t size = matrix.writeToMemory(NULL);
        SkASSERT(SkAlign4(size) == size);
        matrix.writeToMemory(this->reserve(size));
    }

    void writeRegion(const SkRegion& rgn) {
        size_t size = rgn.writeToMemory(NULL);
        SkASSERT(SkAlign4(size) == size);
        rgn.writeToMemory(this->reserve(size));
    }

    
    void writeMul4(const void* values, size_t size) {
        this->write(values, size);
    }

    



    void write(const void* values, size_t size) {
        SkASSERT(SkAlign4(size) == size);
        
        
        
        memcpy(this->reserve(size), values, size);
    }

    



    uint32_t* reservePad(size_t size);

    


    void writePad(const void* src, size_t size);

    





    void writeString(const char* str, size_t len = (size_t)-1);

    




    static size_t WriteStringSize(const char* str, size_t len = (size_t)-1);

    
    
    
    uint32_t* peek32(size_t offset);

    




    void rewindToOffset(size_t offset);

    
    void flatten(void* dst) const;

    
    
    size_t readFromStream(SkStream*, size_t length);

    bool writeToStream(SkWStream*);

private:
    struct Block {
        Block*  fNext;
        char*   fBasePtr;
        size_t  fSizeOfBlock;      
        size_t  fAllocatedSoFar;    

        size_t  available() const { return fSizeOfBlock - fAllocatedSoFar; }
        char*   base() { return fBasePtr; }
        const char* base() const { return fBasePtr; }

        uint32_t* alloc(size_t size) {
            SkASSERT(SkAlign4(size) == size);
            SkASSERT(this->available() >= size);
            void* ptr = this->base() + fAllocatedSoFar;
            fAllocatedSoFar += size;
            SkASSERT(fAllocatedSoFar <= fSizeOfBlock);
            return (uint32_t*)ptr;
        }

        uint32_t* peek32(size_t offset) {
            SkASSERT(offset <= fAllocatedSoFar + 4);
            void* ptr = this->base() + offset;
            return (uint32_t*)ptr;
        }

        void rewind() {
            fNext = NULL;
            fAllocatedSoFar = 0;
            
        }

        static Block* Create(size_t size) {
            SkASSERT(SkIsAlign4(size));
            Block* block = (Block*)sk_malloc_throw(sizeof(Block) + size);
            block->fNext = NULL;
            block->fBasePtr = (char*)(block + 1);
            block->fSizeOfBlock = size;
            block->fAllocatedSoFar = 0;
            return block;
        }

        Block* initFromStorage(void* storage, size_t size) {
            SkASSERT(SkIsAlign4((intptr_t)storage));
            SkASSERT(SkIsAlign4(size));
            Block* block = this;
            block->fNext = NULL;
            block->fBasePtr = (char*)storage;
            block->fSizeOfBlock = size;
            block->fAllocatedSoFar = 0;
            return block;
        }
    };

    enum {
        MIN_BLOCKSIZE = sizeof(SkWriter32::Block) + sizeof(intptr_t)
    };

    Block       fExternalBlock;
    Block*      fHead;
    Block*      fTail;
    size_t      fMinSize;
    uint32_t    fSize;
    
    uint32_t    fWrittenBeforeLastBlock;

    bool isHeadExternallyAllocated() const {
        return fHead == &fExternalBlock;
    }

    Block* newBlock(size_t bytes);

    
    Block* doReserve(size_t bytes);

    SkDEBUGCODE(void validate() const;)
};







template <size_t SIZE> class SkSWriter32 : public SkWriter32 {
public:
    SkSWriter32(size_t minSize) : SkWriter32(minSize, fData.fStorage, SIZE) {}

private:
    union {
        void*   fPtrAlignment;
        double  fDoubleAlignment;
        char    fStorage[SIZE];
    } fData;
};

#endif
