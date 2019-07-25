








#ifndef SkStream_DEFINED
#define SkStream_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

class SkData;

class SK_API SkStream : public SkRefCnt {
public:
    virtual ~SkStream();
    


    virtual bool rewind() = 0;
    


    virtual const char* getFileName();
    







    virtual size_t read(void* buffer, size_t size) = 0;

    

    size_t getLength() { return this->read(NULL, 0); }
    
    


    size_t skip(size_t bytes);

    




    virtual const void* getMemoryBase();

    int8_t   readS8();
    int16_t  readS16();
    int32_t  readS32();

    uint8_t  readU8() { return (uint8_t)this->readS8(); }
    uint16_t readU16() { return (uint16_t)this->readS16(); }
    uint32_t readU32() { return (uint32_t)this->readS32(); }

    bool     readBool() { return this->readU8() != 0; }
    SkScalar readScalar();
    size_t   readPackedUInt();
};

class SK_API SkWStream : SkNoncopyable {
public:
    virtual ~SkWStream();

    




    virtual bool write(const void* buffer, size_t size) = 0;
    virtual void newline();
    virtual void flush();

    
    
    bool    write8(U8CPU);
    bool    write16(U16CPU);
    bool    write32(uint32_t);

    bool    writeText(const char text[]);
    bool    writeDecAsText(int32_t);
    bool    writeBigDecAsText(int64_t, int minDigits = 0);
    bool    writeHexAsText(uint32_t, int minDigits = 0);
    bool    writeScalarAsText(SkScalar);
    
    bool    writeBool(bool v) { return this->write8(v); }
    bool    writeScalar(SkScalar);
    bool    writePackedUInt(size_t);
    
    bool writeStream(SkStream* input, size_t length);

    bool writeData(const SkData*);
};



#include "SkString.h"

struct SkFILE;




class SkFILEStream : public SkStream {
public:
    


    explicit SkFILEStream(const char path[] = NULL);
    virtual ~SkFILEStream();

    

    bool isValid() const { return fFILE != NULL; }
    


    void setPath(const char path[]);

    virtual bool rewind() SK_OVERRIDE;
    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;
    virtual const char* getFileName() SK_OVERRIDE;

private:
    SkFILE*     fFILE;
    SkString    fName;
};



class SkFDStream : public SkStream {
public:
    



    SkFDStream(int fileDesc, bool closeWhenDone);
    virtual ~SkFDStream();
    
    

    bool isValid() const { return fFD >= 0; }
    
    virtual bool rewind() SK_OVERRIDE;
    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;
    virtual const char* getFileName() SK_OVERRIDE { return NULL; }
    
private:
    int     fFD;
    bool    fCloseWhenDone;
};

class SkMemoryStream : public SkStream {
public:
    SkMemoryStream();
    

    SkMemoryStream(size_t length);
    

    SkMemoryStream(const void* data, size_t length, bool copyData = false);
    virtual ~SkMemoryStream();

    



    virtual void setMemory(const void* data, size_t length,
                           bool copyData = false);
    



    void setMemoryOwned(const void* data, size_t length);

    



    SkData* copyToData() const;

    




    SkData* setData(SkData*);

    void skipToAlign4();
    virtual bool rewind() SK_OVERRIDE;
    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;
    virtual const void* getMemoryBase() SK_OVERRIDE;
    const void* getAtPos();
    size_t seek(size_t offset);
    size_t peek() const { return fOffset; }
    
private:
    SkData* fData;
    size_t  fOffset;
};






class SkBufferStream : public SkStream {
public:
    





    SkBufferStream(SkStream* proxy, size_t bufferSize = 0);
    






    SkBufferStream(SkStream* proxy, void* buffer, size_t bufferSize);
    virtual ~SkBufferStream();

    virtual bool        rewind() SK_OVERRIDE;
    virtual const char* getFileName() SK_OVERRIDE;
    virtual size_t      read(void* buffer, size_t size) SK_OVERRIDE;
    virtual const void* getMemoryBase() SK_OVERRIDE;

private:
    enum {
        kDefaultBufferSize  = 128
    };
    
    SkBufferStream(const SkBufferStream&);
    SkBufferStream& operator=(const SkBufferStream&);

    SkStream*   fProxy;
    char*       fBuffer;
    size_t      fOrigBufferSize, fBufferSize, fBufferOffset;
    bool        fWeOwnTheBuffer;

    void    init(void*, size_t);
};



class SkFILEWStream : public SkWStream {
public:
            SkFILEWStream(const char path[]);
    virtual ~SkFILEWStream();

    

    bool isValid() const { return fFILE != NULL; }

    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    virtual void flush() SK_OVERRIDE;
private:
    SkFILE* fFILE;
};

class SkMemoryWStream : public SkWStream {
public:
    SkMemoryWStream(void* buffer, size_t size);
    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    
private:
    char*   fBuffer;
    size_t  fMaxLength;
    size_t  fBytesWritten;
};

class SK_API SkDynamicMemoryWStream : public SkWStream {
public:
    SkDynamicMemoryWStream();
    virtual ~SkDynamicMemoryWStream();

    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    
    
    bool write(const void* buffer, size_t offset, size_t size);
    bool read(void* buffer, size_t offset, size_t size);
    size_t getOffset() const { return fBytesWritten; }
    size_t bytesWritten() const { return fBytesWritten; }

    
    void copyTo(void* dst) const;

    



    SkData* copyToData() const;

    
    void reset();
    void padToAlign4();
private:
    struct Block;
    Block*  fHead;
    Block*  fTail;
    size_t  fBytesWritten;
    mutable SkData* fCopy;  

    void invalidateCopy();
};


class SkDebugWStream : public SkWStream {
public:
    
    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    virtual void newline() SK_OVERRIDE;
};


typedef SkFILEStream SkURLStream;

#endif
