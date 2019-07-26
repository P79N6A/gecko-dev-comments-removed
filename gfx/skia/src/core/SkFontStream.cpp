






#include "SkEndian.h"
#include "SkFontStream.h"
#include "SkStream.h"

struct SkSFNTHeader {
    uint32_t    fVersion;
    uint16_t    fNumTables;
    uint16_t    fSearchRange;
    uint16_t    fEntrySelector;
    uint16_t    fRangeShift;
};

struct SkTTCFHeader {
    uint32_t    fTag;
    uint32_t    fVersion;
    uint32_t    fNumOffsets;
    uint32_t    fOffset0;   
};

union SkSharedTTHeader {
    SkSFNTHeader    fSingle;
    SkTTCFHeader    fCollection;
};

struct SkSFNTDirEntry {
    uint32_t    fTag;
    uint32_t    fChecksum;
    uint32_t    fOffset;
    uint32_t    fLength;
};

static bool read(SkStream* stream, void* buffer, size_t amount) {
    return stream->read(buffer, amount) == amount;
}

static bool skip(SkStream* stream, size_t amount) {
    return stream->skip(amount) == amount;
}








static int count_tables(SkStream* stream, int ttcIndex, size_t* offsetToDir) {
    SkASSERT(ttcIndex >= 0);

    SkAutoSMalloc<1024> storage(sizeof(SkSharedTTHeader));
    SkSharedTTHeader* header = (SkSharedTTHeader*)storage.get();

    if (!read(stream, header, sizeof(SkSharedTTHeader))) {
        return 0;
    }

    
    size_t offset = 0;

    
    uint32_t tag = SkEndian_SwapBE32(header->fCollection.fTag);
    if (SkSetFourByteTag('t', 't', 'c', 'f') == tag) {
        unsigned count = SkEndian_SwapBE32(header->fCollection.fNumOffsets);
        if ((unsigned)ttcIndex >= count) {
            return 0;
        }

        if (ttcIndex > 0) { 
            stream->rewind();
            size_t amount = sizeof(SkSharedTTHeader) + ttcIndex * sizeof(uint32_t);
            header = (SkSharedTTHeader*)storage.reset(amount);
            if (!read(stream, header, amount)) {
                return 0;
            }
        }
        
        offset = SkEndian_SwapBE32((&header->fCollection.fOffset0)[ttcIndex]);
        stream->rewind();
        if (!skip(stream, offset)) {
            return 0;
        }
        if (!read(stream, header, sizeof(SkSFNTHeader))) {
            return 0;
        }
    }

    if (offsetToDir) {
        
        *offsetToDir = offset + sizeof(SkSFNTHeader);
    }
    return SkEndian_SwapBE16(header->fSingle.fNumTables);
}



struct SfntHeader {
    SfntHeader() : fCount(0), fDir(NULL) {}
    ~SfntHeader() { sk_free(fDir); }

    





    bool init(SkStream* stream, int ttcIndex) {
        stream->rewind();

        size_t offsetToDir;
        fCount = count_tables(stream, ttcIndex, &offsetToDir);
        if (0 == fCount) {
            return false;
        }

        stream->rewind();
        if (!skip(stream, offsetToDir)) {
            return false;
        }

        size_t size = fCount * sizeof(SkSFNTDirEntry);
        fDir = reinterpret_cast<SkSFNTDirEntry*>(sk_malloc_throw(size));
        return read(stream, fDir, size);
    }

    int             fCount;
    SkSFNTDirEntry* fDir;
};



int SkFontStream::CountTTCEntries(SkStream* stream) {
    stream->rewind();

    SkSharedTTHeader shared;
    if (!read(stream, &shared, sizeof(shared))) {
        return 0;
    }

    
    uint32_t tag = SkEndian_SwapBE32(shared.fCollection.fTag);
    if (SkSetFourByteTag('t', 't', 'c', 'f') == tag) {
        return SkEndian_SwapBE32(shared.fCollection.fNumOffsets);
    } else {
        return 1;   
    }
}

int SkFontStream::GetTableTags(SkStream* stream, int ttcIndex,
                               SkFontTableTag tags[]) {
    SfntHeader  header;
    if (!header.init(stream, ttcIndex)) {
        return 0;
    }

    if (tags) {
        for (int i = 0; i < header.fCount; i++) {
            tags[i] = SkEndian_SwapBE32(header.fDir[i].fTag);
        }
    }
    return header.fCount;
}

size_t SkFontStream::GetTableData(SkStream* stream, int ttcIndex,
                                  SkFontTableTag tag,
                                  size_t offset, size_t length, void* data) {
    SfntHeader  header;
    if (!header.init(stream, ttcIndex)) {
        return 0;
    }

    for (int i = 0; i < header.fCount; i++) {
        if (SkEndian_SwapBE32(header.fDir[i].fTag) == tag) {
            size_t realOffset = SkEndian_SwapBE32(header.fDir[i].fOffset);
            size_t realLength = SkEndian_SwapBE32(header.fDir[i].fLength);
            
            if (offset >= realLength) {
                return 0;
            }
            
            
            
            if (offset + length < offset) {
                return 0;
            }
            if (length > realLength - offset) {
                length = realLength - offset;
            }
            if (data) {
                
                stream->rewind();
                size_t bytesToSkip = realOffset + offset;
                if (!skip(stream, bytesToSkip)) {
                    return 0;
                }
                if (!read(stream, data, length)) {
                    return 0;
                }
            }
            return length;
        }
    }
    return 0;
}
