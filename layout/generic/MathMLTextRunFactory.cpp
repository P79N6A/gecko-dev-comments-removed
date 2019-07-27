




#include "MathMLTextRunFactory.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/BinarySearch.h"

#include "nsStyleConsts.h"
#include "nsStyleContext.h"
#include "nsTextFrameUtils.h"
#include "nsFontMetrics.h"
#include "nsDeviceContext.h"

using namespace mozilla;






typedef struct
{
  uint32_t mKey;
  uint32_t mReplacement;
} MathVarMapping;












static const MathVarMapping gArabicInitialMapTable[] = {
  { 0x628, 0x1EE21 },
  { 0x62A, 0x1EE35 },
  { 0x62B, 0x1EE36 },
  { 0x62C, 0x1EE22 },
  { 0x62D, 0x1EE27 },
  { 0x62E, 0x1EE37 },
  { 0x633, 0x1EE2E },
  { 0x634, 0x1EE34 },
  { 0x635, 0x1EE31 },
  { 0x636, 0x1EE39 },
  { 0x639, 0x1EE2F },
  { 0x63A, 0x1EE3B },
  { 0x641, 0x1EE30 },
  { 0x642, 0x1EE32 },
  { 0x643, 0x1EE2A },
  { 0x644, 0x1EE2B },
  { 0x645, 0x1EE2C },
  { 0x646, 0x1EE2D },
  { 0x647, 0x1EE24 },
  { 0x64A, 0x1EE29 }
};

static const MathVarMapping gArabicTailedMapTable[] = {
  { 0x62C, 0x1EE42 },
  { 0x62D, 0x1EE47 },
  { 0x62E, 0x1EE57 },
  { 0x633, 0x1EE4E },
  { 0x634, 0x1EE54 },
  { 0x635, 0x1EE51 },
  { 0x636, 0x1EE59 },
  { 0x639, 0x1EE4F },
  { 0x63A, 0x1EE5B },
  { 0x642, 0x1EE52 },
  { 0x644, 0x1EE4B },
  { 0x646, 0x1EE4D },
  { 0x64A, 0x1EE49 },
  { 0x66F, 0x1EE5F },
  { 0x6BA, 0x1EE5D }
};

static const MathVarMapping gArabicStretchedMapTable[] = {
  { 0x628, 0x1EE61 },
  { 0x62A, 0x1EE75 },
  { 0x62B, 0x1EE76 },
  { 0x62C, 0x1EE62 },
  { 0x62D, 0x1EE67 },
  { 0x62E, 0x1EE77 },
  { 0x633, 0x1EE6E },
  { 0x634, 0x1EE74 },
  { 0x635, 0x1EE71 },
  { 0x636, 0x1EE79 },
  { 0x637, 0x1EE68 },
  { 0x638, 0x1EE7A },
  { 0x639, 0x1EE6F },
  { 0x63A, 0x1EE7B },
  { 0x641, 0x1EE70 },
  { 0x642, 0x1EE72 },
  { 0x643, 0x1EE6A },
  { 0x645, 0x1EE6C },
  { 0x646, 0x1EE6D },
  { 0x647, 0x1EE64 },
  { 0x64A, 0x1EE69 },
  { 0x66E, 0x1EE7C },
  { 0x6A1, 0x1EE7E }
};

