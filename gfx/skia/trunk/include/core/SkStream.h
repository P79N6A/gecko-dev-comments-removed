






#ifndef SkStream_DEFINED
#define SkStream_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

class SkData;

class SkStream;
class SkStreamRewindable;
class SkStreamSeekable;
class SkStreamAsset;
class SkStreamMemory;


















class SK_API SkStream : public SkRefCnt { 
public:
    




    static SkStreamAsset* NewFromFile(const char path[]);

    SK_DECLARE_INST_COUNT(SkStream)

    






    virtual size_t read(void* buffer, size_t size) = 0;

    


    size_t skip(size_t size) {
        return this->read(NULL, size);
    }

    



    virtual bool isAtEnd() const = 0;

    int8_t   readS8();
    int16_t  readS16();
    int32_t  readS32();

    uint8_t  readU8() { return (uint8_t)this->readS8(); }
    uint16_t readU16() { return (uint16_t)this->readS16(); }
    uint32_t readU32() { return (uint32_t)this->readS32(); }

    bool     readBool() { return this->readU8() != 0; }
    SkScalar readScalar();
    size_t   readPackedUInt();

    



    SkData* readData();


    


    virtual bool rewind() { return false; }

    


    virtual SkStreamRewindable* duplicate() const { return NULL; }


    
    virtual bool hasPosition() const { return false; }
    
    virtual size_t getPosition() const { return 0; }

    



    virtual bool seek(size_t position) { return false; }

    



    virtual bool move(long offset) { return false; }

    


    virtual SkStreamSeekable* fork() const { return NULL; }


    
    virtual bool hasLength() const { return false; }
    
    virtual size_t getLength() const { return 0; }


    
    
    virtual const void* getMemoryBase() { return NULL; }

private:
    typedef SkRefCnt INHERITED;
};


class SK_API SkStreamRewindable : public SkStream {
public:
    virtual bool rewind() SK_OVERRIDE = 0;
    virtual SkStreamRewindable* duplicate() const SK_OVERRIDE = 0;
};


class SK_API SkStreamSeekable : public SkStreamRewindable {
public:
    virtual SkStreamSeekable* duplicate() const SK_OVERRIDE = 0;

    virtual bool hasPosition() const SK_OVERRIDE { return true; }
    virtual size_t getPosition() const SK_OVERRIDE = 0;
    virtual bool seek(size_t position) SK_OVERRIDE = 0;
    virtual bool move(long offset) SK_OVERRIDE = 0;
    virtual SkStreamSeekable* fork() const SK_OVERRIDE = 0;
};


class SK_API SkStreamAsset : public SkStreamSeekable {
public:
    virtual SkStreamAsset* duplicate() const SK_OVERRIDE = 0;
    virtual SkStreamAsset* fork() const SK_OVERRIDE = 0;

    virtual bool hasLength() const SK_OVERRIDE { return true; }
    virtual size_t getLength() const SK_OVERRIDE = 0;
};


class SK_API SkStreamMemory : public SkStreamAsset {
public:
    virtual SkStreamMemory* duplicate() const SK_OVERRIDE = 0;
    virtual SkStreamMemory* fork() const SK_OVERRIDE = 0;

    virtual const void* getMemoryBase() SK_OVERRIDE = 0;
};

class SK_API SkWStream : SkNoncopyable {
public:
    SK_DECLARE_INST_COUNT_ROOT(SkWStream)

    virtual ~SkWStream();

    




    virtual bool write(const void* buffer, size_t size) = 0;
    virtual void newline();
    virtual void flush();

    virtual size_t bytesWritten() const = 0;

    

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

    bool    writeStream(SkStream* input, size_t length);

    







    bool writeData(const SkData*);

    



    static int SizeOfPackedUInt(size_t value);
};



#include "SkString.h"
#include <stdio.h>

struct SkFILE;


class SK_API SkFILEStream : public SkStreamAsset {
public:
    SK_DECLARE_INST_COUNT(SkFILEStream)

    


    explicit SkFILEStream(const char path[] = NULL);

    enum Ownership {
        kCallerPasses_Ownership,
        kCallerRetains_Ownership
    };
    




    explicit SkFILEStream(FILE* file, Ownership ownership = kCallerPasses_Ownership);

    virtual ~SkFILEStream();

    
    bool isValid() const { return fFILE != NULL; }

    


    void setPath(const char path[]);

    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;
    virtual bool isAtEnd() const SK_OVERRIDE;

    virtual bool rewind() SK_OVERRIDE;
    virtual SkStreamAsset* duplicate() const SK_OVERRIDE;

