








#ifndef SkBuffer_DEFINED
#define SkBuffer_DEFINED

#include "SkScalar.h"








class SkRBuffer : SkNoncopyable {
public:
    SkRBuffer() : fData(0), fPos(0), fStop(0) {}
    


    SkRBuffer(const void* data) {
        fData = (const char*)data;
        fPos = (const char*)data;
        fStop = 0;  
    }
    

    SkRBuffer(const void* data, size_t size) {
        SkASSERT(data != 0 || size == 0);
        fData = (const char*)data;
        fPos = (const char*)data;
        fStop = (const char*)data + size;
    }
    
    


    size_t  pos() const { return fPos - fData; }
    


    size_t  size() const { return fStop - fData; }
    



    bool    eof() const { return fPos >= fStop; }

    


    void read(void* buffer, size_t size) {
        if (size) {
            this->readNoSizeCheck(buffer, size);
        }
    }

    const void* skip(size_t size); 
    size_t  skipToAlign4();

    void*       readPtr() { void* ptr; read(&ptr, sizeof(ptr)); return ptr; }
    SkScalar    readScalar() { SkScalar x; read(&x, 4); return x; }
    uint32_t    readU32() { uint32_t x; read(&x, 4); return x; }
    int32_t     readS32() { int32_t x; read(&x, 4); return x; }
    uint16_t    readU16() { uint16_t x; read(&x, 2); return x; }
    int16_t     readS16() { int16_t x; read(&x, 2); return x; }
    uint8_t     readU8() { uint8_t x; read(&x, 1); return x; }
    bool        readBool() { return this->readU8() != 0; }

protected:
    void    readNoSizeCheck(void* buffer, size_t size);

    const char* fData;
    const char* fPos;
    const char* fStop;
};









class SkWBuffer : SkNoncopyable {
public:
    SkWBuffer() : fData(0), fPos(0), fStop(0) {}
    SkWBuffer(void* data) { reset(data); }
    SkWBuffer(void* data, size_t size) { reset(data, size); }

    void reset(void* data) {
        fData = (char*)data;
        fPos = (char*)data;
        fStop = 0;  
    }

    void reset(void* data, size_t size) {
        SkASSERT(data != 0 || size == 0);
        fData = (char*)data;
        fPos = (char*)data;
        fStop = (char*)data + size;
    }
    
    size_t  pos() const { return fPos - fData; }
    void*   skip(size_t size); 

    void write(const void* buffer, size_t size) {
        if (size) {
            this->writeNoSizeCheck(buffer, size);
        }
    }

    size_t  padToAlign4();

    void    writePtr(const void* x) { this->writeNoSizeCheck(&x, sizeof(x)); }
    void    writeScalar(SkScalar x) { this->writeNoSizeCheck(&x, 4); }
    void    write32(int32_t x) { this->writeNoSizeCheck(&x, 4); }
    void    write16(int16_t x) { this->writeNoSizeCheck(&x, 2); }
    void    write8(int8_t x) { this->writeNoSizeCheck(&x, 1); }
    void    writeBool(bool x) { this->write8(x); }

protected:
    void    writeNoSizeCheck(const void* buffer, size_t size);

    char* fData;
    char* fPos;
    char* fStop;
};

#endif

