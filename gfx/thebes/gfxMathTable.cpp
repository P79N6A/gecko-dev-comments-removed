



#include "gfxMathTable.h"

#include "MathTableStructures.h"
#include "harfbuzz/hb.h"
#include "mozilla/BinarySearch.h"
#include <algorithm>

using namespace mozilla;

gfxMathTable::gfxMathTable(hb_blob_t* aMathTable)
  : mMathTable(aMathTable)
  , mGlyphConstruction(nullptr)
  , mGlyphID(-1)
  , mVertical(false)
{
}

gfxMathTable::~gfxMathTable()
{
  hb_blob_destroy(mMathTable);
}

bool
gfxMathTable::HasValidHeaders()
{
  const char* mathData = hb_blob_get_data(mMathTable, nullptr);
  
  if (!ValidStructure(mathData, sizeof(MATHTableHeader))) {
    return false;
  }
  const MATHTableHeader* header = GetMATHTableHeader();
  if (uint32_t(header->mVersion) != 0x00010000 ||
      !ValidOffset(mathData, uint16_t(header->mMathConstants)) ||
      !ValidOffset(mathData, uint16_t(header->mMathGlyphInfo)) ||
      !ValidOffset(mathData, uint16_t(header->mMathVariants))) {
    return false;
  }

  
  const MathConstants* mathconstants = GetMathConstants();
  const char* start = reinterpret_cast<const char*>(mathconstants);
  if (!ValidStructure(start, sizeof(MathConstants))) {
    return false;
  }

  
  const MathGlyphInfo* mathglyphinfo = GetMathGlyphInfo();
  start = reinterpret_cast<const char*>(mathglyphinfo);
  if (!ValidStructure(start, sizeof(MathGlyphInfo))) {
    return false;
  }

  
  const MathVariants* mathvariants = GetMathVariants();
  start = reinterpret_cast<const char*>(mathvariants);
  if (!ValidStructure(start, sizeof(MathVariants)) ||
      !ValidStructure(start,
                      sizeof(MathVariants) + sizeof(Offset) *
                      (uint16_t(mathvariants->mVertGlyphCount) +
                       uint16_t(mathvariants->mHorizGlyphCount))) ||
      !ValidOffset(start, uint16_t(mathvariants->mVertGlyphCoverage)) ||
      !ValidOffset(start, uint16_t(mathvariants->mHorizGlyphCoverage))) {
    return false;
  }

  return true;
}

int32_t
gfxMathTable::GetMathConstant(gfxFontEntry::MathConstant aConstant)
{
  const MathConstants* mathconstants = GetMathConstants();

  if (aConstant <= gfxFontEntry::ScriptScriptPercentScaleDown) {
    return int16_t(mathconstants->mInt16[aConstant]);
  }

  if (aConstant <= gfxFontEntry::DisplayOperatorMinHeight) {
    return
      uint16_t(mathconstants->
               mUint16[aConstant - gfxFontEntry::DelimitedSubFormulaMinHeight]);
  }

  if (aConstant <= gfxFontEntry::RadicalKernAfterDegree) {
    return int16_t(mathconstants->
                   mMathValues[aConstant - gfxFontEntry::MathLeading].mValue);
  }

  return uint16_t(mathconstants->mRadicalDegreeBottomRaisePercent);
}

bool
gfxMathTable::GetMathItalicsCorrection(uint32_t aGlyphID,
                                       int16_t* aItalicCorrection)
{
  const MathGlyphInfo* mathglyphinfo = GetMathGlyphInfo();

  
  const char* start = reinterpret_cast<const char*>(mathglyphinfo);
  uint16_t offset = mathglyphinfo->mMathItalicsCorrectionInfo;
  if (offset == 0 || !ValidOffset(start, offset)) {
    return false;
  }
  start += offset;

  
  if (!ValidStructure(start, sizeof(MathItalicsCorrectionInfo))) {
    return false;
  }
  const MathItalicsCorrectionInfo* italicsCorrectionInfo =
    reinterpret_cast<const MathItalicsCorrectionInfo*>(start);

  
  offset = italicsCorrectionInfo->mCoverage;
  const Coverage* coverage =
    reinterpret_cast<const Coverage*>(start + offset);
  int32_t i = GetCoverageIndex(coverage, aGlyphID);

  
  uint16_t count = italicsCorrectionInfo->mItalicsCorrectionCount;
  if (i < 0 || i >= count) {
    return false;
  }
  start = reinterpret_cast<const char*>(italicsCorrectionInfo + 1);
  if (!ValidStructure(start, count * sizeof(MathValueRecord))) {
    return false;
  }
  const MathValueRecord* mathValueRecordArray =
    reinterpret_cast<const MathValueRecord*>(start);

  *aItalicCorrection = int16_t(mathValueRecordArray[i].mValue);
  return true;
}

