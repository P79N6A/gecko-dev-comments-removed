








#ifndef SkWriter32_DEFINED
#define SkWriter32_DEFINED

#include "SkData.h"
#include "SkMatrix.h"
#include "SkPath.h"
#include "SkPoint.h"
#include "SkRRect.h"
#include "SkRect.h"
#include "SkRegion.h"
#include "SkScalar.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkTypes.h"

class SK_API SkWriter32 : SkNoncopyable {
public:
    






    SkWriter32(void* external = NULL, size_t externalBytes = 0) {
        this->reset(external, externalBytes);
    }

    
    size_t bytesWritten() const { return fUsed; }

    SK_ATTR_DEPRECATED("use bytesWritten")
    size_t size() const { return this->bytesWritten(); }

    void reset(void* external = NULL, size_t externalBytes = 0) {
        SkASSERT(SkIsAlign4((uintptr_t)external));
        SkASSERT(SkIsAlign4(externalBytes));

        fSnapshot.reset(NULL);
        fData = (uint8_t*)external;
        fCapacity = externalBytes;
        fUsed = 0;
        fExternal = external;
    }

    
    
    const uint32_t* contiguousArray() const {
        return (uint32_t*)fData;
    }

    
    uint32_t* reserve(size_t size) {
        SkASSERT(SkAlign4(size) == size);
        size_t offset = fUsed;
        size_t totalRequired = fUsed + size;
        if (totalRequired > fCapacity) {
            this->growToAtLeast(totalRequired);
        }
        fUsed = totalRequired;
        return (uint32_t*)(fData + offset);
    }

    



    template<typename T>
    const T& readTAt(size_t offset) const {
        SkASSERT(SkAlign4(offset) == offset);
        SkASSERT(offset < fUsed);
        return *(T*)(fData + offset);
    }

    



    template<typename T>
    void overwriteTAt(size_t offset, const T& value) {
        SkASSERT(SkAlign4(offset) == offset);
        SkASSERT(offset < fUsed);
        SkASSERT(fSnapshot.get() == NULL);
        *(T*)(fData + offset) = value;
    }

    bool writeBool(bool value) {
        this->write32(value);
        return value;
    }

    void writeInt(int32_t value) {
        this->write32(value);
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

    void writePtr(void* value) {
        *(void**)this->reserve(sizeof(value)) = value;
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

    void writeIRect(const SkIRect& rect) {
        *(SkIRect*)this->reserve(sizeof(rect)) = rect;
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

    



    uint32_t* reservePad(size_t size) {
        size_t alignedSize = SkAlign4(size);
        uint32_t* p = this->reserve(alignedSize);
        if (alignedSize != size) {
            SkASSERT(alignedSize >= 4);
            p[alignedSize / 4 - 1] = 0;
        }
        return p;
    }

    


    void writePad(const void* src, size_t size) {
        memcpy(this->reservePad(size), src, size);
    }

    







    void writeString(const char* str, size_t len = (size_t)-1);

    




    static size_t WriteStringSize(const char* str, size_t len = (size_t)-1);

    



    void rewindToOffset(size_t offset) {
        SkASSERT(SkAlign4(offset) == offset);
        SkASSERT(offset <= bytesWritten());
        fUsed = offset;
    }

    
    void flatten(void* dst) const {
        memcpy(dst, fData, fUsed);
    }

    bool writeToStream(SkWStream* stream) const {
        return stream->write(fData, fUsed);
    }

    
    
    size_t readFromStream(SkStream* stream, size_t length) {
        return stream->read(this->reservePad(length), length);
    }

    










    SkData* snapshotAsData() const;
private:
    void growToAtLeast(size_t size);

    uint8_t* fData;                    
    size_t fCapacity;                  
    size_t fUsed;                      
    void* fExternal;                   
    SkAutoTMalloc<uint8_t> fInternal;  
    SkAutoTUnref<SkData> fSnapshot;    
};







template <size_t SIZE> class SkSWriter32 : public SkWriter32 {
public:
    SkSWriter32() { this->reset(); }

    void reset() {this->INHERITED::reset(fData.fStorage, SIZE); }

private:
    union {
        void*   fPtrAlignment;
        double  fDoubleAlignment;
        char    fStorage[SIZE];
    } fData;

    typedef SkWriter32 INHERITED;
};

#endif
