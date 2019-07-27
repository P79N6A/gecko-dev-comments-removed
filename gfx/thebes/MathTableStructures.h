










#ifndef MATH_TABLE_STRUCTURE_H
#define MATH_TABLE_STRUCTURE_H

#include "gfxFontUtils.h"

typedef mozilla::AutoSwap_PRUint16 Count16;
typedef mozilla::AutoSwap_PRUint16 GlyphID;
typedef mozilla::AutoSwap_PRUint16 Offset;

struct MathValueRecord {
  mozilla::AutoSwap_PRInt16  mValue;
  Offset                     mDeviceTable;
};

struct RangeRecord {
  GlyphID                    mStart;
  GlyphID                    mEnd;
  mozilla::AutoSwap_PRUint16 mStartCoverageIndex;
};

struct Coverage {
  mozilla::AutoSwap_PRUint16 mFormat;
};

struct CoverageFormat1 {
  mozilla::AutoSwap_PRUint16 mFormat;
  Count16                    mGlyphCount;
  
};

struct CoverageFormat2 {
  mozilla::AutoSwap_PRUint16 mFormat;
  Count16                    mRangeCount;
  
};

struct MATHTableHeader {
  mozilla::AutoSwap_PRUint32 mVersion;
  Offset                     mMathConstants;
  Offset                     mMathGlyphInfo;
  Offset                     mMathVariants;
};

struct MathConstants {
  mozilla::AutoSwap_PRInt16  mInt16[gfxFontEntry::ScriptScriptPercentScaleDown -
                                    gfxFontEntry::ScriptPercentScaleDown + 1];
  mozilla::AutoSwap_PRUint16 mUint16[gfxFontEntry::DisplayOperatorMinHeight -
                                     gfxFontEntry::
                                     DelimitedSubFormulaMinHeight + 1];
  MathValueRecord            mMathValues[gfxFontEntry::RadicalKernAfterDegree -
                                         gfxFontEntry::MathLeading + 1];
  mozilla::AutoSwap_PRUint16 mRadicalDegreeBottomRaisePercent;
};

struct MathGlyphInfo {
  Offset mMathItalicsCorrectionInfo;
  Offset mMathTopAccentAttachment;
  Offset mExtendedShapeCoverage;
  Offset mMathKernInfo;
};

struct MathItalicsCorrectionInfo {
  Offset  mCoverage;
  Count16 mItalicsCorrectionCount;
  
};

struct MathVariants {
  mozilla::AutoSwap_PRUint16 mMinConnectorOverlap;
  Offset                     mVertGlyphCoverage;
  Offset                     mHorizGlyphCoverage;
  Count16                    mVertGlyphCount;
  Count16                    mHorizGlyphCount;
  
  
};

struct MathGlyphVariantRecord {
  GlyphID                    mVariantGlyph;
  mozilla::AutoSwap_PRUint16 mAdvanceMeasurement;
};

struct MathGlyphConstruction {
  Offset                    mGlyphAssembly;
  Count16                   mVariantCount;
  
};

struct GlyphPartRecord {
  GlyphID                    mGlyph;
  mozilla::AutoSwap_PRUint16 mStartConnectorLength;
  mozilla::AutoSwap_PRUint16 mEndConnectorLength;
  mozilla::AutoSwap_PRUint16 mFullAdvance;
  mozilla::AutoSwap_PRUint16 mPartFlags;
};




enum {
  PART_FLAG_EXTENDER = 0x01
};

struct GlyphAssembly {
  MathValueRecord    mItalicsCorrection;
  Count16            mPartCount;
  
};

#endif
