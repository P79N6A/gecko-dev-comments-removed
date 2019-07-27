






#include <ctype.h>

#include "SkData.h"
#include "SkFontHost.h"
#include "SkGlyphCache.h"
#include "SkPaint.h"
#include "SkPDFCatalog.h"
#include "SkPDFDevice.h"
#include "SkPDFFont.h"
#include "SkPDFFontImpl.h"
#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkPDFUtils.h"
#include "SkRefCnt.h"
#include "SkScalar.h"
#include "SkStream.h"
#include "SkTypefacePriv.h"
#include "SkTypes.h"
#include "SkUtils.h"

#if defined (SK_SFNTLY_SUBSETTER)
#include SK_SFNTLY_SUBSETTER
#endif




static const int kPdfSymbolic = 4;

namespace {





bool parsePFBSection(const uint8_t** src, size_t* len, int sectionType,
                     size_t* size) {
    
    
    
    
    const uint8_t* buf = *src;
    if (*len < 2 || buf[0] != 0x80 || buf[1] != sectionType) {
        return false;
    } else if (buf[1] == 3) {
        return true;
    } else if (*len < 6) {
        return false;
    }

    *size = (size_t)buf[2] | ((size_t)buf[3] << 8) | ((size_t)buf[4] << 16) |
            ((size_t)buf[5] << 24);
    size_t consumed = *size + 6;
    if (consumed > *len) {
        return false;
    }
    *src = *src + consumed;
    *len = *len - consumed;
    return true;
}

bool parsePFB(const uint8_t* src, size_t size, size_t* headerLen,
              size_t* dataLen, size_t* trailerLen) {
    const uint8_t* srcPtr = src;
    size_t remaining = size;

    return parsePFBSection(&srcPtr, &remaining, 1, headerLen) &&
           parsePFBSection(&srcPtr, &remaining, 2, dataLen) &&
           parsePFBSection(&srcPtr, &remaining, 1, trailerLen) &&
           parsePFBSection(&srcPtr, &remaining, 3, NULL);
}









bool parsePFA(const char* src, size_t size, size_t* headerLen,
              size_t* hexDataLen, size_t* dataLen, size_t* trailerLen) {
    const char* end = src + size;

    const char* dataPos = strstr(src, "eexec");
    if (!dataPos) {
        return false;
    }
    dataPos += strlen("eexec");
    while ((*dataPos == '\n' || *dataPos == '\r' || *dataPos == ' ') &&
            dataPos < end) {
        dataPos++;
    }
    *headerLen = dataPos - src;

    const char* trailerPos = strstr(dataPos, "cleartomark");
    if (!trailerPos) {
        return false;
    }
    int zeroCount = 0;
    for (trailerPos--; trailerPos > dataPos && zeroCount < 512; trailerPos--) {
        if (*trailerPos == '\n' || *trailerPos == '\r' || *trailerPos == ' ') {
            continue;
        } else if (*trailerPos == '0') {
            zeroCount++;
        } else {
            return false;
        }
    }
    if (zeroCount != 512) {
        return false;
    }

    *hexDataLen = trailerPos - src - *headerLen;
    *trailerLen = size - *headerLen - *hexDataLen;

    
    int nibbles = 0;
    for (; dataPos < trailerPos; dataPos++) {
        if (isspace(*dataPos)) {
            continue;
        }
        if (!isxdigit(*dataPos)) {
            return false;
        }
        nibbles++;
    }
    *dataLen = (nibbles + 1) / 2;

    return true;
}

int8_t hexToBin(uint8_t c) {
    if (!isxdigit(c)) {
        return -1;
    } else if (c <= '9') {
        return c - '0';
    } else if (c <= 'F') {
        return c - 'A' + 10;
    } else if (c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static SkData* handle_type1_stream(SkStream* srcStream, size_t* headerLen,
                                   size_t* dataLen, size_t* trailerLen) {
    
    
    
    
    
    SkDynamicMemoryWStream dynamicStream;
    SkAutoTUnref<SkMemoryStream> staticStream;
    SkData* data = NULL;
    const uint8_t* src;
    size_t srcLen;
    if ((srcLen = srcStream->getLength()) > 0) {
        staticStream.reset(new SkMemoryStream(srcLen + 1));
        src = (const uint8_t*)staticStream->getMemoryBase();
        if (srcStream->getMemoryBase() != NULL) {
            memcpy((void *)src, srcStream->getMemoryBase(), srcLen);
        } else {
            size_t read = 0;
            while (read < srcLen) {
                size_t got = srcStream->read((void *)staticStream->getAtPos(),
                                             srcLen - read);
                if (got == 0) {
                    return NULL;
                }
                read += got;
                staticStream->seek(read);
            }
        }
        ((uint8_t *)src)[srcLen] = 0;
    } else {
        static const size_t kBufSize = 4096;
        uint8_t buf[kBufSize];
        size_t amount;
        while ((amount = srcStream->read(buf, kBufSize)) > 0) {
            dynamicStream.write(buf, amount);
        }
        amount = 0;
        dynamicStream.write(&amount, 1);  
        data = dynamicStream.copyToData();
        src = data->bytes();
        srcLen = data->size() - 1;
    }

    
    
    SkAutoDataUnref aud(data);

    if (parsePFB(src, srcLen, headerLen, dataLen, trailerLen)) {
        static const int kPFBSectionHeaderLength = 6;
        const size_t length = *headerLen + *dataLen + *trailerLen;
        SkASSERT(length > 0);
        SkASSERT(length + (2 * kPFBSectionHeaderLength) <= srcLen);

        SkAutoTMalloc<uint8_t> buffer(length);

        const uint8_t* const srcHeader = src + kPFBSectionHeaderLength;
        
        
        const uint8_t* const srcData
            = srcHeader + *headerLen + kPFBSectionHeaderLength;
        const uint8_t* const srcTrailer = srcData + *headerLen;

        uint8_t* const resultHeader = buffer.get();
        uint8_t* const resultData = resultHeader + *headerLen;
        uint8_t* const resultTrailer = resultData + *dataLen;

        SkASSERT(resultTrailer + *trailerLen == resultHeader + length);

        memcpy(resultHeader,  srcHeader,  *headerLen);
        memcpy(resultData,    srcData,    *dataLen);
        memcpy(resultTrailer, srcTrailer, *trailerLen);

        return SkData::NewFromMalloc(buffer.detach(), length);
    }

    
    size_t hexDataLen;
    if (parsePFA((const char*)src, srcLen, headerLen, &hexDataLen, dataLen,
                 trailerLen)) {
        const size_t length = *headerLen + *dataLen + *trailerLen;
        SkASSERT(length > 0);
        SkAutoTMalloc<uint8_t> buffer(length);

        memcpy(buffer.get(), src, *headerLen);
        uint8_t* const resultData = &(buffer[*headerLen]);

        const uint8_t* hexData = src + *headerLen;
        const uint8_t* trailer = hexData + hexDataLen;
        size_t outputOffset = 0;
        uint8_t dataByte = 0;  
        bool highNibble = true;
        for (; hexData < trailer; hexData++) {
            int8_t curNibble = hexToBin(*hexData);
            if (curNibble < 0) {
                continue;
            }
            if (highNibble) {
                dataByte = curNibble << 4;
                highNibble = false;
            } else {
                dataByte |= curNibble;
                highNibble = true;
                resultData[outputOffset++] = dataByte;
            }
        }
        if (!highNibble) {
            resultData[outputOffset++] = dataByte;
        }
        SkASSERT(outputOffset == *dataLen);

        uint8_t* const resultTrailer = &(buffer[*headerLen + outputOffset]);
        memcpy(resultTrailer, src + *headerLen + hexDataLen, *trailerLen);

        return SkData::NewFromMalloc(buffer.detach(), length);
    }
    return NULL;
}


SkScalar scaleFromFontUnits(int16_t val, uint16_t emSize) {
    SkScalar scaled = SkIntToScalar(val);
    if (emSize == 1000) {
        return scaled;
    } else {
        return SkScalarMulDiv(scaled, 1000, emSize);
    }
}

void setGlyphWidthAndBoundingBox(SkScalar width, SkIRect box,
                                 SkWStream* content) {
    
    SkPDFScalar::Append(width, content);
    content->writeText(" 0 ");
    content->writeDecAsText(box.fLeft);
    content->writeText(" ");
    content->writeDecAsText(box.fTop);
    content->writeText(" ");
    content->writeDecAsText(box.fRight);
    content->writeText(" ");
    content->writeDecAsText(box.fBottom);
    content->writeText(" d1\n");
}

SkPDFArray* makeFontBBox(SkIRect glyphBBox, uint16_t emSize) {
    SkPDFArray* bbox = new SkPDFArray;
    bbox->reserve(4);
    bbox->appendScalar(scaleFromFontUnits(glyphBBox.fLeft, emSize));
    bbox->appendScalar(scaleFromFontUnits(glyphBBox.fBottom, emSize));
    bbox->appendScalar(scaleFromFontUnits(glyphBBox.fRight, emSize));
    bbox->appendScalar(scaleFromFontUnits(glyphBBox.fTop, emSize));
    return bbox;
}

SkPDFArray* appendWidth(const int16_t& width, uint16_t emSize,
                        SkPDFArray* array) {
    array->appendScalar(scaleFromFontUnits(width, emSize));
    return array;
}

SkPDFArray* appendVerticalAdvance(
        const SkAdvancedTypefaceMetrics::VerticalMetric& advance,
        uint16_t emSize, SkPDFArray* array) {
    appendWidth(advance.fVerticalAdvance, emSize, array);
    appendWidth(advance.fOriginXDisp, emSize, array);
    appendWidth(advance.fOriginYDisp, emSize, array);
    return array;
}

template <typename Data>
SkPDFArray* composeAdvanceData(
        SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* advanceInfo,
        uint16_t emSize,
        SkPDFArray* (*appendAdvance)(const Data& advance, uint16_t emSize,
                                     SkPDFArray* array),
        Data* defaultAdvance) {
    SkPDFArray* result = new SkPDFArray();
    for (; advanceInfo != NULL; advanceInfo = advanceInfo->fNext.get()) {
        switch (advanceInfo->fType) {
            case SkAdvancedTypefaceMetrics::WidthRange::kDefault: {
                SkASSERT(advanceInfo->fAdvance.count() == 1);
                *defaultAdvance = advanceInfo->fAdvance[0];
                break;
            }
            case SkAdvancedTypefaceMetrics::WidthRange::kRange: {
                SkAutoTUnref<SkPDFArray> advanceArray(new SkPDFArray());
                for (int j = 0; j < advanceInfo->fAdvance.count(); j++)
                    appendAdvance(advanceInfo->fAdvance[j], emSize,
                                  advanceArray.get());
                result->appendInt(advanceInfo->fStartId);
                result->append(advanceArray.get());
                break;
            }
            case SkAdvancedTypefaceMetrics::WidthRange::kRun: {
                SkASSERT(advanceInfo->fAdvance.count() == 1);
                result->appendInt(advanceInfo->fStartId);
                result->appendInt(advanceInfo->fEndId);
                appendAdvance(advanceInfo->fAdvance[0], emSize, result);
                break;
            }
        }
    }
    return result;
}

}  

static void append_tounicode_header(SkDynamicMemoryWStream* cmap,
                                    uint16_t firstGlyphID,
                                    uint16_t lastGlyphID) {
    
    
    const char* kHeader =
        "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n";
    cmap->writeText(kHeader);

    
    
    
    
    const char* kSysInfo =
        "/CIDSystemInfo\n"
        "<<  /Registry (Adobe)\n"
        "/Ordering (UCS)\n"
        "/Supplement 0\n"
        ">> def\n";
    cmap->writeText(kSysInfo);

    
    
    
    const char* kTypeInfoHeader =
        "/CMapName /Adobe-Identity-UCS def\n"
        "/CMapType 2 def\n"
        "1 begincodespacerange\n";
    cmap->writeText(kTypeInfoHeader);

    
    SkString range;
    range.appendf("<%04X> <%04X>\n", firstGlyphID, lastGlyphID);
    cmap->writeText(range.c_str());

    const char* kTypeInfoFooter = "endcodespacerange\n";
    cmap->writeText(kTypeInfoFooter);
}

static void append_cmap_footer(SkDynamicMemoryWStream* cmap) {
    const char* kFooter =
        "endcmap\n"
        "CMapName currentdict /CMap defineresource pop\n"
        "end\n"
        "end";
    cmap->writeText(kFooter);
}

struct BFChar {
    uint16_t fGlyphId;
    SkUnichar fUnicode;
};

struct BFRange {
    uint16_t fStart;
    uint16_t fEnd;
    SkUnichar fUnicode;
};

static void append_bfchar_section(const SkTDArray<BFChar>& bfchar,
                                  SkDynamicMemoryWStream* cmap) {
    
    for (int i = 0; i < bfchar.count(); i += 100) {
        int count = bfchar.count() - i;
        count = SkMin32(count, 100);
        cmap->writeDecAsText(count);
        cmap->writeText(" beginbfchar\n");
        for (int j = 0; j < count; ++j) {
            cmap->writeText("<");
            cmap->writeHexAsText(bfchar[i + j].fGlyphId, 4);
            cmap->writeText("> <");
            cmap->writeHexAsText(bfchar[i + j].fUnicode, 4);
            cmap->writeText(">\n");
        }
        cmap->writeText("endbfchar\n");
    }
}

static void append_bfrange_section(const SkTDArray<BFRange>& bfrange,
                                   SkDynamicMemoryWStream* cmap) {
    
    for (int i = 0; i < bfrange.count(); i += 100) {
        int count = bfrange.count() - i;
        count = SkMin32(count, 100);
        cmap->writeDecAsText(count);
        cmap->writeText(" beginbfrange\n");
        for (int j = 0; j < count; ++j) {
            cmap->writeText("<");
            cmap->writeHexAsText(bfrange[i + j].fStart, 4);
            cmap->writeText("> <");
            cmap->writeHexAsText(bfrange[i + j].fEnd, 4);
            cmap->writeText("> <");
            cmap->writeHexAsText(bfrange[i + j].fUnicode, 4);
            cmap->writeText(">\n");
        }
        cmap->writeText("endbfrange\n");
    }
}






























void append_cmap_sections(const SkTDArray<SkUnichar>& glyphToUnicode,
                          const SkPDFGlyphSet* subset,
                          SkDynamicMemoryWStream* cmap,
                          bool multiByteGlyphs,
                          uint16_t firstGlyphID,
                          uint16_t lastGlyphID);

void append_cmap_sections(const SkTDArray<SkUnichar>& glyphToUnicode,
                          const SkPDFGlyphSet* subset,
                          SkDynamicMemoryWStream* cmap,
                          bool multiByteGlyphs,
                          uint16_t firstGlyphID,
                          uint16_t lastGlyphID) {
    if (glyphToUnicode.isEmpty()) {
        return;
    }
    int glyphOffset = 0;
    if (!multiByteGlyphs) {
        glyphOffset = firstGlyphID - 1;
    }

    SkTDArray<BFChar> bfcharEntries;
    SkTDArray<BFRange> bfrangeEntries;

    BFRange currentRangeEntry = {0, 0, 0};
    bool rangeEmpty = true;
    const int limit =
            SkMin32(lastGlyphID + 1, glyphToUnicode.count()) - glyphOffset;

    for (int i = firstGlyphID - glyphOffset; i < limit + 1; ++i) {
        bool inSubset = i < limit &&
                        (subset == NULL || subset->has(i + glyphOffset));
        if (!rangeEmpty) {
            
            
            
            bool inRange =
                i == currentRangeEntry.fEnd + 1 &&
                i >> 8 == currentRangeEntry.fStart >> 8 &&
                i < limit &&
                glyphToUnicode[i + glyphOffset] ==
                    currentRangeEntry.fUnicode + i - currentRangeEntry.fStart;
            if (!inSubset || !inRange) {
                if (currentRangeEntry.fEnd > currentRangeEntry.fStart) {
                    bfrangeEntries.push(currentRangeEntry);
                } else {
                    BFChar* entry = bfcharEntries.append();
                    entry->fGlyphId = currentRangeEntry.fStart;
                    entry->fUnicode = currentRangeEntry.fUnicode;
                }
                rangeEmpty = true;
            }
        }
        if (inSubset) {
            currentRangeEntry.fEnd = i;
            if (rangeEmpty) {
              currentRangeEntry.fStart = i;
              currentRangeEntry.fUnicode = glyphToUnicode[i + glyphOffset];
              rangeEmpty = false;
            }
        }
    }

    
    
    append_bfchar_section(bfcharEntries, cmap);
    append_bfrange_section(bfrangeEntries, cmap);
}

static SkPDFStream* generate_tounicode_cmap(
        const SkTDArray<SkUnichar>& glyphToUnicode,
        const SkPDFGlyphSet* subset,
        bool multiByteGlyphs,
        uint16_t firstGlyphID,
        uint16_t lastGlyphID) {
    SkDynamicMemoryWStream cmap;
    if (multiByteGlyphs) {
        append_tounicode_header(&cmap, firstGlyphID, lastGlyphID);
    } else {
        append_tounicode_header(&cmap, 1, lastGlyphID - firstGlyphID + 1);
    }
    append_cmap_sections(glyphToUnicode, subset, &cmap, multiByteGlyphs,
                         firstGlyphID, lastGlyphID);
    append_cmap_footer(&cmap);
    SkAutoTUnref<SkData> cmapData(cmap.copyToData());
    return new SkPDFStream(cmapData.get());
}

#if defined (SK_SFNTLY_SUBSETTER)
static void sk_delete_array(const void* ptr, size_t, void*) {
    
    delete[] (unsigned char*)ptr;
}
#endif

static size_t get_subset_font_stream(const char* fontName,
                                     const SkTypeface* typeface,
                                     const SkTDArray<uint32_t>& subset,
                                     SkPDFStream** fontStream) {
    int ttcIndex;
    SkAutoTUnref<SkStream> fontData(typeface->openStream(&ttcIndex));
    SkASSERT(fontData.get());

    size_t fontSize = fontData->getLength();

#if defined (SK_SFNTLY_SUBSETTER)
    
    SkPDFStream* subsetFontStream = NULL;
    SkTDArray<unsigned char> originalFont;
    originalFont.setCount(SkToInt(fontSize));
    if (fontData->read(originalFont.begin(), fontSize) == fontSize) {
        unsigned char* subsetFont = NULL;
        
        
        SK_COMPILE_ASSERT(sizeof(unsigned int) == sizeof(uint32_t),
                          unsigned_int_not_32_bits);
        int subsetFontSize = SfntlyWrapper::SubsetFont(fontName,
                                                       originalFont.begin(),
                                                       fontSize,
                                                       subset.begin(),
                                                       subset.count(),
                                                       &subsetFont);
        if (subsetFontSize > 0 && subsetFont != NULL) {
            SkAutoDataUnref data(SkData::NewWithProc(subsetFont,
                                                     subsetFontSize,
                                                     sk_delete_array,
                                                     NULL));
            subsetFontStream = new SkPDFStream(data.get());
            fontSize = subsetFontSize;
        }
    }
    if (subsetFontStream) {
        *fontStream = subsetFontStream;
        return fontSize;
    }
    fontData->rewind();
#else
    sk_ignore_unused_variable(fontName);
    sk_ignore_unused_variable(subset);
#endif

    
    *fontStream = new SkPDFStream(fontData.get());
    return fontSize;
}





SkPDFGlyphSet::SkPDFGlyphSet() : fBitSet(SK_MaxU16 + 1) {
}

void SkPDFGlyphSet::set(const uint16_t* glyphIDs, int numGlyphs) {
    for (int i = 0; i < numGlyphs; ++i) {
        fBitSet.setBit(glyphIDs[i], true);
    }
}

bool SkPDFGlyphSet::has(uint16_t glyphID) const {
    return fBitSet.isBitSet(glyphID);
}

void SkPDFGlyphSet::merge(const SkPDFGlyphSet& usage) {
    fBitSet.orBits(usage.fBitSet);
}

void SkPDFGlyphSet::exportTo(SkTDArray<unsigned int>* glyphIDs) const {
    fBitSet.exportTo(glyphIDs);
}




SkPDFGlyphSetMap::FontGlyphSetPair::FontGlyphSetPair(SkPDFFont* font,
                                                     SkPDFGlyphSet* glyphSet)
        : fFont(font),
          fGlyphSet(glyphSet) {
}

SkPDFGlyphSetMap::F2BIter::F2BIter(const SkPDFGlyphSetMap& map) {
    reset(map);
}

const SkPDFGlyphSetMap::FontGlyphSetPair* SkPDFGlyphSetMap::F2BIter::next() const {
    if (fIndex >= fMap->count()) {
        return NULL;
    }
    return &((*fMap)[fIndex++]);
}

void SkPDFGlyphSetMap::F2BIter::reset(const SkPDFGlyphSetMap& map) {
    fMap = &(map.fMap);
    fIndex = 0;
}

SkPDFGlyphSetMap::SkPDFGlyphSetMap() {
}

SkPDFGlyphSetMap::~SkPDFGlyphSetMap() {
    reset();
}

void SkPDFGlyphSetMap::merge(const SkPDFGlyphSetMap& usage) {
    for (int i = 0; i < usage.fMap.count(); ++i) {
        SkPDFGlyphSet* myUsage = getGlyphSetForFont(usage.fMap[i].fFont);
        myUsage->merge(*(usage.fMap[i].fGlyphSet));
    }
}

void SkPDFGlyphSetMap::reset() {
    for (int i = 0; i < fMap.count(); ++i) {
        delete fMap[i].fGlyphSet;  
    }
    fMap.reset();
}

void SkPDFGlyphSetMap::noteGlyphUsage(SkPDFFont* font, const uint16_t* glyphIDs,
                                      int numGlyphs) {
    SkPDFGlyphSet* subset = getGlyphSetForFont(font);
    if (subset) {
        subset->set(glyphIDs, numGlyphs);
    }
}

SkPDFGlyphSet* SkPDFGlyphSetMap::getGlyphSetForFont(SkPDFFont* font) {
    int index = fMap.count();
    for (int i = 0; i < index; ++i) {
        if (fMap[i].fFont == font) {
            return fMap[i].fGlyphSet;
        }
    }
    fMap.append();
    index = fMap.count() - 1;
    fMap[index].fFont = font;
    fMap[index].fGlyphSet = new SkPDFGlyphSet();
    return fMap[index].fGlyphSet;
}

















SkPDFFont::~SkPDFFont() {
    SkAutoMutexAcquire lock(CanonicalFontsMutex());
    int index = -1;
    for (int i = 0 ; i < CanonicalFonts().count() ; i++) {
        if (CanonicalFonts()[i].fFont == this) {
            index = i;
        }
    }

    SkDEBUGCODE(int indexFound;)
    SkASSERT(index == -1 ||
             (Find(fTypeface->uniqueID(),
                   fFirstGlyphID,
                   &indexFound) &&
             index == indexFound));
    if (index >= 0) {
        CanonicalFonts().removeShuffle(index);
    }
    fResources.unrefAll();
}

void SkPDFFont::getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                             SkTSet<SkPDFObject*>* newResourceObjects) {
    GetResourcesHelper(&fResources, knownResourceObjects, newResourceObjects);
}

SkTypeface* SkPDFFont::typeface() {
    return fTypeface.get();
}

SkAdvancedTypefaceMetrics::FontType SkPDFFont::getType() {
    return fFontType;
}

bool SkPDFFont::canEmbed() const {
    if (!fFontInfo.get()) {
        SkASSERT(fFontType == SkAdvancedTypefaceMetrics::kOther_Font);
        return true;
    }
    return (fFontInfo->fFlags &
            SkAdvancedTypefaceMetrics::kNotEmbeddable_FontFlag) == 0;
}

bool SkPDFFont::canSubset() const {
    if (!fFontInfo.get()) {
        SkASSERT(fFontType == SkAdvancedTypefaceMetrics::kOther_Font);
        return true;
    }
    return (fFontInfo->fFlags &
            SkAdvancedTypefaceMetrics::kNotSubsettable_FontFlag) == 0;
}

bool SkPDFFont::hasGlyph(uint16_t id) {
    return (id >= fFirstGlyphID && id <= fLastGlyphID) || id == 0;
}

int SkPDFFont::glyphsToPDFFontEncoding(uint16_t* glyphIDs, int numGlyphs) {
    
    if (this->multiByteGlyphs()) {
        return numGlyphs;
    }

    for (int i = 0; i < numGlyphs; i++) {
        if (glyphIDs[i] == 0) {
            continue;
        }
        if (glyphIDs[i] < fFirstGlyphID || glyphIDs[i] > fLastGlyphID) {
            return i;
        }
        glyphIDs[i] -= (fFirstGlyphID - 1);
    }

    return numGlyphs;
}


SkPDFFont* SkPDFFont::GetFontResource(SkTypeface* typeface, uint16_t glyphID) {
    SkAutoMutexAcquire lock(CanonicalFontsMutex());

    SkAutoResolveDefaultTypeface autoResolve(typeface);
    typeface = autoResolve.get();

    const uint32_t fontID = typeface->uniqueID();
    int relatedFontIndex;
    if (Find(fontID, glyphID, &relatedFontIndex)) {
        CanonicalFonts()[relatedFontIndex].fFont->ref();
        return CanonicalFonts()[relatedFontIndex].fFont;
    }

    SkAutoTUnref<const SkAdvancedTypefaceMetrics> fontMetrics;
    SkPDFDict* relatedFontDescriptor = NULL;
    if (relatedFontIndex >= 0) {
        SkPDFFont* relatedFont = CanonicalFonts()[relatedFontIndex].fFont;
        fontMetrics.reset(relatedFont->fontInfo());
        SkSafeRef(fontMetrics.get());
        relatedFontDescriptor = relatedFont->getFontDescriptor();

        
        
        
        SkAdvancedTypefaceMetrics::FontType fontType =
            fontMetrics.get() ? fontMetrics.get()->fType :
                                SkAdvancedTypefaceMetrics::kOther_Font;

        if (fontType == SkAdvancedTypefaceMetrics::kType1CID_Font ||
            fontType == SkAdvancedTypefaceMetrics::kTrueType_Font) {
            CanonicalFonts()[relatedFontIndex].fFont->ref();
            return CanonicalFonts()[relatedFontIndex].fFont;
        }
    } else {
        SkAdvancedTypefaceMetrics::PerGlyphInfo info;
        info = SkAdvancedTypefaceMetrics::kGlyphNames_PerGlyphInfo;
        info = SkTBitOr<SkAdvancedTypefaceMetrics::PerGlyphInfo>(
                  info, SkAdvancedTypefaceMetrics::kToUnicode_PerGlyphInfo);
#if !defined (SK_SFNTLY_SUBSETTER)
        info = SkTBitOr<SkAdvancedTypefaceMetrics::PerGlyphInfo>(
                  info, SkAdvancedTypefaceMetrics::kHAdvance_PerGlyphInfo);
#endif
        fontMetrics.reset(
            typeface->getAdvancedTypefaceMetrics(info, NULL, 0));
#if defined (SK_SFNTLY_SUBSETTER)
        if (fontMetrics.get() &&
            fontMetrics->fType != SkAdvancedTypefaceMetrics::kTrueType_Font) {
            
            info = SkTBitOr<SkAdvancedTypefaceMetrics::PerGlyphInfo>(
                      info, SkAdvancedTypefaceMetrics::kHAdvance_PerGlyphInfo);
            fontMetrics.reset(
                typeface->getAdvancedTypefaceMetrics(info, NULL, 0));
        }
#endif
    }

    SkPDFFont* font = Create(fontMetrics.get(), typeface, glyphID,
                             relatedFontDescriptor);
    FontRec newEntry(font, fontID, font->fFirstGlyphID);
    CanonicalFonts().push(newEntry);
    return font;  
}

SkPDFFont* SkPDFFont::getFontSubset(const SkPDFGlyphSet*) {
    return NULL;  
}


SkTDArray<SkPDFFont::FontRec>& SkPDFFont::CanonicalFonts() {
    SkPDFFont::CanonicalFontsMutex().assertHeld();
    static SkTDArray<FontRec> gCanonicalFonts;
    return gCanonicalFonts;
}

SK_DECLARE_STATIC_MUTEX(gCanonicalFontsMutex);

SkBaseMutex& SkPDFFont::CanonicalFontsMutex() {
    return gCanonicalFontsMutex;
}


bool SkPDFFont::Find(uint32_t fontID, uint16_t glyphID, int* index) {
    
    FontRec search(NULL, fontID, glyphID);
    *index = CanonicalFonts().find(search);
    if (*index >= 0) {
        return true;
    }
    search.fGlyphID = 0;
    *index = CanonicalFonts().find(search);
    return false;
}

SkPDFFont::SkPDFFont(const SkAdvancedTypefaceMetrics* info,
                     SkTypeface* typeface,
                     SkPDFDict* relatedFontDescriptor)
        : SkPDFDict("Font"),
          fTypeface(ref_or_default(typeface)),
          fFirstGlyphID(1),
          fLastGlyphID(info ? info->fLastGlyphID : 0),
          fFontInfo(SkSafeRef(info)),
          fDescriptor(SkSafeRef(relatedFontDescriptor)) {
    if (info == NULL ||
            info->fFlags & SkAdvancedTypefaceMetrics::kMultiMaster_FontFlag) {
        fFontType = SkAdvancedTypefaceMetrics::kOther_Font;
    } else {
        fFontType = info->fType;
    }
}


SkPDFFont* SkPDFFont::Create(const SkAdvancedTypefaceMetrics* info,
                             SkTypeface* typeface, uint16_t glyphID,
                             SkPDFDict* relatedFontDescriptor) {
    SkAdvancedTypefaceMetrics::FontType type =
        info ? info->fType : SkAdvancedTypefaceMetrics::kOther_Font;

    if (info &&
            (info->fFlags & SkAdvancedTypefaceMetrics::kMultiMaster_FontFlag)) {
        NOT_IMPLEMENTED(true, true);
        return new SkPDFType3Font(info,
                                  typeface,
                                  glyphID);
    }
    if (type == SkAdvancedTypefaceMetrics::kType1CID_Font ||
        type == SkAdvancedTypefaceMetrics::kTrueType_Font) {
        SkASSERT(relatedFontDescriptor == NULL);
        return new SkPDFType0Font(info, typeface);
    }
    if (type == SkAdvancedTypefaceMetrics::kType1_Font) {
        return new SkPDFType1Font(info,
                                  typeface,
                                  glyphID,
                                  relatedFontDescriptor);
    }

    SkASSERT(type == SkAdvancedTypefaceMetrics::kCFF_Font ||
             type == SkAdvancedTypefaceMetrics::kOther_Font);

    return new SkPDFType3Font(info, typeface, glyphID);
}

const SkAdvancedTypefaceMetrics* SkPDFFont::fontInfo() {
    return fFontInfo.get();
}

void SkPDFFont::setFontInfo(const SkAdvancedTypefaceMetrics* info) {
    if (info == NULL || info == fFontInfo.get()) {
        return;
    }
    fFontInfo.reset(info);
    SkSafeRef(info);
}

uint16_t SkPDFFont::firstGlyphID() const {
    return fFirstGlyphID;
}

uint16_t SkPDFFont::lastGlyphID() const {
    return fLastGlyphID;
}

void SkPDFFont::setLastGlyphID(uint16_t glyphID) {
    fLastGlyphID = glyphID;
}

void SkPDFFont::addResource(SkPDFObject* object) {
    SkASSERT(object != NULL);
    fResources.push(object);
    object->ref();
}

SkPDFDict* SkPDFFont::getFontDescriptor() {
    return fDescriptor.get();
}

void SkPDFFont::setFontDescriptor(SkPDFDict* descriptor) {
    fDescriptor.reset(descriptor);
    SkSafeRef(descriptor);
}

bool SkPDFFont::addCommonFontDescriptorEntries(int16_t defaultWidth) {
    if (fDescriptor.get() == NULL) {
        return false;
    }

    const uint16_t emSize = fFontInfo->fEmSize;

    fDescriptor->insertName("FontName", fFontInfo->fFontName);
    fDescriptor->insertInt("Flags", fFontInfo->fStyle | kPdfSymbolic);
    fDescriptor->insertScalar("Ascent",
            scaleFromFontUnits(fFontInfo->fAscent, emSize));
    fDescriptor->insertScalar("Descent",
            scaleFromFontUnits(fFontInfo->fDescent, emSize));
    fDescriptor->insertScalar("StemV",
            scaleFromFontUnits(fFontInfo->fStemV, emSize));

    fDescriptor->insertScalar("CapHeight",
            scaleFromFontUnits(fFontInfo->fCapHeight, emSize));
    fDescriptor->insertInt("ItalicAngle", fFontInfo->fItalicAngle);
    fDescriptor->insert("FontBBox", makeFontBBox(fFontInfo->fBBox,
                                                 fFontInfo->fEmSize))->unref();

    if (defaultWidth > 0) {
        fDescriptor->insertScalar("MissingWidth",
                scaleFromFontUnits(defaultWidth, emSize));
    }
    return true;
}

void SkPDFFont::adjustGlyphRangeForSingleByteEncoding(int16_t glyphID) {
    
    fFirstGlyphID = glyphID - (glyphID - 1) % 255;
    if (fLastGlyphID > fFirstGlyphID + 255 - 1) {
        fLastGlyphID = fFirstGlyphID + 255 - 1;
    }
}

bool SkPDFFont::FontRec::operator==(const SkPDFFont::FontRec& b) const {
    if (fFontID != b.fFontID) {
        return false;
    }
    if (fFont != NULL && b.fFont != NULL) {
        return fFont->fFirstGlyphID == b.fFont->fFirstGlyphID &&
            fFont->fLastGlyphID == b.fFont->fLastGlyphID;
    }
    if (fGlyphID == 0 || b.fGlyphID == 0) {
        return true;
    }

    if (fFont != NULL) {
        return fFont->fFirstGlyphID <= b.fGlyphID &&
            b.fGlyphID <= fFont->fLastGlyphID;
    } else if (b.fFont != NULL) {
        return b.fFont->fFirstGlyphID <= fGlyphID &&
            fGlyphID <= b.fFont->fLastGlyphID;
    }
    return fGlyphID == b.fGlyphID;
}

SkPDFFont::FontRec::FontRec(SkPDFFont* font, uint32_t fontID, uint16_t glyphID)
    : fFont(font),
      fFontID(fontID),
      fGlyphID(glyphID) {
}

void SkPDFFont::populateToUnicodeTable(const SkPDFGlyphSet* subset) {
    if (fFontInfo == NULL || fFontInfo->fGlyphToUnicode.begin() == NULL) {
        return;
    }
    SkAutoTUnref<SkPDFStream> pdfCmap(
        generate_tounicode_cmap(fFontInfo->fGlyphToUnicode, subset,
                                multiByteGlyphs(), firstGlyphID(),
                                lastGlyphID()));
    addResource(pdfCmap.get());
    insert("ToUnicode", new SkPDFObjRef(pdfCmap.get()))->unref();
}





SkPDFType0Font::SkPDFType0Font(const SkAdvancedTypefaceMetrics* info,
                               SkTypeface* typeface)
        : SkPDFFont(info, typeface, NULL) {
    SkDEBUGCODE(fPopulated = false);
    if (!canSubset()) {
        populate(NULL);
    }
}

SkPDFType0Font::~SkPDFType0Font() {}

SkPDFFont* SkPDFType0Font::getFontSubset(const SkPDFGlyphSet* subset) {
    if (!canSubset()) {
        return NULL;
    }
    SkPDFType0Font* newSubset = new SkPDFType0Font(fontInfo(), typeface());
    newSubset->populate(subset);
    return newSubset;
}

#ifdef SK_DEBUG
void SkPDFType0Font::emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                                bool indirect) {
    SkASSERT(fPopulated);
    return INHERITED::emitObject(stream, catalog, indirect);
}
#endif

