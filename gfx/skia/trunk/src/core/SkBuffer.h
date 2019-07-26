








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

    virtual ~SkRBuffer() { }

    


    size_t  pos() const { return fPos - fData; }
    


    size_t  size() const { return fStop - fData; }
    



    bool    eof() const { return fPos >= fStop; }

    


    virtual bool read(void* buffer, size_t size) {
        if (size) {
            this->readNoSizeCheck(buffer, size);
        }
        return true;
    }

    const void* skip(size_t size); 
    size_t  skipToAlign4();

    bool readPtr(void** ptr) { return read(ptr, sizeof(void*)); }
    bool readScalar(SkScalar* x) { return read(x, 4); }
    bool readU32(uint32_t* x) { return read(x, 4); }
    bool readS32(int32_t* x) { return read(x, 4); }
    bool readU16(uint16_t* x) { return read(x, 2); }
    bool readS16(int16_t* x) { return read(x, 2); }
    bool readU8(uint8_t* x) { return read(x, 1); }
    bool readBool(bool* x) {
        uint8_t u8;
        if (this->readU8(&u8)) {
            *x = (u8 != 0);
            return true;
        }
        return false;
    }

protected:
    void    readNoSizeCheck(void* buffer, size_t size);

    const char* fData;
    const char* fPos;
    const char* fStop;
};






class SkRBufferWithSizeCheck : public SkRBuffer {
public:
    SkRBufferWithSizeCheck(const void* data, size_t size) : SkRBuffer(data, size), fError(false) {}

    



    virtual bool read(void* buffer, size_t size) SK_OVERRIDE;

    

    bool isValid() const { return !fError; }
private:
    bool fError;
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

private:
    void    writeNoSizeCheck(const void* buffer, size_t size);

    char* fData;
    char* fPos;
    char* fStop;
};

#endif
