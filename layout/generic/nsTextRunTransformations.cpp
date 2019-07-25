




































#include "nsTextRunTransformations.h"

#include "nsTextFrameUtils.h"
#include "gfxSkipChars.h"
#include "nsGkAtoms.h"

#include "nsStyleConsts.h"
#include "nsStyleContext.h"
#include "gfxContext.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"

#define SZLIG 0x00DF


#define LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE  0x0130
#define LATIN_SMALL_LETTER_DOTLESS_I           0x0131

nsTransformedTextRun *
nsTransformedTextRun::Create(const gfxTextRunFactory::Parameters* aParams,
                             nsTransformingTextRunFactory* aFactory,
                             gfxFontGroup* aFontGroup,
                             const PRUnichar* aString, PRUint32 aLength,
                             const PRUint32 aFlags, nsStyleContext** aStyles,
                             bool aOwnsFactory)
{
  NS_ASSERTION(!(aFlags & gfxTextRunFactory::TEXT_IS_8BIT),
               "didn't expect text to be marked as 8-bit here");

  void *storage = AllocateStorageForTextRun(sizeof(nsTransformedTextRun), aLength);
  if (!storage) {
    return nsnull;
  }

  return new (storage) nsTransformedTextRun(aParams, aFactory, aFontGroup,
                                            aString, aLength,
                                            aFlags, aStyles, aOwnsFactory);
}

void
nsTransformedTextRun::SetCapitalization(PRUint32 aStart, PRUint32 aLength,
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
nsTransformedTextRun::SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                             PRUint8* aBreakBefore,
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
nsTransformedTextRun::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
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
nsTransformedTextRun::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
  return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

nsTransformedTextRun*
nsTransformingTextRunFactory::MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                          const gfxTextRunFactory::Parameters* aParams,
                                          gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                          nsStyleContext** aStyles, bool aOwnsFactory)
{
  return nsTransformedTextRun::Create(aParams, this, aFontGroup,
                                      aString, aLength, aFlags, aStyles, aOwnsFactory);
}

nsTransformedTextRun*
nsTransformingTextRunFactory::MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                          const gfxTextRunFactory::Parameters* aParams,
                                          gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                          nsStyleContext** aStyles, bool aOwnsFactory)
{
  
  
  NS_ConvertASCIItoUTF16 unicodeString(reinterpret_cast<const char*>(aString), aLength);
  return MakeTextRun(unicodeString.get(), aLength, aParams, aFontGroup,
                     aFlags & ~(gfxFontGroup::TEXT_IS_PERSISTENT | gfxFontGroup::TEXT_IS_8BIT),
                     aStyles, aOwnsFactory);
}




















static void
MergeCharactersInTextRun(gfxTextRun* aDest, gfxTextRun* aSrc,
                         bool* aCharsToMerge)
{
  aDest->ResetGlyphRuns();

  gfxTextRun::GlyphRunIterator iter(aSrc, 0, aSrc->GetLength());
  PRUint32 offset = 0;
  nsAutoTArray<gfxTextRun::DetailedGlyph,2> glyphs;
  while (iter.NextRun()) {
    gfxTextRun::GlyphRun* run = iter.GetGlyphRun();
    nsresult rv = aDest->AddGlyphRun(run->mFont, run->mMatchType,
                                     offset, false);
    if (NS_FAILED(rv))
      return;

    bool anyMissing = false;
    PRUint32 mergeRunStart = iter.GetStringStart();
    PRUint32 k;
    for (k = iter.GetStringStart(); k < iter.GetStringEnd(); ++k) {
      gfxTextRun::CompressedGlyph g = aSrc->GetCharacterGlyphs()[k];
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
        NS_ASSERTION(g.IsClusterStart() && g.IsLigatureGroupStart(),
                     "Don't know how to merge this stuff");
        continue;
      }

      NS_ASSERTION(mergeRunStart == k ||
                   (g.IsClusterStart() && g.IsLigatureGroupStart()),
                   "Don't know how to merge this stuff");

      
      
      
      
      
      if (!aCharsToMerge[mergeRunStart]) {
        if (anyMissing) {
          g.SetMissing(glyphs.Length());
        } else {
          g.SetComplex(true, true, glyphs.Length());
        }
        aDest->SetGlyphs(offset, g, glyphs.Elements());
        ++offset;
      }

      glyphs.Clear();
      anyMissing = false;
      mergeRunStart = k + 1;
    }
    NS_ASSERTION(glyphs.Length() == 0,
                 "Leftover glyphs, don't request merging of the last character with its next!");  
  }
  NS_ASSERTION(offset == aDest->GetLength(), "Bad offset calculations");
}

