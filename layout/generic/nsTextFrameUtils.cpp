





































#include "nsTextFrameUtils.h"

#include "nsContentUtils.h"
#include "nsIWordBreaker.h"
#include "gfxFont.h"
#include "nsUnicharUtils.h"
#include "nsBidiUtils.h"







#define UNICODE_ZWSP 0x200B
  
static PRBool IsDiscardable(PRUnichar ch, PRUint32* aFlags)
{
  
  
  
  if (ch == CH_SHY) {
    *aFlags |= nsTextFrameUtils::TEXT_HAS_SHY;
    return PR_TRUE;
  }
  if ((ch & 0xFF00) != 0x2000) {
    
    return PR_FALSE;
  }
  return IS_BIDI_CONTROL_CHAR(ch);
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
                                CompressionMode aCompression,
                                PRUint8* aIncomingFlags,
                                gfxSkipCharsBuilder* aSkipChars,
                                PRUint32* aAnalysisFlags)
{
  PRUint32 flags = 0;
  PRUnichar* outputStart = aOutput;

  PRBool lastCharArabic = PR_FALSE;

  if (aCompression == COMPRESS_NONE) {
    
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUnichar ch = *aText++;
      if (IsDiscardable(ch, &flags)) {
        aSkipChars->SkipChar();
      } else {
        aSkipChars->KeepChar();
        if (ch == '\t') {
          flags |= TEXT_HAS_TAB;
        } else if (ch != ' ' && ch != '\n') {
          
          lastCharArabic = IS_ARABIC_CHAR(ch);
        }
        *aOutput++ = ch;
      }
    }
    if (lastCharArabic) {
      *aIncomingFlags |= INCOMING_ARABICCHAR;
    } else {
      *aIncomingFlags &= ~INCOMING_ARABICCHAR;
    }
    *aIncomingFlags &= ~INCOMING_WHITESPACE;
  } else {
    PRBool inWhitespace = (*aIncomingFlags & INCOMING_WHITESPACE) != 0;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUnichar ch = *aText++;
      PRBool nowInWhitespace;
      if (ch == ' ' &&
          (i + 1 >= aLength ||
           !IsSpaceCombiningSequenceTail(aText, aLength - (i + 1)))) {
        nowInWhitespace = PR_TRUE;
      } else if (ch == '\n' && aCompression == COMPRESS_WHITESPACE_NEWLINE) {
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
          *aOutput++ = ch;
          aSkipChars->KeepChar();
          lastCharArabic = IS_ARABIC_CHAR(ch);
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
    if (lastCharArabic) {
      *aIncomingFlags |= INCOMING_ARABICCHAR;
    } else {
      *aIncomingFlags &= ~INCOMING_ARABICCHAR;
    }
    if (inWhitespace) {
      *aIncomingFlags |= INCOMING_WHITESPACE;
    } else {
      *aIncomingFlags &= ~INCOMING_WHITESPACE;
    }
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
                                CompressionMode aCompression,
                                PRUint8* aIncomingFlags,
                                gfxSkipCharsBuilder* aSkipChars,
                                PRUint32* aAnalysisFlags)
{
  PRUint32 flags = 0;
  PRUint8* outputStart = aOutput;

  if (aCompression == COMPRESS_NONE) {
    
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUint8 ch = *aText++;
      if (IsDiscardable(ch, &flags)) {
        aSkipChars->SkipChar();
      } else {
        aSkipChars->KeepChar();
        if (ch == '\t') {
          flags |= TEXT_HAS_TAB;
        }
        *aOutput++ = ch;
      }
    }
    *aIncomingFlags &= ~(INCOMING_ARABICCHAR | INCOMING_WHITESPACE);
  } else {
    PRBool inWhitespace = (*aIncomingFlags & INCOMING_WHITESPACE) != 0;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      PRUint8 ch = *aText++;
      PRBool nowInWhitespace = ch == ' ' || ch == '\t' ||
        (ch == '\n' && aCompression == COMPRESS_WHITESPACE_NEWLINE);
      if (!nowInWhitespace) {
        if (IsDiscardable(ch, &flags)) {
          aSkipChars->SkipChar();
          nowInWhitespace = inWhitespace;
        } else {
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
    *aIncomingFlags &= ~INCOMING_ARABICCHAR;
    if (inWhitespace) {
      *aIncomingFlags |= INCOMING_WHITESPACE;
    } else {
      *aIncomingFlags &= ~INCOMING_WHITESPACE;
    }
  }

  if (outputStart + aLength != aOutput) {
    flags |= TEXT_WAS_TRANSFORMED;
  }
  *aAnalysisFlags = flags;
  return aOutput;
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
    mRunLength = NS_MIN(length, mRemainingLength);
  } while (!mVisitSkipped && mSkipped);

  return PR_TRUE;
}
