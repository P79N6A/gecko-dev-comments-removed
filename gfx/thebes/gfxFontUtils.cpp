




#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#include "prlog.h"
#endif

#include "mozilla/ArrayUtils.h"

#include "gfxFontUtils.h"

#include "nsServiceManagerUtils.h"

#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"

#include "nsCOMPtr.h"
#include "nsIUUIDGenerator.h"
#include "nsIUnicodeDecoder.h"

#include "harfbuzz/hb.h"

#include "plbase64.h"
#include "prlog.h"

#ifdef PR_LOGGING

#define LOG(log, args) PR_LOG(gfxPlatform::GetLog(log), \
                               PR_LOG_DEBUG, args)

#endif 

#define UNICODE_BMP_LIMIT 0x10000

using namespace mozilla;

#pragma pack(1)

typedef struct {
    AutoSwap_PRUint16 format;
    AutoSwap_PRUint16 reserved;
    AutoSwap_PRUint32 length;
    AutoSwap_PRUint32 language;
    AutoSwap_PRUint32 numGroups;
} Format12CmapHeader;

typedef struct {
    AutoSwap_PRUint32 startCharCode;
    AutoSwap_PRUint32 endCharCode;
    AutoSwap_PRUint32 startGlyphId;
} Format12Group;

#pragma pack()

#if PR_LOGGING
void
gfxSparseBitSet::Dump(const char* aPrefix, eGfxLog aWhichLog) const
{
    NS_ASSERTION(mBlocks.DebugGetHeader(), "mHdr is null, this is bad");
    uint32_t b, numBlocks = mBlocks.Length();

    for (b = 0; b < numBlocks; b++) {
        Block *block = mBlocks[b];
        if (!block) continue;
        char outStr[256];
        int index = 0;
        index += sprintf(&outStr[index], "%s u+%6.6x [", aPrefix, (b << BLOCK_INDEX_SHIFT));
        for (int i = 0; i < 32; i += 4) {
            for (int j = i; j < i + 4; j++) {
                uint8_t bits = block->mBits[j];
                uint8_t flip1 = ((bits & 0xaa) >> 1) | ((bits & 0x55) << 1);
                uint8_t flip2 = ((flip1 & 0xcc) >> 2) | ((flip1 & 0x33) << 2);
                uint8_t flipped = ((flip2 & 0xf0) >> 4) | ((flip2 & 0x0f) << 4);

                index += sprintf(&outStr[index], "%2.2x", flipped);
            }
            if (i + 4 != 32) index += sprintf(&outStr[index], " ");
        }
        index += sprintf(&outStr[index], "]");
        LOG(aWhichLog, ("%s", outStr));
    }
}
#endif


nsresult
gfxFontUtils::ReadCMAPTableFormat12(const uint8_t *aBuf, uint32_t aLength,
                                    gfxSparseBitSet& aCharacterMap) 
{
    
    NS_ENSURE_TRUE(aLength >= sizeof(Format12CmapHeader),
                    NS_ERROR_GFX_CMAP_MALFORMED);

    
    const Format12CmapHeader *cmap12 =
        reinterpret_cast<const Format12CmapHeader*>(aBuf);
    NS_ENSURE_TRUE(uint16_t(cmap12->format) == 12, 
                   NS_ERROR_GFX_CMAP_MALFORMED);
    NS_ENSURE_TRUE(uint16_t(cmap12->reserved) == 0, 
                   NS_ERROR_GFX_CMAP_MALFORMED);

    uint32_t tablelen = cmap12->length;
    NS_ENSURE_TRUE(tablelen >= sizeof(Format12CmapHeader) &&
                   tablelen <= aLength, NS_ERROR_GFX_CMAP_MALFORMED);

    NS_ENSURE_TRUE(cmap12->language == 0, NS_ERROR_GFX_CMAP_MALFORMED);

    
    const uint32_t numGroups = cmap12->numGroups;
    NS_ENSURE_TRUE((tablelen - sizeof(Format12CmapHeader)) /
                       sizeof(Format12Group) >= numGroups,
                   NS_ERROR_GFX_CMAP_MALFORMED);

    
    const Format12Group *group =
        reinterpret_cast<const Format12Group*>(aBuf + sizeof(Format12CmapHeader));

    
    
    uint32_t prevEndCharCode = 0;
    for (uint32_t i = 0; i < numGroups; i++, group++) {
        uint32_t startCharCode = group->startCharCode;
        const uint32_t endCharCode = group->endCharCode;
        NS_ENSURE_TRUE((prevEndCharCode < startCharCode || i == 0) &&
                       startCharCode <= endCharCode &&
                       endCharCode <= CMAP_MAX_CODEPOINT, 
                       NS_ERROR_GFX_CMAP_MALFORMED);
        
        if (group->startGlyphId == 0) {
            startCharCode++;
        }
        if (startCharCode <= endCharCode) {
            aCharacterMap.SetRange(startCharCode, endCharCode);
        }
        prevEndCharCode = endCharCode;
    }

    aCharacterMap.Compact();

    return NS_OK;
}