static const MathVarMapping gArabicLoopedMapTable[] = {
  { 0x627, 0x1EE80 },
  { 0x628, 0x1EE81 },
  { 0x62A, 0x1EE95 },
  { 0x62B, 0x1EE96 },
  { 0x62C, 0x1EE82 },
  { 0x62D, 0x1EE87 },
  { 0x62E, 0x1EE97 },
  { 0x62F, 0x1EE83 },
  { 0x630, 0x1EE98 },
  { 0x631, 0x1EE93 },
  { 0x632, 0x1EE86 },
  { 0x633, 0x1EE8E },
  { 0x634, 0x1EE94 },
  { 0x635, 0x1EE91 },
  { 0x636, 0x1EE99 },
  { 0x637, 0x1EE88 },
  { 0x638, 0x1EE9A },
  { 0x639, 0x1EE8F },
  { 0x63A, 0x1EE9B },
  { 0x641, 0x1EE90 },
  { 0x642, 0x1EE92 },
  { 0x644, 0x1EE8B },
  { 0x645, 0x1EE8C },
  { 0x646, 0x1EE8D },
  { 0x647, 0x1EE84 },
  { 0x648, 0x1EE85 },
  { 0x64A, 0x1EE89 }
};

static const MathVarMapping gArabicDoubleMapTable[] = {
  { 0x628, 0x1EEA1 },
  { 0x62A, 0x1EEB5 },
  { 0x62B, 0x1EEB6 },
  { 0x62C, 0x1EEA2 },
  { 0x62D, 0x1EEA7 },
  { 0x62E, 0x1EEB7 },
  { 0x62F, 0x1EEA3 },
  { 0x630, 0x1EEB8 },
  { 0x631, 0x1EEB3 },
  { 0x632, 0x1EEA6 },
  { 0x633, 0x1EEAE },
  { 0x634, 0x1EEB4 },
  { 0x635, 0x1EEB1 },
  { 0x636, 0x1EEB9 },
  { 0x637, 0x1EEA8 },
  { 0x638, 0x1EEBA },
  { 0x639, 0x1EEAF },
  { 0x63A, 0x1EEBB },
  { 0x641, 0x1EEB0 },
  { 0x642, 0x1EEB2 },
  { 0x644, 0x1EEAB },
  { 0x645, 0x1EEAC },
  { 0x646, 0x1EEAD },
  { 0x648, 0x1EEA5 },
  { 0x64A, 0x1EEA9 }
};

static const MathVarMapping gLatinExceptionMapTable[] = {
  { 0x1D455, 0x210E },
  { 0x1D49D, 0x212C },
  { 0x1D4A0, 0x2130 },
  { 0x1D4A1, 0x2131 },
  { 0x1D4A3, 0x210B },
  { 0x1D4A4, 0x2110 },
  { 0x1D4A7, 0x2112 },
  { 0x1D4A8, 0x2133 },
  { 0x1D4AD, 0x211B },
  { 0x1D4BA, 0x212F },
  { 0x1D4BC, 0x210A },
  { 0x1D4C4, 0x2134 },
  { 0x1D506, 0x212D },
  { 0x1D50B, 0x210C },
  { 0x1D50C, 0x2111 },
  { 0x1D515, 0x211C },
  { 0x1D51D, 0x2128 },
  { 0x1D53A, 0x2102 },
  { 0x1D53F, 0x210D },
  { 0x1D545, 0x2115 },
  { 0x1D547, 0x2119 },
  { 0x1D548, 0x211A },
  { 0x1D549, 0x211D },
  { 0x1D551, 0x2124 }
};

namespace {

struct MathVarMappingWrapper
{
  const MathVarMapping* const mTable;
  MathVarMappingWrapper(const MathVarMapping* aTable) : mTable(aTable) {}
  uint32_t operator[](size_t index) const {
    return mTable[index].mKey;
  }
};

} 



static uint32_t
MathvarMappingSearch(uint32_t aKey, const MathVarMapping* aTable, uint32_t aNumElements)
{
  size_t index;
  if (BinarySearch(MathVarMappingWrapper(aTable), 0, aNumElements, aKey, &index)) {
    return aTable[index].mReplacement;
  }

  return 0;
}

