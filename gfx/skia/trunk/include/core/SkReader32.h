








#ifndef SkReader32_DEFINED
#define SkReader32_DEFINED

#include "SkMatrix.h"
#include "SkPath.h"
#include "SkRegion.h"
#include "SkRRect.h"
#include "SkScalar.h"

class SkString;

class SkReader32 : SkNoncopyable {
public:
    SkReader32() : fCurr(NULL), fStop(NULL), fBase(NULL) {}
    SkReader32(const void* data, size_t size)  {
        this->setMemory(data, size);
    }

    void setMemory(const void* data, size_t size) {
        SkASSERT(ptr_align_4(data));
        SkASSERT(SkAlign4(size) == size);

        fBase = fCurr = (const char*)data;
        fStop = (const char*)data + size;
    }

    size_t size() const { return fStop - fBase; }
    size_t offset() const { return fCurr - fBase; }
    bool eof() const { return fCurr >= fStop; }
    const void* base() const { return fBase; }
    const void* peek() const { return fCurr; }

    size_t available() const { return fStop - fCurr; }
    bool isAvailable(size_t size) const { return size <= this->available(); }

    void rewind() { fCurr = fBase; }

    void setOffset(size_t offset) {
        SkASSERT(SkAlign4(offset) == offset);
        SkASSERT(offset <= this->size());
        fCurr = fBase + offset;
    }

    bool readBool() { return this->readInt() != 0; }

    int32_t readInt() {
        SkASSERT(ptr_align_4(fCurr));
        int32_t value = *(const int32_t*)fCurr;
        fCurr += sizeof(value);
        SkASSERT(fCurr <= fStop);
        return value;
    }

    void* readPtr() {
        void* ptr;
        
        if (4 == sizeof(void*)) {
            ptr = *(void**)fCurr;
        } else {
            memcpy(&ptr, fCurr, sizeof(void*));
        }
        fCurr += sizeof(void*);
        return ptr;
    }

    SkScalar readScalar() {
        SkASSERT(ptr_align_4(fCurr));
        SkScalar value = *(const SkScalar*)fCurr;
        fCurr += sizeof(value);
        SkASSERT(fCurr <= fStop);
        return value;
    }

    const void* skip(size_t size) {
        SkASSERT(ptr_align_4(fCurr));
        const void* addr = fCurr;
        fCurr += SkAlign4(size);
        SkASSERT(fCurr <= fStop);
        return addr;
    }

    template <typename T> const T& skipT() {
        SkASSERT(SkAlign4(sizeof(T)) == sizeof(T));
        return *(const T*)this->skip(sizeof(T));
    }

    void read(void* dst, size_t size) {
        SkASSERT(0 == size || dst != NULL);
        SkASSERT(ptr_align_4(fCurr));
        memcpy(dst, fCurr, size);
        fCurr += SkAlign4(size);
        SkASSERT(fCurr <= fStop);
    }

    uint8_t readU8() { return (uint8_t)this->readInt(); }
    uint16_t readU16() { return (uint16_t)this->readInt(); }
    int32_t readS32() { return this->readInt(); }
    uint32_t readU32() { return this->readInt(); }

    bool readPath(SkPath* path) {
        return readObjectFromMemory(path);
    }

    bool readMatrix(SkMatrix* matrix) {
        return readObjectFromMemory(matrix);
    }

    bool readRRect(SkRRect* rrect) {
        return readObjectFromMemory(rrect);
    }

    bool readRegion(SkRegion* rgn) {
        return readObjectFromMemory(rgn);
    }

    




    const char* readString(size_t* len = NULL);

    



    size_t readIntoString(SkString* copy);

private:
    template <typename T> bool readObjectFromMemory(T* obj) {
        size_t size = obj->readFromMemory(this->peek(), this->available());
        
        bool success = (size > 0) && (size <= this->available()) && (SkAlign4(size) == size);
        
        (void)this->skip(success ? size : this->available());
        return success;
    }

    
    const char* fCurr;  
    const char* fStop;  
    const char* fBase;  

#ifdef SK_DEBUG
    static bool ptr_align_4(const void* ptr) {
        return (((const char*)ptr - (const char*)NULL) & 3) == 0;
    }
#endif
};

#endif