nsresult 
gfxFontUtils::ReadCMAPTableFormat4(const uint8_t *aBuf, uint32_t aLength,
                                   gfxSparseBitSet& aCharacterMap)
{
    enum {
        OffsetFormat = 0,
        OffsetLength = 2,
        OffsetLanguage = 4,
        OffsetSegCountX2 = 6
    };

    NS_ENSURE_TRUE(ReadShortAt(aBuf, OffsetFormat) == 4, 
                   NS_ERROR_GFX_CMAP_MALFORMED);
    uint16_t tablelen = ReadShortAt(aBuf, OffsetLength);
    NS_ENSURE_TRUE(tablelen <= aLength, NS_ERROR_GFX_CMAP_MALFORMED);
    NS_ENSURE_TRUE(tablelen > 16, NS_ERROR_GFX_CMAP_MALFORMED);
    
    
    
    
    
    NS_ENSURE_TRUE((ReadShortAt(aBuf, OffsetLanguage) & 0xfffe) == 0, 
                   NS_ERROR_GFX_CMAP_MALFORMED);

    uint16_t segCountX2 = ReadShortAt(aBuf, OffsetSegCountX2);
    NS_ENSURE_TRUE(tablelen >= 16 + (segCountX2 * 4), 
                   NS_ERROR_GFX_CMAP_MALFORMED);

    const uint16_t segCount = segCountX2 / 2;

    const uint16_t *endCounts = reinterpret_cast<const uint16_t*>(aBuf + 14);
    const uint16_t *startCounts = endCounts + 1  + segCount;
    const uint16_t *idDeltas = startCounts + segCount;
    const uint16_t *idRangeOffsets = idDeltas + segCount;
    uint16_t prevEndCount = 0;
    for (uint16_t i = 0; i < segCount; i++) {
        const uint16_t endCount = ReadShortAt16(endCounts, i);
        const uint16_t startCount = ReadShortAt16(startCounts, i);
        const uint16_t idRangeOffset = ReadShortAt16(idRangeOffsets, i);

        
        
        
        
        NS_ENSURE_TRUE(startCount >= prevEndCount && startCount <= endCount,
                       NS_ERROR_GFX_CMAP_MALFORMED);
        prevEndCount = endCount;

        if (idRangeOffset == 0) {
            
            
            
            const uint16_t skipCode = 65536 - ReadShortAt16(idDeltas, i);
            if (startCount < skipCode) {
                aCharacterMap.SetRange(startCount,
                                       std::min<uint16_t>(skipCode - 1,
                                                          endCount));
            }
            if (skipCode < endCount) {
                aCharacterMap.SetRange(std::max<uint16_t>(startCount,
                                                          skipCode + 1),
                                       endCount);
            }
        } else {
            
            for (uint32_t c = startCount; c <= endCount; ++c) {
                if (c == 0xFFFF)
                    break;

                const uint16_t *gdata = (idRangeOffset/2 
                                         + (c - startCount)
                                         + &idRangeOffsets[i]);

                NS_ENSURE_TRUE((uint8_t*)gdata > aBuf && 
                               (uint8_t*)gdata < aBuf + aLength, 
                               NS_ERROR_GFX_CMAP_MALFORMED);

                
                if (*gdata != 0) {
                    
                    uint16_t glyph = ReadShortAt16(idDeltas, i) + *gdata;
                    if (glyph) {
                        aCharacterMap.set(c);
                    }
                }
            }
        }
    }

    aCharacterMap.Compact();

    return NS_OK;
}

nsresult
gfxFontUtils::ReadCMAPTableFormat14(const uint8_t *aBuf, uint32_t aLength,
                                    uint8_t*& aTable)
{
    enum {
        OffsetFormat = 0,
        OffsetTableLength = 2,
        OffsetNumVarSelectorRecords = 6,
        OffsetVarSelectorRecords = 10,

        SizeOfVarSelectorRecord = 11,
        VSRecOffsetVarSelector = 0,
        VSRecOffsetDefUVSOffset = 3,
        VSRecOffsetNonDefUVSOffset = 7,

        SizeOfDefUVSTable = 4,
        DefUVSOffsetStartUnicodeValue = 0,
        DefUVSOffsetAdditionalCount = 3,

        SizeOfNonDefUVSTable = 5,
        NonDefUVSOffsetUnicodeValue = 0,
        NonDefUVSOffsetGlyphID = 3
    };
    NS_ENSURE_TRUE(aLength >= OffsetVarSelectorRecords,
                   NS_ERROR_GFX_CMAP_MALFORMED);

    NS_ENSURE_TRUE(ReadShortAt(aBuf, OffsetFormat) == 14, 
                   NS_ERROR_GFX_CMAP_MALFORMED);

    uint32_t tablelen = ReadLongAt(aBuf, OffsetTableLength);
    NS_ENSURE_TRUE(tablelen <= aLength, NS_ERROR_GFX_CMAP_MALFORMED);
    NS_ENSURE_TRUE(tablelen >= OffsetVarSelectorRecords,
                   NS_ERROR_GFX_CMAP_MALFORMED);

    const uint32_t numVarSelectorRecords = ReadLongAt(aBuf, OffsetNumVarSelectorRecords);
    NS_ENSURE_TRUE((tablelen - OffsetVarSelectorRecords) /
                   SizeOfVarSelectorRecord >= numVarSelectorRecords,
                   NS_ERROR_GFX_CMAP_MALFORMED);

    const uint8_t *records = aBuf + OffsetVarSelectorRecords;
    for (uint32_t i = 0; i < numVarSelectorRecords; 
         i++, records += SizeOfVarSelectorRecord) {
        const uint32_t varSelector = ReadUint24At(records, VSRecOffsetVarSelector);
        const uint32_t defUVSOffset = ReadLongAt(records, VSRecOffsetDefUVSOffset);
        const uint32_t nonDefUVSOffset = ReadLongAt(records, VSRecOffsetNonDefUVSOffset);
        NS_ENSURE_TRUE(varSelector <= CMAP_MAX_CODEPOINT &&
                       defUVSOffset <= tablelen - 4 &&
                       nonDefUVSOffset <= tablelen - 4, 
                       NS_ERROR_GFX_CMAP_MALFORMED);

        if (defUVSOffset) {
            const uint32_t numUnicodeValueRanges = ReadLongAt(aBuf, defUVSOffset);
            NS_ENSURE_TRUE((tablelen - defUVSOffset) /
                           SizeOfDefUVSTable >= numUnicodeValueRanges,
                           NS_ERROR_GFX_CMAP_MALFORMED);
            const uint8_t *tables = aBuf + defUVSOffset + 4;
            uint32_t prevEndUnicode = 0;
            for (uint32_t j = 0; j < numUnicodeValueRanges; j++, tables += SizeOfDefUVSTable) {
                const uint32_t startUnicode = ReadUint24At(tables, DefUVSOffsetStartUnicodeValue);
                const uint32_t endUnicode = startUnicode + tables[DefUVSOffsetAdditionalCount];
                NS_ENSURE_TRUE((prevEndUnicode < startUnicode || j == 0) &&
                               endUnicode <= CMAP_MAX_CODEPOINT, 
                               NS_ERROR_GFX_CMAP_MALFORMED);
                prevEndUnicode = endUnicode;
            }
        }

        if (nonDefUVSOffset) {
            const uint32_t numUVSMappings = ReadLongAt(aBuf, nonDefUVSOffset);
            NS_ENSURE_TRUE((tablelen - nonDefUVSOffset) /
                           SizeOfNonDefUVSTable >= numUVSMappings,
                           NS_ERROR_GFX_CMAP_MALFORMED);
            const uint8_t *tables = aBuf + nonDefUVSOffset + 4;
            uint32_t prevUnicode = 0;
            for (uint32_t j = 0; j < numUVSMappings; j++, tables += SizeOfNonDefUVSTable) {
                const uint32_t unicodeValue = ReadUint24At(tables, NonDefUVSOffsetUnicodeValue);
                NS_ENSURE_TRUE((prevUnicode < unicodeValue || j == 0) &&
                               unicodeValue <= CMAP_MAX_CODEPOINT, 
                               NS_ERROR_GFX_CMAP_MALFORMED);
                prevUnicode = unicodeValue;
            }
        }
    }

    aTable = new uint8_t[tablelen];
    memcpy(aTable, aBuf, tablelen);

    return NS_OK;
}