bool SkPDFType0Font::populate(const SkPDFGlyphSet* subset) {
    insertName("Subtype", "Type0");
    insertName("BaseFont", fontInfo()->fFontName);
    insertName("Encoding", "Identity-H");

    SkAutoTUnref<SkPDFCIDFont> newCIDFont(
            new SkPDFCIDFont(fontInfo(), typeface(), subset));
    addResource(newCIDFont.get());
    SkAutoTUnref<SkPDFArray> descendantFonts(new SkPDFArray());
    descendantFonts->append(new SkPDFObjRef(newCIDFont.get()))->unref();
    insert("DescendantFonts", descendantFonts.get());

    populateToUnicodeTable(subset);

    SkDEBUGCODE(fPopulated = true);
    return true;
}





SkPDFCIDFont::SkPDFCIDFont(const SkAdvancedTypefaceMetrics* info,
                           SkTypeface* typeface, const SkPDFGlyphSet* subset)
        : SkPDFFont(info, typeface, NULL) {
    populate(subset);
}

SkPDFCIDFont::~SkPDFCIDFont() {}

bool SkPDFCIDFont::addFontDescriptor(int16_t defaultWidth,
                                     const SkTDArray<uint32_t>* subset) {
    SkAutoTUnref<SkPDFDict> descriptor(new SkPDFDict("FontDescriptor"));
    setFontDescriptor(descriptor.get());
    addResource(descriptor.get());
    insert("FontDescriptor", new SkPDFObjRef(descriptor.get()))->unref();
    if (!addCommonFontDescriptorEntries(defaultWidth)) {
        return false;
    }
    if (!canEmbed()) {
        return true;
    }

    switch (getType()) {
        case SkAdvancedTypefaceMetrics::kTrueType_Font: {
            SkAutoTUnref<SkPDFStream> fontStream;
            size_t fontSize = 0;
            if (canSubset()) {
                SkPDFStream* rawStream = NULL;
                fontSize = get_subset_font_stream(fontInfo()->fFontName.c_str(),
                                                  typeface(),
                                                  *subset,
                                                  &rawStream);
                fontStream.reset(rawStream);
            } else {
                int ttcIndex;
                SkAutoTUnref<SkStream> fontData(
                        typeface()->openStream(&ttcIndex));
                fontStream.reset(new SkPDFStream(fontData.get()));
                fontSize = fontData->getLength();
            }
            SkASSERT(fontSize);
            SkASSERT(fontStream.get());
            addResource(fontStream.get());

            fontStream->insertInt("Length1", fontSize);
            descriptor->insert("FontFile2",
                               new SkPDFObjRef(fontStream.get()))->unref();
            break;
        }
        case SkAdvancedTypefaceMetrics::kCFF_Font:
        case SkAdvancedTypefaceMetrics::kType1CID_Font: {
            int ttcIndex;
            SkAutoTUnref<SkStream> fontData(typeface()->openStream(&ttcIndex));
            SkAutoTUnref<SkPDFStream> fontStream(
                new SkPDFStream(fontData.get()));
            addResource(fontStream.get());

            if (getType() == SkAdvancedTypefaceMetrics::kCFF_Font) {
                fontStream->insertName("Subtype", "Type1C");
            } else {
                fontStream->insertName("Subtype", "CIDFontType0c");
            }
            descriptor->insert("FontFile3",
                                new SkPDFObjRef(fontStream.get()))->unref();
            break;
        }
        default:
            SkASSERT(false);
    }
    return true;
}

