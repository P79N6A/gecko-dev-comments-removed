




#include "nsTextRunTransformations.h"

#include "mozilla/MemoryReporting.h"

#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsStyleContext.h"
#include "nsUnicodeProperties.h"
#include "nsSpecialCasingData.h"
#include "mozilla/gfx/2D.h"
#include "nsTextFrameUtils.h"
#include "nsIPersistentProperties2.h"
#include "nsNetUtil.h"


#define LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE  0x0130
#define LATIN_SMALL_LETTER_DOTLESS_I           0x0131




#define GREEK_CAPITAL_LETTER_SIGMA             0x03A3
#define GREEK_SMALL_LETTER_FINAL_SIGMA         0x03C2
#define GREEK_SMALL_LETTER_SIGMA               0x03C3


#define GREEK_LOWER_ALPHA                      0x03B1
#define GREEK_LOWER_ALPHA_TONOS                0x03AC
#define GREEK_LOWER_ALPHA_OXIA                 0x1F71
#define GREEK_LOWER_EPSILON                    0x03B5
#define GREEK_LOWER_EPSILON_TONOS              0x03AD
#define GREEK_LOWER_EPSILON_OXIA               0x1F73
#define GREEK_LOWER_ETA                        0x03B7
#define GREEK_LOWER_ETA_TONOS                  0x03AE
#define GREEK_LOWER_ETA_OXIA                   0x1F75
#define GREEK_LOWER_IOTA                       0x03B9
#define GREEK_LOWER_IOTA_TONOS                 0x03AF
#define GREEK_LOWER_IOTA_OXIA                  0x1F77
#define GREEK_LOWER_IOTA_DIALYTIKA             0x03CA
#define GREEK_LOWER_IOTA_DIALYTIKA_TONOS       0x0390
#define GREEK_LOWER_IOTA_DIALYTIKA_OXIA        0x1FD3
#define GREEK_LOWER_OMICRON                    0x03BF
#define GREEK_LOWER_OMICRON_TONOS              0x03CC
#define GREEK_LOWER_OMICRON_OXIA               0x1F79
#define GREEK_LOWER_UPSILON                    0x03C5
#define GREEK_LOWER_UPSILON_TONOS              0x03CD
#define GREEK_LOWER_UPSILON_OXIA               0x1F7B
#define GREEK_LOWER_UPSILON_DIALYTIKA          0x03CB
#define GREEK_LOWER_UPSILON_DIALYTIKA_TONOS    0x03B0
#define GREEK_LOWER_UPSILON_DIALYTIKA_OXIA     0x1FE3
#define GREEK_LOWER_OMEGA                      0x03C9
#define GREEK_LOWER_OMEGA_TONOS                0x03CE
#define GREEK_LOWER_OMEGA_OXIA                 0x1F7D
#define GREEK_UPPER_ALPHA                      0x0391
#define GREEK_UPPER_EPSILON                    0x0395
#define GREEK_UPPER_ETA                        0x0397
#define GREEK_UPPER_IOTA                       0x0399
#define GREEK_UPPER_IOTA_DIALYTIKA             0x03AA
#define GREEK_UPPER_OMICRON                    0x039F
#define GREEK_UPPER_UPSILON                    0x03A5
#define GREEK_UPPER_UPSILON_DIALYTIKA          0x03AB
#define GREEK_UPPER_OMEGA                      0x03A9
#define GREEK_UPPER_ALPHA_TONOS                0x0386
#define GREEK_UPPER_ALPHA_OXIA                 0x1FBB
#define GREEK_UPPER_EPSILON_TONOS              0x0388
#define GREEK_UPPER_EPSILON_OXIA               0x1FC9
#define GREEK_UPPER_ETA_TONOS                  0x0389
#define GREEK_UPPER_ETA_OXIA                   0x1FCB
#define GREEK_UPPER_IOTA_TONOS                 0x038A
#define GREEK_UPPER_IOTA_OXIA                  0x1FDB
#define GREEK_UPPER_OMICRON_TONOS              0x038C
#define GREEK_UPPER_OMICRON_OXIA               0x1FF9
#define GREEK_UPPER_UPSILON_TONOS              0x038E
#define GREEK_UPPER_UPSILON_OXIA               0x1FEB
#define GREEK_UPPER_OMEGA_TONOS                0x038F
#define GREEK_UPPER_OMEGA_OXIA                 0x1FFB
#define COMBINING_ACUTE_ACCENT                 0x0301
#define COMBINING_DIAERESIS                    0x0308
#define COMBINING_ACUTE_TONE_MARK              0x0341
#define COMBINING_GREEK_DIALYTIKA_TONOS        0x0344