#if defined(XP_MACOSX)
    #define acceptableFormat4(p,e,k) (((p) == PLATFORM_ID_MICROSOFT && (e) == EncodingIDMicrosoft && !(k)) || \
                                      ((p) == PLATFORM_ID_UNICODE))

    #define acceptableUCS4Encoding(p, e, k) \
        (((p) == PLATFORM_ID_MICROSOFT && (e) == EncodingIDUCS4ForMicrosoftPlatform) && (k) != 12 || \
         ((p) == PLATFORM_ID_UNICODE   && \
          ((e) == EncodingIDDefaultForUnicodePlatform || (e) >= EncodingIDUCS4ForUnicodePlatform)))
#else
    #define acceptableFormat4(p,e,k) ((p) == PLATFORM_ID_MICROSOFT && (e) == EncodingIDMicrosoft)

    #define acceptableUCS4Encoding(p, e, k) \
        ((p) == PLATFORM_ID_MICROSOFT && (e) == EncodingIDUCS4ForMicrosoftPlatform)
#endif

#define acceptablePlatform(p) ((p) == PLATFORM_ID_UNICODE || (p) == PLATFORM_ID_MICROSOFT)
#define isSymbol(p,e)         ((p) == PLATFORM_ID_MICROSOFT && (e) == EncodingIDSymbol)
#define isUVSEncoding(p, e)   ((p) == PLATFORM_ID_UNICODE && (e) == EncodingIDUVSForUnicodePlatform)

uint32_t
gfxFontUtils::FindPreferredSubtable(const uint8_t *aBuf, uint32_t aBufLength,
                                    uint32_t *aTableOffset,
                                    uint32_t *aUVSTableOffset,
                                    bool *aSymbolEncoding)
{
    enum {
        OffsetVersion = 0,
        OffsetNumTables = 2,
        SizeOfHeader = 4,

        TableOffsetPlatformID = 0,
        TableOffsetEncodingID = 2,
        TableOffsetOffset = 4,
        SizeOfTable = 8,

        SubtableOffsetFormat = 0
    };
    enum {
        EncodingIDSymbol = 0,
        EncodingIDMicrosoft = 1,
        EncodingIDDefaultForUnicodePlatform = 0,
        EncodingIDUCS4ForUnicodePlatform = 3,
        EncodingIDUVSForUnicodePlatform = 5,
        EncodingIDUCS4ForMicrosoftPlatform = 10
    };

    if (aUVSTableOffset) {
        *aUVSTableOffset = 0;
    }

    if (!aBuf || aBufLength < SizeOfHeader) {
        
        return 0;
    }

    
    uint16_t numTables = ReadShortAt(aBuf, OffsetNumTables);
    if (aBufLength < uint32_t(SizeOfHeader + numTables * SizeOfTable)) {
        return 0;
    }

    
    uint32_t keepFormat = 0;

    const uint8_t *table = aBuf + SizeOfHeader;
    for (uint16_t i = 0; i < numTables; ++i, table += SizeOfTable) {
        const uint16_t platformID = ReadShortAt(table, TableOffsetPlatformID);
        if (!acceptablePlatform(platformID))
            continue;

        const uint16_t encodingID = ReadShortAt(table, TableOffsetEncodingID);
        const uint32_t offset = ReadLongAt(table, TableOffsetOffset);
        if (aBufLength - 2 < offset) {
            
            return 0;
        }

        const uint8_t *subtable = aBuf + offset;
        const uint16_t format = ReadShortAt(subtable, SubtableOffsetFormat);

        if (isSymbol(platformID, encodingID)) {
            keepFormat = format;
            *aTableOffset = offset;
            *aSymbolEncoding = true;
            break;
        } else if (format == 4 && acceptableFormat4(platformID, encodingID, keepFormat)) {
            keepFormat = format;
            *aTableOffset = offset;
            *aSymbolEncoding = false;
        } else if (format == 12 && acceptableUCS4Encoding(platformID, encodingID, keepFormat)) {
            keepFormat = format;
            *aTableOffset = offset;
            *aSymbolEncoding = false;
            if (platformID > PLATFORM_ID_UNICODE || !aUVSTableOffset || *aUVSTableOffset) {
                break; 
            }
        } else if (format == 14 && isUVSEncoding(platformID, encodingID) && aUVSTableOffset) {
            *aUVSTableOffset = offset;
            if (keepFormat == 12) {
                break;
            }
        }
    }

    return keepFormat;
}

nsresult
gfxFontUtils::ReadCMAP(const uint8_t *aBuf, uint32_t aBufLength,
                       gfxSparseBitSet& aCharacterMap,
                       uint32_t& aUVSOffset,
                       bool& aUnicodeFont, bool& aSymbolFont)
{
    uint32_t offset;
    bool     symbol;
    uint32_t format = FindPreferredSubtable(aBuf, aBufLength,
                                            &offset, &aUVSOffset, &symbol);

    if (format == 4) {
        if (symbol) {
            aUnicodeFont = false;
            aSymbolFont = true;
        } else {
            aUnicodeFont = true;
            aSymbolFont = false;
        }
        return ReadCMAPTableFormat4(aBuf + offset, aBufLength - offset,
                                    aCharacterMap);
    }

    if (format == 12) {
        aUnicodeFont = true;
        aSymbolFont = false;
        return ReadCMAPTableFormat12(aBuf + offset, aBufLength - offset,
                                     aCharacterMap);
    }

    return NS_ERROR_FAILURE;
}

#pragma pack(1)

