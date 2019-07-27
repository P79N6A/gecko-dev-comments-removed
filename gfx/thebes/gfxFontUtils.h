




#ifndef GFX_FONT_UTILS_H
#define GFX_FONT_UTILS_H

#include "gfxPlatform.h"
#include "nsComponentManagerUtils.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/Likely.h"
#include "mozilla/Endian.h"
#include "mozilla/MemoryReporting.h"

#include "zlib.h"
#include <algorithm>


#ifdef __MINGW32__
#undef min
#undef max
#endif

typedef struct hb_blob_t hb_blob_t;

class gfxSparseBitSet {
private:
    enum { BLOCK_SIZE = 32 };   
    enum { BLOCK_SIZE_BITS = BLOCK_SIZE * 8 };
    enum { BLOCK_INDEX_SHIFT = 8 };

    struct Block {
        Block(const Block& aBlock) { memcpy(mBits, aBlock.mBits, sizeof(mBits)); }
        explicit Block(unsigned char memsetValue = 0) { memset(mBits, memsetValue, BLOCK_SIZE); }
        uint8_t mBits[BLOCK_SIZE];
    };

public:
    gfxSparseBitSet() { }
    gfxSparseBitSet(const gfxSparseBitSet& aBitset) {
        uint32_t len = aBitset.mBlocks.Length();
        mBlocks.AppendElements(len);
        for (uint32_t i = 0; i < len; ++i) {
            Block *block = aBitset.mBlocks[i];
            if (block)
                mBlocks[i] = new Block(*block);
        }
    }

    bool Equals(const gfxSparseBitSet *aOther) const {
        if (mBlocks.Length() != aOther->mBlocks.Length()) {
            return false;
        }
        size_t n = mBlocks.Length();
        for (size_t i = 0; i < n; ++i) {
            const Block *b1 = mBlocks[i];
            const Block *b2 = aOther->mBlocks[i];
            if (!b1 != !b2) {
                return false;
            }
            if (!b1) {
                continue;
            }
            if (memcmp(&b1->mBits, &b2->mBits, BLOCK_SIZE) != 0) {
                return false;
            }
        }
        return true;
    }

    bool test(uint32_t aIndex) const {
        NS_ASSERTION(mBlocks.DebugGetHeader(), "mHdr is null, this is bad");
        uint32_t blockIndex = aIndex/BLOCK_SIZE_BITS;
        if (blockIndex >= mBlocks.Length())
            return false;
        Block *block = mBlocks[blockIndex];
        if (!block)
            return false;
        return ((block->mBits[(aIndex>>3) & (BLOCK_SIZE - 1)]) & (1 << (aIndex & 0x7))) != 0;
    }

#if PR_LOGGING
    
    void Dump(const char* aPrefix, eGfxLog aWhichLog) const;
#endif

    bool TestRange(uint32_t aStart, uint32_t aEnd) {
        uint32_t startBlock, endBlock, blockLen;
        
        
        startBlock = aStart >> BLOCK_INDEX_SHIFT;
        blockLen = mBlocks.Length();
        if (startBlock >= blockLen) return false;
        
        
        uint32_t blockIndex;
        bool hasBlocksInRange = false;

        endBlock = aEnd >> BLOCK_INDEX_SHIFT;
        for (blockIndex = startBlock; blockIndex <= endBlock; blockIndex++) {
            if (blockIndex < blockLen && mBlocks[blockIndex])
                hasBlocksInRange = true;
        }
        if (!hasBlocksInRange) return false;

        Block *block;
        uint32_t i, start, end;
        
        
        if ((block = mBlocks[startBlock])) {
            start = aStart;
            end = std::min(aEnd, ((startBlock+1) << BLOCK_INDEX_SHIFT) - 1);
            for (i = start; i <= end; i++) {
                if ((block->mBits[(i>>3) & (BLOCK_SIZE - 1)]) & (1 << (i & 0x7)))
                    return true;
            }
        }
        if (endBlock == startBlock) return false;

        
        for (blockIndex = startBlock + 1; blockIndex < endBlock; blockIndex++) {
            uint32_t index;
            
            if (blockIndex >= blockLen || !(block = mBlocks[blockIndex])) continue;
            for (index = 0; index < BLOCK_SIZE; index++) {
                if (block->mBits[index]) 
                    return true;
            }
        }
        
        
        if (endBlock < blockLen && (block = mBlocks[endBlock])) {
            start = endBlock << BLOCK_INDEX_SHIFT;
            end = aEnd;
            for (i = start; i <= end; i++) {
                if ((block->mBits[(i>>3) & (BLOCK_SIZE - 1)]) & (1 << (i & 0x7)))
                    return true;
            }
        }
        
        return false;
    }
    