bool SkPDFCIDFont::populate(const SkPDFGlyphSet* subset) {
    
    if (fontInfo()->fType == SkAdvancedTypefaceMetrics::kTrueType_Font) {
        
        SkTDArray<uint32_t> glyphIDs;
        if (subset) {
            
            if (!subset->has(0)) {
                glyphIDs.push(0);
            }
            subset->exportTo(&glyphIDs);
        }

        SkAdvancedTypefaceMetrics::PerGlyphInfo info;
        info = SkAdvancedTypefaceMetrics::kGlyphNames_PerGlyphInfo;
        info = SkTBitOr<SkAdvancedTypefaceMetrics::PerGlyphInfo>(
                  info, SkAdvancedTypefaceMetrics::kHAdvance_PerGlyphInfo);
        uint32_t* glyphs = (glyphIDs.count() == 0) ? NULL : glyphIDs.begin();
        uint32_t glyphsCount = glyphs ? glyphIDs.count() : 0;
        SkAutoTUnref<const SkAdvancedTypefaceMetrics> fontMetrics(
            typeface()->getAdvancedTypefaceMetrics(info, glyphs, glyphsCount));
        setFontInfo(fontMetrics.get());
        addFontDescriptor(0, &glyphIDs);
    } else {
        
        addFontDescriptor(0, NULL);
    }

    insertName("BaseFont", fontInfo()->fFontName);

    if (getType() == SkAdvancedTypefaceMetrics::kType1CID_Font) {
        insertName("Subtype", "CIDFontType0");
    } else if (getType() == SkAdvancedTypefaceMetrics::kTrueType_Font) {
        insertName("Subtype", "CIDFontType2");
        insertName("CIDToGIDMap", "Identity");
    } else {
        SkASSERT(false);
    }

    SkAutoTUnref<SkPDFDict> sysInfo(new SkPDFDict);
    sysInfo->insert("Registry", new SkPDFString("Adobe"))->unref();
    sysInfo->insert("Ordering", new SkPDFString("Identity"))->unref();
    sysInfo->insertInt("Supplement", 0);
    insert("CIDSystemInfo", sysInfo.get());

    if (fontInfo()->fGlyphWidths.get()) {
        int16_t defaultWidth = 0;
        SkAutoTUnref<SkPDFArray> widths(
            composeAdvanceData(fontInfo()->fGlyphWidths.get(),
                               fontInfo()->fEmSize, &appendWidth,
                               &defaultWidth));
        if (widths->size())
            insert("W", widths.get());
        if (defaultWidth != 0) {
            insertScalar("DW", scaleFromFontUnits(defaultWidth,
                                                  fontInfo()->fEmSize));
        }
    }
    if (fontInfo()->fVerticalMetrics.get()) {
        struct SkAdvancedTypefaceMetrics::VerticalMetric defaultAdvance;
        defaultAdvance.fVerticalAdvance = 0;
        defaultAdvance.fOriginXDisp = 0;
        defaultAdvance.fOriginYDisp = 0;
        SkAutoTUnref<SkPDFArray> advances(
            composeAdvanceData(fontInfo()->fVerticalMetrics.get(),
                               fontInfo()->fEmSize, &appendVerticalAdvance,
                               &defaultAdvance));
        if (advances->size())
            insert("W2", advances.get());
        if (defaultAdvance.fVerticalAdvance ||
                defaultAdvance.fOriginXDisp ||
                defaultAdvance.fOriginYDisp) {
            insert("DW2", appendVerticalAdvance(defaultAdvance,
                                                fontInfo()->fEmSize,
                                                new SkPDFArray))->unref();
        }
    }

    return true;
}