typedef struct {
    AutoSwap_PRUint16 format;
    AutoSwap_PRUint16 length;
    AutoSwap_PRUint16 language;
    AutoSwap_PRUint16 segCountX2;
    AutoSwap_PRUint16 searchRange;
    AutoSwap_PRUint16 entrySelector;
    AutoSwap_PRUint16 rangeShift;

    AutoSwap_PRUint16 arrays[1];
} Format4Cmap;

typedef struct {
    AutoSwap_PRUint16 format;
    AutoSwap_PRUint32 length;
    AutoSwap_PRUint32 numVarSelectorRecords;

    typedef struct {
        AutoSwap_PRUint24 varSelector;
        AutoSwap_PRUint32 defaultUVSOffset;
        AutoSwap_PRUint32 nonDefaultUVSOffset;
    } VarSelectorRecord;

    VarSelectorRecord varSelectorRecords[1];
} Format14Cmap;

typedef struct {
    AutoSwap_PRUint32 numUVSMappings;

    typedef struct {
        AutoSwap_PRUint24 unicodeValue;
        AutoSwap_PRUint16 glyphID;
    } UVSMapping;

    UVSMapping uvsMappings[1];
} NonDefUVSTable;

#pragma pack()

uint32_t
gfxFontUtils::MapCharToGlyphFormat4(const uint8_t *aBuf, char16_t aCh)
{
    const Format4Cmap *cmap4 = reinterpret_cast<const Format4Cmap*>(aBuf);
    uint16_t segCount;
    const AutoSwap_PRUint16 *endCodes;
    const AutoSwap_PRUint16 *startCodes;
    const AutoSwap_PRUint16 *idDelta;
    const AutoSwap_PRUint16 *idRangeOffset;
    uint16_t probe;
    uint16_t rangeShiftOver2;
    uint16_t index;

    segCount = (uint16_t)(cmap4->segCountX2) / 2;

    endCodes = &cmap4->arrays[0];
    startCodes = &cmap4->arrays[segCount + 1]; 
    idDelta = &startCodes[segCount];
    idRangeOffset = &idDelta[segCount];

    probe = 1 << (uint16_t)(cmap4->entrySelector);
    rangeShiftOver2 = (uint16_t)(cmap4->rangeShift) / 2;

    if ((uint16_t)(startCodes[rangeShiftOver2]) <= aCh) {
        index = rangeShiftOver2;
    } else {
        index = 0;
    }

    while (probe > 1) {
        probe >>= 1;
        if ((uint16_t)(startCodes[index + probe]) <= aCh) {
            index += probe;
        }
    }

    if (aCh >= (uint16_t)(startCodes[index]) && aCh <= (uint16_t)(endCodes[index])) {
        uint16_t result;
        if ((uint16_t)(idRangeOffset[index]) == 0) {
            result = aCh;
        } else {
            uint16_t offset = aCh - (uint16_t)(startCodes[index]);
            const AutoSwap_PRUint16 *glyphIndexTable =
                (const AutoSwap_PRUint16*)((const char*)&idRangeOffset[index] +
                                           (uint16_t)(idRangeOffset[index]));
            result = glyphIndexTable[offset];
        }

        
        result += (uint16_t)(idDelta[index]);
        return result;
    }

    return 0;
}

uint32_t
gfxFontUtils::MapCharToGlyphFormat12(const uint8_t *aBuf, uint32_t aCh)
{
    const Format12CmapHeader *cmap12 =
        reinterpret_cast<const Format12CmapHeader*>(aBuf);

    
    
    uint32_t numGroups = cmap12->numGroups;

    
    const Format12Group *groups =
        reinterpret_cast<const Format12Group*>(aBuf + sizeof(Format12CmapHeader));

    
    
    
    
    
    
    uint32_t powerOf2 = mozilla::FindHighestBit(numGroups);
    uint32_t rangeOffset = numGroups - powerOf2;
    uint32_t range = 0;
    uint32_t startCharCode;

    if (groups[rangeOffset].startCharCode <= aCh) {
        range = rangeOffset;
    }

    
    while (powerOf2 > 1) {
        powerOf2 >>= 1;
        if (groups[range + powerOf2].startCharCode <= aCh) {
            range += powerOf2;
        }
    }

    
    
    startCharCode = groups[range].startCharCode;
    if (startCharCode <= aCh && groups[range].endCharCode >= aCh) {
        return groups[range].startGlyphId + aCh - startCharCode;
    }

    
    return 0;
}

uint16_t
gfxFontUtils::MapUVSToGlyphFormat14(const uint8_t *aBuf, uint32_t aCh, uint32_t aVS)
{
    const Format14Cmap *cmap14 = reinterpret_cast<const Format14Cmap*>(aBuf);

    
    uint32_t min = 0;
    uint32_t max = cmap14->numVarSelectorRecords;
    uint32_t nonDefUVSOffset = 0;
    while (min < max) {
        uint32_t index = (min + max) >> 1;
        uint32_t varSelector = cmap14->varSelectorRecords[index].varSelector;
        if (aVS == varSelector) {
            nonDefUVSOffset = cmap14->varSelectorRecords[index].nonDefaultUVSOffset;
            break;
        }
        if (aVS < varSelector) {
            max = index;
        } else {
            min = index + 1;
        }
    }
    if (!nonDefUVSOffset) {
        return 0;
    }

    const NonDefUVSTable *table = reinterpret_cast<const NonDefUVSTable*>
                                      (aBuf + nonDefUVSOffset);

    
    min = 0;
    max = table->numUVSMappings;
    while (min < max) {
        uint32_t index = (min + max) >> 1;
        uint32_t unicodeValue = table->uvsMappings[index].unicodeValue;
        if (aCh == unicodeValue) {
            return table->uvsMappings[index].glyphID;
        }
        if (aCh < unicodeValue) {
            max = index;
        } else {
            min = index + 1;
        }
    }

    return 0;
}