enum GreekCasingState {
  kStart,
  kAlpha,
  kEpsilon,
  kEta,
  kIota,
  kOmicron,
  kUpsilon,
  kOmega,
  kAlphaAcc,
  kEpsilonAcc,
  kEtaAcc,
  kIotaAcc,
  kOmicronAcc,
  kUpsilonAcc,
  kOmegaAcc,
  kOmicronUpsilon,
  kDiaeresis
};

static uint32_t
GreekUpperCase(uint32_t aCh, GreekCasingState* aState)
{
  switch (aCh) {
  case GREEK_UPPER_ALPHA:
  case GREEK_LOWER_ALPHA:
    *aState = kAlpha;
    return GREEK_UPPER_ALPHA;

  case GREEK_UPPER_EPSILON:
  case GREEK_LOWER_EPSILON:
    *aState = kEpsilon;
    return GREEK_UPPER_EPSILON;

  case GREEK_UPPER_ETA:
  case GREEK_LOWER_ETA:
    *aState = kEta;
    return GREEK_UPPER_ETA;

  case GREEK_UPPER_IOTA:
    *aState = kIota;
    return GREEK_UPPER_IOTA;

  case GREEK_UPPER_OMICRON:
  case GREEK_LOWER_OMICRON:
    *aState = kOmicron;
    return GREEK_UPPER_OMICRON;

  case GREEK_UPPER_UPSILON:
    switch (*aState) {
    case kOmicron:
      *aState = kOmicronUpsilon;
      break;
    default:
      *aState = kUpsilon;
      break;
    }
    return GREEK_UPPER_UPSILON;

  case GREEK_UPPER_OMEGA:
  case GREEK_LOWER_OMEGA:
    *aState = kOmega;
    return GREEK_UPPER_OMEGA;

  
  case GREEK_LOWER_IOTA:
    switch (*aState) {
    case kAlphaAcc:
    case kEpsilonAcc:
    case kOmicronAcc:
    case kUpsilonAcc:
      *aState = kStart;
      return GREEK_UPPER_IOTA_DIALYTIKA;
    default:
      break;
    }
    *aState = kIota;
    return GREEK_UPPER_IOTA;

  case GREEK_LOWER_UPSILON:
    switch (*aState) {
    case kAlphaAcc:
    case kEpsilonAcc:
    case kEtaAcc:
    case kOmicronAcc:
      *aState = kStart;
      return GREEK_UPPER_UPSILON_DIALYTIKA;
    case kOmicron:
      *aState = kOmicronUpsilon;
      break;
    default:
      *aState = kUpsilon;
      break;
    }
    return GREEK_UPPER_UPSILON;

  case GREEK_UPPER_IOTA_DIALYTIKA:
  case GREEK_LOWER_IOTA_DIALYTIKA:
  case GREEK_UPPER_UPSILON_DIALYTIKA:
  case GREEK_LOWER_UPSILON_DIALYTIKA:
  case COMBINING_DIAERESIS:
    *aState = kDiaeresis;
    return ToUpperCase(aCh);

  
  
  case COMBINING_ACUTE_ACCENT:
  case COMBINING_ACUTE_TONE_MARK:
    switch (*aState) {
    case kAlpha:
      *aState = kAlphaAcc;
      return uint32_t(-1); 
    case kEpsilon:
      *aState = kEpsilonAcc;
      return uint32_t(-1);
    case kEta:
      *aState = kEtaAcc;
      return uint32_t(-1);
    case kIota:
      *aState = kIotaAcc;
      return uint32_t(-1);
    case kOmicron:
      *aState = kOmicronAcc;
      return uint32_t(-1);
    case kUpsilon:
      *aState = kUpsilonAcc;
      return uint32_t(-1);
    case kOmicronUpsilon:
      *aState = kStart; 
      return uint32_t(-1);
    case kOmega:
      *aState = kOmegaAcc;
      return uint32_t(-1);
    case kDiaeresis:
      *aState = kStart;
      return uint32_t(-1);
    default:
      break;
    }
    break;

  
  
  case GREEK_LOWER_IOTA_DIALYTIKA_TONOS:
  case GREEK_LOWER_IOTA_DIALYTIKA_OXIA:
    *aState = kStart;
    return GREEK_UPPER_IOTA_DIALYTIKA;

  case GREEK_LOWER_UPSILON_DIALYTIKA_TONOS:
  case GREEK_LOWER_UPSILON_DIALYTIKA_OXIA:
    *aState = kStart;
    return GREEK_UPPER_UPSILON_DIALYTIKA;

  case COMBINING_GREEK_DIALYTIKA_TONOS:
    *aState = kStart;
    return COMBINING_DIAERESIS;

  
  
  case GREEK_LOWER_ALPHA_TONOS:
  case GREEK_LOWER_ALPHA_OXIA:
  case GREEK_UPPER_ALPHA_TONOS:
  case GREEK_UPPER_ALPHA_OXIA:
    *aState = kAlphaAcc;
    return GREEK_UPPER_ALPHA;

  case GREEK_LOWER_EPSILON_TONOS:
  case GREEK_LOWER_EPSILON_OXIA:
  case GREEK_UPPER_EPSILON_TONOS:
  case GREEK_UPPER_EPSILON_OXIA:
    *aState = kEpsilonAcc;
    return GREEK_UPPER_EPSILON;

  case GREEK_LOWER_ETA_TONOS:
  case GREEK_LOWER_ETA_OXIA:
  case GREEK_UPPER_ETA_TONOS:
  case GREEK_UPPER_ETA_OXIA:
    *aState = kEtaAcc;
    return GREEK_UPPER_ETA;

  case GREEK_LOWER_IOTA_TONOS:
  case GREEK_LOWER_IOTA_OXIA:
  case GREEK_UPPER_IOTA_TONOS:
  case GREEK_UPPER_IOTA_OXIA:
    *aState = kIotaAcc;
    return GREEK_UPPER_IOTA;

  case GREEK_LOWER_OMICRON_TONOS:
  case GREEK_LOWER_OMICRON_OXIA:
  case GREEK_UPPER_OMICRON_TONOS:
  case GREEK_UPPER_OMICRON_OXIA:
    *aState = kOmicronAcc;
    return GREEK_UPPER_OMICRON;

  case GREEK_LOWER_UPSILON_TONOS:
  case GREEK_LOWER_UPSILON_OXIA:
  case GREEK_UPPER_UPSILON_TONOS:
  case GREEK_UPPER_UPSILON_OXIA:
    switch (*aState) {
    case kOmicron:
      *aState = kStart; 
      break;
    default:
      *aState = kUpsilonAcc;
      break;
    }
    return GREEK_UPPER_UPSILON;

  case GREEK_LOWER_OMEGA_TONOS:
  case GREEK_LOWER_OMEGA_OXIA:
  case GREEK_UPPER_OMEGA_TONOS:
  case GREEK_UPPER_OMEGA_OXIA:
    *aState = kOmegaAcc;
    return GREEK_UPPER_OMEGA;
  }

  
  *aState = kStart;
  return ToUpperCase(aCh);
}