SkPDFType1Font::SkPDFType1Font(const SkAdvancedTypefaceMetrics* info,
                               SkTypeface* typeface,
                               uint16_t glyphID,
                               SkPDFDict* relatedFontDescriptor)
        : SkPDFFont(info, typeface, relatedFontDescriptor) {
    populate(glyphID);
}

SkPDFType1Font::~SkPDFType1Font() {}

bool SkPDFType1Font::addFontDescriptor(int16_t defaultWidth) {
    if (getFontDescriptor() != NULL) {
        SkPDFDict* descriptor = getFontDescriptor();
        addResource(descriptor);
        insert("FontDescriptor", new SkPDFObjRef(descriptor))->unref();
        return true;
    }

    SkAutoTUnref<SkPDFDict> descriptor(new SkPDFDict("FontDescriptor"));
    setFontDescriptor(descriptor.get());

    int ttcIndex;
    size_t header SK_INIT_TO_AVOID_WARNING;
    size_t data SK_INIT_TO_AVOID_WARNING;
    size_t trailer SK_INIT_TO_AVOID_WARNING;
    SkAutoTUnref<SkStream> rawFontData(typeface()->openStream(&ttcIndex));
    SkData* fontData = handle_type1_stream(rawFontData.get(), &header, &data,
                                           &trailer);
    if (fontData == NULL) {
        return false;
    }
    if (canEmbed()) {
        SkAutoTUnref<SkPDFStream> fontStream(new SkPDFStream(fontData));
        addResource(fontStream.get());
        fontStream->insertInt("Length1", header);
        fontStream->insertInt("Length2", data);
        fontStream->insertInt("Length3", trailer);
        descriptor->insert("FontFile",
                           new SkPDFObjRef(fontStream.get()))->unref();
    }

    addResource(descriptor.get());
    insert("FontDescriptor", new SkPDFObjRef(descriptor.get()))->unref();

    return addCommonFontDescriptorEntries(defaultWidth);
}

