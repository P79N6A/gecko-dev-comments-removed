




































#include "nsTextFrameUtils.h"

#include "nsContentUtils.h"
#include "nsIWordBreaker.h"
#include "gfxFont.h"
#include "nsTextTransformer.h"
#include "nsCompressedCharMap.h"
#include "nsUnicharUtils.h"
















#include "punct_marks.ccmap"
DEFINE_CCMAP(gPuncCharsCCMap, const);
  
#define UNICODE_ZWSP 0x200B
  
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
  PRUint32 flags = 0;
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
        }
        *aOutput++ = ch;
      }
    }
    *aIncomingWhitespace = PR_FALSE;
  } else {
    PRBool inWhitespace = *aIncomingWhitespace;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUnichar ch = *aText++;
      PRBool nowInWhitespace;
      if (ch == ' ' &&
          (i + 1 >= aLength ||
           !IsSpaceCombiningSequenceTail(aText, aLength - (i + 1)))) {
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
    *aIncomingWhitespace = inWhitespace;
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
  PRUint8* outputStart = aOutput;

  if (!aCompressWhitespace) {
    
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUint8 ch = *aText++;
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
    *aIncomingWhitespace = PR_FALSE;
  } else {
    PRBool inWhitespace = *aIncomingWhitespace;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUint8 ch = *aText++;
      PRBool nowInWhitespace = ch == ' ' || ch == '\t' || ch == '\n' || ch == '\f';
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
    *aIncomingWhitespace = inWhitespace;
  }

  if (outputStart + aLength != aOutput) {
    flags |= TEXT_WAS_TRANSFORMED;
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
  NS_ASSERTION(aLength > 0, "Can't handle empty string");
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
  NS_ASSERTION(aOffset + aLength <= textLength,
               "Substring out of bounds");
  NS_ASSERTION(aPosition >= aOffset && aPosition < aOffset + aLength,
               "Position out of bounds");
  *aWordIsWhitespace = aText->Is2b()
    ? IsWordBreakerWhitespace(aText->Get2b() + aPosition, textLength - aPosition)
    : Classify8BitChar(aText->Get1b()[aPosition]) == CLASS_SPACE;

  PRInt32 len = 0; 
  if (aText->Is2b()) {
    nsIWordBreaker* wordBreaker = nsContentUtils::WordBreaker();
    const PRUnichar* text = aText->Get2b();
    
    
    
    for (;;) {
      if (aDirection > 0) {
        
        PRInt32 nextWordPos = wordBreaker->NextWord(text, textLength, aPosition + len);
        if (nextWordPos < 0)
          break;
        len = nextWordPos - aPosition - 1;
      } else {
        
        PRInt32 nextWordPos = wordBreaker->PrevWord(text, textLength, aPosition + len);
        if (nextWordPos < 0)
          break;
        len = aPosition - nextWordPos;
      }
      if (aTextRun->IsClusterStart(aIterator->ConvertOriginalToSkipped(aPosition + len*aDirection)))
        break;
    }
  } else {
    const char* text = aText->Get1b();
    SimpleCharClass cl = Classify8BitChar(text[aPosition]);
    
    
    PRInt32 nextWordPos;
    do {
      ++len;
      nextWordPos = aPosition + aDirection*len;
      if (nextWordPos < aOffset || nextWordPos >= aOffset + aLength)
        break;
    } while (Classify8BitChar(text[nextWordPos]) == cl ||
             !aTextRun->IsClusterStart(aIterator->ConvertOriginalToSkipped(nextWordPos)));
  }

  
  PRInt32 i;
  PRBool punctPrev = IsPunctuationMark(aText->CharAt(aPosition));
  for (i = 1; i < len; ++i) {
    PRInt32 pos = aPosition + i*aDirection;
    
    PRBool punct = IsPunctuationMark(aText->CharAt(pos));
    if (punct != punctPrev &&
        aTextRun->IsClusterStart(aIterator->ConvertOriginalToSkipped(pos))) {
      PRBool punctIsBefore = aDirection < 0 ? punct : punctPrev;
      if (punctIsBefore ? aBreakAfterPunctuation : aBreakBeforePunctuation)
        break;
    }
    punctPrev = punct;
  }
  PRInt32 pos = aPosition + i*aDirection;
  if (pos < aOffset || pos >= aOffset + aLength)
    return -1;
  return i;
}

PRBool nsSkipCharsRunIterator::NextRun() {
  do {
    if (mRunLength) {
      mIterator.AdvanceOriginal(mRunLength);
      NS_ASSERTION(mRunLength > 0, "No characters in run (initial length too large?)");
      if (!mSkipped || mLengthIncludesSkipped) {
        mRemainingLength -= mRunLength;
      }
    }
    if (!mRemainingLength)
      return PR_FALSE;
    PRInt32 length;
    mSkipped = mIterator.IsOriginalCharSkipped(&length);
    mRunLength = PR_MIN(length, mRemainingLength);
  } while (!mVisitSkipped && mSkipped);

  return PR_TRUE;
}
