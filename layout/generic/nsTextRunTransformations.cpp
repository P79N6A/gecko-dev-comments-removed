




































#include "nsTextRunTransformations.h"

#include "nsTextTransformer.h"
#include "nsTextFrameUtils.h"
#include "gfxSkipChars.h"

#include "nsICaseConversion.h"
#include "nsStyleConsts.h"
#include "nsStyleContext.h"
#include "gfxContext.h"

#define SZLIG 0x00DF





class nsTransformedTextRun : public gfxTextRun {
public:
  nsTransformedTextRun(const gfxTextRunFactory::Parameters* aParams,
                       nsTransformingTextRunFactory* aFactory,
                       gfxFontGroup* aFontGroup,
                       const PRUnichar* aString, PRUint32 aLength,
                       const PRUint32 aFlags, nsStyleContext** aStyles)
    : gfxTextRun(aParams, aString, aLength, aFontGroup, aFlags),
      mFactory(aFactory), mRefContext(aParams->mContext)
  {
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      mStyles.AppendElement(aStyles[i]);
    }
    for (i = 0; i < aParams->mInitialBreakCount; ++i) {
      mLineBreaks.AppendElement(aParams->mInitialBreaks[i]);
    }
  }

  virtual PRBool SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                        PRPackedBool* aBreakBefore)
  {
    PRBool changed = gfxTextRun::SetPotentialLineBreaks(aStart, aLength, aBreakBefore);
    mFactory->RebuildTextRun(this);
    return changed;
  }
  virtual PRBool SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                               PRBool aLineBreakBefore, PRBool aLineBreakAfter,
                               gfxFloat* aAdvanceWidthDelta);

  nsAutoPtr<nsTransformingTextRunFactory> mFactory;
  nsRefPtr<gfxContext>                    mRefContext;
  nsTArray<PRUint32>                      mLineBreaks;
  nsTArray<nsRefPtr<nsStyleContext> >     mStyles;
};

PRBool
nsTransformedTextRun::SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                    PRBool aLineBreakBefore, PRBool aLineBreakAfter,
                                    gfxFloat* aAdvanceWidthDelta)
{
  nsTArray<PRUint32> newBreaks;
  PRUint32 i;
  PRBool changed = PR_FALSE;
  for (i = 0; i < mLineBreaks.Length(); ++i) {
    PRUint32 pos = mLineBreaks[i];
    if (pos >= aStart)
      break;
    newBreaks.AppendElement(pos);
  }
  if (aLineBreakBefore != (i < mLineBreaks.Length() &&
                           mLineBreaks[i] == aStart)) {
    changed = PR_TRUE;
  }
  if (aLineBreakBefore) {
    newBreaks.AppendElement(aStart);
  }
  if (aLineBreakAfter != (i + 1 < mLineBreaks.Length() &&
                          mLineBreaks[i + 1] == aStart + aLength)) {
    changed = PR_TRUE;
  }
  if (aLineBreakAfter) {
    newBreaks.AppendElement(aStart + aLength);
  }
  for (; i < mLineBreaks.Length(); ++i) {
    if (mLineBreaks[i] > aStart + aLength)
      break;
    changed = PR_TRUE;
  }
  if (!changed) {
    if (aAdvanceWidthDelta) {
      *aAdvanceWidthDelta = 0;
    }
    return PR_FALSE;
  }

  newBreaks.AppendElements(mLineBreaks.Elements() + i, mLineBreaks.Length() - i);
  mLineBreaks.SwapElements(newBreaks);

  gfxFloat currentAdvance = GetAdvanceWidth(aStart, aLength, nsnull);
  mFactory->RebuildTextRun(this);
  if (aAdvanceWidthDelta) {
    *aAdvanceWidthDelta = GetAdvanceWidth(aStart, aLength, nsnull) - currentAdvance;
  }
  return PR_TRUE;
}

gfxTextRun*
nsTransformingTextRunFactory::MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                          const gfxTextRunFactory::Parameters* aParams,
                                          gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                          nsStyleContext** aStyles)
{
  PRUnichar* text = nsnull;
  if (!(aFlags & gfxFontGroup::TEXT_IS_PERSISTENT)) {
    text = new PRUnichar[aLength];
    if (!text)
      return nsnull;
    memcpy(text, aString, aLength*sizeof(PRUnichar));
  }
  nsTransformedTextRun* textRun =
    new nsTransformedTextRun(aParams, this, aFontGroup,
                             text ? text : aString, aLength, aFlags, aStyles);
  if (!textRun) {
    delete[] text;
    return nsnull;
  }
  RebuildTextRun(textRun);
  return textRun;
}