    void set(uint32_t aIndex) {
        uint32_t blockIndex = aIndex/BLOCK_SIZE_BITS;
        if (blockIndex >= mBlocks.Length()) {
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(blockIndex + 1 - mBlocks.Length());
            if (MOZ_UNLIKELY(!blocks)) 
                return;
        }
        Block *block = mBlocks[blockIndex];
        if (!block) {
            block = new Block;
            mBlocks[blockIndex] = block;
        }
        block->mBits[(aIndex>>3) & (BLOCK_SIZE - 1)] |= 1 << (aIndex & 0x7);
    }

    void set(uint32_t aIndex, bool aValue) {
        if (aValue)
            set(aIndex);
        else
            clear(aIndex);
    }

    void SetRange(uint32_t aStart, uint32_t aEnd) {
        const uint32_t startIndex = aStart/BLOCK_SIZE_BITS;
        const uint32_t endIndex = aEnd/BLOCK_SIZE_BITS;

        if (endIndex >= mBlocks.Length()) {
            uint32_t numNewBlocks = endIndex + 1 - mBlocks.Length();
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(numNewBlocks);
            if (MOZ_UNLIKELY(!blocks)) 
                return;
        }

        for (uint32_t i = startIndex; i <= endIndex; ++i) {
            const uint32_t blockFirstBit = i * BLOCK_SIZE_BITS;
            const uint32_t blockLastBit = blockFirstBit + BLOCK_SIZE_BITS - 1;

            Block *block = mBlocks[i];
            if (!block) {
                bool fullBlock = false;
                if (aStart <= blockFirstBit && aEnd >= blockLastBit)
                    fullBlock = true;

                block = new Block(fullBlock ? 0xFF : 0);
                mBlocks[i] = block;

                if (fullBlock)
                    continue;
            }

            const uint32_t start = aStart > blockFirstBit ? aStart - blockFirstBit : 0;
            const uint32_t end = std::min<uint32_t>(aEnd - blockFirstBit, BLOCK_SIZE_BITS - 1);

            for (uint32_t bit = start; bit <= end; ++bit) {
                block->mBits[bit>>3] |= 1 << (bit & 0x7);
            }
        }
    }

    void clear(uint32_t aIndex) {
        uint32_t blockIndex = aIndex/BLOCK_SIZE_BITS;
        if (blockIndex >= mBlocks.Length()) {
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(blockIndex + 1 - mBlocks.Length());
            if (MOZ_UNLIKELY(!blocks)) 
                return;
        }
        Block *block = mBlocks[blockIndex];
        if (!block) {
            return;
        }
        block->mBits[(aIndex>>3) & (BLOCK_SIZE - 1)] &= ~(1 << (aIndex & 0x7));
    }