nsTransformedTextRun *
nsTransformedTextRun::Create(const gfxTextRunFactory::Parameters* aParams,
                             nsTransformingTextRunFactory* aFactory,
                             gfxFontGroup* aFontGroup,
                             const char16_t* aString, uint32_t aLength,
                             const uint32_t aFlags, nsStyleContext** aStyles,
                             bool aOwnsFactory)
{
  NS_ASSERTION(!(aFlags & gfxTextRunFactory::TEXT_IS_8BIT),
               "didn't expect text to be marked as 8-bit here");

  void *storage = AllocateStorageForTextRun(sizeof(nsTransformedTextRun), aLength);
  if (!storage) {
    return nullptr;
  }

  return new (storage) nsTransformedTextRun(aParams, aFactory, aFontGroup,
                                            aString, aLength,
                                            aFlags, aStyles, aOwnsFactory);
}

void
nsTransformedTextRun::SetCapitalization(uint32_t aStart, uint32_t aLength,
                                        bool* aCapitalization,
                                        gfxContext* aRefContext)
{
  if (mCapitalize.IsEmpty()) {
    if (!mCapitalize.AppendElements(GetLength()))
      return;
    memset(mCapitalize.Elements(), 0, GetLength()*sizeof(bool));
  }
  memcpy(mCapitalize.Elements() + aStart, aCapitalization, aLength*sizeof(bool));
  mNeedsRebuild = true;
}

