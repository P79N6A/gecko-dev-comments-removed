




































#include "nsTextFrameUtils.h"

#include "nsContentUtils.h"
#include "nsIWordBreaker.h"
#include "gfxFont.h"
#include "nsTextTransformer.h"
#include "nsCompressedCharMap.h"
#include "nsUnicharUtils.h"
















#include "punct_marks.ccmap"
DEFINE_CCMAP(gPuncCharsCCMap, const);
  
PRBool
nsTextFrameUtils::IsPunctuationMark(PRUnichar aChar)
{
  return CCMAP_HAS_CHAR(gPuncCharsCCMap, aChar);
}

static PRBool IsDiscardable(PRUnichar ch, PRUint32* aFlags)
{
  
  
  
  if (ch == CH_SHY) {
    *aFlags |= nsTextFrameUtils::TEXT_HAS_SHY;
    return PR_TRUE;
  }
  if ((ch & 0xFF00) != 0x2000) {
    
    return PR_FALSE;
  }
  return IS_BIDI_CONTROL(ch);
}

static PRBool IsDiscardable(PRUint8 ch, PRUint32* aFlags)
{
  if (ch == CH_SHY) {
    *aFlags |= nsTextFrameUtils::TEXT_HAS_SHY;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRUnichar*
nsTextFrameUtils::TransformText(const PRUnichar* aText, PRUint32 aLength,
                                PRUnichar* aOutput,
                                PRBool aCompressWhitespace,
                                PRPackedBool* aIncomingWhitespace,
                                gfxSkipCharsBuilder* aSkipChars,
                                PRUint32* aAnalysisFlags)
{
  
  PRUint32 flags = TEXT_HAS_NON_ASCII;
  PRUnichar* outputStart = aOutput;

  if (!aCompressWhitespace) {
    
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUnichar ch = *aText++;
      if (ch == '\t') {
        flags |= TEXT_HAS_TAB|TEXT_WAS_TRANSFORMED;
        aSkipChars->KeepChar();
        *aOutput++ = ' ';
      } else if (IsDiscardable(ch, &flags)) {
        aSkipChars->SkipChar();
      } else {
        aSkipChars->KeepChar();
        if (ch == CH_NBSP) {
          ch = ' ';
          flags |= TEXT_WAS_TRANSFORMED;
        } else if (IS_SURROGATE(ch)) {
          flags |= gfxTextRunFactory::TEXT_HAS_SURROGATES;
        }
        *aOutput++ = ch;
      }
    }
  } else {
    PRBool inWhitespace = *aIncomingWhitespace;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUnichar ch = *aText++;
      PRBool nowInWhitespace;
      if (ch == ' ' &&
          (i + 1 >= aLength ||
           !IsSpaceCombiningSequenceTail(&aText[1], aLength - (i + 1)))) {
        nowInWhitespace = PR_TRUE;
      } else if (ch == '\n') {
        if (i > 0 && IS_CJ_CHAR(aText[-1]) &&
            i + 1 < aLength && IS_CJ_CHAR(aText[1])) {
          
          
          aSkipChars->SkipChar();
          continue;
        }
        nowInWhitespace = PR_TRUE;
      } else {
        nowInWhitespace = ch == '\t';
      }

      if (!nowInWhitespace) {
        if (IsDiscardable(ch, &flags)) {
          aSkipChars->SkipChar();
          nowInWhitespace = inWhitespace;
        } else {
          if (ch == CH_NBSP) {
            ch = ' ';
            flags |= TEXT_WAS_TRANSFORMED;
          } else if (IS_SURROGATE(ch)) {
            flags |= gfxTextRunFactory::TEXT_HAS_SURROGATES;
          }
          *aOutput++ = ch;
          aSkipChars->KeepChar();
        }
      } else {
        if (inWhitespace) {
          aSkipChars->SkipChar();
        } else {
          if (ch != ' ') {
            flags |= TEXT_WAS_TRANSFORMED;
          }
          *aOutput++ = ' ';
          aSkipChars->KeepChar();
        }
      }
      inWhitespace = nowInWhitespace;
    }
  }

  if (outputStart < aOutput) {
    *aIncomingWhitespace = aOutput[-1] == ' ';
  }
  if (outputStart + aLength != aOutput) {
    flags |= TEXT_WAS_TRANSFORMED;
  }
  *aAnalysisFlags = flags;
  return aOutput;
}

PRUint8*
nsTextFrameUtils::TransformText(const PRUint8* aText, PRUint32 aLength,
                                PRUint8* aOutput,
                                PRBool aCompressWhitespace,
                                PRPackedBool* aIncomingWhitespace,
                                gfxSkipCharsBuilder* aSkipChars,
                                PRUint32* aAnalysisFlags)
{
  PRUint32 flags = 0;
  PRUint8 allBits = 0;
  PRUint8* outputStart = aOutput;

  if (!aCompressWhitespace) {
    
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUint8 ch = *aText++;
      allBits |= ch;
      if (ch == '\t') {
        flags |= TEXT_HAS_TAB|TEXT_WAS_TRANSFORMED;
        aSkipChars->KeepChar();
        *aOutput++ = ' ';
      } else if (IsDiscardable(ch, &flags)) {
        aSkipChars->SkipChar();
      } else {
        aSkipChars->KeepChar();
        if (ch == CH_NBSP) {
          ch = ' ';
          flags |= TEXT_WAS_TRANSFORMED;
        }
        *aOutput++ = ch;
      }
    }
  } else {
    PRBool inWhitespace = *aIncomingWhitespace;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUint8 ch = *aText++;
      allBits |= ch;
      PRBool nowInWhitespace = ch == ' ' || ch == '\t' || ch == '\n';
      if (!nowInWhitespace) {
        if (IsDiscardable(ch, &flags)) {
          aSkipChars->SkipChar();
          nowInWhitespace = inWhitespace;
        } else {
          if (ch == CH_NBSP) {
            ch = ' ';
            flags |= TEXT_WAS_TRANSFORMED;
          }
          *aOutput++ = ch;
          aSkipChars->KeepChar();
        }
      } else {
        if (inWhitespace) {
          aSkipChars->SkipChar();
        } else {
          if (ch != ' ') {
            flags |= TEXT_WAS_TRANSFORMED;
          }
          *aOutput++ = ' ';
          aSkipChars->KeepChar();
        }
      }
      inWhitespace = nowInWhitespace;
    }
  }

  if (outputStart < aOutput) {
    *aIncomingWhitespace = aOutput[-1] == ' ';
  }
  if (outputStart + aLength != aOutput) {
    flags |= TEXT_WAS_TRANSFORMED;
  }
  if (allBits & 0x80) {
    flags |= TEXT_HAS_NON_ASCII;
  }
  *aAnalysisFlags = flags;
  return aOutput;
}