    void ClearRange(uint32_t aStart, uint32_t aEnd) {
        const uint32_t startIndex = aStart/BLOCK_SIZE_BITS;
        const uint32_t endIndex = aEnd/BLOCK_SIZE_BITS;

        if (endIndex >= mBlocks.Length()) {
            uint32_t numNewBlocks = endIndex + 1 - mBlocks.Length();
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(numNewBlocks);
            if (MOZ_UNLIKELY(!blocks)) 
                return;
        }

        for (uint32_t i = startIndex; i <= endIndex; ++i) {
            const uint32_t blockFirstBit = i * BLOCK_SIZE_BITS;

            Block *block = mBlocks[i];
            if (!block) {
                
                
                continue;
            }

            const uint32_t start = aStart > blockFirstBit ? aStart - blockFirstBit : 0;
            const uint32_t end = std::min<uint32_t>(aEnd - blockFirstBit, BLOCK_SIZE_BITS - 1);

            for (uint32_t bit = start; bit <= end; ++bit) {
                block->mBits[bit>>3] &= ~(1 << (bit & 0x7));
            }
        }
    }

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        size_t total = mBlocks.SizeOfExcludingThis(aMallocSizeOf);
        for (uint32_t i = 0; i < mBlocks.Length(); i++) {
            if (mBlocks[i]) {
                total += aMallocSizeOf(mBlocks[i]);
            }
        }
        return total;
    }

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
    }

    
    void reset() {
        uint32_t i;
        for (i = 0; i < mBlocks.Length(); i++)
            mBlocks[i] = nullptr;    
    }

    
    void Union(const gfxSparseBitSet& aBitset) {
        
        uint32_t blockCount = aBitset.mBlocks.Length();
        if (blockCount > mBlocks.Length()) {
            uint32_t needed = blockCount - mBlocks.Length();
            nsAutoPtr<Block> *blocks = mBlocks.AppendElements(needed);
            if (MOZ_UNLIKELY(!blocks)) { 
                return;
            }
        }
        
        for (uint32_t i = 0; i < blockCount; ++i) {
            
            if (!aBitset.mBlocks[i]) {
                continue;
            }
            
            if (!mBlocks[i]) {
                mBlocks[i] = new Block(*aBitset.mBlocks[i]);
                continue;
            }
            
            uint32_t *dst = reinterpret_cast<uint32_t*>(mBlocks[i]->mBits);
            const uint32_t *src =
                reinterpret_cast<const uint32_t*>(aBitset.mBlocks[i]->mBits);
            for (uint32_t j = 0; j < BLOCK_SIZE / 4; ++j) {
                dst[j] |= src[j];
            }
        }
    }

    void Compact() {
        mBlocks.Compact();
    }

    uint32_t GetChecksum() const {
        uint32_t check = adler32(0, Z_NULL, 0);
        for (uint32_t i = 0; i < mBlocks.Length(); i++) {
            if (mBlocks[i]) {
                const Block *block = mBlocks[i];
                check = adler32(check, (uint8_t*) (&i), 4);
                check = adler32(check, (uint8_t*) block, sizeof(Block));
            }
        }
        return check;
    }

private:
    nsTArray< nsAutoPtr<Block> > mBlocks;
};

#define TRUETYPE_TAG(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

namespace mozilla {



#pragma pack(1)

struct AutoSwap_PRUint16 {
#ifdef __SUNPRO_CC
    AutoSwap_PRUint16& operator = (const uint16_t aValue)
    {
        this->value = mozilla::NativeEndian::swapToBigEndian(aValue);
        return *this;
    }
#else
    MOZ_IMPLICIT AutoSwap_PRUint16(uint16_t aValue)
    {
        value = mozilla::NativeEndian::swapToBigEndian(aValue);
    }
#endif
    operator uint16_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

    operator uint32_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

    operator uint64_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

private:
    uint16_t value;
};

struct AutoSwap_PRInt16 {
#ifdef __SUNPRO_CC
    AutoSwap_PRInt16& operator = (const int16_t aValue)
    {
        this->value = mozilla::NativeEndian::swapToBigEndian(aValue);
        return *this;
    }
#else
    MOZ_IMPLICIT AutoSwap_PRInt16(int16_t aValue)
    {
        value = mozilla::NativeEndian::swapToBigEndian(aValue);
    }
#endif
    operator int16_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