bool
nsTransformedTextRun::SetPotentialLineBreaks(uint32_t aStart, uint32_t aLength,
                                             uint8_t* aBreakBefore,
                                             gfxContext* aRefContext)
{
  bool changed = gfxTextRun::SetPotentialLineBreaks(aStart, aLength,
      aBreakBefore, aRefContext);
  if (changed) {
    mNeedsRebuild = true;
  }
  return changed;
}

size_t
nsTransformedTextRun::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
{
  size_t total = gfxTextRun::SizeOfExcludingThis(aMallocSizeOf);
  total += mStyles.SizeOfExcludingThis(aMallocSizeOf);
  total += mCapitalize.SizeOfExcludingThis(aMallocSizeOf);
  if (mOwnsFactory) {
    total += aMallocSizeOf(mFactory);
  }
  return total;
}

size_t
nsTransformedTextRun::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

nsTransformedTextRun*
nsTransformingTextRunFactory::MakeTextRun(const char16_t* aString, uint32_t aLength,
                                          const gfxTextRunFactory::Parameters* aParams,
                                          gfxFontGroup* aFontGroup, uint32_t aFlags,
                                          nsStyleContext** aStyles, bool aOwnsFactory)
{
  return nsTransformedTextRun::Create(aParams, this, aFontGroup,
                                      aString, aLength, aFlags, aStyles, aOwnsFactory);
}

nsTransformedTextRun*
nsTransformingTextRunFactory::MakeTextRun(const uint8_t* aString, uint32_t aLength,
                                          const gfxTextRunFactory::Parameters* aParams,
                                          gfxFontGroup* aFontGroup, uint32_t aFlags,
                                          nsStyleContext** aStyles, bool aOwnsFactory)
{
  
  
  NS_ConvertASCIItoUTF16 unicodeString(reinterpret_cast<const char*>(aString), aLength);
  return MakeTextRun(unicodeString.get(), aLength, aParams, aFontGroup,
                     aFlags & ~(gfxFontGroup::TEXT_IS_PERSISTENT | gfxFontGroup::TEXT_IS_8BIT),
                     aStyles, aOwnsFactory);
}