bool SkPDFType1Font::populate(int16_t glyphID) {
    SkASSERT(!fontInfo()->fVerticalMetrics.get());
    SkASSERT(fontInfo()->fGlyphWidths.get());

    adjustGlyphRangeForSingleByteEncoding(glyphID);

    int16_t defaultWidth = 0;
    const SkAdvancedTypefaceMetrics::WidthRange* widthRangeEntry = NULL;
    const SkAdvancedTypefaceMetrics::WidthRange* widthEntry;
    for (widthEntry = fontInfo()->fGlyphWidths.get();
            widthEntry != NULL;
            widthEntry = widthEntry->fNext.get()) {
        switch (widthEntry->fType) {
            case SkAdvancedTypefaceMetrics::WidthRange::kDefault:
                defaultWidth = widthEntry->fAdvance[0];
                break;
            case SkAdvancedTypefaceMetrics::WidthRange::kRun:
                SkASSERT(false);
                break;
            case SkAdvancedTypefaceMetrics::WidthRange::kRange:
                SkASSERT(widthRangeEntry == NULL);
                widthRangeEntry = widthEntry;
                break;
        }
    }

    if (!addFontDescriptor(defaultWidth)) {
        return false;
    }

    insertName("Subtype", "Type1");
    insertName("BaseFont", fontInfo()->fFontName);

    addWidthInfoFromRange(defaultWidth, widthRangeEntry);

    SkAutoTUnref<SkPDFDict> encoding(new SkPDFDict("Encoding"));
    insert("Encoding", encoding.get());

    SkAutoTUnref<SkPDFArray> encDiffs(new SkPDFArray);
    encoding->insert("Differences", encDiffs.get());

    encDiffs->reserve(lastGlyphID() - firstGlyphID() + 2);
    encDiffs->appendInt(1);
    for (int gID = firstGlyphID(); gID <= lastGlyphID(); gID++) {
        encDiffs->appendName(fontInfo()->fGlyphNames->get()[gID].c_str());
    }

    return true;
}

