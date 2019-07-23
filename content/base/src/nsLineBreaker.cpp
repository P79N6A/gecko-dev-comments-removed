





































#include "nsLineBreaker.h"
#include "nsContentUtils.h"
#include "nsILineBreaker.h"

nsLineBreaker::nsLineBreaker()
  : mCurrentWordContainsComplexChar(PR_FALSE),
    mAfterSpace(PR_FALSE)
{
}

nsLineBreaker::~nsLineBreaker()
{
  NS_ASSERTION(mCurrentWord.Length() == 0, "Should have Reset() before destruction!");
}

nsresult
nsLineBreaker::FlushCurrentWord()
{
  nsAutoTArray<PRPackedBool,4000> breakState;
  if (!breakState.AppendElements(mCurrentWord.Length()))
    return NS_ERROR_OUT_OF_MEMORY;

  if (!mCurrentWordContainsComplexChar) {
    
    memset(breakState.Elements(), PR_FALSE, mCurrentWord.Length());
  } else {
    nsContentUtils::LineBreaker()->
      GetJISx4051Breaks(mCurrentWord.Elements(), mCurrentWord.Length(), breakState.Elements());
  }

  PRUint32 i;
  PRUint32 offset = 0;
  for (i = 0; i < mTextItems.Length(); ++i) {
    TextItem* ti = &mTextItems[i];
    NS_ASSERTION(ti->mLength > 0, "Zero length word contribution?");

    if (!(ti->mFlags & BREAK_ALLOW_INITIAL) && ti->mSinkOffset == 0) {
      breakState[offset] = PR_FALSE;
    }
    if (!(ti->mFlags & BREAK_ALLOW_INSIDE)) {
      PRUint32 exclude = ti->mSinkOffset == 0 ? 1 : 0;
      memset(breakState.Elements() + offset + exclude, PR_FALSE, ti->mLength - exclude);
    }

    
    
    
    PRUint32 skipSet = i == 0 ? 1 : 0;
    if (ti->mSink) {
      ti->mSink->SetBreaks(ti->mSinkOffset + skipSet, ti->mLength - skipSet,
                           breakState.Elements() + offset + skipSet);
    }
    offset += ti->mLength;
  }

  mCurrentWord.Clear();
  mTextItems.Clear();
  mCurrentWordContainsComplexChar = PR_FALSE;
  return NS_OK;
}

nsresult
nsLineBreaker::AppendText(nsIAtom* aLangGroup, const PRUnichar* aText, PRUint32 aLength,
                          PRUint32 aFlags, nsILineBreakSink* aSink)
{
  NS_ASSERTION(aLength > 0, "Appending empty text...");

  PRUint32 offset = 0;

  
  if (mCurrentWord.Length() > 0) {
    NS_ASSERTION(!mAfterSpace, "These should not be set");

    while (offset < aLength && !IsSpace(aText[offset])) {
      mCurrentWord.AppendElement(aText[offset]);
      if (!mCurrentWordContainsComplexChar && IsComplexChar(aText[offset])) {
        mCurrentWordContainsComplexChar = PR_TRUE;
      }
      ++offset;
    }

    if (offset > 0) {
      mTextItems.AppendElement(TextItem(aSink, 0, offset, aFlags));
    }

    if (offset == aLength)
      return NS_OK;

    
    nsresult rv = FlushCurrentWord();
    if (NS_FAILED(rv))
      return rv;
  }

  nsAutoTArray<PRPackedBool,4000> breakState;
  if (!breakState.AppendElements(aLength))
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint32 start = offset;
  if (!aSink && !aFlags) {
    
    offset = aLength;
    while (offset > start) {
      --offset;
      if (IsSpace(aText[offset]))
        break;
    }
  }
  PRUint32 wordStart = offset;
  PRBool wordHasComplexChar = PR_FALSE;

  for (;;) {
    PRUnichar ch = aText[offset];
    PRBool isSpace = IsSpace(ch);

    breakState[offset] = mAfterSpace && !isSpace &&
      (aFlags & (offset == 0 ? BREAK_ALLOW_INITIAL : BREAK_ALLOW_INSIDE));
    mAfterSpace = isSpace;

    if (isSpace) {
      if (offset > wordStart && wordHasComplexChar) {
        if (aFlags & BREAK_ALLOW_INSIDE) {
          
          
          PRPackedBool currentStart = breakState[wordStart];
          nsContentUtils::LineBreaker()->
            GetJISx4051Breaks(aText + wordStart, offset - wordStart,
                              breakState.Elements() + wordStart);
          breakState[wordStart] = currentStart;
        }
        wordHasComplexChar = PR_FALSE;
      }

      ++offset;
      if (offset >= aLength)
        break;
      wordStart = offset;
    } else {
      if (!wordHasComplexChar && IsComplexChar(ch)) {
        wordHasComplexChar = PR_TRUE;
      }
      ++offset;
      if (offset >= aLength) {
        
        mCurrentWordContainsComplexChar = wordHasComplexChar;
        PRUint32 len = offset - wordStart;
        PRUnichar* elems = mCurrentWord.AppendElements(len);
        if (!elems)
          return NS_ERROR_OUT_OF_MEMORY;
        memcpy(elems, aText + wordStart, sizeof(PRUnichar)*len);
        mTextItems.AppendElement(TextItem(aSink, wordStart, len, aFlags));
        
        offset = wordStart + 1;
        break;
      }
    }
  }

  if (aSink) {
    aSink->SetBreaks(start, offset - start, breakState.Elements() + start);
  }
  return NS_OK;
}