    operator uint32_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

private:
    int16_t  value;
};

struct AutoSwap_PRUint32 {
#ifdef __SUNPRO_CC
    AutoSwap_PRUint32& operator = (const uint32_t aValue)
    {
        this->value = mozilla::NativeEndian::swapToBigEndian(aValue);
        return *this;
    }
#else
    MOZ_IMPLICIT AutoSwap_PRUint32(uint32_t aValue)
    {
        value = mozilla::NativeEndian::swapToBigEndian(aValue);
    }
#endif
    operator uint32_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

private:
    uint32_t  value;
};

struct AutoSwap_PRInt32 {
#ifdef __SUNPRO_CC
    AutoSwap_PRInt32& operator = (const int32_t aValue)
    {
        this->value = mozilla::NativeEndian::swapToBigEndian(aValue);
        return *this;
    }
#else
    MOZ_IMPLICIT AutoSwap_PRInt32(int32_t aValue)
    {
        value = mozilla::NativeEndian::swapToBigEndian(aValue);
    }
#endif
    operator int32_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

private:
    int32_t  value;
};

struct AutoSwap_PRUint64 {
#ifdef __SUNPRO_CC
    AutoSwap_PRUint64& operator = (const uint64_t aValue)
    {
        this->value = mozilla::NativeEndian::swapToBigEndian(aValue);
        return *this;
    }
#else
    MOZ_IMPLICIT AutoSwap_PRUint64(uint64_t aValue)
    {
        value = mozilla::NativeEndian::swapToBigEndian(aValue);
    }
#endif
    operator uint64_t() const
    {
        return mozilla::NativeEndian::swapFromBigEndian(value);
    }

private:
    uint64_t  value;
};

struct AutoSwap_PRUint24 {
    operator uint32_t() const { return value[0] << 16 | value[1] << 8 | value[2]; }
private:
    AutoSwap_PRUint24() { }
    uint8_t  value[3];
};

struct SFNTHeader {
    AutoSwap_PRUint32    sfntVersion;            
    AutoSwap_PRUint16    numTables;              
    AutoSwap_PRUint16    searchRange;            
    AutoSwap_PRUint16    entrySelector;          
    AutoSwap_PRUint16    rangeShift;             
};

struct TableDirEntry {
    AutoSwap_PRUint32    tag;                    
    AutoSwap_PRUint32    checkSum;               
    AutoSwap_PRUint32    offset;                 
    AutoSwap_PRUint32    length;                 
};

struct HeadTable {
    enum {
        HEAD_VERSION = 0x00010000,
        HEAD_MAGIC_NUMBER = 0x5F0F3CF5,
        HEAD_CHECKSUM_CALC_CONST = 0xB1B0AFBA
    };