uint32_t
gfxMathTable::GetMathVariantsSize(uint32_t aGlyphID, bool aVertical,
                                  uint16_t aSize)
{
  
  SelectGlyphConstruction(aGlyphID, aVertical);
  if (!mGlyphConstruction) {
    return 0;
  }

  
  
  uint16_t count = mGlyphConstruction->mVariantCount;
  const char* start = reinterpret_cast<const char*>(mGlyphConstruction + 1);
  if (aSize >= count ||
      !ValidStructure(start, count * sizeof(MathGlyphVariantRecord))) {
    return 0;
  }

  
  const MathGlyphVariantRecord* recordArray =
    reinterpret_cast<const MathGlyphVariantRecord*>(start);
  return uint32_t(recordArray[aSize].mVariantGlyph);
}

bool
gfxMathTable::GetMathVariantsParts(uint32_t aGlyphID, bool aVertical,
                                   uint32_t aGlyphs[4])
{
  
  const GlyphAssembly* glyphAssembly = GetGlyphAssembly(aGlyphID, aVertical);
  if (!glyphAssembly) {
    return false;
  }

  
  uint16_t count = glyphAssembly->mPartCount;
  const char* start = reinterpret_cast<const char*>(glyphAssembly + 1);
  if (!ValidStructure(start, count * sizeof(GlyphPartRecord))) {
    return false;
  }
  const GlyphPartRecord* recordArray =
    reinterpret_cast<const GlyphPartRecord*>(start);

  
  
  
  
  
  
  
  
  
  

  
  uint16_t nonExtenderCount = 0;
  for (uint16_t i = 0; i < count; i++) {
    if (!(uint16_t(recordArray[i].mPartFlags) & PART_FLAG_EXTENDER)) {
      nonExtenderCount++;
    }
  }
  if (nonExtenderCount > 3) {
    
    return false;
  }

  

  
  
  
  
  
  
  uint8_t state = 0;

  
  uint32_t extenderChar = 0;

  
  memset(aGlyphs, 0, sizeof(uint32_t) * 4);

  for (uint16_t i = 0; i < count; i++) {

    bool isExtender = uint16_t(recordArray[i].mPartFlags) & PART_FLAG_EXTENDER;
    uint32_t glyph = recordArray[i].mGlyph;

    if ((state == 1 || state == 2) && nonExtenderCount < 3) {
      
      state += 2;
    }

    if (isExtender) {
      if (!extenderChar) {
        extenderChar = glyph;
        aGlyphs[3] = extenderChar;
      } else if (extenderChar != glyph)  {
        
        return false;
      }

      if (state == 0) { 
        
        state = 1;
      } else if (state == 2) { 
        
        state = 3;
      } else if (state >= 4) {
        
        return false;
      }

      continue;
    }

    if (state == 0) {
      
      aGlyphs[mVertical ? 2 : 0] = glyph;
      state = 1;
      continue;
    }

    if (state == 1 || state == 2) {
      
      aGlyphs[1] = glyph;
      state = 3;
      continue;
    }

    if (state == 3 || state == 4) {
      
      aGlyphs[mVertical ? 0 : 2] = glyph;
      state = 5;
    }

  }

  return true;
}

bool
gfxMathTable::ValidStructure(const char* aStart, uint16_t aSize)
{
  unsigned int mathDataLength;
  const char* mathData = hb_blob_get_data(mMathTable, &mathDataLength);
  return (mathData <= aStart &&
          aStart + aSize <= mathData + mathDataLength);
}

bool
gfxMathTable::ValidOffset(const char* aStart, uint16_t aOffset)
{
  unsigned int mathDataLength;
  const char* mathData = hb_blob_get_data(mMathTable, &mathDataLength);
  return (mathData <= aStart + aOffset &&
          aStart + aOffset < mathData + mathDataLength);
}

const MATHTableHeader*
gfxMathTable::GetMATHTableHeader()
{
  const char* mathData = hb_blob_get_data(mMathTable, nullptr);
  return reinterpret_cast<const MATHTableHeader*>(mathData);
}

const MathConstants*
gfxMathTable::GetMathConstants()
{
  const char* mathData = hb_blob_get_data(mMathTable, nullptr);
  return
    reinterpret_cast<const MathConstants*>(mathData +
                                           uint16_t(GetMATHTableHeader()->
                                                    mMathConstants));
}

const MathGlyphInfo*
gfxMathTable::GetMathGlyphInfo()
{
  const char* mathData = hb_blob_get_data(mMathTable, nullptr);
  return
    reinterpret_cast<const MathGlyphInfo*>(mathData +
                                           uint16_t(GetMATHTableHeader()->
                                                    mMathGlyphInfo));
}

const MathVariants*
gfxMathTable::GetMathVariants()
{
  const char* mathData = hb_blob_get_data(mMathTable, nullptr);
  return
    reinterpret_cast<const MathVariants*>(mathData +
                                          uint16_t(GetMATHTableHeader()->
                                                   mMathVariants));
}