enum SimpleCharClass {
  CLASS_ALNUM,
  CLASS_PUNCT,
  CLASS_SPACE
};


static PRBool IsWordBreakerWhitespace(const PRUnichar* aChars, PRInt32 aLength)
{
  PRUnichar ch = aChars[0];
  if (ch == '\t' || ch == '\n' || ch == '\r')
    return PR_TRUE;
  if (ch == ' ' &&
      !nsTextFrameUtils::IsSpaceCombiningSequenceTail(aChars + 1, aLength - 1))
    return PR_TRUE;
  return PR_FALSE;
}


static SimpleCharClass Classify8BitChar(PRUint8 aChar)
{
  if (aChar == ' ' || aChar == '\t' || aChar == '\n' || aChar == '\r')
    return CLASS_SPACE;
  if ((aChar >= 'a' && aChar <= 'z') || (aChar >= 'A' || aChar <= 'Z') ||
      (aChar >= '0' && aChar <= '9') || (aChar >= 128))
    return CLASS_ALNUM;
  return CLASS_PUNCT;
}

PRInt32
nsTextFrameUtils::FindWordBoundary(const nsTextFragment* aText,
                                   gfxTextRun* aTextRun,
                                   gfxSkipCharsIterator* aIterator,
                                   PRInt32 aOffset, PRInt32 aLength,
                                   PRInt32 aPosition, PRInt32 aDirection,
                                   PRBool aBreakBeforePunctuation,
                                   PRBool aBreakAfterPunctuation,
                                   PRBool* aWordIsWhitespace)
{
  
  PRInt32 textLength = aText->GetLength();
  *aWordIsWhitespace = aText->Is2b()
    ? IsWordBreakerWhitespace(aText->Get2b() + aPosition, textLength - aPosition)
    : Classify8BitChar(aText->Get1b()[aPosition]) == CLASS_SPACE;

  PRInt32 nextWordPos; 
  if (aText->Is2b()) {
    nsIWordBreaker* wordBreaker = nsContentUtils::WordBreaker();
    const PRUnichar* text = aText->Get2b();
    PRInt32 pos = aPosition;
    
    
    
    for (;;) {
      nextWordPos = aDirection > 0
        ? wordBreaker->NextWord(text, textLength, pos)
        : wordBreaker->PrevWord(text, textLength, pos);
      if (nextWordPos < 0)
        break;
      if (aTextRun->GetCharFlags(aIterator->ConvertOriginalToSkipped(nextWordPos)) & gfxTextRun::CLUSTER_START)
        break;
      pos = nextWordPos;
    }
  } else {
    const char* text = aText->Get1b();
    SimpleCharClass cl = Classify8BitChar(text[aPosition]);
    nextWordPos = aPosition;
    
    
    do {
      nextWordPos += aDirection;
      if (nextWordPos < aOffset || nextWordPos >= aOffset + aLength) {
        nextWordPos = NS_WORDBREAKER_NEED_MORE_TEXT;
        break;
      }
    } while (Classify8BitChar(text[nextWordPos]) == cl ||
             !(aTextRun->GetCharFlags(aIterator->ConvertOriginalToSkipped(nextWordPos)) & gfxTextRun::CLUSTER_START));
  }

  
  PRInt32 i;
  PRBool punctPrev = IsPunctuationMark(aText->CharAt(aPosition));
  for (i = aPosition + aDirection;
       i != nextWordPos && i >= aOffset && i < aOffset + aLength;
       i += aDirection) {
    
    PRBool punct = IsPunctuationMark(aText->CharAt(i));
    if (punct != punctPrev &&
        (aTextRun->GetCharFlags(aIterator->ConvertOriginalToSkipped(i)) & gfxTextRun::CLUSTER_START)) {
      PRBool punctIsBefore = aDirection < 0 ? punct : punctPrev;
      if (punctIsBefore ? aBreakAfterPunctuation : aBreakBeforePunctuation)
        break;
    }
    punctPrev = punct;
  }
  if (i < aOffset || i >= aOffset + aLength)
    return -1;
  return i;
}

PRBool nsSkipCharsRunIterator::NextRun() {
  do {
    if (!mRemainingLength)
      return PR_FALSE;
    if (mRunLength) {
      mIterator.AdvanceOriginal(mRunLength);
      NS_ASSERTION(mRunLength > 0, "No characters in run (initial length too large?)");
      if (!mSkipped || mLengthIncludesSkipped) {
        mRemainingLength -= mRunLength;
      }
    }
    PRInt32 length;
    mSkipped = mIterator.IsOriginalCharSkipped(&length);
    mRunLength = PR_MIN(length, mRemainingLength);
  } while (!mVisitSkipped && mSkipped);

  return PR_TRUE;
}