    AutoSwap_PRUint32    tableVersionNumber;    
    AutoSwap_PRUint32    fontRevision;          
    AutoSwap_PRUint32    checkSumAdjustment;    
    AutoSwap_PRUint32    magicNumber;           
    AutoSwap_PRUint16    flags;
    AutoSwap_PRUint16    unitsPerEm;            
    AutoSwap_PRUint64    created;               
    AutoSwap_PRUint64    modified;              
    AutoSwap_PRInt16     xMin;                  
    AutoSwap_PRInt16     yMin;                  
    AutoSwap_PRInt16     xMax;                  
    AutoSwap_PRInt16     yMax;                  
    AutoSwap_PRUint16    macStyle;              
    AutoSwap_PRUint16    lowestRecPPEM;         
    AutoSwap_PRInt16     fontDirectionHint;
    AutoSwap_PRInt16     indexToLocFormat;
    AutoSwap_PRInt16     glyphDataFormat;
};

struct OS2Table {
    AutoSwap_PRUint16    version;                
    AutoSwap_PRInt16     xAvgCharWidth;
    AutoSwap_PRUint16    usWeightClass;
    AutoSwap_PRUint16    usWidthClass;
    AutoSwap_PRUint16    fsType;
    AutoSwap_PRInt16     ySubscriptXSize;
    AutoSwap_PRInt16     ySubscriptYSize;
    AutoSwap_PRInt16     ySubscriptXOffset;
    AutoSwap_PRInt16     ySubscriptYOffset;
    AutoSwap_PRInt16     ySuperscriptXSize;
    AutoSwap_PRInt16     ySuperscriptYSize;
    AutoSwap_PRInt16     ySuperscriptXOffset;
    AutoSwap_PRInt16     ySuperscriptYOffset;
    AutoSwap_PRInt16     yStrikeoutSize;
    AutoSwap_PRInt16     yStrikeoutPosition;
    AutoSwap_PRInt16     sFamilyClass;
    uint8_t              panose[10];
    AutoSwap_PRUint32    unicodeRange1;
    AutoSwap_PRUint32    unicodeRange2;
    AutoSwap_PRUint32    unicodeRange3;
    AutoSwap_PRUint32    unicodeRange4;
    uint8_t              achVendID[4];
    AutoSwap_PRUint16    fsSelection;
    AutoSwap_PRUint16    usFirstCharIndex;
    AutoSwap_PRUint16    usLastCharIndex;
    AutoSwap_PRInt16     sTypoAscender;
    AutoSwap_PRInt16     sTypoDescender;
    AutoSwap_PRInt16     sTypoLineGap;
    AutoSwap_PRUint16    usWinAscent;
    AutoSwap_PRUint16    usWinDescent;
    AutoSwap_PRUint32    codePageRange1;
    AutoSwap_PRUint32    codePageRange2;
    AutoSwap_PRInt16     sxHeight;
    AutoSwap_PRInt16     sCapHeight;
    AutoSwap_PRUint16    usDefaultChar;
    AutoSwap_PRUint16    usBreakChar;
    AutoSwap_PRUint16    usMaxContext;
};

struct PostTable {
    AutoSwap_PRUint32    version;
    AutoSwap_PRInt32     italicAngle;
    AutoSwap_PRInt16     underlinePosition;
    AutoSwap_PRUint16    underlineThickness;
    AutoSwap_PRUint32    isFixedPitch;
    AutoSwap_PRUint32    minMemType42;
    AutoSwap_PRUint32    maxMemType42;
    AutoSwap_PRUint32    minMemType1;
    AutoSwap_PRUint32    maxMemType1;
};




struct MetricsHeader {
    AutoSwap_PRUint32    version;
    AutoSwap_PRInt16     ascender;
    AutoSwap_PRInt16     descender;
    AutoSwap_PRInt16     lineGap;
    AutoSwap_PRUint16    advanceWidthMax;
    AutoSwap_PRInt16     minLeftSideBearing;
    AutoSwap_PRInt16     minRightSideBearing;
    AutoSwap_PRInt16     xMaxExtent;
    AutoSwap_PRInt16     caretSlopeRise;
    AutoSwap_PRInt16     caretSlopeRun;
    AutoSwap_PRInt16     caretOffset;
    AutoSwap_PRInt16     reserved1;
    AutoSwap_PRInt16     reserved2;
    AutoSwap_PRInt16     reserved3;
    AutoSwap_PRInt16     reserved4;
    AutoSwap_PRInt16     metricDataFormat;
    AutoSwap_PRUint16    numOfLongMetrics;
};

struct MaxpTableHeader {
    AutoSwap_PRUint32    version; 
    AutoSwap_PRUint16    numGlyphs;

};



struct KernTableVersion0 {
    AutoSwap_PRUint16    version; 
    AutoSwap_PRUint16    nTables;
};

struct KernTableSubtableHeaderVersion0 {
    AutoSwap_PRUint16    version;
    AutoSwap_PRUint16    length;
    AutoSwap_PRUint16    coverage;
};



struct KernTableVersion1 {
    AutoSwap_PRUint32    version; 
    AutoSwap_PRUint32    nTables;
};

struct KernTableSubtableHeaderVersion1 {
    AutoSwap_PRUint32    length;
    AutoSwap_PRUint16    coverage;
    AutoSwap_PRUint16    tupleIndex;
};

struct COLRHeader {
    AutoSwap_PRUint16    version;
    AutoSwap_PRUint16    numBaseGlyphRecord;
    AutoSwap_PRUint32    offsetBaseGlyphRecord;
    AutoSwap_PRUint32    offsetLayerRecord;
    AutoSwap_PRUint16    numLayerRecords;
};

struct CPALHeaderVersion0 {
    AutoSwap_PRUint16    version;
    AutoSwap_PRUint16    numPaletteEntries;
    AutoSwap_PRUint16    numPalettes;
    AutoSwap_PRUint16    numColorRecords;
    AutoSwap_PRUint32    offsetFirstColorRecord;
};

#pragma pack()



inline uint32_t
FindHighestBit(uint32_t value)
{
    
    value |= (value >> 1);
    value |= (value >> 2);
    value |= (value >> 4);
    value |= (value >> 8);
    value |= (value >> 16);
    
    return (value & ~(value >> 1));
}

} 


struct FontDataOverlay {
    
    uint32_t  overlaySrc;    
    uint32_t  overlaySrcLen; 
    uint32_t  overlayDest;   
};
    