const GlyphAssembly*
gfxMathTable::GetGlyphAssembly(uint32_t aGlyphID, bool aVertical)
{
  
  SelectGlyphConstruction(aGlyphID, aVertical);
  if (!mGlyphConstruction) {
    return nullptr;
  }

  
  const char* start = reinterpret_cast<const char*>(mGlyphConstruction);
  uint16_t offset = mGlyphConstruction->mGlyphAssembly;
  if (offset == 0 || !ValidOffset(start, offset)) {
    return nullptr;
  }
  start += offset;

  
  if (!ValidStructure(start, sizeof(GlyphAssembly))) {
    return nullptr;
  }
  return reinterpret_cast<const GlyphAssembly*>(start);
}

namespace {

struct GlyphArrayWrapper
{
  const GlyphID* const mGlyphArray;
  GlyphArrayWrapper(const GlyphID* const aGlyphArray) : mGlyphArray(aGlyphArray)
  {}
  uint16_t operator[](size_t index) const {
    return mGlyphArray[index];
  }
};

struct RangeRecordComparator
{
  const uint32_t mGlyph;
  RangeRecordComparator(uint32_t aGlyph) : mGlyph(aGlyph) {}
  int operator()(const RangeRecord& aRecord) const {
    if (mGlyph < static_cast<uint16_t>(aRecord.mStart)) {
      return -1;
    }
    if (mGlyph > static_cast<uint16_t>(aRecord.mEnd)) {
      return 1;
    }
    return 0;
  }
};

} 

int32_t
gfxMathTable::GetCoverageIndex(const Coverage* aCoverage, uint32_t aGlyph)
{
  using mozilla::BinarySearch;
  using mozilla::BinarySearchIf;

  if (uint16_t(aCoverage->mFormat) == 1) {
    
    const CoverageFormat1* table =
      reinterpret_cast<const CoverageFormat1*>(aCoverage);
    const uint16_t count = table->mGlyphCount;
    const char* start = reinterpret_cast<const char*>(table + 1);
    if (ValidStructure(start, count * sizeof(GlyphID))) {
      const GlyphID* glyphArray = reinterpret_cast<const GlyphID*>(start);
      size_t index;

      if (BinarySearch(GlyphArrayWrapper(glyphArray), 0, count, aGlyph, &index)) {
        return index;
      }
    }
  } else if (uint16_t(aCoverage->mFormat) == 2) {
    
    const CoverageFormat2* table =
      reinterpret_cast<const CoverageFormat2*>(aCoverage);
    const uint16_t count = table->mRangeCount;
    const char* start = reinterpret_cast<const char*>(table + 1);
    if (ValidStructure(start, count * sizeof(RangeRecord))) {
      const RangeRecord* rangeArray = reinterpret_cast<const RangeRecord*>(start);
      size_t index;

      if (BinarySearchIf(rangeArray, 0, count, RangeRecordComparator(aGlyph),
                         &index)) {
        uint16_t rStart = rangeArray[index].mStart;
        uint16_t startCoverageIndex = rangeArray[index].mStartCoverageIndex;
        return (startCoverageIndex + aGlyph - rStart);
      }
    }
  }
  return -1;
}

void
gfxMathTable::SelectGlyphConstruction(uint32_t aGlyphID, bool aVertical)
{
  if (mGlyphID == aGlyphID && mVertical == aVertical) {
    
    return;
  }

  
  mVertical = aVertical;
  mGlyphID = aGlyphID;
  mGlyphConstruction = nullptr;

  
  const MathVariants* mathvariants = GetMathVariants();
  const char* start = reinterpret_cast<const char*>(mathvariants);
  uint16_t offset = (aVertical ?
                     mathvariants->mVertGlyphCoverage :
                     mathvariants->mHorizGlyphCoverage);
  const Coverage* coverage =
    reinterpret_cast<const Coverage*>(start + offset);
  int32_t i = GetCoverageIndex(coverage, aGlyphID);

  
  uint16_t count = (aVertical ?
                    mathvariants->mVertGlyphCount :
                    mathvariants->mHorizGlyphCount);
  start = reinterpret_cast<const char*>(mathvariants + 1);
  if (i < 0 || i >= count) {
    return;
  }
  if (!aVertical) {
    start += uint16_t(mathvariants->mVertGlyphCount) * sizeof(Offset);
  }
  if (!ValidStructure(start, count * sizeof(Offset))) {
    return;
  }
  const Offset* offsetArray = reinterpret_cast<const Offset*>(start);
  offset = uint16_t(offsetArray[i]);

  
  start = reinterpret_cast<const char*>(mathvariants);
  if (!ValidStructure(start + offset, sizeof(MathGlyphConstruction))) {
    return;
  }
  mGlyphConstruction =
    reinterpret_cast<const MathGlyphConstruction*>(start + offset);
}
