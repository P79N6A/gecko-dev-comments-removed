










































#include "nsTextFragment.h"
#include "nsCRT.h"
#include "nsReadableUtils.h"
#include "nsMemory.h"
#include "nsBidiUtils.h"
#include "nsUnicharUtils.h"
#include "nsUTF8Utils.h"
#include "mozilla/SSE.h"

#define TEXTFRAG_WHITE_AFTER_NEWLINE 50
#define TEXTFRAG_MAX_NEWLINES 7


static char* sSpaceSharedString[TEXTFRAG_MAX_NEWLINES + 1];
static char* sTabSharedString[TEXTFRAG_MAX_NEWLINES + 1];
static char sSingleCharSharedString[256];


nsresult
nsTextFragment::Init()
{
  
  PRUint32 i;
  for (i = 0; i <= TEXTFRAG_MAX_NEWLINES; ++i) {
    sSpaceSharedString[i] = new char[1 + i + TEXTFRAG_WHITE_AFTER_NEWLINE];
    sTabSharedString[i] = new char[1 + i + TEXTFRAG_WHITE_AFTER_NEWLINE];
    NS_ENSURE_TRUE(sSpaceSharedString[i] && sTabSharedString[i],
                   NS_ERROR_OUT_OF_MEMORY);
    sSpaceSharedString[i][0] = ' ';
    sTabSharedString[i][0] = ' ';
    PRUint32 j;
    for (j = 1; j < 1 + i; ++j) {
      sSpaceSharedString[i][j] = '\n';
      sTabSharedString[i][j] = '\n';
    }
    for (; j < (1 + i + TEXTFRAG_WHITE_AFTER_NEWLINE); ++j) {
      sSpaceSharedString[i][j] = ' ';
      sTabSharedString[i][j] = '\t';
    }
  }

  
  for (i = 0; i < 256; ++i) {
    sSingleCharSharedString[i] = i;
  }

  return NS_OK;
}


void
nsTextFragment::Shutdown()
{
  PRUint32  i;
  for (i = 0; i <= TEXTFRAG_MAX_NEWLINES; ++i) {
    delete [] sSpaceSharedString[i];
    delete [] sTabSharedString[i];
    sSpaceSharedString[i] = nsnull;
    sTabSharedString[i] = nsnull;
  }
}

nsTextFragment::~nsTextFragment()
{
  ReleaseText();
  MOZ_COUNT_DTOR(nsTextFragment);
}

void
nsTextFragment::ReleaseText()
{
  if (mState.mLength && m1b && mState.mInHeap) {
    nsMemory::Free(m2b); 
  }

  m1b = nsnull;

  
  mAllBits = 0;
}

nsTextFragment&
nsTextFragment::operator=(const nsTextFragment& aOther)
{
  ReleaseText();

  if (aOther.mState.mLength) {
    if (!aOther.mState.mInHeap) {
      m1b = aOther.m1b; 
    }
    else {
      m2b = static_cast<PRUnichar*>
                       (nsMemory::Clone(aOther.m2b, aOther.mState.mLength *
                                    (aOther.mState.mIs2b ? sizeof(PRUnichar) : sizeof(char))));
    }

    if (m1b) {
      mAllBits = aOther.mAllBits;
    }
  }

  return *this;
}