enum gfxUserFontType {
    GFX_USERFONT_UNKNOWN = 0,
    GFX_USERFONT_OPENTYPE = 1,
    GFX_USERFONT_SVG = 2,
    GFX_USERFONT_WOFF = 3,
    GFX_USERFONT_WOFF2 = 4
};
#define GFX_PREF_WOFF2_ENABLED "gfx.downloadable_fonts.woff2.enabled"

extern const uint8_t sCJKCompatSVSTable[];

class gfxFontUtils {

public:
    
    enum {
        NAME_ID_FAMILY = 1,
        NAME_ID_STYLE = 2,
        NAME_ID_UNIQUE = 3,
        NAME_ID_FULL = 4,
        NAME_ID_VERSION = 5,
        NAME_ID_POSTSCRIPT = 6,
        NAME_ID_PREFERRED_FAMILY = 16,
        NAME_ID_PREFERRED_STYLE = 17,

        PLATFORM_ALL = -1,
        PLATFORM_ID_UNICODE = 0,           
        PLATFORM_ID_MAC = 1,
        PLATFORM_ID_ISO = 2,
        PLATFORM_ID_MICROSOFT = 3,

        ENCODING_ID_MAC_ROMAN = 0,         
        ENCODING_ID_MAC_JAPANESE = 1,      
        ENCODING_ID_MAC_TRAD_CHINESE = 2,  
        ENCODING_ID_MAC_KOREAN = 3,        
        ENCODING_ID_MAC_ARABIC = 4,
        ENCODING_ID_MAC_HEBREW = 5,
        ENCODING_ID_MAC_GREEK = 6,
        ENCODING_ID_MAC_CYRILLIC = 7,
        ENCODING_ID_MAC_DEVANAGARI = 9,
        ENCODING_ID_MAC_GURMUKHI = 10,
        ENCODING_ID_MAC_GUJARATI = 11,
        ENCODING_ID_MAC_SIMP_CHINESE = 25,

        ENCODING_ID_MICROSOFT_SYMBOL = 0,  
        ENCODING_ID_MICROSOFT_UNICODEBMP = 1,
        ENCODING_ID_MICROSOFT_SHIFTJIS = 2,
        ENCODING_ID_MICROSOFT_PRC = 3,
        ENCODING_ID_MICROSOFT_BIG5 = 4,
        ENCODING_ID_MICROSOFT_WANSUNG = 5,
        ENCODING_ID_MICROSOFT_JOHAB  = 6,
        ENCODING_ID_MICROSOFT_UNICODEFULL = 10,

        LANG_ALL = -1,
        LANG_ID_MAC_ENGLISH = 0,      
        LANG_ID_MAC_HEBREW = 10,      
        LANG_ID_MAC_JAPANESE = 11,    
        LANG_ID_MAC_ARABIC = 12,
        LANG_ID_MAC_ICELANDIC = 15,
        LANG_ID_MAC_TURKISH = 17,
        LANG_ID_MAC_TRAD_CHINESE = 19,
        LANG_ID_MAC_URDU = 20,
        LANG_ID_MAC_KOREAN = 23,
        LANG_ID_MAC_POLISH = 25,
        LANG_ID_MAC_FARSI = 31,
        LANG_ID_MAC_SIMP_CHINESE = 33,
        LANG_ID_MAC_ROMANIAN = 37,
        LANG_ID_MAC_CZECH = 38,
        LANG_ID_MAC_SLOVAK = 39,

        LANG_ID_MICROSOFT_EN_US = 0x0409,        
        
        CMAP_MAX_CODEPOINT = 0x10ffff     
                                          
    };

    
    struct NameHeader {
        mozilla::AutoSwap_PRUint16    format;       
        mozilla::AutoSwap_PRUint16    count;        
        mozilla::AutoSwap_PRUint16    stringOffset; 
                                                    
    };

    struct NameRecord {
        mozilla::AutoSwap_PRUint16    platformID;   
        mozilla::AutoSwap_PRUint16    encodingID;   
        mozilla::AutoSwap_PRUint16    languageID;   
        mozilla::AutoSwap_PRUint16    nameID;       
        mozilla::AutoSwap_PRUint16    length;       
        mozilla::AutoSwap_PRUint16    offset;       
                                                    
    };

    