#define GREEK_UPPER_THETA               0x03F4
#define HOLE_GREEK_UPPER_THETA          0x03A2
#define NABLA                           0x2207
#define PARTIAL_DIFFERENTIAL            0x2202
#define GREEK_UPPER_ALPHA               0x0391
#define GREEK_UPPER_OMEGA               0x03A9
#define GREEK_LOWER_ALPHA               0x03B1
#define GREEK_LOWER_OMEGA               0x03C9
#define GREEK_LUNATE_EPSILON_SYMBOL     0x03F5
#define GREEK_THETA_SYMBOL              0x03D1
#define GREEK_KAPPA_SYMBOL              0x03F0
#define GREEK_PHI_SYMBOL                0x03D5
#define GREEK_RHO_SYMBOL                0x03F1
#define GREEK_PI_SYMBOL                 0x03D6
#define GREEK_LETTER_DIGAMMA            0x03DC
#define GREEK_SMALL_LETTER_DIGAMMA      0x03DD
#define MATH_BOLD_CAPITAL_DIGAMMA       0x1D7CA
#define MATH_BOLD_SMALL_DIGAMMA         0x1D7CB

#define LATIN_SMALL_LETTER_DOTLESS_I    0x0131
#define LATIN_SMALL_LETTER_DOTLESS_J    0x0237

#define MATH_ITALIC_SMALL_DOTLESS_I     0x1D6A4
#define MATH_ITALIC_SMALL_DOTLESS_J     0x1D6A5

#define MATH_BOLD_UPPER_A               0x1D400
#define MATH_ITALIC_UPPER_A             0x1D434
#define MATH_BOLD_SMALL_A               0x1D41A
#define MATH_BOLD_UPPER_ALPHA           0x1D6A8
#define MATH_BOLD_SMALL_ALPHA           0x1D6C2
#define MATH_ITALIC_UPPER_ALPHA         0x1D6E2
#define MATH_BOLD_DIGIT_ZERO            0x1D7CE
#define MATH_DOUBLE_STRUCK_ZERO         0x1D7D8

#define MATH_BOLD_UPPER_THETA           0x1D6B9
#define MATH_BOLD_NABLA                 0x1D6C1
#define MATH_BOLD_PARTIAL_DIFFERENTIAL  0x1D6DB
#define MATH_BOLD_EPSILON_SYMBOL        0x1D6DC
#define MATH_BOLD_THETA_SYMBOL          0x1D6DD
#define MATH_BOLD_KAPPA_SYMBOL          0x1D6DE
#define MATH_BOLD_PHI_SYMBOL            0x1D6DF
#define MATH_BOLD_RHO_SYMBOL            0x1D6E0
#define MATH_BOLD_PI_SYMBOL             0x1D6E1















