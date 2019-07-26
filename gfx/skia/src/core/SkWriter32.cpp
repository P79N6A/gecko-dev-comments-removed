






#include "SkWriter32.h"

struct SkWriter32::Block {
    Block*  fNext;
    size_t  fSizeOfBlock;      
    size_t  fAllocatedSoFar;    

    size_t  available() const { return fSizeOfBlock - fAllocatedSoFar; }
    char*   base() { return (char*)(this + 1); }
    const char* base() const { return (const char*)(this + 1); }

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
        SkASSERT(SkAlign4(size) == size);
        Block* block = (Block*)sk_malloc_throw(sizeof(Block) + size);
        block->fNext = NULL;
        block->fSizeOfBlock = size;
        block->fAllocatedSoFar = 0;
        return block;
    }

    static Block* CreateFromStorage(void* storage, size_t size) {
        SkASSERT(SkIsAlign4((intptr_t)storage));
        Block* block = (Block*)storage;
        block->fNext = NULL;
        block->fSizeOfBlock = size - sizeof(Block);
        block->fAllocatedSoFar = 0;
        return block;
    }

};

#define MIN_BLOCKSIZE   (sizeof(SkWriter32::Block) + sizeof(intptr_t))



SkWriter32::SkWriter32(size_t minSize, void* storage, size_t storageSize) {
    fMinSize = minSize;
    fSize = 0;
    fSingleBlock = NULL;
    fSingleBlockSize = 0;

    storageSize &= ~3;  
    if (storageSize >= MIN_BLOCKSIZE) {
        fHead = fTail = Block::CreateFromStorage(storage, storageSize);
        fHeadIsExternalStorage = true;
    } else {
        fHead = fTail = NULL;
        fHeadIsExternalStorage = false;
    }
}

SkWriter32::~SkWriter32() {
    this->reset();
}

void SkWriter32::reset() {
    Block* block = fHead;

    if (fHeadIsExternalStorage) {
        SkASSERT(block);
        
        block = block->fNext;
    }
    while (block) {
        Block* next = block->fNext;
        sk_free(block);
        block = next;
    }

    fSize = 0;
    fSingleBlock = NULL;
    if (fHeadIsExternalStorage) {
        SkASSERT(fHead);
        fHead->rewind();
        fTail = fHead;
    } else {
        fHead = fTail = NULL;
    }
}

void SkWriter32::reset(void* block, size_t size) {
    this->reset();
    SkASSERT(0 == ((fSingleBlock - (char*)0) & 3));   
    fSingleBlock = (char*)block;
    fSingleBlockSize = (size & ~3);
}

uint32_t* SkWriter32::reserve(size_t size) {
    SkASSERT(SkAlign4(size) == size);

    if (fSingleBlock) {
        uint32_t* ptr = (uint32_t*)(fSingleBlock + fSize);
        fSize += size;
        SkASSERT(fSize <= fSingleBlockSize);
        return ptr;
    }

    Block* block = fTail;

    if (NULL == block) {
        SkASSERT(NULL == fHead);
        fHead = fTail = block = Block::Create(SkMax32(size, fMinSize));
    } else if (block->available() < size) {
        fTail = Block::Create(SkMax32(size, fMinSize));
        block->fNext = fTail;
        block = fTail;
    }

    fSize += size;

    return block->alloc(size);
}

uint32_t* SkWriter32::peek32(size_t offset) {
    SkDEBUGCODE(this->validate();)

    SkASSERT(SkAlign4(offset) == offset);
    SkASSERT(offset <= fSize);

    if (fSingleBlock) {
        return (uint32_t*)(fSingleBlock + offset);
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
        this->reset(NULL, 0);
        return;
    }

    SkDEBUGCODE(this->validate();)

    SkASSERT(SkAlign4(offset) == offset);
    SkASSERT(offset <= fSize);
    fSize = offset;

    if (fSingleBlock) {
        return;
    }

    
    Block* block = fHead;
    SkASSERT(NULL != block);
    while (offset >= block->fAllocatedSoFar) {
        offset -= block->fAllocatedSoFar;
        block = block->fNext;
        SkASSERT(NULL != block);
    }

    
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

    SkDEBUGCODE(this->validate();)
}

void SkWriter32::flatten(void* dst) const {
    if (fSingleBlock) {
        memcpy(dst, fSingleBlock, fSize);
        return;
    }

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

void SkWriter32::writePad(const void* src, size_t size) {
    if (size > 0) {
        size_t alignedSize = SkAlign4(size);
        char* dst = (char*)this->reserve(alignedSize);
        
        
        uint32_t* padding = (uint32_t*)(dst + (alignedSize - 4));
        *padding = 0;
        
        memcpy(dst, src, size);
    }
}

#include "SkStream.h"

size_t SkWriter32::readFromStream(SkStream* stream, size_t length) {
    if (fSingleBlock) {
        SkASSERT(fSingleBlockSize >= fSize);
        size_t remaining = fSingleBlockSize - fSize;
        if (length > remaining) {
            length = remaining;
        }
        stream->read(fSingleBlock + fSize, length);
        fSize += length;
        return length;
    }

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
    if (fSingleBlock) {
        return stream->write(fSingleBlock, fSize);
    }

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
    SkASSERT(SkIsAlign4(fSingleBlockSize));

    if (fSingleBlock) {
        SkASSERT(fSize <= fSingleBlockSize);
        return;
    }

    size_t accum = 0;
    const Block* block = fHead;
    while (block) {
        SkASSERT(SkIsAlign4(block->fSizeOfBlock));
        SkASSERT(SkIsAlign4(block->fAllocatedSoFar));
        SkASSERT(block->fAllocatedSoFar <= block->fSizeOfBlock);
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