    static inline uint16_t
    ReadShortAt(const uint8_t *aBuf, uint32_t aIndex)
    {
        return (aBuf[aIndex] << 8) | aBuf[aIndex + 1];
    }

    static inline uint16_t
    ReadShortAt16(const uint16_t *aBuf, uint32_t aIndex)
    {
        const uint8_t *buf = reinterpret_cast<const uint8_t*>(aBuf);
        uint32_t index = aIndex << 1;
        return (buf[index] << 8) | buf[index+1];
    }

    static inline uint32_t
    ReadUint24At(const uint8_t *aBuf, uint32_t aIndex)
    {
        return ((aBuf[aIndex] << 16) | (aBuf[aIndex + 1] << 8) |
                (aBuf[aIndex + 2]));
    }

    static inline uint32_t
    ReadLongAt(const uint8_t *aBuf, uint32_t aIndex)
    {
        return ((aBuf[aIndex] << 24) | (aBuf[aIndex + 1] << 16) | 
                (aBuf[aIndex + 2] << 8) | (aBuf[aIndex + 3]));
    }

    static nsresult
    ReadCMAPTableFormat10(const uint8_t *aBuf, uint32_t aLength,
                          gfxSparseBitSet& aCharacterMap);

    static nsresult
    ReadCMAPTableFormat12(const uint8_t *aBuf, uint32_t aLength, 
                          gfxSparseBitSet& aCharacterMap);

    static nsresult 
    ReadCMAPTableFormat4(const uint8_t *aBuf, uint32_t aLength, 
                         gfxSparseBitSet& aCharacterMap);

    static nsresult
    ReadCMAPTableFormat14(const uint8_t *aBuf, uint32_t aLength, 
                          uint8_t*& aTable);

    static uint32_t
    FindPreferredSubtable(const uint8_t *aBuf, uint32_t aBufLength,
                          uint32_t *aTableOffset, uint32_t *aUVSTableOffset,
                          bool *aSymbolEncoding);

    static nsresult
    ReadCMAP(const uint8_t *aBuf, uint32_t aBufLength,
             gfxSparseBitSet& aCharacterMap,
             uint32_t& aUVSOffset,
             bool& aUnicodeFont, bool& aSymbolFont);

    static uint32_t
    MapCharToGlyphFormat4(const uint8_t *aBuf, char16_t aCh);

    static uint32_t
    MapCharToGlyphFormat10(const uint8_t *aBuf, uint32_t aCh);

    static uint32_t
    MapCharToGlyphFormat12(const uint8_t *aBuf, uint32_t aCh);

    static uint16_t
    MapUVSToGlyphFormat14(const uint8_t *aBuf, uint32_t aCh, uint32_t aVS);

    
    
    
    static MOZ_ALWAYS_INLINE uint32_t
    GetUVSFallback(uint32_t aCh, uint32_t aVS) {
        aCh = MapUVSToGlyphFormat14(sCJKCompatSVSTable, aCh, aVS);
        return aCh >= 0xFB00 ? aCh + (0x2F800 - 0xFB00) : aCh;
    }

    static uint32_t
    MapCharToGlyph(const uint8_t *aCmapBuf, uint32_t aBufLength,
                   uint32_t aUnicode, uint32_t aVarSelector = 0);

#ifdef XP_WIN
    
    
    static bool
    IsCffFont(const uint8_t* aFontData);
#endif

    
    static gfxUserFontType
    DetermineFontDataType(const uint8_t *aFontData, uint32_t aFontDataLength);

    
    
    
    
    static nsresult
    GetFullNameFromSFNT(const uint8_t* aFontData, uint32_t aLength,
                        nsAString& aFullName);

    
    
    static nsresult
    GetFullNameFromTable(hb_blob_t *aNameTable,
                         nsAString& aFullName);

    
    static nsresult
    GetFamilyNameFromTable(hb_blob_t *aNameTable,
                           nsAString& aFamilyName);

    
    
    static nsresult
    RenameFont(const nsAString& aName, const uint8_t *aFontData, 
               uint32_t aFontDataLength, FallibleTArray<uint8_t> *aNewFont);
    
    
    static nsresult
    ReadNames(const char *aNameData, uint32_t aDataLen, uint32_t aNameID,
              int32_t aPlatformID, nsTArray<nsString>& aNames);

    
    