uint32_t
gfxFontUtils::MapCharToGlyph(const uint8_t *aCmapBuf, uint32_t aBufLength,
                             uint32_t aUnicode, uint32_t aVarSelector)
{
    uint32_t offset, uvsOffset;
    bool     symbol;
    uint32_t format = FindPreferredSubtable(aCmapBuf, aBufLength, &offset,
                                            &uvsOffset, &symbol);

    uint32_t gid;
    switch (format) {
    case 4:
        gid = aUnicode < UNICODE_BMP_LIMIT ?
            MapCharToGlyphFormat4(aCmapBuf + offset, char16_t(aUnicode)) : 0;
        break;
    case 12:
        gid = MapCharToGlyphFormat12(aCmapBuf + offset, aUnicode);
        break;
    default:
        NS_WARNING("unsupported cmap format, glyphs will be missing");
        gid = 0;
    }

    if (aVarSelector && uvsOffset && gid) {
        uint32_t varGID =
            gfxFontUtils::MapUVSToGlyphFormat14(aCmapBuf + uvsOffset,
                                                aUnicode, aVarSelector);
        if (!varGID) {
            aUnicode = gfxFontUtils::GetUVSFallback(aUnicode, aVarSelector);
            if (aUnicode) {
                switch (format) {
                case 4:
                    if (aUnicode < UNICODE_BMP_LIMIT) {
                        varGID = MapCharToGlyphFormat4(aCmapBuf + offset,
                                                       char16_t(aUnicode));
                    }
                    break;
                case 12:
                    varGID = MapCharToGlyphFormat12(aCmapBuf + offset,
                                                    aUnicode);
                    break;
                }
            }
        }
        if (varGID) {
            gid = varGID;
        }

        
        
    }

    return gid;
}

void gfxFontUtils::GetPrefsFontList(const char *aPrefName, nsTArray<nsString>& aFontList)
{
    const char16_t kComma = char16_t(',');
    
    aFontList.Clear();
    
    
    nsAdoptingString fontlistValue = Preferences::GetString(aPrefName);
    if (!fontlistValue) {
        return;
    }

    
    nsAutoString fontname;
    const char16_t *p, *p_end;
    fontlistValue.BeginReading(p);
    fontlistValue.EndReading(p_end);

     while (p < p_end) {
        const char16_t *nameStart = p;
        while (++p != p_end && *p != kComma)
         ;

        
        fontname = Substring(nameStart, p);
        fontname.CompressWhitespace(true, true);
        
        
        aFontList.AppendElement(fontname);
        ++p;
    }

}





#define MAX_B64_LEN 32

nsresult gfxFontUtils::MakeUniqueUserFontName(nsAString& aName)
{
    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1");
    NS_ENSURE_TRUE(uuidgen, NS_ERROR_OUT_OF_MEMORY);

    nsID guid;

    NS_ASSERTION(sizeof(guid) * 2 <= MAX_B64_LEN, "size of nsID has changed!");

    nsresult rv = uuidgen->GenerateUUIDInPlace(&guid);
    NS_ENSURE_SUCCESS(rv, rv);

    char guidB64[MAX_B64_LEN] = {0};

    if (!PL_Base64Encode(reinterpret_cast<char*>(&guid), sizeof(guid), guidB64))
        return NS_ERROR_FAILURE;

    
    char *p;
    for (p = guidB64; *p; p++) {
        if (*p == '/')
            *p = '-';
    }

    aName.Assign(NS_LITERAL_STRING("uf"));
    aName.AppendASCII(guidB64);
    return NS_OK;
}





#pragma pack(1)





struct NameRecordData {
    uint32_t  offset;
    uint32_t  length;
};

#pragma pack()

static bool
IsValidSFNTVersion(uint32_t version)
{
    
    
    return version == 0x10000 ||
           version == TRUETYPE_TAG('O','T','T','O') ||
           version == TRUETYPE_TAG('t','r','u','e');
}


static void
CopySwapUTF16(const uint16_t *aInBuf, uint16_t *aOutBuf, uint32_t aLen)
{
    const uint16_t *end = aInBuf + aLen;
    while (aInBuf < end) {
        uint16_t value = *aInBuf;
        *aOutBuf = (value >> 8) | (value & 0xff) << 8;
        aOutBuf++;
        aInBuf++;
    }
}

gfxUserFontType
gfxFontUtils::DetermineFontDataType(const uint8_t *aFontData, uint32_t aFontDataLength)
{
    
    
    if (aFontDataLength >= sizeof(SFNTHeader)) {
        const SFNTHeader *sfntHeader = reinterpret_cast<const SFNTHeader*>(aFontData);
        uint32_t sfntVersion = sfntHeader->sfntVersion;
        if (IsValidSFNTVersion(sfntVersion)) {
            return GFX_USERFONT_OPENTYPE;
        }
    }
    
    
    if (aFontDataLength >= sizeof(AutoSwap_PRUint32)) {
        const AutoSwap_PRUint32 *version = 
            reinterpret_cast<const AutoSwap_PRUint32*>(aFontData);
        if (uint32_t(*version) == TRUETYPE_TAG('w','O','F','F')) {
            return GFX_USERFONT_WOFF;
        }
    }
    
    
    
    return GFX_USERFONT_UNKNOWN;
}