void
MergeCharactersInTextRun(gfxTextRun* aDest, gfxTextRun* aSrc,
                         const bool* aCharsToMerge, const bool* aDeletedChars)
{
  aDest->ResetGlyphRuns();

  gfxTextRun::GlyphRunIterator iter(aSrc, 0, aSrc->GetLength());
  uint32_t offset = 0;
  nsAutoTArray<gfxTextRun::DetailedGlyph,2> glyphs;
  while (iter.NextRun()) {
    gfxTextRun::GlyphRun* run = iter.GetGlyphRun();
    nsresult rv = aDest->AddGlyphRun(run->mFont, run->mMatchType,
                                     offset, false);
    if (NS_FAILED(rv))
      return;

    bool anyMissing = false;
    uint32_t mergeRunStart = iter.GetStringStart();
    const gfxTextRun::CompressedGlyph *srcGlyphs = aSrc->GetCharacterGlyphs();
    gfxTextRun::CompressedGlyph mergedGlyph = srcGlyphs[mergeRunStart];
    uint32_t stringEnd = iter.GetStringEnd();
    for (uint32_t k = iter.GetStringStart(); k < stringEnd; ++k) {
      const gfxTextRun::CompressedGlyph g = srcGlyphs[k];
      if (g.IsSimpleGlyph()) {
        if (!anyMissing) {
          gfxTextRun::DetailedGlyph details;
          details.mGlyphID = g.GetSimpleGlyph();
          details.mAdvance = g.GetSimpleAdvance();
          details.mXOffset = 0;
          details.mYOffset = 0;
          glyphs.AppendElement(details);
        }
      } else {
        if (g.IsMissing()) {
          anyMissing = true;
          glyphs.Clear();
        }
        if (g.GetGlyphCount() > 0) {
          glyphs.AppendElements(aSrc->GetDetailedGlyphs(k), g.GetGlyphCount());
        }
      }

      if (k + 1 < iter.GetStringEnd() && aCharsToMerge[k + 1]) {
        
        
        continue;
      }

      
      
      
      
      
      
      NS_WARN_IF_FALSE(!aCharsToMerge[mergeRunStart],
                       "unable to merge across a glyph run boundary, "
                       "glyph(s) discarded");
      if (!aCharsToMerge[mergeRunStart]) {
        if (anyMissing) {
          mergedGlyph.SetMissing(glyphs.Length());
        } else {
          mergedGlyph.SetComplex(mergedGlyph.IsClusterStart(),
                                 mergedGlyph.IsLigatureGroupStart(),
                                 glyphs.Length());
        }
        aDest->SetGlyphs(offset, mergedGlyph, glyphs.Elements());
        ++offset;

        while (offset < aDest->GetLength() && aDeletedChars[offset]) {
          aDest->SetGlyphs(offset++, gfxTextRun::CompressedGlyph(), nullptr);
        }
      }

      glyphs.Clear();
      anyMissing = false;
      mergeRunStart = k + 1;
      if (mergeRunStart < stringEnd) {
        mergedGlyph = srcGlyphs[mergeRunStart];
      }
    }
    NS_ASSERTION(glyphs.Length() == 0,
                 "Leftover glyphs, don't request merging of the last character with its next!");  
  }
  NS_ASSERTION(offset == aDest->GetLength(), "Bad offset calculations");
}

gfxTextRunFactory::Parameters
GetParametersForInner(nsTransformedTextRun* aTextRun, uint32_t* aFlags,
    gfxContext* aRefContext)
{
  gfxTextRunFactory::Parameters params =
    { aRefContext, nullptr, nullptr,
      nullptr, 0, aTextRun->GetAppUnitsPerDevUnit()
    };
  *aFlags = aTextRun->GetFlags() & ~gfxFontGroup::TEXT_IS_PERSISTENT;
  return params;
}