static gfxTextRunFactory::Parameters
GetParametersForInner(nsTransformedTextRun* aTextRun, PRUint32* aFlags,
    gfxContext* aRefContext)
{
  gfxTextRunFactory::Parameters params =
    { aRefContext, nsnull, nsnull,
      nsnull, 0, aTextRun->GetAppUnitsPerDevUnit()
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

  PRUint32 flags;
  gfxTextRunFactory::Parameters innerParams =
      GetParametersForInner(aTextRun, &flags, aRefContext);

  PRUint32 length = aTextRun->GetLength();
  const PRUnichar* str = aTextRun->mString.BeginReading();
  nsRefPtr<nsStyleContext>* styles = aTextRun->mStyles.Elements();
  
  nsAutoPtr<gfxTextRun> inner(fontGroup->MakeTextRun(str, length, &innerParams, flags));
  if (!inner.get())
    return;

  nsCaseTransformTextRunFactory uppercaseFactory(nsnull, true);

  aTextRun->ResetGlyphRuns();

  PRUint32 runStart = 0;
  bool runIsLowercase = false;
  nsAutoTArray<nsStyleContext*,50> styleArray;
  nsAutoTArray<PRUint8,50> canBreakBeforeArray;

  PRUint32 i;
  for (i = 0; i <= length; ++i) {
    bool isLowercase = false;
    if (i < length) {
      
      
      if (!inner->IsClusterStart(i)) {
        isLowercase = runIsLowercase;
      } else {
        if (styles[i]->GetStyleFont()->mFont.variant == NS_STYLE_FONT_VARIANT_SMALL_CAPS) {
          PRUint32 ch = str[i];
          if (NS_IS_HIGH_SURROGATE(ch) && i < length - 1 && NS_IS_LOW_SURROGATE(str[i + 1])) {
            ch = SURROGATE_TO_UCS4(ch, str[i + 1]);
          }
          PRUint32 ch2 = ToUpperCase(ch);
          isLowercase = ch != ch2 || ch == SZLIG;
        } else {
          
        }
      }
    }

    if ((i == length || runIsLowercase != isLowercase) && runStart < i) {
      nsAutoPtr<nsTransformedTextRun> transformedChild;
      nsAutoPtr<gfxTextRun> cachedChild;
      gfxTextRun* child;

      if (runIsLowercase) {
        transformedChild = uppercaseFactory.MakeTextRun(str + runStart, i - runStart,
            &innerParams, smallFont, flags, styleArray.Elements(), false);
        child = transformedChild;
      } else {
        cachedChild =
          fontGroup->MakeTextRun(str + runStart, i - runStart,
                                 &innerParams, flags);
        child = cachedChild.get();
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
      runIsLowercase = isLowercase;
      styleArray.AppendElement(styles[i]);
      canBreakBeforeArray.AppendElement(aTextRun->CanBreakLineBefore(i));
    }
  }
}

void
nsCaseTransformTextRunFactory::RebuildTextRun(nsTransformedTextRun* aTextRun,
    gfxContext* aRefContext)
{
  PRUint32 length = aTextRun->GetLength();
  const PRUnichar* str = aTextRun->mString.BeginReading();
  nsRefPtr<nsStyleContext>* styles = aTextRun->mStyles.Elements();

  nsAutoString convertedString;
  nsAutoTArray<bool,50> charsToMergeArray;
  nsAutoTArray<nsStyleContext*,50> styleArray;
  nsAutoTArray<PRUint8,50> canBreakBeforeArray;
  PRUint32 extraCharsCount = 0;

  
  
  
  
  
  enum {
    eNone,    
    eTurkish, 
    eDutch    
  } languageSpecificCasing = eNone;

  const nsIAtom* lang = nsnull;
  bool capitalizeDutchIJ = false;  
  PRUint32 i;
  for (i = 0; i < length; ++i) {
    PRUint32 ch = str[i];
    nsStyleContext* styleContext = styles[i];

    charsToMergeArray.AppendElement(false);
    styleArray.AppendElement(styleContext);
    canBreakBeforeArray.AppendElement(aTextRun->CanBreakLineBefore(i));

    PRUint8 style = mAllUppercase ? NS_STYLE_TEXT_TRANSFORM_UPPERCASE
      : styleContext->GetStyleText()->mTextTransform;
    bool extraChar = false;

    if (NS_IS_HIGH_SURROGATE(ch) && i < length - 1 && NS_IS_LOW_SURROGATE(str[i + 1])) {
      ch = SURROGATE_TO_UCS4(ch, str[i + 1]);
    }

    if (lang != styleContext->GetStyleFont()->mLanguage) {
      lang = styleContext->GetStyleFont()->mLanguage;
      if (lang == nsGkAtoms::tr || lang == nsGkAtoms::az ||
          lang == nsGkAtoms::ba || lang == nsGkAtoms::crh ||
          lang == nsGkAtoms::tt) {
        languageSpecificCasing = eTurkish;
      } else if (lang == nsGkAtoms::nl) {
        languageSpecificCasing = eDutch;
      } else {
        languageSpecificCasing = eNone;
      }
    }

    switch (style) {
    case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
      if (languageSpecificCasing == eTurkish && ch == 'I') {
        ch = LATIN_SMALL_LETTER_DOTLESS_I;
      } else {
        ch = ToLowerCase(ch);
      }
      break;
    case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
      if (ch == SZLIG) {
        convertedString.Append('S');
        extraChar = true;
        ch = 'S';
      } else if (languageSpecificCasing == eTurkish && ch == 'i') {
        ch = LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE;
      } else {
        ch = ToUpperCase(ch);
      }
      break;
    case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
      if (capitalizeDutchIJ && ch == 'j') {
        ch = 'J';
        capitalizeDutchIJ = false;
        break;
      }
      capitalizeDutchIJ = false;
      if (i < aTextRun->mCapitalize.Length() && aTextRun->mCapitalize[i]) {
        if (ch == SZLIG) {
          convertedString.Append('S');
          extraChar = true;
          ch = 'S';
        } else if (languageSpecificCasing == eTurkish && ch == 'i') {
          ch = LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE;
        } else if (languageSpecificCasing == eDutch && ch == 'i') {
          ch = 'I';
          capitalizeDutchIJ = true;
        } else {
          ch = ToTitleCase(ch);
        }
      }
      break;
    default:
      break;
    }

    if (IS_IN_BMP(ch)) {
      convertedString.Append(ch);
    } else {
      convertedString.Append(H_SURROGATE(ch));
      convertedString.Append(L_SURROGATE(ch));
      i++;
      charsToMergeArray.AppendElement(false);
      styleArray.AppendElement(styleContext);
      canBreakBeforeArray.AppendElement(false);
    }

    if (extraChar) {
      ++extraCharsCount;
      charsToMergeArray.AppendElement(true);
      styleArray.AppendElement(styleContext);
      canBreakBeforeArray.AppendElement(false);
    }
  }

  PRUint32 flags;
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

  if (extraCharsCount > 0) {
    
    MergeCharactersInTextRun(aTextRun, child, charsToMergeArray.Elements());
  } else {
    
    
    
    aTextRun->ResetGlyphRuns();
    aTextRun->CopyGlyphDataFrom(child, 0, child->GetLength(), 0);
  }
}