nsresult
gfxFontUtils::RenameFont(const nsAString& aName, const uint8_t *aFontData, 
                         uint32_t aFontDataLength, FallibleTArray<uint8_t> *aNewFont)
{
    NS_ASSERTION(aNewFont, "null font data array");
    
    uint64_t dataLength(aFontDataLength);

    
    static const uint32_t neededNameIDs[] = {NAME_ID_FAMILY, 
                                             NAME_ID_STYLE,
                                             NAME_ID_UNIQUE,
                                             NAME_ID_FULL,
                                             NAME_ID_POSTSCRIPT};

    
    uint16_t nameCount = ArrayLength(neededNameIDs);

    
    uint16_t nameStrLength = (aName.Length() + 1) * sizeof(char16_t); 

    
    uint32_t nameTableSize = (sizeof(NameHeader) +
                              sizeof(NameRecord) * nameCount +
                              nameStrLength +
                              3) & ~3;
                              
    if (dataLength + nameTableSize > UINT32_MAX)
        return NS_ERROR_FAILURE;
        
    
    uint32_t paddedFontDataSize = (aFontDataLength + 3) & ~3;
    uint32_t adjFontDataSize = paddedFontDataSize + nameTableSize;

    
    if (!aNewFont->AppendElements(adjFontDataSize))
        return NS_ERROR_OUT_OF_MEMORY;

    
    uint8_t *newFontData = reinterpret_cast<uint8_t*>(aNewFont->Elements());
    
    
    memset(newFontData + aFontDataLength, 0, paddedFontDataSize - aFontDataLength);

    
    memcpy(newFontData, aFontData, aFontDataLength);
    
    
    memset(newFontData + adjFontDataSize - 4, 0, 4);
    
    NameHeader *nameHeader = reinterpret_cast<NameHeader*>(newFontData +
                                                            paddedFontDataSize);
    
    
    nameHeader->format = 0;
    nameHeader->count = nameCount;
    nameHeader->stringOffset = sizeof(NameHeader) + nameCount * sizeof(NameRecord);
    
    
    uint32_t i;
    NameRecord *nameRecord = reinterpret_cast<NameRecord*>(nameHeader + 1);
    
    for (i = 0; i < nameCount; i++, nameRecord++) {
        nameRecord->platformID = PLATFORM_ID_MICROSOFT;
        nameRecord->encodingID = ENCODING_ID_MICROSOFT_UNICODEBMP;
        nameRecord->languageID = LANG_ID_MICROSOFT_EN_US;
        nameRecord->nameID = neededNameIDs[i];
        nameRecord->offset = 0;
        nameRecord->length = nameStrLength;
    }
    
    
    char16_t *strData = reinterpret_cast<char16_t*>(nameRecord);

    mozilla::NativeEndian::copyAndSwapToBigEndian(strData,
                                                  aName.BeginReading(),
                                                  aName.Length());
    strData[aName.Length()] = 0; 
    
    
    SFNTHeader *sfntHeader = reinterpret_cast<SFNTHeader*>(newFontData);

    
    TableDirEntry *dirEntry = 
        reinterpret_cast<TableDirEntry*>(newFontData + sizeof(SFNTHeader));

    uint32_t numTables = sfntHeader->numTables;
    
    for (i = 0; i < numTables; i++, dirEntry++) {
        if (dirEntry->tag == TRUETYPE_TAG('n','a','m','e')) {
            break;
        }
    }
    
    
    NS_ASSERTION(i < numTables, "attempt to rename font with no name table");

    
    
    
    uint32_t checkSum = 0;
    AutoSwap_PRUint32 *nameData = reinterpret_cast<AutoSwap_PRUint32*> (nameHeader);
    AutoSwap_PRUint32 *nameDataEnd = nameData + (nameTableSize >> 2);
    
    while (nameData < nameDataEnd)
        checkSum = checkSum + *nameData++;
    
    
    dirEntry->offset = paddedFontDataSize;
    dirEntry->length = nameTableSize;
    dirEntry->checkSum = checkSum;
    
    
    uint32_t checksum = 0;
    
    
    uint32_t headerLen = sizeof(SFNTHeader) + sizeof(TableDirEntry) * numTables;
    const AutoSwap_PRUint32 *headerData = 
        reinterpret_cast<const AutoSwap_PRUint32*>(newFontData);

    
    for (i = 0; i < (headerLen >> 2); i++, headerData++) {
        checksum += *headerData;
    }
    
    uint32_t headOffset = 0;
    dirEntry = reinterpret_cast<TableDirEntry*>(newFontData + sizeof(SFNTHeader));

    for (i = 0; i < numTables; i++, dirEntry++) {
        if (dirEntry->tag == TRUETYPE_TAG('h','e','a','d')) {
            headOffset = dirEntry->offset;
        }
        checksum += dirEntry->checkSum;
    }
    
    NS_ASSERTION(headOffset != 0, "no head table for font");
    
    HeadTable *headData = reinterpret_cast<HeadTable*>(newFontData + headOffset);

    headData->checkSumAdjustment = HeadTable::HEAD_CHECKSUM_CALC_CONST - checksum;

    return NS_OK;
}





