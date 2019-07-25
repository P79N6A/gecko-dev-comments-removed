






#include "SkEndian.h"
#include "SkFontHost.h"
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








static int count_tables(SkStream* stream, size_t* offsetToDir = NULL) {
    SkSharedTTHeader shared;
    if (stream->read(&shared, sizeof(shared)) != sizeof(shared)) {
        return 0;
    }

    
    size_t offset = 0;

    
    uint32_t tag = SkEndian_SwapBE32(shared.fCollection.fTag);
    if (SkSetFourByteTag('t', 't', 'c', 'f') == tag) {
        if (shared.fCollection.fNumOffsets == 0) {
            return 0;
        }
        
        offset = SkEndian_SwapBE32(shared.fCollection.fOffset0);
        stream->rewind();
        if (stream->skip(offset) != offset) {
            return 0;
        }
        if (stream->read(&shared, sizeof(shared)) != sizeof(shared)) {
            return 0;
        }
    }

    if (offsetToDir) {
        
        *offsetToDir = offset + sizeof(SkSFNTHeader);
    }
    return SkEndian_SwapBE16(shared.fSingle.fNumTables);
}



struct SfntHeader {
    SfntHeader() : fCount(0), fDir(NULL) {}
    ~SfntHeader() { sk_free(fDir); }

    





    bool init(SkStream* stream) {
        size_t offsetToDir;
        fCount = count_tables(stream, &offsetToDir);
        if (0 == fCount) {
            return false;
        }

        stream->rewind();
        if (stream->skip(offsetToDir) != offsetToDir) {
            return false;
        }

        size_t size = fCount * sizeof(SkSFNTDirEntry);
        fDir = reinterpret_cast<SkSFNTDirEntry*>(sk_malloc_throw(size));
        return stream->read(fDir, size) == size;
    }

    int             fCount;
    SkSFNTDirEntry* fDir;
};



int SkFontHost::CountTables(SkFontID fontID) {
    SkStream* stream = SkFontHost::OpenStream(fontID);
    if (NULL == stream) {
        return 0;
    }

    SkAutoUnref au(stream);
    return count_tables(stream);
}

int SkFontHost::GetTableTags(SkFontID fontID, SkFontTableTag tags[]) {
    SkStream* stream = SkFontHost::OpenStream(fontID);
    if (NULL == stream) {
        return 0;
    }

    SkAutoUnref au(stream);
    SfntHeader  header;
    if (!header.init(stream)) {
        return 0;
    }

    for (int i = 0; i < header.fCount; i++) {
        tags[i] = SkEndian_SwapBE32(header.fDir[i].fTag);
    }
    return header.fCount;
}

size_t SkFontHost::GetTableSize(SkFontID fontID, SkFontTableTag tag) {
    SkStream* stream = SkFontHost::OpenStream(fontID);
    if (NULL == stream) {
        return 0;
    }

    SkAutoUnref au(stream);
    SfntHeader  header;
    if (!header.init(stream)) {
        return 0;
    }

    for (int i = 0; i < header.fCount; i++) {
        if (SkEndian_SwapBE32(header.fDir[i].fTag) == tag) {
            return SkEndian_SwapBE32(header.fDir[i].fLength);
        }
    }
    return 0;
}

size_t SkFontHost::GetTableData(SkFontID fontID, SkFontTableTag tag,
                                size_t offset, size_t length, void* data) {
    SkStream* stream = SkFontHost::OpenStream(fontID);
    if (NULL == stream) {
        return 0;
    }

    SkAutoUnref au(stream);
    SfntHeader  header;
    if (!header.init(stream)) {
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
            if (offset + length > realLength) {
                length = realLength - offset;
            }
            
            stream->rewind();
            size_t bytesToSkip = realOffset + offset;
            if (stream->skip(bytesToSkip) != bytesToSkip) {
                return 0;
            }
            if (stream->read(data, length) != length) {
                return 0;
            }
            return length;
        }
    }
    return 0;
}