    static nsresult
    ReadCanonicalName(hb_blob_t *aNameTable, uint32_t aNameID,
                      nsString& aName);

    static nsresult
    ReadCanonicalName(const char *aNameData, uint32_t aDataLen,
                      uint32_t aNameID, nsString& aName);

    
    
    
    static bool
    DecodeFontName(const char *aBuf, int32_t aLength, 
                   uint32_t aPlatformCode, uint32_t aScriptCode,
                   uint32_t aLangCode, nsAString& dest);

    static inline bool IsJoinCauser(uint32_t ch) {
        return (ch == 0x200D);
    }

    static inline bool IsJoinControl(uint32_t ch) {
        return (ch == 0x200C || ch == 0x200D);
    }

    enum {
        kUnicodeVS1 = 0xFE00,
        kUnicodeVS16 = 0xFE0F,
        kUnicodeVS17 = 0xE0100,
        kUnicodeVS256 = 0xE01EF
    };

    static inline bool IsVarSelector(uint32_t ch) {
        return (ch >= kUnicodeVS1 && ch <= kUnicodeVS16) ||
               (ch >= kUnicodeVS17 && ch <= kUnicodeVS256);
    }

    static inline bool IsInvalid(uint32_t ch) {
        return (ch == 0xFFFD);
    }

    
    
    
    enum {
        kUnicodeBidiScriptsStart = 0x0590,
        kUnicodeBidiScriptsEnd = 0x08FF,
        kUnicodeBidiPresentationStart = 0xFB1D,
        kUnicodeBidiPresentationEnd = 0xFEFC,
        kUnicodeFirstHighSurrogateBlock = 0xD800,
        kUnicodeRLM = 0x200F,
        kUnicodeRLE = 0x202B,
        kUnicodeRLO = 0x202E
    };

    static inline bool PotentialRTLChar(char16_t aCh) {
        if (aCh >= kUnicodeBidiScriptsStart && aCh <= kUnicodeBidiScriptsEnd)
            
            return true;

        if (aCh == kUnicodeRLM || aCh == kUnicodeRLE || aCh == kUnicodeRLO)
            
            return true;

        if (aCh >= kUnicodeBidiPresentationStart &&
            aCh <= kUnicodeBidiPresentationEnd)
            
            return true;

        if ((aCh & 0xFF00) == kUnicodeFirstHighSurrogateBlock)
            
            
            return true;

        
        return false;
    }

    
    
    static void ParseFontList(const nsAString& aFamilyList,
                              nsTArray<nsString>& aFontList);

    
    static void AppendPrefsFontList(const char *aPrefName,
                                    nsTArray<nsString>& aFontList);

    
    static void GetPrefsFontList(const char *aPrefName, 
                                 nsTArray<nsString>& aFontList);

    
    static nsresult MakeUniqueUserFontName(nsAString& aName);

    
    static bool ValidateColorGlyphs(hb_blob_t* aCOLR, hb_blob_t* aCPAL);
    static bool GetColorGlyphLayers(hb_blob_t* aCOLR,
                                    hb_blob_t* aCPAL,
                                    uint32_t aGlyphId,
                                    nsTArray<uint16_t> &aGlyphs,
                                    nsTArray<mozilla::gfx::Color> &aColors);

protected:
    friend struct MacCharsetMappingComparator;

    static nsresult
    ReadNames(const char *aNameData, uint32_t aDataLen, uint32_t aNameID,
              int32_t aLangID, int32_t aPlatformID, nsTArray<nsString>& aNames);

    
    
    static const char*
    GetCharsetForFontName(uint16_t aPlatform, uint16_t aScript, uint16_t aLanguage);

    struct MacFontNameCharsetMapping {
        uint16_t    mEncoding;
        uint16_t    mLanguage;
        const char *mCharsetName;

        bool operator<(const MacFontNameCharsetMapping& rhs) const {
            return (mEncoding < rhs.mEncoding) ||
                   ((mEncoding == rhs.mEncoding) && (mLanguage < rhs.mLanguage));
        }
    };
    static const MacFontNameCharsetMapping gMacFontNameCharsets[];
    static const char* gISOFontNameCharsets[];
    static const char* gMSFontNameCharsets[];
};


#endif