gfxTextRun*
nsTransformingTextRunFactory::MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                          const gfxTextRunFactory::Parameters* aParams,
                                          gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                          nsStyleContext** aStyles)
{
  
  
  NS_ConvertASCIItoUTF16 unicodeString(NS_REINTERPRET_CAST(const char*, aString), aLength);
  return MakeTextRun(unicodeString.get(), aLength, aParams, aFontGroup,
                     aFlags & ~(gfxFontGroup::TEXT_IS_PERSISTENT | gfxFontGroup::TEXT_IS_8BIT),
                     aStyles);
}

static PRUint32
CountGlyphs(const gfxTextRun::DetailedGlyph* aDetails) {
  PRUint32 glyphCount;
  for (glyphCount = 0; !aDetails[glyphCount].mIsLastGlyph; ++glyphCount) {
  }
  return glyphCount + 1;
}




static void
AppendTextRun(gfxTextRun* aDest, gfxTextRun* aSrc, PRUint32 aOffset)
{
  PRUint32 numGlyphRuns;
  const gfxTextRun::GlyphRun* glyphRuns = aSrc->GetGlyphRuns(&numGlyphRuns);
  PRUint32 j;
  PRUint32 offset = aOffset;
  for (j = 0; j < numGlyphRuns; ++j) {
    PRUint32 runOffset = glyphRuns[j].mCharacterOffset;
    PRUint32 len =
      (j + 1 < numGlyphRuns ? glyphRuns[j + 1].mCharacterOffset : aSrc->GetLength()) -
      runOffset;
    nsresult rv = aDest->AddGlyphRun(glyphRuns[j].mFont, offset);
    if (NS_FAILED(rv))
      return;

    PRUint32 k;
    for (k = 0; k < len; ++k) {
      gfxTextRun::CompressedGlyph g = aSrc->GetCharacterGlyphs()[runOffset + k];
      if (g.IsComplexCluster()) {
        const gfxTextRun::DetailedGlyph* details = aSrc->GetDetailedGlyphs(runOffset + k);
        aDest->SetDetailedGlyphs(offset, details, CountGlyphs(details));
      } else {
        aDest->SetCharacterGlyph(offset, g);
      }
      ++offset;
    }
  }
  NS_ASSERTION(offset - aOffset == aSrc->GetLength(),
               "Something went wrong in our length calculations...");
}

















static void
MergeCharactersInTextRun(gfxTextRun* aDest, gfxTextRun* aSrc,
                         PRPackedBool* aCharsToMerge)
{
  aDest->ResetGlyphRuns();

  PRUint32 numGlyphRuns;
  const gfxTextRun::GlyphRun* glyphRuns = aSrc->GetGlyphRuns(&numGlyphRuns);
  PRUint32 offset = 0;
  PRUint32 j;
  for (j = 0; j < numGlyphRuns; ++j) {
    PRUint32 runOffset = glyphRuns[j].mCharacterOffset;
    PRUint32 len =
      (j + 1 < numGlyphRuns ? glyphRuns[j + 1].mCharacterOffset : aSrc->GetLength()) -
      runOffset;
    nsresult rv = aDest->AddGlyphRun(glyphRuns[j].mFont, offset);
    if (NS_FAILED(rv))
      return;

    PRUint32 k;
    for (k = 0; k < len; ++k) {
      if (aCharsToMerge[runOffset + k])
        continue;

      gfxTextRun::CompressedGlyph g = aSrc->GetCharacterGlyphs()[runOffset + k];
      if (g.IsSimpleGlyph() || g.IsComplexCluster()) {
        PRUint32 mergedCount = 1;
        PRBool multipleGlyphs = PR_FALSE;
        while (k + mergedCount < len) {
          gfxTextRun::CompressedGlyph h = aSrc->GetCharacterGlyphs()[runOffset + k + mergedCount];
          if (!aCharsToMerge[runOffset + k + mergedCount] &&
              !h.IsClusterContinuation() && !h.IsLigatureContinuation())
            break;
          if (h.IsComplexCluster() || h.IsSimpleGlyph()) {
            multipleGlyphs = PR_TRUE;
          }
          ++mergedCount;
        }
        if (g.IsSimpleGlyph() && !multipleGlyphs) {
          aDest->SetCharacterGlyph(offset, g);
        } else {
          
          nsAutoTArray<gfxTextRun::DetailedGlyph,2> detailedGlyphs;
          PRUint32 m;
          for (m = 0; m < mergedCount; ++m) {
            gfxTextRun::CompressedGlyph h = aSrc->GetCharacterGlyphs()[runOffset + k + m];
            if (h.IsSimpleGlyph()) {
              gfxTextRun::DetailedGlyph* details = detailedGlyphs.AppendElement();
              if (!details)
                return;
              details->mGlyphID = h.GetSimpleGlyph();
              details->mAdvance = h.GetSimpleAdvance();
              details->mXOffset = 0;
              details->mYOffset = 0;
            } else if (h.IsComplexCluster()) {
              const gfxTextRun::DetailedGlyph* srcDetails = aSrc->GetDetailedGlyphs(runOffset + k + m);
              detailedGlyphs.AppendElements(srcDetails, CountGlyphs(srcDetails));
            }
            detailedGlyphs[detailedGlyphs.Length() - 1].mIsLastGlyph = PR_FALSE;
          }
          detailedGlyphs[detailedGlyphs.Length() - 1].mIsLastGlyph = PR_TRUE;
          aDest->SetDetailedGlyphs(offset, detailedGlyphs.Elements(), detailedGlyphs.Length());
        }
      } else {
        aDest->SetCharacterGlyph(offset, g);
      }
      ++offset;
    }
  }
  NS_ASSERTION(offset == aDest->GetLength(), "Bad offset calculations");
}