static uint32_t
MathVariant(uint32_t aCh, uint8_t aMathVar)
{
  uint32_t baseChar;
  enum CharacterType {
    kIsLatin,
    kIsGreekish,
    kIsNumber,
    kIsArabic,
  };
  CharacterType varType;

  int8_t multiplier;

  if (aMathVar <= NS_MATHML_MATHVARIANT_NORMAL) {
    
    return aCh;
  }
  if (aMathVar > NS_MATHML_MATHVARIANT_STRETCHED) {
    NS_ASSERTION(false, "Illegal mathvariant value");
    return aCh;
  }

  
  if (aCh == HOLE_GREEK_UPPER_THETA) {
    
    return aCh;
  }
  if (aCh == GREEK_LETTER_DIGAMMA) {
    if (aMathVar == NS_MATHML_MATHVARIANT_BOLD) {
      return MATH_BOLD_CAPITAL_DIGAMMA;
    }
    return aCh;
  }
  if (aCh == GREEK_SMALL_LETTER_DIGAMMA) {
    if (aMathVar == NS_MATHML_MATHVARIANT_BOLD) {
      return MATH_BOLD_SMALL_DIGAMMA;
    }
    return aCh;
  }
  if (aCh == LATIN_SMALL_LETTER_DOTLESS_I) {
    if (aMathVar == NS_MATHML_MATHVARIANT_ITALIC) {
      return MATH_ITALIC_SMALL_DOTLESS_I;
    }
    return aCh;
  }
  if (aCh == LATIN_SMALL_LETTER_DOTLESS_J) {
    if (aMathVar == NS_MATHML_MATHVARIANT_ITALIC) {
      return MATH_ITALIC_SMALL_DOTLESS_J;
    }
    return aCh;
  }

  
  
  
  
  
  
  if ('A' <= aCh && aCh <= 'Z') {
    baseChar = aCh - 'A';
    varType = kIsLatin;
  } else if ('a' <= aCh && aCh <= 'z') {
    
    
    
    
    baseChar = MATH_BOLD_SMALL_A-MATH_BOLD_UPPER_A + aCh - 'a';
    varType = kIsLatin;
  } else if ('0' <= aCh && aCh <= '9') {
    baseChar = aCh - '0';
    varType = kIsNumber;
  } else if (GREEK_UPPER_ALPHA <= aCh && aCh <= GREEK_UPPER_OMEGA) {
    baseChar = aCh-GREEK_UPPER_ALPHA;
    varType = kIsGreekish;
  } else if (GREEK_LOWER_ALPHA <= aCh && aCh <= GREEK_LOWER_OMEGA) {
    
    
    
    
    baseChar =  MATH_BOLD_SMALL_ALPHA - MATH_BOLD_UPPER_ALPHA
                + aCh-GREEK_LOWER_ALPHA;
    varType = kIsGreekish;
  } else if (0x0600 <= aCh && aCh <= 0x06FF) {
    
    varType = kIsArabic;
  } else {
    switch (aCh) {
      case GREEK_UPPER_THETA:
        baseChar = MATH_BOLD_UPPER_THETA-MATH_BOLD_UPPER_ALPHA;
        break;
      case NABLA:
        baseChar = MATH_BOLD_NABLA-MATH_BOLD_UPPER_ALPHA;
        break;
      case PARTIAL_DIFFERENTIAL:
        baseChar = MATH_BOLD_PARTIAL_DIFFERENTIAL - MATH_BOLD_UPPER_ALPHA;
        break;
      case GREEK_LUNATE_EPSILON_SYMBOL:
        baseChar = MATH_BOLD_EPSILON_SYMBOL - MATH_BOLD_UPPER_ALPHA;
        break;
      case GREEK_THETA_SYMBOL:
        baseChar = MATH_BOLD_THETA_SYMBOL - MATH_BOLD_UPPER_ALPHA;
        break;
      case GREEK_KAPPA_SYMBOL:
        baseChar = MATH_BOLD_KAPPA_SYMBOL - MATH_BOLD_UPPER_ALPHA;
        break;
      case GREEK_PHI_SYMBOL:
        baseChar = MATH_BOLD_PHI_SYMBOL - MATH_BOLD_UPPER_ALPHA;
        break;
      case GREEK_RHO_SYMBOL:
        baseChar = MATH_BOLD_RHO_SYMBOL - MATH_BOLD_UPPER_ALPHA;
        break;
      case GREEK_PI_SYMBOL:
        baseChar = MATH_BOLD_PI_SYMBOL - MATH_BOLD_UPPER_ALPHA;
        break;
      default:
        return aCh;
    }

    varType = kIsGreekish;
  }

  if (varType == kIsNumber) {
    switch (aMathVar) {
      
      
      
      
      
      case NS_MATHML_MATHVARIANT_BOLD:
        multiplier = 0;
        break;
      case NS_MATHML_MATHVARIANT_DOUBLE_STRUCK:
        multiplier = 1;
        break;
      case NS_MATHML_MATHVARIANT_SANS_SERIF:
        multiplier = 2;
        break;
      case NS_MATHML_MATHVARIANT_BOLD_SANS_SERIF:
        multiplier = 3;
        break;
      case NS_MATHML_MATHVARIANT_MONOSPACE:
        multiplier = 4;
        break;
      default:
        
        return aCh;
    }
    
    
    
    
    
    
    
    return baseChar+multiplier*(MATH_DOUBLE_STRUCK_ZERO-MATH_BOLD_DIGIT_ZERO)
             +MATH_BOLD_DIGIT_ZERO;
  } else if (varType == kIsGreekish) {
    switch (aMathVar) {
      case NS_MATHML_MATHVARIANT_BOLD:
        multiplier = 0;
        break;
      case NS_MATHML_MATHVARIANT_ITALIC:
        multiplier = 1;
        break;
      case NS_MATHML_MATHVARIANT_BOLD_ITALIC:
        multiplier = 2;
        break;
      case NS_MATHML_MATHVARIANT_BOLD_SANS_SERIF:
        multiplier = 3;
        break;
      case NS_MATHML_MATHVARIANT_SANS_SERIF_BOLD_ITALIC:
        multiplier = 4;
        break;
      default:
        
        return aCh;
    }
    
    return baseChar + MATH_BOLD_UPPER_ALPHA +
             multiplier*(MATH_ITALIC_UPPER_ALPHA - MATH_BOLD_UPPER_ALPHA);
  }

  uint32_t tempChar;
  uint32_t newChar;
  if (varType == kIsArabic) {
    const MathVarMapping* mapTable;
    uint32_t tableLength;
    switch (aMathVar) {
      



      case NS_MATHML_MATHVARIANT_INITIAL:
        mapTable = gArabicInitialMapTable;
        tableLength = ArrayLength(gArabicInitialMapTable);
        break;
      case NS_MATHML_MATHVARIANT_TAILED:
        mapTable = gArabicTailedMapTable;
        tableLength = ArrayLength(gArabicTailedMapTable);
        break;
      case NS_MATHML_MATHVARIANT_STRETCHED:
        mapTable = gArabicStretchedMapTable;
        tableLength = ArrayLength(gArabicStretchedMapTable);
        break;
      case NS_MATHML_MATHVARIANT_LOOPED:
        mapTable = gArabicLoopedMapTable;
        tableLength = ArrayLength(gArabicLoopedMapTable);
        break;
      case NS_MATHML_MATHVARIANT_DOUBLE_STRUCK:
        mapTable = gArabicDoubleMapTable;
        tableLength = ArrayLength(gArabicDoubleMapTable);
        break;
      default:
        
        return aCh;
    }
    newChar = MathvarMappingSearch(aCh, mapTable, tableLength);
  } else {
    
    if (aMathVar > NS_MATHML_MATHVARIANT_MONOSPACE) {
      
      return aCh;
    }
    multiplier = aMathVar - 2;
    
    
    
    
    
    tempChar =  baseChar + MATH_BOLD_UPPER_A +
                multiplier*(MATH_ITALIC_UPPER_A - MATH_BOLD_UPPER_A);
    
    
    
    newChar = MathvarMappingSearch(tempChar, gLatinExceptionMapTable,
                                   ArrayLength(gLatinExceptionMapTable));
  }

  if (newChar) {
    return newChar;
  } else if (varType == kIsLatin) {
    return tempChar;
  } else {
    
    return aCh;
  }

}

