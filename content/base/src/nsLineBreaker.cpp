





































#include "nsLineBreaker.h"
#include "nsContentUtils.h"
#include "nsILineBreaker.h"

#define UNICODE_ZWSP 0x200b

static inline int
IS_SPACE(PRUnichar u)
{
  return u == 0x0020 || u == UNICODE_ZWSP;
}

static inline int
IS_SPACE(PRUint8 u)
{
  return u == 0x0020;
}

static inline int
IS_CJK_CHAR(PRUnichar u)
{
  return (0x1100 <= u && u <= 0x11ff) ||
         (0x2e80 <= u && u <= 0xd7ff) ||
         (0xf900 <= u && u <= 0xfaff) ||
         (0xff00 <= u && u <= 0xffef);
}

nsLineBreaker::nsLineBreaker()
  : mCurrentWordContainsCJK(PR_FALSE),
    mBreakBeforeNonWhitespace(PR_FALSE)
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

  if (!mCurrentWordContainsCJK) {
    
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

    if (!(ti->mFlags & BREAK_NONWHITESPACE_BEFORE) && ti->mSinkOffset == 0) {
      breakState[offset] = PR_FALSE;
    }
    if (!(ti->mFlags & BREAK_NONWHITESPACE_INSIDE)) {
      PRUint32 exclude = ti->mSinkOffset == 0 ? 1 : 0;
      memset(breakState.Elements() + offset + exclude, PR_FALSE, ti->mLength - exclude);
    }

    
    
    
    PRUint32 skipSet = i == 0 ? 1 : 0;
    ti->mSink->SetBreaks(ti->mSinkOffset + skipSet, ti->mLength - skipSet,
                         breakState.Elements() + offset + skipSet);
    offset += ti->mLength;
  }

  mCurrentWord.Clear();
  mTextItems.Clear();
  mCurrentWordContainsCJK = PR_FALSE;
  return NS_OK;
}

nsresult
nsLineBreaker::AppendText(nsIAtom* aLangGroup, const PRUnichar* aText, PRUint32 aLength,
                          PRUint32 aFlags, nsILineBreakSink* aSink)
{
  if (aLength == 0) {
    
    nsresult rv = FlushCurrentWord();
    if (NS_FAILED(rv))
      return rv;
    mBreakBeforeNonWhitespace = (aFlags & BREAK_WHITESPACE_END) != 0;
    return NS_OK;
  }

  PRUint32 offset = 0;

  
  if (mCurrentWord.Length() > 0) {
    NS_ASSERTION(!mBreakBeforeNonWhitespace, "These should not be set");

    while (offset < aLength && !IS_SPACE(aText[offset])) {
      mCurrentWord.AppendElement(aText[offset]);
      if (!mCurrentWordContainsCJK && IS_CJK_CHAR(aText[offset])) {
        mCurrentWordContainsCJK = PR_TRUE;
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
  PRUint32 wordStart = offset;
  PRBool wordHasCJK = PR_FALSE;

  PRBool breakNextIfNonWhitespace = mBreakBeforeNonWhitespace;
  for (;;) {
    PRUnichar ch = aText[offset];
    PRBool isSpace = IS_SPACE(ch);

    breakState[offset] = breakNextIfNonWhitespace && !isSpace;
    breakNextIfNonWhitespace = PR_FALSE;

    if (isSpace) {
      if (offset > wordStart && wordHasCJK) {
        if (aFlags & BREAK_NONWHITESPACE_INSIDE) {
          
          
          PRPackedBool currentStart = breakState[wordStart];
          nsContentUtils::LineBreaker()->
            GetJISx4051Breaks(aText + wordStart, offset - wordStart,
                              breakState.Elements() + wordStart);
          breakState[wordStart] = currentStart;
        }
        wordHasCJK = PR_FALSE;
      }

      if (aFlags & BREAK_WHITESPACE_END) {
        breakNextIfNonWhitespace = PR_TRUE;
      }
      ++offset;
      if (offset >= aLength)
        break;
      wordStart = offset;
    } else {
      if (!wordHasCJK && IS_CJK_CHAR(ch)) {
        wordHasCJK = PR_TRUE;
      }
      ++offset;
      if (offset >= aLength) {
        
        mCurrentWordContainsCJK = wordHasCJK;
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

  aSink->SetBreaks(start, offset - start, breakState.Elements() + start);
  mBreakBeforeNonWhitespace = breakNextIfNonWhitespace;
  return NS_OK;
}

nsresult
nsLineBreaker::AppendText(nsIAtom* aLangGroup, const PRUint8* aText, PRUint32 aLength,
                          PRUint32 aFlags, nsILineBreakSink* aSink)
{
  if (aLength == 0) {
    
    nsresult rv = FlushCurrentWord();
    if (NS_FAILED(rv))
      return rv;
    mBreakBeforeNonWhitespace = (aFlags & BREAK_WHITESPACE_END) != 0;
    return NS_OK;
  }

  PRUint32 offset = 0;

  
  if (mCurrentWord.Length() > 0) {
    NS_ASSERTION(!mBreakBeforeNonWhitespace, "These should not be set");

    while (offset < aLength && !IS_SPACE(aText[offset])) {
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
  if (!breakState.AppendElements(aLength))
    return NS_ERROR_OUT_OF_MEMORY;

  PRUint32 start = offset;
  PRUint32 wordStart = offset;

  PRBool breakNextIfNonWhitespace = mBreakBeforeNonWhitespace;
  for (;;) {
    PRUint8 ch = aText[offset];
    PRBool isSpace = IS_SPACE(ch);

    breakState[offset] = breakNextIfNonWhitespace && !isSpace;
    breakNextIfNonWhitespace = PR_FALSE;

    if (isSpace) {
      if (aFlags & BREAK_WHITESPACE_END) {
        breakNextIfNonWhitespace = PR_TRUE;
      }
      ++offset;
      if (offset >= aLength)
        break;
      wordStart = offset;
    } else {
      ++offset;
      if (offset >= aLength) {
        
        mCurrentWordContainsCJK = PR_FALSE;
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

  aSink->SetBreaks(start, offset - start, breakState.Elements() + start);
  mBreakBeforeNonWhitespace = breakNextIfNonWhitespace;
  return NS_OK;
}