static gfxTextRunFactory::Parameters
GetParametersForInner(nsTransformedTextRun* aTextRun, PRUint32* aFlags)
{
  gfxTextRunFactory::Parameters params =
    { aTextRun->mRefContext, nsnull, nsnull,
      nsnull, nsnull, PRUint32(aTextRun->GetAppUnitsPerDevUnit())
    };
  *aFlags = aTextRun->GetFlags() & ~gfxFontGroup::TEXT_IS_PERSISTENT;
  return params;
}

void
nsFontVariantTextRunFactory::RebuildTextRun(nsTransformedTextRun* aTextRun)
{
  nsICaseConversion* converter = nsTextTransformer::GetCaseConv();
  if (!converter)
    return;

  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();
  gfxFontStyle fontStyle = *fontGroup->GetStyle();
  fontStyle.size *= 0.8;
  nsRefPtr<gfxFontGroup> smallFont = fontGroup->Copy(&fontStyle);
  if (!smallFont)
    return;

  PRUint32 flags;
  gfxTextRunFactory::Parameters innerParams = GetParametersForInner(aTextRun, &flags);

  PRUint32 length = aTextRun->GetLength();
  const PRUnichar* str = aTextRun->GetTextUnicode();
  nsRefPtr<nsStyleContext>* styles = aTextRun->mStyles.Elements();
  
  nsAutoPtr<gfxTextRun> inner;
  inner = fontGroup->MakeTextRun(str, length, &innerParams, flags);
  if (!inner)
    return;

  nsCaseTransformTextRunFactory uppercaseFactory(nsnull, PR_TRUE);

  aTextRun->ResetGlyphRuns();

  PRUint32 runStart = 0;
  PRPackedBool runIsLowercase = PR_FALSE;
  nsAutoTArray<nsStyleContext*,50> styleArray;
  nsAutoTArray<PRPackedBool,50> canBreakBeforeArray;
  nsAutoTArray<PRUint32,10> lineBreakBeforeArray;

  PRUint32 nextLineBreak = 0;
  PRUint32 i;
  for (i = 0; i <= length; ++i) {
    if (nextLineBreak < aTextRun->mLineBreaks.Length() &&
        aTextRun->mLineBreaks[nextLineBreak] == i) {
      lineBreakBeforeArray.AppendElement(i - runStart);
      ++nextLineBreak;
    }

    PRBool isLowercase = PR_FALSE;
    if (i < length) {
      
      
      if (!inner->IsClusterStart(i))
        continue;

      if (styles[i]->GetStyleFont()->mFont.variant == NS_STYLE_FONT_VARIANT_SMALL_CAPS) {
        PRUnichar ch = str[i];
        PRUnichar ch2;
        converter->ToUpper(ch, &ch2);
        isLowercase = ch != ch2 || ch == SZLIG;
      } else {
        
      }
    }

    if ((i == length || runIsLowercase != isLowercase) && runStart < i) {
      nsAutoPtr<gfxTextRun> child;
      
      innerParams.mInitialBreaks = lineBreakBeforeArray.Elements();
      innerParams.mInitialBreakCount = lineBreakBeforeArray.Length();
      if (runIsLowercase) {
        child = uppercaseFactory.MakeTextRun(str + runStart, i - runStart,
                                             &innerParams, smallFont, flags,
                                             styleArray.Elements());
      } else {
        child = fontGroup->
            MakeTextRun(str + runStart, i - runStart, &innerParams, flags);
      }
      if (!child)
        return;
      
      
      NS_ASSERTION(canBreakBeforeArray.Length() == i - runStart,
                   "lost some break-before values?");
      child->SetPotentialLineBreaks(0, canBreakBeforeArray.Length(), canBreakBeforeArray.Elements());
      AppendTextRun(aTextRun, child, runStart);

      runStart = i;
      runIsLowercase = isLowercase;
      styleArray.Clear();
      canBreakBeforeArray.Clear();
      lineBreakBeforeArray.Clear();
      if (nextLineBreak > 0 && aTextRun->mLineBreaks[nextLineBreak - 1] == i) {
        lineBreakBeforeArray.AppendElement(0);
      }
    }

    if (i < length) {
      styleArray.AppendElement(styles[i]);
      canBreakBeforeArray.AppendElement(aTextRun->CanBreakLineBefore(i));
    }
  }
  NS_ASSERTION(nextLineBreak == aTextRun->mLineBreaks.Length(),
               "lost track of line breaks somehow");
}