nsresult
gfxFontUtils::GetFullNameFromSFNT(const uint8_t* aFontData, uint32_t aLength,
                                  nsAString& aFullName)
{
    aFullName.AssignLiteral("(MISSING NAME)"); 

    NS_ENSURE_TRUE(aLength >= sizeof(SFNTHeader), NS_ERROR_UNEXPECTED);
    const SFNTHeader *sfntHeader =
        reinterpret_cast<const SFNTHeader*>(aFontData);
    const TableDirEntry *dirEntry =
        reinterpret_cast<const TableDirEntry*>(aFontData + sizeof(SFNTHeader));
    uint32_t numTables = sfntHeader->numTables;
    NS_ENSURE_TRUE(aLength >=
                   sizeof(SFNTHeader) + numTables * sizeof(TableDirEntry),
                   NS_ERROR_UNEXPECTED);
    bool foundName = false;
    for (uint32_t i = 0; i < numTables; i++, dirEntry++) {
        if (dirEntry->tag == TRUETYPE_TAG('n','a','m','e')) {
            foundName = true;
            break;
        }
    }
    
    
    NS_ENSURE_TRUE(foundName, NS_ERROR_NOT_AVAILABLE);

    uint32_t len = dirEntry->length;
    NS_ENSURE_TRUE(aLength > len && aLength - len >= dirEntry->offset,
                   NS_ERROR_UNEXPECTED);

    hb_blob_t *nameBlob =
        hb_blob_create((const char*)aFontData + dirEntry->offset, len,
                       HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    nsresult rv = GetFullNameFromTable(nameBlob, aFullName);
    hb_blob_destroy(nameBlob);

    return rv;
}

nsresult
gfxFontUtils::GetFullNameFromTable(hb_blob_t *aNameTable,
                                   nsAString& aFullName)
{
    nsAutoString name;
    nsresult rv =
        gfxFontUtils::ReadCanonicalName(aNameTable,
                                        gfxFontUtils::NAME_ID_FULL,
                                        name);
    if (NS_SUCCEEDED(rv) && !name.IsEmpty()) {
        aFullName = name;
        return NS_OK;
    }
    rv = gfxFontUtils::ReadCanonicalName(aNameTable,
                                         gfxFontUtils::NAME_ID_FAMILY,
                                         name);
    if (NS_SUCCEEDED(rv) && !name.IsEmpty()) {
        nsAutoString styleName;
        rv = gfxFontUtils::ReadCanonicalName(aNameTable,
                                             gfxFontUtils::NAME_ID_STYLE,
                                             styleName);
        if (NS_SUCCEEDED(rv) && !styleName.IsEmpty()) {
            name.Append(' ');
            name.Append(styleName);
            aFullName = name;
        }
        return NS_OK;
    }

    return NS_ERROR_NOT_AVAILABLE;
}

nsresult
gfxFontUtils::GetFamilyNameFromTable(hb_blob_t *aNameTable,
                                     nsAString& aFullName)
{
    nsAutoString name;
    nsresult rv =
        gfxFontUtils::ReadCanonicalName(aNameTable,
                                        gfxFontUtils::NAME_ID_FAMILY,
                                        name);
    if (NS_SUCCEEDED(rv) && !name.IsEmpty()) {
        aFullName = name;
        return NS_OK;
    }
    return NS_ERROR_NOT_AVAILABLE;
}

enum {
#if defined(XP_MACOSX)
    CANONICAL_LANG_ID = gfxFontUtils::LANG_ID_MAC_ENGLISH,
    PLATFORM_ID       = gfxFontUtils::PLATFORM_ID_MAC
#else
    CANONICAL_LANG_ID = gfxFontUtils::LANG_ID_MICROSOFT_EN_US,
    PLATFORM_ID       = gfxFontUtils::PLATFORM_ID_MICROSOFT
#endif
};    

nsresult
gfxFontUtils::ReadNames(const char *aNameData, uint32_t aDataLen,
                        uint32_t aNameID, int32_t aPlatformID,
                        nsTArray<nsString>& aNames)
{
    return ReadNames(aNameData, aDataLen, aNameID, LANG_ALL,
                     aPlatformID, aNames);
}

nsresult
gfxFontUtils::ReadCanonicalName(hb_blob_t *aNameTable, uint32_t aNameID,
                                nsString& aName)
{
    uint32_t nameTableLen;
    const char *nameTable = hb_blob_get_data(aNameTable, &nameTableLen);
    return ReadCanonicalName(nameTable, nameTableLen, aNameID, aName);
}

nsresult
gfxFontUtils::ReadCanonicalName(const char *aNameData, uint32_t aDataLen,
                                uint32_t aNameID, nsString& aName)
{
    nsresult rv;
    
    nsTArray<nsString> names;
    
    
    rv = ReadNames(aNameData, aDataLen, aNameID, CANONICAL_LANG_ID, 
                   PLATFORM_ID, names);
    NS_ENSURE_SUCCESS(rv, rv);
        
    
    if (names.Length() == 0) {
        rv = ReadNames(aNameData, aDataLen, aNameID, LANG_ALL,
                       PLATFORM_ID, names);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    
#if defined(XP_MACOSX)
    
    if (names.Length() == 0) {
        rv = ReadNames(aNameData, aDataLen, aNameID, LANG_ID_MICROSOFT_EN_US,
                       PLATFORM_ID_MICROSOFT, names);
        NS_ENSURE_SUCCESS(rv, rv);
        
        
        if (names.Length() == 0) {
            rv = ReadNames(aNameData, aDataLen, aNameID, LANG_ALL,
                           PLATFORM_ID_MICROSOFT, names);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }
#endif

    
    
    if (names.Length()) {
        aName.Assign(names[0]);
        return NS_OK;
    }
        
    return NS_ERROR_FAILURE;
}










#define ANY 0xffff
const gfxFontUtils::MacFontNameCharsetMapping gfxFontUtils::gMacFontNameCharsets[] =
{
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_ENGLISH,      "macintosh"       },
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_ICELANDIC,    "x-mac-icelandic" },
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_TURKISH,      "x-mac-turkish"   },
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_POLISH,       "x-mac-ce"        },
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_ROMANIAN,     "x-mac-romanian"  },
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_CZECH,        "x-mac-ce"        },
    { ENCODING_ID_MAC_ROMAN,        LANG_ID_MAC_SLOVAK,       "x-mac-ce"        },
    { ENCODING_ID_MAC_ROMAN,        ANY,                      "macintosh"       },
    { ENCODING_ID_MAC_JAPANESE,     LANG_ID_MAC_JAPANESE,     "Shift_JIS"       },
    { ENCODING_ID_MAC_JAPANESE,     ANY,                      "Shift_JIS"       },
    { ENCODING_ID_MAC_TRAD_CHINESE, LANG_ID_MAC_TRAD_CHINESE, "Big5"            },
    { ENCODING_ID_MAC_TRAD_CHINESE, ANY,                      "Big5"            },
    { ENCODING_ID_MAC_KOREAN,       LANG_ID_MAC_KOREAN,       "EUC-KR"          },
    { ENCODING_ID_MAC_KOREAN,       ANY,                      "EUC-KR"          },
    { ENCODING_ID_MAC_ARABIC,       LANG_ID_MAC_ARABIC,       "x-mac-arabic"    },
    { ENCODING_ID_MAC_ARABIC,       LANG_ID_MAC_URDU,         "x-mac-farsi"     },
    { ENCODING_ID_MAC_ARABIC,       LANG_ID_MAC_FARSI,        "x-mac-farsi"     },
    { ENCODING_ID_MAC_ARABIC,       ANY,                      "x-mac-arabic"    },
    { ENCODING_ID_MAC_HEBREW,       LANG_ID_MAC_HEBREW,       "x-mac-hebrew"    },
    { ENCODING_ID_MAC_HEBREW,       ANY,                      "x-mac-hebrew"    },
    { ENCODING_ID_MAC_GREEK,        ANY,                      "x-mac-greek"     },
    { ENCODING_ID_MAC_CYRILLIC,     ANY,                      "x-mac-cyrillic"  },
    { ENCODING_ID_MAC_DEVANAGARI,   ANY,                      "x-mac-devanagari"},
    { ENCODING_ID_MAC_GURMUKHI,     ANY,                      "x-mac-gurmukhi"  },
    { ENCODING_ID_MAC_GUJARATI,     ANY,                      "x-mac-gujarati"  },
    { ENCODING_ID_MAC_SIMP_CHINESE, LANG_ID_MAC_SIMP_CHINESE, "GB2312"          },
    { ENCODING_ID_MAC_SIMP_CHINESE, ANY,                      "GB2312"          }
};

const char* gfxFontUtils::gISOFontNameCharsets[] = 
{
     "us-ascii"   ,
     nullptr       , 
     "ISO-8859-1"
};

const char* gfxFontUtils::gMSFontNameCharsets[] =
{
          ""          ,
      ""          ,
        "Shift_JIS" ,
             nullptr      ,
            "Big5"      ,
         nullptr      ,
           nullptr      ,
                              nullptr      ,
                              nullptr      ,
                              nullptr      ,
     ""
};






