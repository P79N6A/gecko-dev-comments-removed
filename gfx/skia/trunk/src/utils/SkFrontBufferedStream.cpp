






#include "SkFrontBufferedStream.h"
#include "SkStream.h"
#include "SkTemplates.h"

class FrontBufferedStream : public SkStreamRewindable {
public:
    
    FrontBufferedStream(SkStream*, size_t bufferSize);

    virtual size_t read(void* buffer, size_t size) SK_OVERRIDE;

    virtual bool isAtEnd() const SK_OVERRIDE;

    virtual bool rewind() SK_OVERRIDE;

    virtual bool hasPosition() const SK_OVERRIDE { return true; }

    virtual size_t getPosition() const SK_OVERRIDE { return fOffset; }

    virtual bool hasLength() const SK_OVERRIDE { return fHasLength; }

    virtual size_t getLength() const SK_OVERRIDE { return fLength; }

    virtual SkStreamRewindable* duplicate() const SK_OVERRIDE { return NULL; }

private:
    SkAutoTUnref<SkStream>  fStream;
    const bool              fHasLength;
    const size_t            fLength;
    
    size_t                  fOffset;
    
    
    size_t                  fBufferedSoFar;
    
    const size_t            fBufferSize;
    
    
    SkAutoTMalloc<char>     fBuffer;

    
    
    
    size_t readFromBuffer(char* dst, size_t size);

    
    
    
    size_t bufferAndWriteTo(char* dst, size_t size);

    
    
    
    size_t readDirectlyFromStream(char* dst, size_t size);

    typedef SkStream INHERITED;
};

SkStreamRewindable* SkFrontBufferedStream::Create(SkStream* stream, size_t bufferSize) {
    if (NULL == stream) {
        return NULL;
    }
    return SkNEW_ARGS(FrontBufferedStream, (stream, bufferSize));
}

FrontBufferedStream::FrontBufferedStream(SkStream* stream, size_t bufferSize)
    : fStream(SkRef(stream))
    , fHasLength(stream->hasPosition() && stream->hasLength())
    , fLength(stream->getLength() - stream->getPosition())
    , fOffset(0)
    , fBufferedSoFar(0)
    , fBufferSize(bufferSize)
    , fBuffer(bufferSize) {}

bool FrontBufferedStream::isAtEnd() const {
    if (fOffset < fBufferedSoFar) {
        
        
        return false;
    }

    return fStream->isAtEnd();
}

bool FrontBufferedStream::rewind() {
    
    if (fOffset <= fBufferSize) {
        fOffset = 0;
        return true;
    }
    return false;
}

size_t FrontBufferedStream::readFromBuffer(char* dst, size_t size) {
    SkASSERT(fOffset < fBufferedSoFar);
    
    
    
    const size_t bytesToCopy = SkTMin(size, fBufferedSoFar - fOffset);
    if (dst != NULL) {
        memcpy(dst, fBuffer + fOffset, bytesToCopy);
    }

    
    
    fOffset += bytesToCopy;
    SkASSERT(fOffset <= fBufferedSoFar);

    return bytesToCopy;
}

size_t FrontBufferedStream::bufferAndWriteTo(char* dst, size_t size) {
    SkASSERT(size > 0);
    SkASSERT(fOffset >= fBufferedSoFar);
    
    
    const size_t bytesToBuffer = SkTMin(size, fBufferSize - fBufferedSoFar);
    char* buffer = fBuffer + fOffset;
    const size_t buffered = fStream->read(buffer, bytesToBuffer);

    fBufferedSoFar += buffered;
    fOffset = fBufferedSoFar;
    SkASSERT(fBufferedSoFar <= fBufferSize);

    
    if (dst != NULL) {
        memcpy(dst, buffer, buffered);
    }

    return buffered;
}

size_t FrontBufferedStream::readDirectlyFromStream(char* dst, size_t size) {
    SkASSERT(size > 0);
    
    SkASSERT(fBufferSize == fBufferedSoFar && fOffset >= fBufferSize);

    const size_t bytesReadDirectly = fStream->read(dst, size);
    fOffset += bytesReadDirectly;

    
    
    if (bytesReadDirectly > 0) {
        fBuffer.reset(0);
    }

    return bytesReadDirectly;
}

size_t FrontBufferedStream::read(void* voidDst, size_t size) {
    
    char* dst = reinterpret_cast<char*>(voidDst);
    SkDEBUGCODE(const size_t totalSize = size;)
    const size_t start = fOffset;

    
    if (fOffset < fBufferedSoFar) {
        const size_t bytesCopied = this->readFromBuffer(dst, size);

        
        
        size -= bytesCopied;
        SkASSERT(size + (fOffset - start) == totalSize);
        if (dst != NULL) {
            dst += bytesCopied;
        }
    }

    
    
    if (size > 0 && fBufferedSoFar < fBufferSize) {
        const size_t buffered = this->bufferAndWriteTo(dst, size);

        
        
        size -= buffered;
        SkASSERT(size + (fOffset - start) == totalSize);
        if (dst != NULL) {
            dst += buffered;
        }
    }

    if (size > 0 && !fStream->isAtEnd()) {
        SkDEBUGCODE(const size_t bytesReadDirectly =) this->readDirectlyFromStream(dst, size);
        SkDEBUGCODE(size -= bytesReadDirectly;)
        SkASSERT(size + (fOffset - start) == totalSize);
    }

    return fOffset - start;
}
