




#include "nsTextRunTransformations.h"

#include "mozilla/MemoryReporting.h"

#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsStyleContext.h"
#include "nsUnicharUtils.h"
#include "nsUnicodeProperties.h"
#include "nsSpecialCasingData.h"
#include "mozilla/gfx/2D.h"
#include "nsTextFrameUtils.h"
#include "nsIPersistentProperties2.h"
#include "nsNetUtil.h"
#include "GreekCasing.h"
#include "IrishCasing.h"


#define LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE  0x0130
#define LATIN_SMALL_LETTER_DOTLESS_I           0x0131




#define GREEK_CAPITAL_LETTER_SIGMA             0x03A3
#define GREEK_SMALL_LETTER_FINAL_SIGMA         0x03C2
#define GREEK_SMALL_LETTER_SIGMA               0x03C3

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
                                     offset, false, run->mOrientation);
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






enum LanguageSpecificCasingBehavior {
  eLSCB_None,    
  eLSCB_Dutch,   
  eLSCB_Greek,   
  eLSCB_Irish,   
  eLSCB_Turkish  
};

static LanguageSpecificCasingBehavior
GetCasingFor(const nsIAtom* aLang)
{
  if (!aLang) {
      return eLSCB_None;
  }
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
  if (aLang == nsGkAtoms::ga) {
    return eLSCB_Irish;
  }

  
  nsAtomString langStr(const_cast<nsIAtom*>(aLang));
  int index = langStr.FindChar('-');
  if (index > 0) {
    langStr.Truncate(index);
    nsCOMPtr<nsIAtom> truncatedLang = do_GetAtom(langStr);
    return GetCasingFor(truncatedLang);
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
  bool ntPrefix = false; 
                         
  uint32_t sigmaIndex = uint32_t(-1);
  nsIUGenCategory::nsUGenCategory cat;

  uint8_t style = aAllUppercase ? NS_STYLE_TEXT_TRANSFORM_UPPERCASE : 0;
  const nsIAtom* lang = aLanguage;

  LanguageSpecificCasingBehavior languageSpecificCasing = GetCasingFor(lang);
  mozilla::GreekCasing::State greekState;
  mozilla::IrishCasing::State irishState;
  uint32_t irishMark = uint32_t(-1); 

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
        greekState.Reset();
        irishState.Reset();
        irishMark = uint32_t(-1);
      }
    }

    int extraChars = 0;
    const mozilla::unicode::MultiCharMapping *mcm;
    bool inhibitBreakBefore = false; 

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

      if (languageSpecificCasing == eLSCB_Irish &&
          cat == nsIUGenCategory::kLetter) {
        
        if (!prevIsLetter && (ch == 'n' || ch == 't')) {
          ntPrefix = true;
        } else {
          if (ntPrefix && mozilla::IrishCasing::IsUpperVowel(ch)) {
            aConvertedString.Append('-');
            ++extraChars;
          }
          ntPrefix = false;
        }
      } else {
        ntPrefix = false;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      
      
      
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
        ch = mozilla::GreekCasing::UpperCase(ch, greekState);
        break;
      }

      if (languageSpecificCasing == eLSCB_Irish) {
        bool mark;
        uint8_t action;
        ch = mozilla::IrishCasing::UpperCase(ch, irishState, mark, action);
        if (mark) {
          irishMark = aConvertedString.Length();
          break;
        } else if (action) {
          nsString& str = aConvertedString; 
          switch (action) {
          case 1:
            
            NS_ASSERTION(str.Length() > 0 && irishMark < str.Length(),
                         "bad irishMark!");
            str.SetCharAt(ToLowerCase(str[irishMark]), irishMark);
            irishMark = uint32_t(-1);
            break;
          case 2:
            
            NS_ASSERTION(str.Length() >= 2 && irishMark == str.Length() - 2,
                         "bad irishMark!");
            str.SetCharAt(ToLowerCase(str[irishMark]), irishMark);
            str.SetCharAt(ToLowerCase(str[irishMark + 1]), irishMark + 1);
            irishMark = uint32_t(-1);
            break;
          case 3:
            
            
            NS_ASSERTION(str.Length() >= 2 && irishMark == str.Length() - 2,
                         "bad irishMark!");
            str.Replace(irishMark, 2, ToLowerCase(str[irishMark]));
            aDeletedCharsArray[irishMark + 1] = true;
            
            
            aCharsToMergeArray.SetLength(aCharsToMergeArray.Length() - 1);
            if (aTextRun) {
              aStyleArray->SetLength(aStyleArray->Length() - 1);
              aCanBreakBeforeArray->SetLength(aCanBreakBeforeArray->Length() - 1);
              inhibitBreakBefore = true;
            }
            mergeNeeded = true;
            irishMark = uint32_t(-1);
            break;
          }
          
          
          
          break;
        }
        
        
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
        aCanBreakBeforeArray->AppendElement(inhibitBreakBefore ? false :
                                            aTextRun->CanBreakLineBefore(i));
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