nsresult
nsLineBreaker::AppendText(nsIAtom* aLangGroup, const PRUint8* aText, PRUint32 aLength,
                          PRUint32 aFlags, nsILineBreakSink* aSink)
{
  NS_ASSERTION(aLength > 0, "Appending empty text...");

  PRUint32 offset = 0;

  
  if (mCurrentWord.Length() > 0) {
    NS_ASSERTION(!mAfterSpace, "These should not be set");

    while (offset < aLength && !IsSpace(aText[offset])) {
      mCurrentWord.AppendElement(aText[offset]);
      ++offset;
    }

    if (offset > 0) {
      mTextItems.AppendElement(TextItem(aSink, 0, offset, aFlags));
    }

    if (offset == aLength) {
      
      return NS_OK;
    }

    
    nsresult rv = FlushCurrentWord();
    if (NS_FAILED(rv))
      return rv;
  }

  nsAutoTArray<PRPackedBool,4000> breakState;
  if (aSink) {
    if (!breakState.AppendElements(aLength))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUint32 start = offset;
  if (!aSink && !aFlags) {
    
    offset = aLength;
    while (offset > start) {
      --offset;
      if (IsSpace(aText[offset]))
        break;
    }
  }
  PRUint32 wordStart = offset;

  for (;;) {
    PRUint8 ch = aText[offset];
    PRBool isSpace = IsSpace(ch);

    if (aSink) {
      breakState[offset] = mAfterSpace && !isSpace &&
        (aFlags & (offset == 0 ? BREAK_ALLOW_INITIAL : BREAK_ALLOW_INSIDE));
    }
    mAfterSpace = isSpace;

    if (isSpace) {
      
      
      ++offset;
      if (offset >= aLength)
        break;
      wordStart = offset;
    } else {
      ++offset;
      if (offset >= aLength) {
        
        mCurrentWordContainsComplexChar = PR_FALSE;
        PRUint32 len = offset - wordStart;
        PRUnichar* elems = mCurrentWord.AppendElements(len);
        if (!elems)
          return NS_ERROR_OUT_OF_MEMORY;
        PRUint32 i;
        for (i = wordStart; i < offset; ++i) {
          elems[i - wordStart] = aText[i];
        }
        mTextItems.AppendElement(TextItem(aSink, wordStart, len, aFlags));
        
        offset = wordStart + 1;
        break;
      }
      
      
    }
  }

  if (aSink) {
    aSink->SetBreaks(start, offset - start, breakState.Elements() + start);
  }
  return NS_OK;
}

nsresult
nsLineBreaker::AppendInvisibleWhitespace() {
  
  nsresult rv = FlushCurrentWord();
  if (NS_FAILED(rv))
    return rv;
  mAfterSpace = PR_TRUE;
  return NS_OK;  
}