void
nsFontVariantTextRunFactory::RebuildTextRun(nsTransformedTextRun* aTextRun,
    gfxContext* aRefContext)
{
  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();
  gfxFontStyle fontStyle = *fontGroup->GetStyle();
  fontStyle.size *= 0.8;
  nsRefPtr<gfxFontGroup> smallFont = fontGroup->Copy(&fontStyle);
  if (!smallFont)
    return;

  uint32_t flags;
  gfxTextRunFactory::Parameters innerParams =
      GetParametersForInner(aTextRun, &flags, aRefContext);

  uint32_t length = aTextRun->GetLength();
  const char16_t* str = aTextRun->mString.BeginReading();
  nsRefPtr<nsStyleContext>* styles = aTextRun->mStyles.Elements();
  
  nsAutoPtr<gfxTextRun> inner(fontGroup->MakeTextRun(str, length, &innerParams, flags));
  if (!inner.get())
    return;

  nsCaseTransformTextRunFactory uppercaseFactory(nullptr, true);

  aTextRun->ResetGlyphRuns();

  uint32_t runStart = 0;
  nsAutoTArray<nsStyleContext*,50> styleArray;
  nsAutoTArray<uint8_t,50> canBreakBeforeArray;

  enum RunCaseState {
    kUpperOrCaseless, 
    kLowercase,       
    kSpecialUpper     
  };
  RunCaseState runCase = kUpperOrCaseless;

  
  
  
  
  
  
  for (uint32_t i = 0; i <= length; ++i) {
    RunCaseState chCase = kUpperOrCaseless;
    
    
    if (i < length) {
      nsStyleContext* styleContext = styles[i];
      
      
      if (!inner->IsClusterStart(i)) {
        chCase = runCase;
      } else {
        if (styleContext->StyleFont()->mFont.variant == NS_STYLE_FONT_VARIANT_SMALL_CAPS) {
          uint32_t ch = str[i];
          if (NS_IS_HIGH_SURROGATE(ch) && i < length - 1 && NS_IS_LOW_SURROGATE(str[i + 1])) {
            ch = SURROGATE_TO_UCS4(ch, str[i + 1]);
          }
          uint32_t ch2 = ToUpperCase(ch);
          if (ch != ch2 || mozilla::unicode::SpecialUpper(ch)) {
            chCase = kLowercase;
          } else if (styleContext->StyleFont()->mLanguage == nsGkAtoms::el) {
            
            
            
            
            GreekCasingState state = kStart; 
            ch2 = GreekUpperCase(ch, &state);
            if (ch != ch2) {
              chCase = kSpecialUpper;
            }
          }
        } else {
          
        }
      }
    }

    
    
    
    
    
    if ((i == length || runCase != chCase) && runStart < i) {
      nsAutoPtr<nsTransformedTextRun> transformedChild;
      nsAutoPtr<gfxTextRun> cachedChild;
      gfxTextRun* child;

      switch (runCase) {
      case kUpperOrCaseless:
        cachedChild =
          fontGroup->MakeTextRun(str + runStart, i - runStart, &innerParams,
                                 flags);
        child = cachedChild.get();
        break;
      case kLowercase:
        transformedChild =
          uppercaseFactory.MakeTextRun(str + runStart, i - runStart,
                                       &innerParams, smallFont, flags,
                                       styleArray.Elements(), false);
        child = transformedChild;
        break;
      case kSpecialUpper:
        transformedChild =
          uppercaseFactory.MakeTextRun(str + runStart, i - runStart,
                                       &innerParams, fontGroup, flags,
                                       styleArray.Elements(), false);
        child = transformedChild;
        break;
      }
      if (!child)
        return;
      
      
      NS_ASSERTION(canBreakBeforeArray.Length() == i - runStart,
                   "lost some break-before values?");
      child->SetPotentialLineBreaks(0, canBreakBeforeArray.Length(),
          canBreakBeforeArray.Elements(), aRefContext);
      if (transformedChild) {
        transformedChild->FinishSettingProperties(aRefContext);
      }
      aTextRun->CopyGlyphDataFrom(child, 0, child->GetLength(), runStart);

      runStart = i;
      styleArray.Clear();
      canBreakBeforeArray.Clear();
    }

    if (i < length) {
      runCase = chCase;
      styleArray.AppendElement(styles[i]);
      canBreakBeforeArray.AppendElement(aTextRun->CanBreakLineBefore(i));
    }
  }
}






enum LanguageSpecificCasingBehavior {
  eLSCB_None,    
  eLSCB_Turkish, 
  eLSCB_Dutch,   
  eLSCB_Greek    
};

static LanguageSpecificCasingBehavior
GetCasingFor(const nsIAtom* aLang)
{
  if (aLang == nsGkAtoms::tr ||
      aLang == nsGkAtoms::az ||
      aLang == nsGkAtoms::ba ||
      aLang == nsGkAtoms::crh ||
      aLang == nsGkAtoms::tt) {
    return eLSCB_Turkish;
  }
  if (aLang == nsGkAtoms::nl) {
    return eLSCB_Dutch;
  }
  if (aLang == nsGkAtoms::el) {
    return eLSCB_Greek;
  }
  return eLSCB_None;
}