void SkPDFType1Font::addWidthInfoFromRange(
        int16_t defaultWidth,
        const SkAdvancedTypefaceMetrics::WidthRange* widthRangeEntry) {
    SkAutoTUnref<SkPDFArray> widthArray(new SkPDFArray());
    int firstChar = 0;
    if (widthRangeEntry) {
        const uint16_t emSize = fontInfo()->fEmSize;
        int startIndex = firstGlyphID() - widthRangeEntry->fStartId;
        int endIndex = startIndex + lastGlyphID() - firstGlyphID() + 1;
        if (startIndex < 0)
            startIndex = 0;
        if (endIndex > widthRangeEntry->fAdvance.count())
            endIndex = widthRangeEntry->fAdvance.count();
        if (widthRangeEntry->fStartId == 0) {
            appendWidth(widthRangeEntry->fAdvance[0], emSize, widthArray.get());
        } else {
            firstChar = startIndex + widthRangeEntry->fStartId;
        }
        for (int i = startIndex; i < endIndex; i++) {
            appendWidth(widthRangeEntry->fAdvance[i], emSize, widthArray.get());
        }
    } else {
        appendWidth(defaultWidth, 1000, widthArray.get());
    }
    insertInt("FirstChar", firstChar);
    insertInt("LastChar", firstChar + widthArray->size() - 1);
    insert("Widths", widthArray.get());
}