static inline PRBool
Is8BitUnvectorized(const PRUnichar *str, const PRUnichar *end)
{
#if PR_BYTES_PER_WORD == 4
  const size_t mask = 0xff00ff00;
  const PRUint32 alignMask = 0x3;
  const PRUint32 numUnicharsPerWord = 2;
#elif PR_BYTES_PER_WORD == 8
  const size_t mask = 0xff00ff00ff00ff00;
  const PRUint32 alignMask = 0x7;
  const PRUint32 numUnicharsPerWord = 4;
#else
#error Unknown platform!
#endif

  const PRInt32 len = end - str;
  PRInt32 i = 0;

  
  PRInt32 alignLen =
    NS_MIN(len, PRInt32(((-NS_PTR_TO_INT32(str)) & alignMask) / sizeof(PRUnichar)));
  for (; i < alignLen; i++) {
    if (str[i] > 255)
      return PR_FALSE;
  }

  
  const PRInt32 wordWalkEnd = ((len - i) / numUnicharsPerWord) * numUnicharsPerWord;
  for (; i < wordWalkEnd; i += numUnicharsPerWord) {
    const size_t word = *reinterpret_cast<const size_t*>(str + i);
    if (word & mask)
      return PR_FALSE;
  }

  
  for (; i < len; i++) {
    if (str[i] > 255)
      return PR_FALSE;
  }

  return PR_TRUE;
}

#ifdef MOZILLA_MAY_SUPPORT_SSE2
namespace mozilla {
  namespace SSE2 {
    PRBool Is8Bit(const PRUnichar *str, const PRUnichar *end);
  }
}
#endif

static inline PRBool
Is8Bit(const PRUnichar *str, const PRUnichar *end)
{
#ifdef MOZILLA_MAY_SUPPORT_SSE2
  if (mozilla::supports_sse2()) {
    return mozilla::SSE2::Is8Bit(str, end);
  }
#endif

  return Is8BitUnvectorized(str, end);
}

void
nsTextFragment::SetTo(const PRUnichar* aBuffer, PRInt32 aLength)
{
  ReleaseText();

  if (aLength == 0) {
    return;
  }
  
  PRUnichar firstChar = *aBuffer;
  if (aLength == 1 && firstChar < 256) {
    m1b = sSingleCharSharedString + firstChar;
    mState.mInHeap = PR_FALSE;
    mState.mIs2b = PR_FALSE;
    mState.mLength = 1;

    return;
  }

  const PRUnichar *ucp = aBuffer;
  const PRUnichar *uend = aBuffer + aLength;

  
  if (aLength <= 1 + TEXTFRAG_WHITE_AFTER_NEWLINE + TEXTFRAG_MAX_NEWLINES &&
     (firstChar == ' ' || firstChar == '\n' || firstChar == '\t')) {
    if (firstChar == ' ') {
      ++ucp;
    }

    const PRUnichar* start = ucp;
    while (ucp < uend && *ucp == '\n') {
      ++ucp;
    }
    const PRUnichar* endNewLine = ucp;

    PRUnichar space = ucp < uend && *ucp == '\t' ? '\t' : ' ';
    while (ucp < uend && *ucp == space) {
      ++ucp;
    }

    if (ucp == uend &&
        endNewLine - start <= TEXTFRAG_MAX_NEWLINES &&
        ucp - endNewLine <= TEXTFRAG_WHITE_AFTER_NEWLINE) {
      char** strings = space == ' ' ? sSpaceSharedString : sTabSharedString;
      m1b = strings[endNewLine - start];

      
      if (firstChar != ' ') {
        ++m1b;
      }

      mState.mInHeap = PR_FALSE;
      mState.mIs2b = PR_FALSE;
      mState.mLength = aLength;

      return;        
    }
  }

  
  PRBool need2 = !Is8Bit(ucp, uend);

  if (need2) {
    
    m2b = (PRUnichar *)nsMemory::Clone(aBuffer,
                                       aLength * sizeof(PRUnichar));
    if (!m2b) {
      return;
    }
  } else {
    
    char* buff = (char *)nsMemory::Alloc(aLength * sizeof(char));
    if (!buff) {
      return;
    }

    
    LossyConvertEncoding16to8 converter(buff);
    copy_string(aBuffer, aBuffer+aLength, converter);
    m1b = buff;
  }

  
  mState.mInHeap = PR_TRUE;
  mState.mIs2b = need2;
  mState.mLength = aLength;
}