bool
nsCaseTransformTextRunFactory::TransformString(
    const nsAString& aString,
    nsString& aConvertedString,
    bool aAllUppercase,
    const nsIAtom* aLanguage,
    nsTArray<bool>& aCharsToMergeArray,
    nsTArray<bool>& aDeletedCharsArray,
    nsTransformedTextRun* aTextRun,
    nsTArray<uint8_t>* aCanBreakBeforeArray,
    nsTArray<nsStyleContext*>* aStyleArray)
{
  NS_PRECONDITION(!aTextRun || (aCanBreakBeforeArray && aStyleArray),
                  "either none or all three optional parameters required");

  uint32_t length = aString.Length();
  const char16_t* str = aString.BeginReading();

  bool mergeNeeded = false;

  bool capitalizeDutchIJ = false;
  bool prevIsLetter = false;
  uint32_t sigmaIndex = uint32_t(-1);
  nsIUGenCategory::nsUGenCategory cat;

  uint8_t style = aAllUppercase ? NS_STYLE_TEXT_TRANSFORM_UPPERCASE : 0;
  const nsIAtom* lang = aLanguage;

  LanguageSpecificCasingBehavior languageSpecificCasing = GetCasingFor(lang);
  GreekCasingState greekState = kStart;

  for (uint32_t i = 0; i < length; ++i) {
    uint32_t ch = str[i];

    nsStyleContext* styleContext;
    if (aTextRun) {
      styleContext = aTextRun->mStyles[i];
      style = aAllUppercase ? NS_STYLE_TEXT_TRANSFORM_UPPERCASE :
        styleContext->StyleText()->mTextTransform;

      if (lang != styleContext->StyleFont()->mLanguage) {
        lang = styleContext->StyleFont()->mLanguage;
        languageSpecificCasing = GetCasingFor(lang);
        greekState = kStart;
      }
    }

    int extraChars = 0;
    const mozilla::unicode::MultiCharMapping *mcm;

    if (NS_IS_HIGH_SURROGATE(ch) && i < length - 1 &&
        NS_IS_LOW_SURROGATE(str[i + 1])) {
      ch = SURROGATE_TO_UCS4(ch, str[i + 1]);
    }

    switch (style) {
    case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
      if (languageSpecificCasing == eLSCB_Turkish) {
        if (ch == 'I') {
          ch = LATIN_SMALL_LETTER_DOTLESS_I;
          prevIsLetter = true;
          sigmaIndex = uint32_t(-1);
          break;
        }
        if (ch == LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE) {
          ch = 'i';
          prevIsLetter = true;
          sigmaIndex = uint32_t(-1);
          break;
        }
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      cat = mozilla::unicode::GetGenCategory(ch);

      
      
      
      if (sigmaIndex != uint32_t(-1)) {
        if (cat == nsIUGenCategory::kLetter) {
          aConvertedString.SetCharAt(GREEK_SMALL_LETTER_SIGMA, sigmaIndex);
        }
      }

      if (ch == GREEK_CAPITAL_LETTER_SIGMA) {
        
        
        
        if (prevIsLetter) {
          ch = GREEK_SMALL_LETTER_FINAL_SIGMA;
          sigmaIndex = aConvertedString.Length();
        } else {
          
          
          ch = GREEK_SMALL_LETTER_SIGMA;
          sigmaIndex = uint32_t(-1);
        }
        prevIsLetter = true;
        break;
      }

      
      
      
      if (cat != nsIUGenCategory::kMark) {
        prevIsLetter = (cat == nsIUGenCategory::kLetter);
        sigmaIndex = uint32_t(-1);
      }

      mcm = mozilla::unicode::SpecialLower(ch);
      if (mcm) {
        int j = 0;
        while (j < 2 && mcm->mMappedChars[j + 1]) {
          aConvertedString.Append(mcm->mMappedChars[j]);
          ++extraChars;
          ++j;
        }
        ch = mcm->mMappedChars[j];
        break;
      }

      ch = ToLowerCase(ch);
      break;

    case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
      if (languageSpecificCasing == eLSCB_Turkish && ch == 'i') {
        ch = LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE;
        break;
      }

      if (languageSpecificCasing == eLSCB_Greek) {
        ch = GreekUpperCase(ch, &greekState);
        break;
      }

      mcm = mozilla::unicode::SpecialUpper(ch);
      if (mcm) {
        int j = 0;
        while (j < 2 && mcm->mMappedChars[j + 1]) {
          aConvertedString.Append(mcm->mMappedChars[j]);
          ++extraChars;
          ++j;
        }
        ch = mcm->mMappedChars[j];
        break;
      }

      ch = ToUpperCase(ch);
      break;

    case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
      if (aTextRun) {
        if (capitalizeDutchIJ && ch == 'j') {
          ch = 'J';
          capitalizeDutchIJ = false;
          break;
        }
        capitalizeDutchIJ = false;
        if (i < aTextRun->mCapitalize.Length() && aTextRun->mCapitalize[i]) {
          if (languageSpecificCasing == eLSCB_Turkish && ch == 'i') {
            ch = LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE;
            break;
          }
          if (languageSpecificCasing == eLSCB_Dutch && ch == 'i') {
            ch = 'I';
            capitalizeDutchIJ = true;
            break;
          }

          mcm = mozilla::unicode::SpecialTitle(ch);
          if (mcm) {
            int j = 0;
            while (j < 2 && mcm->mMappedChars[j + 1]) {
              aConvertedString.Append(mcm->mMappedChars[j]);
              ++extraChars;
              ++j;
            }
            ch = mcm->mMappedChars[j];
            break;
          }

          ch = ToTitleCase(ch);
        }
      }
      break;

    case NS_STYLE_TEXT_TRANSFORM_FULLWIDTH:
      ch = mozilla::unicode::GetFullWidth(ch);
      break;

    default:
      break;
    }

    if (ch == uint32_t(-1)) {
      aDeletedCharsArray.AppendElement(true);
      mergeNeeded = true;
    } else {
      aDeletedCharsArray.AppendElement(false);
      aCharsToMergeArray.AppendElement(false);
      if (aTextRun) {
        aStyleArray->AppendElement(styleContext);
        aCanBreakBeforeArray->AppendElement(aTextRun->CanBreakLineBefore(i));
      }

      if (IS_IN_BMP(ch)) {
        aConvertedString.Append(ch);
      } else {
        aConvertedString.Append(H_SURROGATE(ch));
        aConvertedString.Append(L_SURROGATE(ch));
        ++i;
        aDeletedCharsArray.AppendElement(true); 
                                                
        ++extraChars;
      }

      while (extraChars-- > 0) {
        mergeNeeded = true;
        aCharsToMergeArray.AppendElement(true);
        if (aTextRun) {
          aStyleArray->AppendElement(styleContext);
          aCanBreakBeforeArray->AppendElement(false);
        }
      }
    }
  }

  return mergeNeeded;
}

void
nsCaseTransformTextRunFactory::RebuildTextRun(nsTransformedTextRun* aTextRun,
    gfxContext* aRefContext)
{
  nsAutoString convertedString;
  nsAutoTArray<bool,50> charsToMergeArray;
  nsAutoTArray<bool,50> deletedCharsArray;
  nsAutoTArray<uint8_t,50> canBreakBeforeArray;
  nsAutoTArray<nsStyleContext*,50> styleArray;

  bool mergeNeeded = TransformString(aTextRun->mString,
                                     convertedString,
                                     mAllUppercase,
                                     nullptr,
                                     charsToMergeArray,
                                     deletedCharsArray,
                                     aTextRun,
                                     &canBreakBeforeArray,
                                     &styleArray);

  uint32_t flags;
  gfxTextRunFactory::Parameters innerParams =
      GetParametersForInner(aTextRun, &flags, aRefContext);
  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();

  nsAutoPtr<nsTransformedTextRun> transformedChild;
  nsAutoPtr<gfxTextRun> cachedChild;
  gfxTextRun* child;

  if (mInnerTransformingTextRunFactory) {
    transformedChild = mInnerTransformingTextRunFactory->MakeTextRun(
        convertedString.BeginReading(), convertedString.Length(),
        &innerParams, fontGroup, flags, styleArray.Elements(), false);
    child = transformedChild.get();
  } else {
    cachedChild = fontGroup->MakeTextRun(
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
