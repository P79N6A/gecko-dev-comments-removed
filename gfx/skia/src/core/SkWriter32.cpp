






#include "SkWriter32.h"

SkWriter32::SkWriter32(size_t minSize, void* storage, size_t storageSize) {
    fMinSize = minSize;
    fSize = 0;
    fWrittenBeforeLastBlock = 0;
    fHead = fTail = NULL;

    if (storageSize) {
        this->reset(storage, storageSize);
    }
}

SkWriter32::~SkWriter32() {
    this->reset();
}

void SkWriter32::reset() {
    Block* block = fHead;

    if (this->isHeadExternallyAllocated()) {
        SkASSERT(block);
        
        block = block->fNext;
    }
    while (block) {
        Block* next = block->fNext;
        sk_free(block);
        block = next;
    }

    fSize = 0;
    fWrittenBeforeLastBlock = 0;
    fHead = fTail = NULL;
}

void SkWriter32::reset(void* storage, size_t storageSize) {
    this->reset();

    storageSize &= ~3;  
    if (storageSize > 0 && SkIsAlign4((intptr_t)storage)) {
        fHead = fTail = fExternalBlock.initFromStorage(storage, storageSize);
    }
}

SkWriter32::Block* SkWriter32::doReserve(size_t size) {
    SkASSERT(SkAlign4(size) == size);

    Block* block = fTail;
    SkASSERT(NULL == block || block->available() < size);

    if (NULL == block) {
        SkASSERT(NULL == fHead);
        fHead = fTail = block = Block::Create(SkMax32(size, fMinSize));
        SkASSERT(0 == fWrittenBeforeLastBlock);
    } else {
        SkASSERT(fSize > 0);
        fWrittenBeforeLastBlock = fSize;

        fTail = Block::Create(SkMax32(size, fMinSize));
        block->fNext = fTail;
        block = fTail;
    }
    return block;
}

uint32_t* SkWriter32::peek32(size_t offset) {
    SkDEBUGCODE(this->validate();)

    SkASSERT(SkAlign4(offset) == offset);
    SkASSERT(offset <= fSize);

    
    if (offset >= fWrittenBeforeLastBlock) {
        return fTail->peek32(offset - fWrittenBeforeLastBlock);
    }

    Block* block = fHead;
    SkASSERT(NULL != block);

    while (offset >= block->fAllocatedSoFar) {
        offset -= block->fAllocatedSoFar;
        block = block->fNext;
        SkASSERT(NULL != block);
    }
    return block->peek32(offset);
}

void SkWriter32::rewindToOffset(size_t offset) {
    if (offset >= fSize) {
        return;
    }
    if (0 == offset) {
        this->reset();
        return;
    }

    SkDEBUGCODE(this->validate();)

    SkASSERT(SkAlign4(offset) == offset);
    SkASSERT(offset <= fSize);
    fSize = offset;

    
    if (offset >= fWrittenBeforeLastBlock) {
        fTail->fAllocatedSoFar = offset - fWrittenBeforeLastBlock;
    } else {
        
        

        size_t globalOffset = offset;
        Block* block = fHead;
        SkASSERT(NULL != block);
        while (offset >= block->fAllocatedSoFar) {
            offset -= block->fAllocatedSoFar;
            block = block->fNext;
            SkASSERT(NULL != block);
        }

        
        fWrittenBeforeLastBlock = globalOffset - offset;

        
        block->fAllocatedSoFar = offset;
        
        fTail = block;
        Block* next = block->fNext;
        block->fNext = NULL;
        
        block = next;
        while (block) {
            Block* next = block->fNext;
            sk_free(block);
            block = next;
        }
    }
    SkDEBUGCODE(this->validate();)
}

void SkWriter32::flatten(void* dst) const {
    const Block* block = fHead;
    SkDEBUGCODE(size_t total = 0;)

    while (block) {
        size_t allocated = block->fAllocatedSoFar;
        memcpy(dst, block->base(), allocated);
        dst = (char*)dst + allocated;
        block = block->fNext;

        SkDEBUGCODE(total += allocated;)
        SkASSERT(total <= fSize);
    }
    SkASSERT(total == fSize);
}

uint32_t* SkWriter32::reservePad(size_t size) {
    if (size > 0) {
        size_t alignedSize = SkAlign4(size);
        char* dst = (char*)this->reserve(alignedSize);
        
        uint32_t* padding = (uint32_t*)(dst + (alignedSize - 4));
        *padding = 0;
        return (uint32_t*) dst;
    }
    return this->reserve(0);
}

void SkWriter32::writePad(const void* src, size_t size) {
    if (size > 0) {
        char* dst = (char*)this->reservePad(size);
        
        memcpy(dst, src, size);
    }
}

#include "SkStream.h"

size_t SkWriter32::readFromStream(SkStream* stream, size_t length) {
    char scratch[1024];
    const size_t MAX = sizeof(scratch);
    size_t remaining = length;

    while (remaining != 0) {
        size_t n = remaining;
        if (n > MAX) {
            n = MAX;
        }
        size_t bytes = stream->read(scratch, n);
        this->writePad(scratch, bytes);
        remaining -= bytes;
        if (bytes != n) {
            break;
        }
    }
    return length - remaining;
}

bool SkWriter32::writeToStream(SkWStream* stream) {
    const Block* block = fHead;
    while (block) {
        if (!stream->write(block->base(), block->fAllocatedSoFar)) {
            return false;
        }
        block = block->fNext;
    }
    return true;
}

#ifdef SK_DEBUG
void SkWriter32::validate() const {
    SkASSERT(SkIsAlign4(fSize));

    size_t accum = 0;
    const Block* block = fHead;
    while (block) {
        SkASSERT(SkIsAlign4(block->fSizeOfBlock));
        SkASSERT(SkIsAlign4(block->fAllocatedSoFar));
        SkASSERT(block->fAllocatedSoFar <= block->fSizeOfBlock);
        if (NULL == block->fNext) {
            SkASSERT(fTail == block);
            SkASSERT(fWrittenBeforeLastBlock == accum);
        }
        accum += block->fAllocatedSoFar;
        SkASSERT(accum <= fSize);
        block = block->fNext;
    }
    SkASSERT(accum == fSize);
}
#endif



#include "SkReader32.h"
#include "SkString.h"





const char* SkReader32::readString(size_t* outLen) {
    size_t len = this->readInt();
    const void* ptr = this->peek();

    
    size_t alignedSize = SkAlign4(len + 1);
    this->skip(alignedSize);

    if (outLen) {
        *outLen = len;
    }
    return (const char*)ptr;
}

size_t SkReader32::readIntoString(SkString* copy) {
    size_t len;
    const char* ptr = this->readString(&len);
    if (copy) {
        copy->set(ptr, len);
    }
    return len;
}

void SkWriter32::writeString(const char str[], size_t len) {
    if ((long)len < 0) {
        SkASSERT(str);
        len = strlen(str);
    }
    this->write32(len);
    
    size_t alignedLen = SkAlign4(len + 1);
    char* ptr = (char*)this->reserve(alignedLen);
    {
        
        uint32_t* padding = (uint32_t*)(ptr + (alignedLen - 4));
        *padding = 0;
    }
    
    memcpy(ptr, str, len);
}

size_t SkWriter32::WriteStringSize(const char* str, size_t len) {
    if ((long)len < 0) {
        SkASSERT(str);
        len = strlen(str);
    }
    const size_t lenBytes = 4;    
    
    return SkAlign4(lenBytes + len + 1);
}