void
nsTextFragment::CopyTo(PRUnichar *aDest, PRInt32 aOffset, PRInt32 aCount)
{
  NS_ASSERTION(aOffset >= 0, "Bad offset passed to nsTextFragment::CopyTo()!");
  NS_ASSERTION(aCount >= 0, "Bad count passed to nsTextFragment::CopyTo()!");

  if (aOffset < 0) {
    aOffset = 0;
  }

  if (PRUint32(aOffset + aCount) > GetLength()) {
    aCount = mState.mLength - aOffset;
  }

  if (aCount != 0) {
    if (mState.mIs2b) {
      memcpy(aDest, m2b + aOffset, sizeof(PRUnichar) * aCount);
    } else {
      const char *cp = m1b + aOffset;
      const char *end = cp + aCount;
      LossyConvertEncoding8to16 converter(aDest);
      copy_string(cp, end, converter);
    }
  }
}

void
nsTextFragment::Append(const PRUnichar* aBuffer, PRUint32 aLength)
{
  
  
  if (mState.mLength == 0) {
    SetTo(aBuffer, aLength);

    return;
  }

  

  if (mState.mIs2b) {
    
    PRUnichar* buff = (PRUnichar*)nsMemory::Realloc(m2b, (mState.mLength + aLength) * sizeof(PRUnichar));
    if (!buff) {
      return;
    }
    
    memcpy(buff + mState.mLength, aBuffer, aLength * sizeof(PRUnichar));
    mState.mLength += aLength;
    m2b = buff;

    return;
  }

  

  if (!Is8Bit(aBuffer, aBuffer + aLength)) {
    
    
    PRUnichar* buff = (PRUnichar*)nsMemory::Alloc((mState.mLength + aLength) *
                                                  sizeof(PRUnichar));
    if (!buff) {
      return;
    }

    
    LossyConvertEncoding8to16 converter(buff);
    copy_string(m1b, m1b+mState.mLength, converter);

    memcpy(buff + mState.mLength, aBuffer, aLength * sizeof(PRUnichar));

    mState.mLength += aLength;
    mState.mIs2b = PR_TRUE;

    if (mState.mInHeap) {
      nsMemory::Free(m2b);
    }
    m2b = buff;

    mState.mInHeap = PR_TRUE;

    return;
  }

  
  char* buff;
  if (mState.mInHeap) {
    buff = (char*)nsMemory::Realloc(const_cast<char*>(m1b),
                                    (mState.mLength + aLength) * sizeof(char));
    if (!buff) {
      return;
    }
  }
  else {
    buff = (char*)nsMemory::Alloc((mState.mLength + aLength) * sizeof(char));
    if (!buff) {
      return;
    }

    memcpy(buff, m1b, mState.mLength);
    mState.mInHeap = PR_TRUE;
  }

  
  LossyConvertEncoding16to8 converter(buff + mState.mLength);
  copy_string(aBuffer, aBuffer + aLength, converter);

  m1b = buff;
  mState.mLength += aLength;

}



void
nsTextFragment::UpdateBidiFlag(const PRUnichar* aBuffer, PRUint32 aLength)
{
  if (mState.mIs2b && !mState.mIsBidi) {
    const PRUnichar* cp = aBuffer;
    const PRUnichar* end = cp + aLength;
    while (cp < end) {
      PRUnichar ch1 = *cp++;
      PRUint32 utf32Char = ch1;
      if (NS_IS_HIGH_SURROGATE(ch1) &&
          cp < end &&
          NS_IS_LOW_SURROGATE(*cp)) {
        PRUnichar ch2 = *cp++;
        utf32Char = SURROGATE_TO_UCS4(ch1, ch2);
      }
      if (UTF32_CHAR_IS_BIDI(utf32Char) || IS_BIDI_CONTROL_CHAR(utf32Char)) {
        mState.mIsBidi = PR_TRUE;
        break;
      }
    }
  }
}