void
nsCaseTransformTextRunFactory::RebuildTextRun(nsTransformedTextRun* aTextRun)
{
  nsICaseConversion* converter = nsTextTransformer::GetCaseConv();
  if (!converter)
    return;

  PRUint32 length = aTextRun->GetLength();
  const PRUnichar* str = aTextRun->GetTextUnicode();
  nsRefPtr<nsStyleContext>* styles = aTextRun->mStyles.Elements();

  nsAutoString convertedString;
  nsAutoTArray<PRPackedBool,50> charsToMergeArray;
  nsAutoTArray<nsStyleContext*,50> styleArray;
  nsAutoTArray<PRPackedBool,50> canBreakBeforeArray;
  nsAutoTArray<PRUint32,10> lineBreakBeforeArray;
  PRUint32 nextLineBreak = 0;
  PRUint32 extraCharsCount = 0;

  PRUint32 i;
  for (i = 0; i < length; ++i) {
    PRUnichar ch = str[i];

    charsToMergeArray.AppendElement(PR_FALSE);
    styleArray.AppendElement(styles[i]);
    canBreakBeforeArray.AppendElement(aTextRun->CanBreakLineBefore(i));
    if (nextLineBreak < aTextRun->mLineBreaks.Length() &&
        aTextRun->mLineBreaks[nextLineBreak] == i) {
      lineBreakBeforeArray.AppendElement(i + extraCharsCount);
      ++nextLineBreak;
    }

    PRUint8 style = mAllUppercase ? NS_STYLE_TEXT_TRANSFORM_UPPERCASE
      : styles[i]->GetStyleText()->mTextTransform;
    PRBool extraChar = PR_FALSE;

    switch (style) {
    case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
      converter->ToLower(ch, &ch);
      break;
    case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
      if (ch == SZLIG) {
        convertedString.Append('S');
        extraChar = PR_TRUE;
        ch = 'S';
      } else {
        converter->ToUpper(ch, &ch);
      }
      break;
    case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
      if (aTextRun->CanBreakLineBefore(i)) {
        if (ch == SZLIG) {
          convertedString.Append('S');
          extraChar = PR_TRUE;
          ch = 'S';
        } else {
          converter->ToTitle(ch, &ch);
        }
      }
      break;
    default:
      break;
    }

    convertedString.Append(ch);
    if (extraChar) {
      ++extraCharsCount;
      charsToMergeArray.AppendElement(PR_TRUE);
      styleArray.AppendElement(styles[i]);
      canBreakBeforeArray.AppendElement(PR_FALSE);
    }
  }
  if (nextLineBreak < aTextRun->mLineBreaks.Length() &&
      aTextRun->mLineBreaks[nextLineBreak] == length) {
    lineBreakBeforeArray.AppendElement(length + extraCharsCount);
    ++nextLineBreak;
  }
  NS_ASSERTION(nextLineBreak == aTextRun->mLineBreaks.Length(),
               "lost track of line breaks somehow");

  PRUint32 flags;
  gfxTextRunFactory::Parameters innerParams = GetParametersForInner(aTextRun, &flags);
  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();

  nsAutoPtr<gfxTextRun> child;
  
  innerParams.mInitialBreaks = lineBreakBeforeArray.Elements();
  innerParams.mInitialBreakCount = lineBreakBeforeArray.Length();
  
  flags |= gfxFontGroup::TEXT_IS_PERSISTENT;
  if (mInnerTransformingTextRunFactory) {
    child = mInnerTransformingTextRunFactory->MakeTextRun(
        convertedString.BeginReading(), convertedString.Length(),
        &innerParams, fontGroup, flags, styleArray.Elements());
  } else {
    child = fontGroup->MakeTextRun(
        convertedString.BeginReading(), convertedString.Length(), &innerParams,
        flags);
  }
  if (!child)
    return;
  
  
  NS_ASSERTION(convertedString.Length() == canBreakBeforeArray.Length(),
               "Dropped characters or break-before values somewhere!");
  child->SetPotentialLineBreaks(0, canBreakBeforeArray.Length(), canBreakBeforeArray.Elements());
  
  MergeCharactersInTextRun(aTextRun, child, charsToMergeArray.Elements());
}