SkPDFType3Font::SkPDFType3Font(const SkAdvancedTypefaceMetrics* info,
                               SkTypeface* typeface,
                               uint16_t glyphID)
        : SkPDFFont(info, typeface, NULL) {
    populate(glyphID);
}

SkPDFType3Font::~SkPDFType3Font() {}

bool SkPDFType3Font::populate(int16_t glyphID) {
    SkPaint paint;
    paint.setTypeface(typeface());
    paint.setTextSize(1000);
    SkAutoGlyphCache autoCache(paint, NULL, NULL);
    SkGlyphCache* cache = autoCache.getCache();
    
    if (lastGlyphID() == 0) {
        setLastGlyphID(cache->getGlyphCount() - 1);
    }

    adjustGlyphRangeForSingleByteEncoding(glyphID);

    insertName("Subtype", "Type3");
    
    SkMatrix fontMatrix;
    fontMatrix.setScale(SkScalarInvert(1000), -SkScalarInvert(1000));
    insert("FontMatrix", SkPDFUtils::MatrixToArray(fontMatrix))->unref();

    SkAutoTUnref<SkPDFDict> charProcs(new SkPDFDict);
    insert("CharProcs", charProcs.get());

    SkAutoTUnref<SkPDFDict> encoding(new SkPDFDict("Encoding"));
    insert("Encoding", encoding.get());

    SkAutoTUnref<SkPDFArray> encDiffs(new SkPDFArray);
    encoding->insert("Differences", encDiffs.get());
    encDiffs->reserve(lastGlyphID() - firstGlyphID() + 2);
    encDiffs->appendInt(1);

    SkAutoTUnref<SkPDFArray> widthArray(new SkPDFArray());

    SkIRect bbox = SkIRect::MakeEmpty();
    for (int gID = firstGlyphID(); gID <= lastGlyphID(); gID++) {
        SkString characterName;
        characterName.printf("gid%d", gID);
        encDiffs->appendName(characterName.c_str());

        const SkGlyph& glyph = cache->getGlyphIDMetrics(gID);
        widthArray->appendScalar(SkFixedToScalar(glyph.fAdvanceX));
        SkIRect glyphBBox = SkIRect::MakeXYWH(glyph.fLeft, glyph.fTop,
                                              glyph.fWidth, glyph.fHeight);
        bbox.join(glyphBBox);

        SkDynamicMemoryWStream content;
        setGlyphWidthAndBoundingBox(SkFixedToScalar(glyph.fAdvanceX), glyphBBox,
                                    &content);
        const SkPath* path = cache->findPath(glyph);
        if (path) {
            SkPDFUtils::EmitPath(*path, paint.getStyle(), &content);
            SkPDFUtils::PaintPath(paint.getStyle(), path->getFillType(),
                                  &content);
        }
        SkAutoTUnref<SkMemoryStream> glyphStream(new SkMemoryStream());
        glyphStream->setData(content.copyToData())->unref();

        SkAutoTUnref<SkPDFStream> glyphDescription(
            new SkPDFStream(glyphStream.get()));
        addResource(glyphDescription.get());
        charProcs->insert(characterName.c_str(),
                          new SkPDFObjRef(glyphDescription.get()))->unref();
    }

    insert("FontBBox", makeFontBBox(bbox, 1000))->unref();
    insertInt("FirstChar", 1);
    insertInt("LastChar", lastGlyphID() - firstGlyphID() + 1);
    insert("Widths", widthArray.get());
    insertName("CIDToGIDMap", "Identity");

    populateToUnicodeTable(NULL);
    return true;
}