    virtual size_t getPosition() const SK_OVERRIDE;
    virtual bool seek(size_t position) SK_OVERRIDE;
    virtual bool move(long offset) SK_OVERRIDE;
    virtual SkStreamAsset* fork() const SK_OVERRIDE;

    virtual size_t getLength() const SK_OVERRIDE;

    virtual const void* getMemoryBase() SK_OVERRIDE;

private:
    SkFILE*     fFILE;
    SkString    fName;
    Ownership   fOwnership;
    
    mutable SkAutoTUnref<SkData> fData;

    typedef SkStreamAsset INHERITED;
};

class SK_API SkMemoryStream : public SkStreamMemory {
public:
    SK_DECLARE_INST_COUNT(SkMemoryStream)

    SkMemoryStream();

    
    SkMemoryStream(size_t length);

    
    SkMemoryStream(const void* data, size_t length, bool copyData = false);

    


    SkMemoryStream(SkData*);

    virtual ~SkMemoryStream();

    



    virtual void setMemory(const void* data, size_t length,
                           bool copyData = false);
    



    void setMemoryOwned(const void* data, size_t length);

    


    SkData* copyToData() const;

    




    SkData* setData(SkData*);

    void skipToAlign4();
    const void* getAtPos();
    size_t peek() const { return fOffset; }

    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;
    virtual bool isAtEnd() const SK_OVERRIDE;

    virtual bool rewind() SK_OVERRIDE;
    virtual SkMemoryStream* duplicate() const SK_OVERRIDE;

    virtual size_t getPosition() const SK_OVERRIDE;
    virtual bool seek(size_t position) SK_OVERRIDE;
    virtual bool move(long offset) SK_OVERRIDE;
    virtual SkMemoryStream* fork() const SK_OVERRIDE;

    virtual size_t getLength() const SK_OVERRIDE;

    virtual const void* getMemoryBase() SK_OVERRIDE;

private:
    SkData* fData;
    size_t  fOffset;

    typedef SkStreamMemory INHERITED;
};



class SK_API SkFILEWStream : public SkWStream {
public:
    SK_DECLARE_INST_COUNT(SkFILEWStream)

    SkFILEWStream(const char path[]);
    virtual ~SkFILEWStream();

    

    bool isValid() const { return fFILE != NULL; }

    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    virtual void flush() SK_OVERRIDE;
    virtual size_t bytesWritten() const SK_OVERRIDE;

private:
    SkFILE* fFILE;

    typedef SkWStream INHERITED;
};

class SkMemoryWStream : public SkWStream {
public:
    SK_DECLARE_INST_COUNT(SkMemoryWStream)

    SkMemoryWStream(void* buffer, size_t size);
    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    virtual size_t bytesWritten() const SK_OVERRIDE { return fBytesWritten; }

private:
    char*   fBuffer;
    size_t  fMaxLength;
    size_t  fBytesWritten;

    typedef SkWStream INHERITED;
};

class SK_API SkDynamicMemoryWStream : public SkWStream {
public:
    SK_DECLARE_INST_COUNT(SkDynamicMemoryWStream)

    SkDynamicMemoryWStream();
    virtual ~SkDynamicMemoryWStream();

    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    virtual size_t bytesWritten() const SK_OVERRIDE { return fBytesWritten; }
    
    
    bool write(const void* buffer, size_t offset, size_t size);
    bool read(void* buffer, size_t offset, size_t size);
    size_t getOffset() const { return fBytesWritten; }

    
    void copyTo(void* dst) const;

    



    SkData* copyToData() const;

    
    SkStreamAsset* detachAsStream();

    
    void reset();
    void padToAlign4();
private:
    struct Block;
    Block*  fHead;
    Block*  fTail;
    size_t  fBytesWritten;
    mutable SkData* fCopy;  

    void invalidateCopy();

    
    friend class SkBlockMemoryStream;
    friend class SkBlockMemoryRefCnt;

    typedef SkWStream INHERITED;
};


class SK_API SkDebugWStream : public SkWStream {
public:
    SkDebugWStream() : fBytesWritten(0) {}
    SK_DECLARE_INST_COUNT(SkDebugWStream)

    
    virtual bool write(const void* buffer, size_t size) SK_OVERRIDE;
    virtual void newline() SK_OVERRIDE;
    virtual size_t bytesWritten() const SK_OVERRIDE { return fBytesWritten; }

private:
    size_t fBytesWritten;
    typedef SkWStream INHERITED;
};


typedef SkFILEStream SkURLStream;

#endif