void
MathMLTextRunFactory::RebuildTextRun(nsTransformedTextRun* aTextRun,
                                     gfxContext* aRefContext)
{
  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();

  nsAutoString convertedString;
  nsAutoTArray<bool,50> charsToMergeArray;
  nsAutoTArray<bool,50> deletedCharsArray;
  nsAutoTArray<nsStyleContext*,50> styleArray;
  nsAutoTArray<uint8_t,50> canBreakBeforeArray;
  bool mergeNeeded = false;

  bool singleCharMI =
    aTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SINGLE_CHAR_MI;

  uint32_t length = aTextRun->GetLength();
  const char16_t* str = aTextRun->mString.BeginReading();
  nsRefPtr<nsStyleContext>* styles = aTextRun->mStyles.Elements();
  nsFont font;
  if (length) {
    font = styles[0]->StyleFont()->mFont;

    if (mSSTYScriptLevel) {
      bool found = false;
      
      for (uint32_t i = 0; i < font.fontFeatureSettings.Length(); i++) {
        if (font.fontFeatureSettings[i].mTag == TRUETYPE_TAG('s', 's', 't', 'y')) {
          found = true;
          break;
        }
      }
      if (!found) {
        uint8_t sstyLevel = 0;
        float scriptScaling = pow(styles[0]->StyleFont()->mScriptSizeMultiplier,
                                  mSSTYScriptLevel);
        static_assert(NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER < 1,
                      "Shouldn't it make things smaller?");
        
















        if (scriptScaling <= (NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER +
                              (NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER *
                               NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER))/2) {
          
          
          sstyLevel = 2;
        } else if (scriptScaling <= NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER) {
          sstyLevel = 1;
        }
        if (sstyLevel) {
          gfxFontFeature settingSSTY;
          settingSSTY.mTag = TRUETYPE_TAG('s','s','t','y');
          settingSSTY.mValue = sstyLevel;
          font.fontFeatureSettings.AppendElement(settingSSTY);
        }
      }
    }
  }

  uint8_t mathVar = NS_MATHML_MATHVARIANT_NONE;
  bool doMathvariantStyling = true;

  for (uint32_t i = 0; i < length; ++i) {
    int extraChars = 0;
    nsStyleContext* styleContext = styles[i];
    mathVar = styleContext->StyleFont()->mMathVariant;

    if (singleCharMI && mathVar == NS_MATHML_MATHVARIANT_NONE) {
      
      
      
      
      if (mFlags & MATH_FONT_WEIGHT_BOLD) {
        font.weight = NS_FONT_WEIGHT_BOLD;
        if (mFlags & MATH_FONT_STYLING_NORMAL) {
          font.style = NS_FONT_STYLE_NORMAL;
        } else {
          font.style = NS_FONT_STYLE_ITALIC;
        }
      } else if (mFlags & MATH_FONT_STYLING_NORMAL) {
        font.style = NS_FONT_STYLE_NORMAL;
        font.weight = NS_FONT_WEIGHT_NORMAL;
      } else {
        mathVar = NS_MATHML_MATHVARIANT_ITALIC;
      }
    }

    uint32_t ch = str[i];
    if (NS_IS_HIGH_SURROGATE(ch) && i < length - 1 &&
        NS_IS_LOW_SURROGATE(str[i + 1])) {
      ch = SURROGATE_TO_UCS4(ch, str[i + 1]);
    }
    uint32_t ch2 = MathVariant(ch, mathVar);

    if (mathVar == NS_MATHML_MATHVARIANT_BOLD ||
        mathVar == NS_MATHML_MATHVARIANT_BOLD_ITALIC ||
        mathVar == NS_MATHML_MATHVARIANT_ITALIC) {
      if (ch == ch2  && ch != 0x20 && ch != 0xA0) {
        
        
        
        doMathvariantStyling = false;
      }
      if (ch2 != ch) {
        
        
        
        uint8_t matchType;
        nsRefPtr<gfxFont> mathFont = fontGroup->
          FindFontForChar(ch2, 0, HB_SCRIPT_COMMON, nullptr, &matchType);
        if (mathFont) {
          
          
          doMathvariantStyling = false;
        } else {
          
          ch2 = ch;
        }
      }
    }

    deletedCharsArray.AppendElement(false);
    charsToMergeArray.AppendElement(false);
    styleArray.AppendElement(styleContext);
    canBreakBeforeArray.AppendElement(aTextRun->CanBreakLineBefore(i));

    if (IS_IN_BMP(ch2)) {
      convertedString.Append(ch2);
    } else {
      convertedString.Append(H_SURROGATE(ch2));
      convertedString.Append(L_SURROGATE(ch2));
      ++extraChars;
      if (!IS_IN_BMP(ch)) {
        deletedCharsArray.AppendElement(true); 
                                          
        ++i;
      }
    }

    while (extraChars-- > 0) {
      mergeNeeded = true;
      charsToMergeArray.AppendElement(true);
      styleArray.AppendElement(styleContext);
      canBreakBeforeArray.AppendElement(false);
    }
  }

  uint32_t flags;
  gfxTextRunFactory::Parameters innerParams =
      GetParametersForInner(aTextRun, &flags, aRefContext);

  nsAutoPtr<nsTransformedTextRun> transformedChild;
  nsAutoPtr<gfxTextRun> cachedChild;
  gfxTextRun* child;

  if (mathVar == NS_MATHML_MATHVARIANT_BOLD && doMathvariantStyling) {
    font.style = NS_FONT_STYLE_NORMAL;
    font.weight = NS_FONT_WEIGHT_BOLD;
  } else if (mathVar == NS_MATHML_MATHVARIANT_ITALIC && doMathvariantStyling) {
    font.style = NS_FONT_STYLE_ITALIC;
    font.weight = NS_FONT_WEIGHT_NORMAL;
  } else if (mathVar == NS_MATHML_MATHVARIANT_BOLD_ITALIC &&
             doMathvariantStyling) {
    font.style = NS_FONT_STYLE_ITALIC;
    font.weight = NS_FONT_WEIGHT_BOLD;
  } else if (mathVar != NS_MATHML_MATHVARIANT_NONE) {
    
    
    
    font.style = NS_FONT_STYLE_NORMAL;
    font.weight = NS_FONT_WEIGHT_NORMAL;
  }
  gfxFontGroup* newFontGroup = nullptr;

  
  if (length) {
    nsPresContext* pc = styles[0]->PresContext();
    nsRefPtr<nsFontMetrics> metrics;
    pc->DeviceContext()->GetMetricsFor(font,
                                       styles[0]->StyleFont()->mLanguage,
                                       pc->GetUserFontSet(),
                                       pc->GetTextPerfMetrics(),
                                       *getter_AddRefs(metrics));
    if (metrics) {
      newFontGroup = metrics->GetThebesFontGroup();
    }
  }

  if (!newFontGroup) {
    
    
    newFontGroup = fontGroup;
  }

  if (mInnerTransformingTextRunFactory) {
    transformedChild = mInnerTransformingTextRunFactory->MakeTextRun(
        convertedString.BeginReading(), convertedString.Length(),
        &innerParams, newFontGroup, flags, styleArray.Elements(), false);
    child = transformedChild.get();
  } else {
    cachedChild = newFontGroup->MakeTextRun(
        convertedString.BeginReading(), convertedString.Length(),
        &innerParams, flags);
    child = cachedChild.get();
  }
  if (!child)
    return;
  
  
  NS_ASSERTION(convertedString.Length() == canBreakBeforeArray.Length(),
               "Dropped characters or break-before values somewhere!");
  child->SetPotentialLineBreaks(0, canBreakBeforeArray.Length(),
      canBreakBeforeArray.Elements(), aRefContext);
  if (transformedChild) {
    transformedChild->FinishSettingProperties(aRefContext);
  }

  if (mergeNeeded) {
    
    NS_ASSERTION(charsToMergeArray.Length() == child->GetLength(),
                 "source length mismatch");
    NS_ASSERTION(deletedCharsArray.Length() == aTextRun->GetLength(),
                 "destination length mismatch");
    MergeCharactersInTextRun(aTextRun, child, charsToMergeArray.Elements(),
                             deletedCharsArray.Elements());
  } else {
    
    
    
    aTextRun->ResetGlyphRuns();
    aTextRun->CopyGlyphDataFrom(child, 0, child->GetLength(), 0);
  }
}