const char*
gfxFontUtils::GetCharsetForFontName(uint16_t aPlatform, uint16_t aScript, uint16_t aLanguage)
{
    switch (aPlatform)
    {
    case PLATFORM_ID_UNICODE:
        return "";

    case PLATFORM_ID_MAC:
        {
            uint32_t lo = 0, hi = ArrayLength(gMacFontNameCharsets);
            MacFontNameCharsetMapping searchValue = { aScript, aLanguage, nullptr };
            for (uint32_t i = 0; i < 2; ++i) {
                
                while (lo < hi) {
                    uint32_t mid = (lo + hi) / 2;
                    const MacFontNameCharsetMapping& entry = gMacFontNameCharsets[mid];
                    if (entry < searchValue) {
                        lo = mid + 1;
                        continue;
                    }
                    if (searchValue < entry) {
                        hi = mid;
                        continue;
                    }
                    
                    return entry.mCharsetName;
                }

                
                hi = ArrayLength(gMacFontNameCharsets);
                searchValue.mLanguage = ANY;
            }
        }
        break;

    case PLATFORM_ID_ISO:
        if (aScript < ArrayLength(gISOFontNameCharsets)) {
            return gISOFontNameCharsets[aScript];
        }
        break;

    case PLATFORM_ID_MICROSOFT:
        if (aScript < ArrayLength(gMSFontNameCharsets)) {
            return gMSFontNameCharsets[aScript];
        }
        break;
    }

    return nullptr;
}



bool
gfxFontUtils::DecodeFontName(const char *aNameData, int32_t aByteLen, 
                             uint32_t aPlatformCode, uint32_t aScriptCode,
                             uint32_t aLangCode, nsAString& aName)
{
    if (aByteLen <= 0) {
        NS_WARNING("empty font name");
        aName.SetLength(0);
        return true;
    }

    const char *csName = GetCharsetForFontName(aPlatformCode, aScriptCode, aLangCode);

    if (!csName) {
        
#ifdef DEBUG
        char warnBuf[128];
        if (aByteLen > 64)
            aByteLen = 64;
        sprintf(warnBuf, "skipping font name, unknown charset %d:%d:%d for <%.*s>",
                aPlatformCode, aScriptCode, aLangCode, aByteLen, aNameData);
        NS_WARNING(warnBuf);
#endif
        return false;
    }

    if (csName[0] == 0) {
        
        uint32_t strLen = aByteLen / 2;
#ifdef IS_LITTLE_ENDIAN
        aName.SetLength(strLen);
        CopySwapUTF16(reinterpret_cast<const uint16_t*>(aNameData),
                      reinterpret_cast<uint16_t*>(aName.BeginWriting()), strLen);
#else
        aName.Assign(reinterpret_cast<const char16_t*>(aNameData), strLen);
#endif    
        return true;
    }

    nsCOMPtr<nsIUnicodeDecoder> decoder =
        mozilla::dom::EncodingUtils::DecoderForEncoding(csName);
    if (!decoder) {
        NS_WARNING("failed to get the decoder for a font name string");
        return false;
    }

    int32_t destLength;
    nsresult rv = decoder->GetMaxLength(aNameData, aByteLen, &destLength);
    if (NS_FAILED(rv)) {
        NS_WARNING("decoder->GetMaxLength failed, invalid font name?");
        return false;
    }

    
    aName.SetLength(destLength);
    rv = decoder->Convert(aNameData, &aByteLen,
                          aName.BeginWriting(), &destLength);
    if (NS_FAILED(rv)) {
        NS_WARNING("decoder->Convert failed, invalid font name?");
        return false;
    }
    aName.Truncate(destLength); 

    return true;
}

nsresult
gfxFontUtils::ReadNames(const char *aNameData, uint32_t aDataLen,
                        uint32_t aNameID,
                        int32_t aLangID, int32_t aPlatformID,
                        nsTArray<nsString>& aNames)
{
    NS_ASSERTION(aDataLen != 0, "null name table");

    if (!aDataLen) {
        return NS_ERROR_FAILURE;
    }

    
    const NameHeader *nameHeader = reinterpret_cast<const NameHeader*>(aNameData);

    uint32_t nameCount = nameHeader->count;

    
    if (uint64_t(nameCount) * sizeof(NameRecord) > aDataLen) {
        NS_WARNING("invalid font (name table data)");
        return NS_ERROR_FAILURE;
    }

    
    const NameRecord *nameRecord
        = reinterpret_cast<const NameRecord*>(aNameData + sizeof(NameHeader));
    uint64_t nameStringsBase = uint64_t(nameHeader->stringOffset);

    uint32_t i;
    for (i = 0; i < nameCount; i++, nameRecord++) {
        uint32_t platformID;

        
        if (uint32_t(nameRecord->nameID) != aNameID)
            continue;

        
        platformID = nameRecord->platformID;
        if (aPlatformID != PLATFORM_ALL
            && uint32_t(nameRecord->platformID) != PLATFORM_ID)
            continue;

        
        if (aLangID != LANG_ALL
              && uint32_t(nameRecord->languageID) != uint32_t(aLangID))
            continue;

        

        
        uint32_t namelen = nameRecord->length;
        uint32_t nameoff = nameRecord->offset;  

        if (nameStringsBase + uint64_t(nameoff) + uint64_t(namelen)
                > aDataLen) {
            NS_WARNING("invalid font (name table strings)");
            return NS_ERROR_FAILURE;
        }

        
        nsAutoString name;

        DecodeFontName(aNameData + nameStringsBase + nameoff, namelen,
                       platformID, uint32_t(nameRecord->encodingID),
                       uint32_t(nameRecord->languageID), name);

        uint32_t k, numNames;
        bool foundName = false;

        numNames = aNames.Length();
        for (k = 0; k < numNames; k++) {
            if (name.Equals(aNames[k])) {
                foundName = true;
                break;
            }
        }

        if (!foundName)
            aNames.AppendElement(name);

    }

    return NS_OK;
}

#ifdef XP_WIN


bool
gfxFontUtils::IsCffFont(const uint8_t* aFontData)
{
    
    
    const SFNTHeader *sfntHeader = reinterpret_cast<const SFNTHeader*>(aFontData);
    return (sfntHeader->sfntVersion == TRUETYPE_TAG('O','T','T','O'));
}

#endif

