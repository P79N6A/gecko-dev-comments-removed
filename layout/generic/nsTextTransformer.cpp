










































#include <ctype.h>
#include "nsCOMPtr.h"
#include "nsTextTransformer.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsStyleConsts.h"
#include "nsILineBreaker.h"
#include "nsIWordBreaker.h"
#include "nsIServiceManager.h"
#include "nsUnicharUtilCIID.h"
#include "nsUnicharUtils.h"
#include "nsICaseConversion.h"
#include "prenv.h"
#include "nsGkAtoms.h"

nsICaseConversion* nsTextTransformer::gCaseConv = nsnull;
PRBool nsTextTransformer::sWordSelectListenerPrefChecked = PR_FALSE;
PRBool nsTextTransformer::sWordSelectEatSpaceAfter = PR_FALSE;
PRBool nsTextTransformer::sWordSelectStopAtPunctuation = PR_FALSE;
static const char kWordSelectEatSpaceAfterPref[] = "layout.word_select.eat_space_to_next_word";
static const char kWordSelectStopAtPunctuationPref[] = "layout.word_select.stop_at_punctuation";


int
nsTextTransformer::WordSelectPrefCallback(const char* aPref, void* aClosure)
{
  sWordSelectEatSpaceAfter = nsContentUtils::GetBoolPref(kWordSelectEatSpaceAfterPref);
  sWordSelectStopAtPunctuation = nsContentUtils::GetBoolPref(kWordSelectStopAtPunctuationPref);

  return 0;
}

nsAutoTextBuffer::nsAutoTextBuffer()
  : mBuffer(mAutoBuffer),
    mBufferLen(NS_TEXT_TRANSFORMER_AUTO_WORD_BUF_SIZE)
{
}

nsAutoTextBuffer::~nsAutoTextBuffer()
{
  if (mBuffer && (mBuffer != mAutoBuffer)) {
    delete [] mBuffer;
  }
}

nsresult
nsAutoTextBuffer::GrowBy(PRInt32 aAtLeast, PRBool aCopyToHead)
{
  PRInt32 newSize = mBufferLen * 2;
  if (newSize < mBufferLen + aAtLeast) {
    newSize = mBufferLen + aAtLeast + 100;
  }
  return GrowTo(newSize, aCopyToHead);
}

nsresult
nsAutoTextBuffer::GrowTo(PRInt32 aNewSize, PRBool aCopyToHead)
{
  if (aNewSize > mBufferLen) {
    PRUnichar* newBuffer = new PRUnichar[aNewSize];
    if (!newBuffer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(&newBuffer[aCopyToHead ? 0 : mBufferLen],
           mBuffer, sizeof(PRUnichar) * mBufferLen);
    if (mBuffer != mAutoBuffer) {
      delete [] mBuffer;
    }
    mBuffer = newBuffer;
    mBufferLen = aNewSize;
  }
  return NS_OK;
}



nsresult
nsTextTransformer::Initialize()
{
  
  if ( !sWordSelectListenerPrefChecked ) {
    sWordSelectListenerPrefChecked = PR_TRUE;

    sWordSelectEatSpaceAfter =
      nsContentUtils::GetBoolPref(kWordSelectEatSpaceAfterPref);
    sWordSelectStopAtPunctuation =
      nsContentUtils::GetBoolPref(kWordSelectStopAtPunctuationPref);

    nsContentUtils::RegisterPrefCallback(kWordSelectEatSpaceAfterPref,
                                         WordSelectPrefCallback, nsnull);
    nsContentUtils::RegisterPrefCallback(kWordSelectStopAtPunctuationPref,
                                         WordSelectPrefCallback, nsnull);
  }

  return NS_OK;
}

nsresult
nsTextTransformer::EnsureCaseConv()
{
  nsresult res = NS_OK;
  if (!gCaseConv) {
    res = CallGetService(NS_UNICHARUTIL_CONTRACTID, &gCaseConv);
    NS_ASSERTION( NS_SUCCEEDED(res), "cannot get UnicharUtil");
    NS_ASSERTION( gCaseConv != NULL, "cannot get UnicharUtil");
  }
  return res;
}

nsICaseConversion*
nsTextTransformer::GetCaseConv()
{
  EnsureCaseConv();
  return gCaseConv;
}

void
nsTextTransformer::Shutdown()
{
  nsContentUtils::UnregisterPrefCallback(kWordSelectEatSpaceAfterPref,
                                         WordSelectPrefCallback, nsnull);
  nsContentUtils::UnregisterPrefCallback(kWordSelectStopAtPunctuationPref,
                                         WordSelectPrefCallback, nsnull);

  NS_IF_RELEASE(gCaseConv);
}


#define MAX_UNIBYTE 127

nsTextTransformer::nsTextTransformer(nsPresContext* aPresContext)
  : mFrag(nsnull),
    mOffset(0),
    mMode(eNormal),
    mBufferPos(0),
    mTextTransform(NS_STYLE_TEXT_TRANSFORM_NONE),
    mFlags(0)
{
  MOZ_COUNT_CTOR(nsTextTransformer);

  mLanguageSpecificTransformType =
    aPresContext->LanguageSpecificTransformType();

#ifdef IBMBIDI
  mPresContext = aPresContext;
#endif
  
#ifdef DEBUG
  static PRBool firstTime = PR_TRUE;
  if (firstTime) {
    firstTime = PR_FALSE;
    SelfTest(aPresContext);
  }
#endif
}

nsTextTransformer::~nsTextTransformer()
{
  MOZ_COUNT_DTOR(nsTextTransformer);
}

nsresult
nsTextTransformer::Init(nsIFrame* aFrame,
                        nsIContent* aContent,
                        PRInt32 aStartingOffset,
                        PRBool aForceArabicShaping,
                        PRBool aLeaveAsAscii)
{
  












  if (mPresContext->BidiEnabled()) {
    SetFrameIsRTL(NS_GET_EMBEDDING_LEVEL(aFrame) & 1);
    mCharType = (nsCharType)NS_PTR_TO_INT32(mPresContext->PropertyTable()->GetProperty(aFrame, nsGkAtoms::charType));
    if (mCharType == eCharType_RightToLeftArabic) {
      if (aForceArabicShaping) {
        SetNeedsArabicShaping(PR_TRUE);
      }
      else {
        if (!mPresContext->IsBidiSystem()) {
          SetNeedsArabicShaping(PR_TRUE);
        }
      }
    }
    SetNeedsNumericShaping(PR_TRUE);
  }

  
  mFrag = aContent->GetText();
  if (mFrag) {
    
    if (aStartingOffset < 0) {
      NS_WARNING("bad starting offset");
      aStartingOffset = 0;
    }
    else if (aStartingOffset > mFrag->GetLength()) {
      NS_WARNING("bad starting offset");
      aStartingOffset = mFrag->GetLength();
    }
    mOffset = aStartingOffset;

    
    const nsStyleText* styleText = aFrame->GetStyleText();
    if (NS_STYLE_WHITESPACE_PRE == styleText->mWhiteSpace) {
      mMode = ePreformatted;
    }
    else if (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == styleText->mWhiteSpace) {
      mMode = ePreWrap;
    }
    mTextTransform = styleText->mTextTransform;
    
    if (aLeaveAsAscii) { 
      SetLeaveAsAscii(PR_TRUE);	    
      
      
      if (mFrag->Is2b() || (eNormal != mMode) ||
          (mLanguageSpecificTransformType !=
           eLanguageSpecificTransformType_None))
        
        SetLeaveAsAscii(PR_FALSE);           
    } 
    else 
      SetLeaveAsAscii(PR_FALSE);
  }
  return NS_OK;
}




PRInt32
nsTextTransformer::ScanNormalWhiteSpace_F(PRInt32 aFragLen)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;

  for (; offset < aFragLen; offset++) {
    PRUnichar ch = frag->CharAt(offset);
    if (!XP_IS_SPACE(ch)) {
      
      
      if (!IS_DISCARDED(ch)) {
        break;
      }
    }
  }

  
  if (mBufferPos >= mTransformBuf.mBufferLen) {
    mTransformBuf.GrowBy(128);
  }

  if (TransformedTextIsAscii()) {
    unsigned char*  bp = (unsigned char*)mTransformBuf.mBuffer;
    bp[mBufferPos++] = ' ';
  } else {
    mTransformBuf.mBuffer[mBufferPos++] = PRUnichar(' ');
  }
  return offset;
}
  
void
nsTextTransformer::ConvertTransformedTextToUnicode()
{
  
  PRInt32         lastChar = mBufferPos - 1;
  unsigned char*  cp1 = (unsigned char*)mTransformBuf.mBuffer + lastChar;
  PRUnichar*      cp2 = mTransformBuf.mBuffer + lastChar;
  
  NS_ASSERTION(mTransformBuf.mBufferLen >= mBufferPos,
               "transform buffer is too small");
  for (PRInt32 count = mBufferPos; count > 0; count--) {
    *cp2-- = PRUnichar(*cp1--);
  }
}


PRInt32
nsTextTransformer::ScanNormalAsciiText_F(PRInt32  aFragLen,
                                         PRInt32* aWordLen,
                                         PRBool*  aWasTransformed)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRInt32 prevBufferPos = mBufferPos;
  const unsigned char* cp = (const unsigned char*)frag->Get1b() + offset;
  union {
    unsigned char* bp1;
    PRUnichar* bp2;
  };
  bp2 = mTransformBuf.GetBuffer();
  if (TransformedTextIsAscii()) {
    bp1 += mBufferPos;
  } else {
    bp2 += mBufferPos;
  }

  for (; offset < aFragLen; offset++) {
    unsigned char ch = *cp++;
    if (XP_IS_SPACE(ch)) {
      break;
    }
    if (CH_NBSP == ch) {
      ch = ' ';
      *aWasTransformed = PR_TRUE;
    }
    else if (IS_DISCARDED(ch)) {
      
      continue;
    }
    if (ch > MAX_UNIBYTE) {
      
      
      SetHasMultibyte(PR_TRUE);        
		
      if (TransformedTextIsAscii()) { 
        SetTransformedTextIsAscii(PR_FALSE);
        *aWasTransformed = PR_TRUE;

        
        if (mBufferPos > 0) {
          ConvertTransformedTextToUnicode();
          bp2 = mTransformBuf.GetBuffer() + mBufferPos;
        }
      }
    }
    if (mBufferPos >= mTransformBuf.mBufferLen) {
      nsresult rv = mTransformBuf.GrowBy(128);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp2 = mTransformBuf.GetBuffer();
      if (TransformedTextIsAscii()) {
        bp1 += mBufferPos;
      } else {
        bp2 += mBufferPos;
      }
    }
    if (TransformedTextIsAscii()) {
      *bp1++ = ch;
    } else {
      *bp2++ = PRUnichar(ch);
    }
    mBufferPos++;
  }

  *aWordLen = mBufferPos - prevBufferPos;
  return offset;
}

PRInt32
nsTextTransformer::ScanNormalAsciiText_F_ForWordBreak(PRInt32  aFragLen,
                                                      PRInt32* aWordLen,
                                                      PRBool*  aWasTransformed,
                                                      PRBool   aIsKeyboardSelect)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRInt32 prevBufferPos = mBufferPos;
  PRBool breakAfterThis = PR_FALSE;
  const unsigned char* cp = (const unsigned char*)frag->Get1b() + offset;
  union {
    unsigned char* bp1;
    PRUnichar* bp2;
  };
  bp2 = mTransformBuf.GetBuffer();
  if (TransformedTextIsAscii()) {
    bp1 += mBufferPos;
  } else {
    bp2 += mBufferPos;
  }
  PRBool readingAlphaNumeric = PR_TRUE; 

  
  
  
  
  
  if (sWordSelectStopAtPunctuation && offset < aFragLen)
    readingAlphaNumeric = isalnum((unsigned char)*cp) || !IS_ASCII_CHAR(*cp);
  
  for (; offset < aFragLen && !breakAfterThis; offset++) {
    unsigned char ch = *cp++;
    if (CH_NBSP == ch) {
      ch = ' ';
      *aWasTransformed = PR_TRUE;
      if (offset == mOffset)
        breakAfterThis = PR_TRUE;
      else
        break;
    }
    else if (XP_IS_SPACE(ch)) {
      break;
    }
    else if (sWordSelectStopAtPunctuation && 
             readingAlphaNumeric && !isalnum(ch) && IS_ASCII_CHAR(ch)) {
      if (!aIsKeyboardSelect)
        break;
      
      
      readingAlphaNumeric = PR_FALSE;
    }
    else if (sWordSelectStopAtPunctuation && 
            !readingAlphaNumeric && (isalnum(ch) || !IS_ASCII_CHAR(ch))) {
      
      break;
    }
    else if (IS_DISCARDED(ch)) {
      
      continue;
    }
    if (ch > MAX_UNIBYTE) {
      
      
      SetHasMultibyte(PR_TRUE);

      if (TransformedTextIsAscii()) {
        SetTransformedTextIsAscii(PR_FALSE);
        *aWasTransformed = PR_TRUE;

        
        if (mBufferPos > 0) {
          ConvertTransformedTextToUnicode();
          bp2 = mTransformBuf.GetBuffer() + mBufferPos;
        }
      }
    }
    if (mBufferPos >= mTransformBuf.mBufferLen) {
      nsresult rv = mTransformBuf.GrowBy(128);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp2 = mTransformBuf.GetBuffer();
      if (TransformedTextIsAscii()) {
        bp1 += mBufferPos;
      } else {
        bp2 += mBufferPos;
      }
    }
    if (TransformedTextIsAscii()) {
      *bp1++ = ch;
    } else {
      *bp2++ = PRUnichar(ch);
    }
    mBufferPos++;
  }

  *aWordLen = mBufferPos - prevBufferPos;
  return offset;
}



PRInt32
nsTextTransformer::ScanNormalUnicodeText_F(PRInt32  aFragLen,
                                           PRBool   aForLineBreak,
                                           PRInt32* aWordLen,
                                           PRBool*  aWasTransformed)
{
  const nsTextFragment* frag = mFrag;
  const PRUnichar* cp0 = frag->Get2b();
  PRInt32 offset = mOffset;

  PRUnichar firstChar = frag->CharAt(offset++);

#ifdef IBMBIDI
  
  
  while (offset < aFragLen && IS_BIDI_CONTROL(firstChar) ) {
    firstChar = frag->CharAt(offset++);
  }
#endif 

  if (firstChar > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);

  
  
  PRInt32 numChars = 1;
  if (offset < aFragLen) {
    const PRUnichar* cp = cp0 + offset;
    PRBool breakBetween = PR_FALSE;
    if (aForLineBreak) {
      breakBetween = nsContentUtils::LineBreaker()->BreakInBetween(
          &firstChar, 1, cp, (aFragLen-offset));
    }
    else {
      breakBetween = nsContentUtils::WordBreaker()->BreakInBetween(
          &firstChar, 1, cp, (aFragLen-offset));
    }

    
    
    
    if (CH_NBSP == firstChar) {
      firstChar = ' ';
      *aWasTransformed = PR_TRUE;
    }
    nsresult rv = mTransformBuf.GrowTo(mBufferPos + 1);
    if (NS_FAILED(rv)) {
		*aWordLen = 0;
		return offset - 1;
	}

    mTransformBuf.mBuffer[mBufferPos++] = firstChar;

    if (!breakBetween) {
      
      PRInt32 next;
      if (aForLineBreak) {
        next = nsContentUtils::LineBreaker()->Next(cp0, aFragLen, offset);
      }
      else {
        next = nsContentUtils::WordBreaker()->NextWord(cp0, aFragLen, offset);
      }
      if (aForLineBreak && next == NS_LINEBREAKER_NEED_MORE_TEXT ||
          next == NS_WORDBREAKER_NEED_MORE_TEXT)
        next = aFragLen;
      numChars = (PRInt32) (next - (PRUint32) offset) + 1;

      
      
      nsresult rv = mTransformBuf.GrowTo(mBufferPos + numChars);
      if (NS_FAILED(rv)) {
        numChars = mTransformBuf.GetBufferLength() - mBufferPos;
      }

      offset += numChars - 1;

      
      
      
      
      PRUnichar* bp = &mTransformBuf.mBuffer[mBufferPos];
      const PRUnichar* end = cp + numChars - 1;
      while (cp < end) {
        PRUnichar ch = *cp++;
        if (CH_NBSP == ch) {
          ch = ' ';
          *aWasTransformed = PR_TRUE;
        }
        else if (IS_DISCARDED(ch) || (ch == 0x0a) || (ch == 0x0d)) {
          
          numChars--;
          continue;
        }
        if (ch > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);
        *bp++ = ch;
        mBufferPos++;
      }
    }
  }
  else 
  { 
    
    
    
    
    if (CH_NBSP == firstChar) {
      firstChar = ' ';
      *aWasTransformed = PR_TRUE;
    }
    nsresult rv = mTransformBuf.GrowTo(mBufferPos + 1);
    if (NS_FAILED(rv)) {
		*aWordLen = 0;
		return offset - 1;
	}
    mTransformBuf.mBuffer[mBufferPos++] = firstChar;
  }

  *aWordLen = numChars;
  return offset;
}


PRInt32
nsTextTransformer::ScanPreWrapWhiteSpace_F(PRInt32 aFragLen, PRInt32* aWordLen)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRUnichar* bp = mTransformBuf.GetBuffer() + mBufferPos;
  PRUnichar* endbp = mTransformBuf.GetBufferEnd();
  PRInt32 prevBufferPos = mBufferPos;

  for (; offset < aFragLen; offset++) {
    
    
    PRUnichar ch = frag->CharAt(offset);
    if (!XP_IS_SPACE(ch) || (ch == '\t') || (ch == '\n')) {
      if (IS_DISCARDED(ch)) {
        
        continue;
      }
      break;
    }
    if (bp == endbp) {
      PRInt32 oldLength = bp - mTransformBuf.GetBuffer();
      nsresult rv = mTransformBuf.GrowBy(1000);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp = mTransformBuf.GetBuffer() + oldLength;
      endbp = mTransformBuf.GetBufferEnd();
    }
    *bp++ = ' ';
    mBufferPos++;
  }

  *aWordLen = mBufferPos - prevBufferPos;
  return offset;
}


PRInt32
nsTextTransformer::ScanPreData_F(PRInt32  aFragLen,
                                 PRInt32* aWordLen,
                                 PRBool*  aWasTransformed)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRUnichar* bp = mTransformBuf.GetBuffer() + mBufferPos;
  PRUnichar* endbp = mTransformBuf.GetBufferEnd();
  PRInt32 prevBufferPos = mBufferPos;

  for (; offset < aFragLen; offset++) {
    
    
    PRUnichar ch = frag->CharAt(offset);
    if ((ch == '\t') || (ch == '\n')) {
      break;
    }
    if (CH_NBSP == ch) {
      ch = ' ';
      *aWasTransformed = PR_TRUE;
    }
    else if (IS_DISCARDED(ch)) {
      continue;
    }
    if (ch > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);
    if (bp == endbp) {
      PRInt32 oldLength = bp - mTransformBuf.GetBuffer();
      nsresult rv = mTransformBuf.GrowBy(1000);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp = mTransformBuf.GetBuffer() + oldLength;
      endbp = mTransformBuf.GetBufferEnd();
    }
    *bp++ = ch;
    mBufferPos++;
  }

  *aWordLen = mBufferPos - prevBufferPos;
  return offset;
}


PRInt32
nsTextTransformer::ScanPreAsciiData_F(PRInt32  aFragLen,
                                      PRInt32* aWordLen,
                                      PRBool*  aWasTransformed)
{
  const nsTextFragment* frag = mFrag;
  PRUnichar* bp = mTransformBuf.GetBuffer() + mBufferPos;
  PRUnichar* endbp = mTransformBuf.GetBufferEnd();
  const unsigned char* cp = (const unsigned char*) frag->Get1b();
  const unsigned char* end = cp + aFragLen;
  PRInt32 prevBufferPos = mBufferPos;
  cp += mOffset;

  while (cp < end) {
    PRUnichar ch = (PRUnichar) *cp++;
    if ((ch == '\t') || (ch == '\n')) {
      cp--;
      break;
    }
    if (CH_NBSP == ch) {
      ch = ' ';
      *aWasTransformed = PR_TRUE;
    }
    else if (IS_DISCARDED(ch)) {
      continue;
    }
    if (ch > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);
    if (bp == endbp) {
      PRInt32 oldLength = bp - mTransformBuf.GetBuffer();
      nsresult rv = mTransformBuf.GrowBy(1000);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp = mTransformBuf.GetBuffer() + oldLength;
      endbp = mTransformBuf.GetBufferEnd();
    }
    *bp++ = ch;
    mBufferPos++;
  }

  *aWordLen = mBufferPos - prevBufferPos;
  return cp - ((const unsigned char*)frag->Get1b());
}



static void
AsciiToLowerCase(unsigned char* aText, PRInt32 aWordLen)
{
  while (aWordLen-- > 0) {
    *aText = tolower(*aText);
    aText++;
  }
}

static void
AsciiToUpperCase(unsigned char* aText, PRInt32 aWordLen)
{
  while (aWordLen-- > 0) {
    *aText = toupper(*aText);
    aText++;
  }
}

#define kSzlig 0x00DF
static PRInt32 CountGermanSzlig(const PRUnichar* aText, PRInt32 len)
{
  PRInt32 i,cnt;
  for(i=0,cnt=0; i<len; i++, aText++)
  {
     if(kSzlig == *aText)
         cnt++;
  }
  return cnt;
}
static void ReplaceGermanSzligToSS(PRUnichar* aText, PRInt32 len, PRInt32 szCnt)
{
  PRUnichar *src, *dest;
  src = aText + len - 1;
  dest = src + szCnt;
  while( (src!=dest) && (src >= aText) )
  {
      if(kSzlig == *src )
      {     
        *dest-- = PRUnichar('S');
        *dest-- = PRUnichar('S');
        src--;
      } else {
        *dest-- = *src--;
      }
  }
}

void
nsTextTransformer::LanguageSpecificTransform(PRUnichar* aText, PRInt32 aLen,
                                             PRBool* aWasTransformed)
{
  if (mLanguageSpecificTransformType ==
      eLanguageSpecificTransformType_Japanese) {
    for (PRInt32 i = 0; i < aLen; i++) {
      if (aText[i] == 0x5C) { 
        aText[i] = 0xA5; 
        SetHasMultibyte(PR_TRUE);        
        *aWasTransformed = PR_TRUE;
      }
#if 0
      



      else if (aText[i] == 0x7E) { 
        aText[i] = 0x203E; 
        SetHasMultibyte(PR_TRUE);        
        *aWasTransformed = PR_TRUE;
      }
#endif
    }
  }
  
  
}

PRUnichar*
nsTextTransformer::GetNextWord(PRBool aInWord,
                               PRInt32* aWordLenResult,
                               PRInt32* aContentLenResult,
                               PRBool* aIsWhiteSpaceResult,
                               PRBool* aWasTransformed,
                               PRBool aResetTransformBuf,
                               PRBool aForLineBreak,
                               PRBool aIsKeyboardSelect)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 fragLen = frag->GetLength();
  if (*aWordLenResult > 0 && *aWordLenResult < fragLen) {
    fragLen = *aWordLenResult;
  }
  PRInt32 offset = mOffset;
  PRInt32 wordLen = 0;
  PRBool isWhitespace = PR_FALSE;
  PRUnichar* result = nsnull;
  PRInt32 prevBufferPos;
  PRBool skippedWhitespace = PR_FALSE;

  
  *aWasTransformed = PR_FALSE;

  
  
  if (aResetTransformBuf) {
    mBufferPos = 0;
    SetTransformedTextIsAscii(LeaveAsAscii() && !HasMultibyte());
  }
  prevBufferPos = mBufferPos;

  
  
  if((! aForLineBreak) && (eNormal != mMode))
     mMode = eNormal;

  while (offset < fragLen) {
    PRUnichar firstChar = frag->CharAt(offset);

    
    if (IS_DISCARDED(firstChar)) {
      offset++;
      continue;
    }

    switch (mMode) {
      default:
      case eNormal:
        if (XP_IS_SPACE(firstChar)) {
          offset = ScanNormalWhiteSpace_F(fragLen);

          
          
          if (firstChar == '\n' && 
              offset - mOffset == 1 && 
              mOffset > 0 &&
              offset < fragLen) 
          {
            PRUnichar lastChar = frag->CharAt(mOffset - 1);
            PRUnichar nextChar = frag->CharAt(offset);
            if (IS_CJ_CHAR(lastChar) && IS_CJ_CHAR(nextChar)) {
              skippedWhitespace = PR_TRUE;
              --mBufferPos;
              mOffset = offset;
              continue;            }
          }
          if (firstChar != ' ') {
            *aWasTransformed = PR_TRUE;
          }
          wordLen = 1;
          isWhitespace = PR_TRUE;
        }
        else if (CH_NBSP == firstChar && !aForLineBreak) {
          wordLen = 1;
          isWhitespace = PR_TRUE;
          *aWasTransformed = PR_TRUE;

          
          if (mBufferPos >= mTransformBuf.mBufferLen) {
             mTransformBuf.GrowBy(128);
          }

          offset++;
          if (TransformedTextIsAscii()) {
            ((unsigned char*)mTransformBuf.mBuffer)[mBufferPos++] = ' ';
          } else {
            mTransformBuf.mBuffer[mBufferPos++] = PRUnichar(' ');
          }
        }
        else if (frag->Is2b()) {
#ifdef IBMBIDI
          wordLen = *aWordLenResult;
#endif
          offset = ScanNormalUnicodeText_F(fragLen, aForLineBreak, &wordLen, aWasTransformed);
        }
        else {
          if (!aForLineBreak)
            offset = ScanNormalAsciiText_F_ForWordBreak(fragLen, &wordLen, 
                                                        aWasTransformed, 
                                                        aIsKeyboardSelect);
          else
            offset = ScanNormalAsciiText_F(fragLen, &wordLen, aWasTransformed);
        }
        break;

      case ePreformatted:
        if (('\n' == firstChar) || ('\t' == firstChar)) {
          mTransformBuf.mBuffer[mBufferPos++] = firstChar;
          offset++;
          wordLen = 1;
          isWhitespace = PR_TRUE;
        }
        else if (frag->Is2b()) {
#ifdef IBMBIDI
          wordLen = *aWordLenResult;
#endif
          offset = ScanPreData_F(fragLen, &wordLen, aWasTransformed);
        }
        else {
          offset = ScanPreAsciiData_F(fragLen, &wordLen, aWasTransformed);
        }
        break;

      case ePreWrap:
        if (XP_IS_SPACE(firstChar)) {
          if (('\n' == firstChar) || ('\t' == firstChar)) {
            mTransformBuf.mBuffer[mBufferPos++] = firstChar;
            offset++;
            wordLen = 1;
          }
          else {
            offset = ScanPreWrapWhiteSpace_F(fragLen, &wordLen);
          }
          isWhitespace = PR_TRUE;
        }
        else if (frag->Is2b()) {
#ifdef IBMBIDI
          wordLen = *aWordLenResult;
#endif
          offset = ScanNormalUnicodeText_F(fragLen, aForLineBreak, &wordLen, aWasTransformed);
        }
        else {
          if (!aForLineBreak)
            offset = ScanNormalAsciiText_F_ForWordBreak(fragLen, &wordLen, aWasTransformed, 
                                                        aIsKeyboardSelect);
          else
            offset = ScanNormalAsciiText_F(fragLen, &wordLen, aWasTransformed);
        }
        break;
    }

    if (TransformedTextIsAscii()) {
      unsigned char* wordPtr = (unsigned char*)mTransformBuf.mBuffer + prevBufferPos;
      
      if (!isWhitespace) {
        switch (mTextTransform) {
        case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
          if (!aInWord)
            *wordPtr = toupper(*wordPtr);
          break;
        case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
          AsciiToLowerCase(wordPtr, wordLen);
          break;
        case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
          AsciiToUpperCase(wordPtr, wordLen);
          break;
        }
        NS_ASSERTION(mLanguageSpecificTransformType ==
                     eLanguageSpecificTransformType_None,
                     "should not be ASCII for language specific transforms");
      }
      result = (PRUnichar*)wordPtr;

    } else {
      result = &mTransformBuf.mBuffer[prevBufferPos];

      if (!isWhitespace) {
        switch (mTextTransform) {
        case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
          if(NS_SUCCEEDED(EnsureCaseConv()))
            gCaseConv->ToTitle(result, result, wordLen, !aInWord);
          
          if(kSzlig == *result)
          {
              if (mBufferPos + 1 >= mTransformBuf.mBufferLen) {
                mTransformBuf.GrowBy(128);
                result = &mTransformBuf.mBuffer[prevBufferPos];
              }
              PRUnichar* src = result +  mBufferPos-prevBufferPos;
              while(src>result) 
              {
                *src = *(src-1);
                src--;
              }
              result[0] = PRUnichar('S');
              result[1] = PRUnichar('S');
              wordLen++;
          }
          break;
        case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
          if(NS_SUCCEEDED(EnsureCaseConv()))
            gCaseConv->ToLower(result, result, wordLen);
          break;
        case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
          {
            if(NS_SUCCEEDED(EnsureCaseConv()))
              gCaseConv->ToUpper(result, result, wordLen);

            
            PRInt32 szligCnt = CountGermanSzlig(result, wordLen);
            if(szligCnt > 0) {
              
              if (mBufferPos + szligCnt >= mTransformBuf.mBufferLen)
              {
                mTransformBuf.GrowBy(128);
                result = &mTransformBuf.mBuffer[prevBufferPos];
              }
              ReplaceGermanSzligToSS(result, mBufferPos-prevBufferPos, szligCnt);
              wordLen += szligCnt;
            }
          }
          break;
        }
        if (mLanguageSpecificTransformType !=
            eLanguageSpecificTransformType_None) {
          LanguageSpecificTransform(result, wordLen, aWasTransformed);
        }
        if (NeedsArabicShaping()) {
          DoArabicShaping(result, wordLen, aWasTransformed);
        }
        if (NeedsNumericShaping()) {
          DoNumericShaping(result, wordLen, aWasTransformed);
        }
      }
    }

    break;
  }

  *aIsWhiteSpaceResult = isWhitespace;
  *aWordLenResult = wordLen;
  *aContentLenResult = offset - mOffset;

  
  *aContentLenResult += (skippedWhitespace ? 1 : 0);

  
  
  if ((mTextTransform != NS_STYLE_TEXT_TRANSFORM_NONE) ||
      (*aWordLenResult != *aContentLenResult)) {
    *aWasTransformed = PR_TRUE;
    mBufferPos = prevBufferPos + *aWordLenResult;
  }

  mOffset = offset;

  NS_ASSERTION(mBufferPos == prevBufferPos + *aWordLenResult, "internal error");
  return result;
}




PRInt32
nsTextTransformer::ScanNormalWhiteSpace_B()
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;

  while (--offset >= 0) {
    PRUnichar ch = frag->CharAt(offset);
    if (!XP_IS_SPACE(ch)) {
      
      
      if (!IS_DISCARDED(ch)) {
        break;
      }
    }
  }

  mTransformBuf.mBuffer[mTransformBuf.mBufferLen - 1] = ' ';
  return offset;
}


PRInt32
nsTextTransformer::ScanNormalAsciiText_B(PRInt32* aWordLen, PRBool aIsKeyboardSelect)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRUnichar* bp = mTransformBuf.GetBufferEnd();
  PRUnichar* startbp = mTransformBuf.GetBuffer();

  PRUnichar ch = frag->CharAt(offset - 1);
  
  
  
  
  PRBool readingAlphaNumeric = isalnum(ch) || !IS_ASCII_CHAR(ch);

  while (--offset >= 0) {
    PRUnichar ch = frag->CharAt(offset);
    if (CH_NBSP == ch) {
      ch = ' ';
    }
    if (XP_IS_SPACE(ch)) {
      break;
    }
    else if (IS_DISCARDED(ch)) {
      continue;
    } 
    else if (sWordSelectStopAtPunctuation && readingAlphaNumeric && 
             !isalnum(ch) && IS_ASCII_CHAR(ch)) {
      
      break;
    }
    else if (sWordSelectStopAtPunctuation && !readingAlphaNumeric &&
             (isalnum(ch) || !IS_ASCII_CHAR(ch))) {
      if (!aIsKeyboardSelect)
        break;
      readingAlphaNumeric = PR_TRUE;
    }
    
    if (ch > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);
    if (bp == startbp) {
      PRInt32 oldLength = mTransformBuf.mBufferLen;
      nsresult rv = mTransformBuf.GrowBy(1000);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp = mTransformBuf.GetBufferEnd() - oldLength;
      startbp = mTransformBuf.GetBuffer();
    }
    *--bp = ch;
  }

  *aWordLen = mTransformBuf.GetBufferEnd() - bp;
  return offset;
}


PRInt32
nsTextTransformer::ScanNormalUnicodeText_B(PRBool aForLineBreak,
                                           PRInt32* aWordLen)
{
  const nsTextFragment* frag = mFrag;
  const PRUnichar* cp0 = frag->Get2b();
  PRInt32 offset = mOffset - 1;

  PRUnichar firstChar = frag->CharAt(offset);

#ifdef IBMBIDI
  PRInt32 limit = (*aWordLen > 0) ? *aWordLen : 0;
  
  while (offset > limit && IS_BIDI_CONTROL(firstChar) ) {
    firstChar = frag->CharAt(--offset);
  }
#endif

  mTransformBuf.mBuffer[mTransformBuf.mBufferLen - 1] = firstChar;
  if (firstChar > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);

  PRInt32 numChars = 1;

#ifdef IBMBIDI
  if (offset > limit) {
#else
  if (offset > 0) {
#endif
    const PRUnichar* cp = cp0 + offset;
    PRBool breakBetween = PR_FALSE;
    if (aForLineBreak) {
      breakBetween = nsContentUtils::LineBreaker()->BreakInBetween(cp0, 
          offset + 1, mTransformBuf.GetBufferEnd()-1, 1);
    }
    else {
      breakBetween = nsContentUtils::WordBreaker()->BreakInBetween(cp0,
          offset + 1, mTransformBuf.GetBufferEnd()-1, 1);
    }

    if (!breakBetween) {
      
      PRInt32 prev;
      if (aForLineBreak) {
        prev = nsContentUtils::LineBreaker()->Prev(cp0, offset, offset);
      }
      else {
        prev = nsContentUtils::WordBreaker()->PrevWord(cp0, offset, offset);
      }
      if (aForLineBreak && prev == NS_LINEBREAKER_NEED_MORE_TEXT ||
          prev == NS_WORDBREAKER_NEED_MORE_TEXT)
        prev = 0;
      
      numChars = (PRInt32) ((PRUint32) offset - prev) + 1;

      
      nsresult rv = mTransformBuf.GrowTo(numChars);
      if (NS_FAILED(rv)) {
        numChars = mTransformBuf.GetBufferLength();
      }

      
      
      
      PRUnichar* bp = mTransformBuf.GetBufferEnd() - 1;
      const PRUnichar* end = cp - numChars + 1;
      while (cp > end) {
        PRUnichar ch = *--cp;
        if (CH_NBSP == ch) {
          ch = ' ';
        }
        else if (IS_DISCARDED(ch)) {
          continue;
        }
        if (ch > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);
        *--bp = ch;
      }

      
      offset = offset - numChars;
      numChars =  mTransformBuf.GetBufferEnd() - bp;
    }
  }
  else 
	  offset--;

  *aWordLen = numChars;
  return offset;
}


PRInt32
nsTextTransformer::ScanPreWrapWhiteSpace_B(PRInt32* aWordLen)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRUnichar* bp = mTransformBuf.GetBufferEnd();
  PRUnichar* startbp = mTransformBuf.GetBuffer();

  while (--offset >= 0) {
    PRUnichar ch = frag->CharAt(offset);
    if (!XP_IS_SPACE(ch) || (ch == '\t') || (ch == '\n')) {
      
      if (IS_DISCARDED(ch)) {
        continue;
      }
      break;
    }
    if (bp == startbp) {
      PRInt32 oldLength = mTransformBuf.mBufferLen;
      nsresult rv = mTransformBuf.GrowBy(1000);
      if (NS_FAILED(rv)) {
        
        break;
      }
      bp = mTransformBuf.GetBufferEnd() - oldLength;
      startbp = mTransformBuf.GetBuffer();
    }
    *--bp = ' ';
  }

  *aWordLen = mTransformBuf.GetBufferEnd() - bp;
  return offset;
}


PRInt32
nsTextTransformer::ScanPreData_B(PRInt32* aWordLen)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRUnichar* bp = mTransformBuf.GetBufferEnd();
  PRUnichar* startbp = mTransformBuf.GetBuffer();

  while (--offset >= 0) {
    PRUnichar ch = frag->CharAt(offset);
    if ((ch == '\t') || (ch == '\n')) {
      break;
    }
    if (CH_NBSP == ch) {
      ch = ' ';
    }
    else if (IS_DISCARDED(ch)) {
      continue;
    }
    if (ch > MAX_UNIBYTE) SetHasMultibyte(PR_TRUE);
    if (bp == startbp) {
      PRInt32 oldLength = mTransformBuf.mBufferLen;
      nsresult rv = mTransformBuf.GrowBy(1000);
      if (NS_FAILED(rv)) {
        
        offset++;
        break;
      }
      bp = mTransformBuf.GetBufferEnd() - oldLength;
      startbp = mTransformBuf.GetBuffer();
    }
    *--bp = ch;
  }

  *aWordLen = mTransformBuf.GetBufferEnd() - bp;
  return offset;
}



PRUnichar*
nsTextTransformer::GetPrevWord(PRBool aInWord,
                               PRInt32* aWordLenResult,
                               PRInt32* aContentLenResult,
                               PRBool* aIsWhiteSpaceResult,
                               PRBool aForLineBreak,
                               PRBool aIsKeyboardSelect)
{
  const nsTextFragment* frag = mFrag;
  PRInt32 offset = mOffset;
  PRInt32 wordLen = 0;
  PRBool isWhitespace = PR_FALSE;
  PRUnichar* result = nsnull;

  
  
  if((! aForLineBreak) && (eNormal != mMode))
     mMode = eNormal;

#ifdef IBMBIDI
  PRInt32 limit = (*aWordLenResult > 0) ? *aWordLenResult : 0;
  while (--offset >= limit) {
#else
  while (--offset >= 0) {
#endif
    PRUnichar firstChar = frag->CharAt(offset);

    
    if (IS_DISCARDED(firstChar)) {
      continue;
    }

    switch (mMode) {
      default:
      case eNormal:
        if (XP_IS_SPACE(firstChar)) {
          offset = ScanNormalWhiteSpace_B();
          wordLen = 1;
          isWhitespace = PR_TRUE;
        }
        else if (CH_NBSP == firstChar && !aForLineBreak) {
          wordLen = 1;
          isWhitespace = PR_TRUE;
          mTransformBuf.mBuffer[mTransformBuf.mBufferLen - 1] = ' ';
          offset--;
        } else if (frag->Is2b()) {
#ifdef IBMBIDI
          wordLen = *aWordLenResult;
#endif
          offset = ScanNormalUnicodeText_B(aForLineBreak, &wordLen);
        }
        else {
          offset = ScanNormalAsciiText_B(&wordLen, aIsKeyboardSelect);
        }
        break;

      case ePreformatted:
        if (('\n' == firstChar) || ('\t' == firstChar)) {
          mTransformBuf.mBuffer[mTransformBuf.mBufferLen-1] = firstChar;
          offset--;  
          wordLen = 1;
          isWhitespace = PR_TRUE;
        }
        else {
          offset = ScanPreData_B(&wordLen);
        }
        break;

      case ePreWrap:
        if (XP_IS_SPACE(firstChar)) {
          if (('\n' == firstChar) || ('\t' == firstChar)) {
            mTransformBuf.mBuffer[mTransformBuf.mBufferLen-1] = firstChar;
            offset--;  
            wordLen = 1;
          }
          else {
            offset = ScanPreWrapWhiteSpace_B(&wordLen);
          }
          isWhitespace = PR_TRUE;
        }
        else if (frag->Is2b()) {
#ifdef IBMBIDI
          wordLen = *aWordLenResult;
#endif
          offset = ScanNormalUnicodeText_B(aForLineBreak, &wordLen);
        }
        else {
          offset = ScanNormalAsciiText_B(&wordLen, aIsKeyboardSelect);
        }
        break;
    }

    
    
    offset = offset + 1;

    result = mTransformBuf.GetBufferEnd() - wordLen;

    if (!isWhitespace) {
      switch (mTextTransform) {
        case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
          if(NS_SUCCEEDED(EnsureCaseConv()))
            gCaseConv->ToTitle(result, result, wordLen, !aInWord);
          break;
        case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
          if(NS_SUCCEEDED(EnsureCaseConv()))
            gCaseConv->ToLower(result, result, wordLen);
          break;
        case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
          if(NS_SUCCEEDED(EnsureCaseConv()))
            gCaseConv->ToUpper(result, result, wordLen);
          break;
      }
    }
    break;
  }

  *aWordLenResult = wordLen;
  *aContentLenResult = mOffset - offset;
  *aIsWhiteSpaceResult = isWhitespace;

  mOffset = offset;
  return result;
}

void
nsTextTransformer::DoArabicShaping(PRUnichar* aText, 
                                   PRInt32& aTextLength, 
                                   PRBool* aWasTransformed)
{
  if (aTextLength <= 0)
    return;
  
  PRInt32 newLen;
  
  
  PRBool isLogical = FrameIsRTL();

  nsAutoString buf;
  if (!EnsureStringLength(buf, aTextLength)) {
    
    aTextLength = 0;
    return;
  }
  PRUnichar* buffer = buf.BeginWriting();
  
  ArabicShaping(aText, buf.Length(), buffer, (PRUint32 *)&newLen, isLogical, isLogical);

  if (newLen <= aTextLength) {
    aTextLength = newLen;
  } else {
    
    
    NS_ERROR("ArabicShaping should not have increased the text length");
  }
  *aWasTransformed = PR_TRUE;
  memcpy(aText, buffer, aTextLength * sizeof(PRUnichar));
}

void
nsTextTransformer::DoNumericShaping(PRUnichar* aText, 
                                    PRInt32& aTextLength, 
                                    PRBool* aWasTransformed)
{
  if (aTextLength <= 0)
    return;

  PRUint32 bidiOptions = mPresContext->GetBidi();

  switch (GET_BIDI_OPTION_NUMERAL(bidiOptions)) {

    case IBMBIDI_NUMERAL_HINDI:
      HandleNumbers(aText, aTextLength, IBMBIDI_NUMERAL_HINDI);
      break;

    case IBMBIDI_NUMERAL_ARABIC:
      HandleNumbers(aText, aTextLength, IBMBIDI_NUMERAL_ARABIC);
      break;

    case IBMBIDI_NUMERAL_REGULAR:

      switch (mCharType) {

        case eCharType_EuropeanNumber:
          HandleNumbers(aText, aTextLength, IBMBIDI_NUMERAL_ARABIC);
          break;

        case eCharType_ArabicNumber:
          HandleNumbers(aText, aTextLength, IBMBIDI_NUMERAL_HINDI);
          break;

        default:
          break;
      }
      break;

    case IBMBIDI_NUMERAL_HINDICONTEXT:
      if (((GET_BIDI_OPTION_DIRECTION(bidiOptions)==IBMBIDI_TEXTDIRECTION_RTL) &&
           (IS_ARABIC_DIGIT (aText[0]))) ||
          (eCharType_ArabicNumber == mCharType))
        HandleNumbers(aText, aTextLength, IBMBIDI_NUMERAL_HINDI);
      else if (eCharType_EuropeanNumber == mCharType)
        HandleNumbers(aText, aTextLength, IBMBIDI_NUMERAL_ARABIC);
      break;

    case IBMBIDI_NUMERAL_NOMINAL:
    default:
      break;
  }
}






#ifdef DEBUG
struct SelfTestSection {
  int length;
  int* data;
};

#define NUM_MODES 3

struct SelfTestData {
  const PRUnichar* text;
  SelfTestSection modes[NUM_MODES];
};

static PRUint8 preModeValue[NUM_MODES] = {
  NS_STYLE_WHITESPACE_NORMAL,
  NS_STYLE_WHITESPACE_PRE,
  NS_STYLE_WHITESPACE_MOZ_PRE_WRAP
};

static PRUnichar test1text[] = {
  'o', 'n', 'c', 'e', ' ', 'u', 'p', 'o', 'n', '\t',
  'a', ' ', 's', 'h', 'o', 'r', 't', ' ', 't', 'i', 'm', 'e', 0
};
static int test1Results[] = { 4, 1, 4, 1, 1, 1, 5, 1, 4 };
static int test1PreResults[] = { 9, 1, 12 };
static int test1PreWrapResults[] = { 4, 1, 4, 1, 1, 1, 5, 1, 4 };

static PRUnichar test2text[] = {
  0xF6, 'n', 'c', 'e', ' ', 0xFB, 'p', 'o', 'n', '\t',
  0xE3, ' ', 's', 'h', 0xF3, 'r', 't', ' ', 't', 0xEE, 'm', 'e', ' ', 0
};
static int test2Results[] = { 4, 1, 4, 1, 1, 1, 5, 1, 4, 1 };
static int test2PreResults[] = { 9, 1, 13 };
static int test2PreWrapResults[] = { 4, 1, 4, 1, 1, 1, 5, 1, 4, 1 };

static PRUnichar test3text[] = {
  0x0152, 'n', 'c', 'e', ' ', 'x', 'y', '\t', 'z', 'y', ' ', 0
};
static int test3Results[] = { 4, 1, 2, 1, 2, 1, };
static int test3PreResults[] = { 7, 1, 3, };
static int test3PreWrapResults[] = { 4, 1, 2, 1, 2, 1, };

static PRUnichar test4text[] = {
  'o', 'n', CH_SHY, 'c', 'e', ' ', CH_SHY, ' ', 'u', 'p', 'o', 'n', '\t',
  'a', ' ', 's', 'h', 'o', 'r', 't', ' ', 't', 'i', 'm', 'e', 0
};
static int test4Results[] = { 4, 1, 4, 1, 1, 1, 5, 1, 4 };
static int test4PreResults[] = { 10, 1, 12 };
static int test4PreWrapResults[] = { 4, 2, 4, 1, 1, 1, 5, 1, 4 };

static PRUnichar test5text[] = {
  CH_SHY, 0
};
static int test5Results[] = { 0 };
static int test5PreResults[] = { 0 };
static int test5PreWrapResults[] = { 0 };

#if 0
static PRUnichar test6text[] = {
  0x30d5, 0x30b8, 0x30c6, 0x30ec, 0x30d3, 0x306e, 0x97f3, 0x697d,
  0x756a, 0x7d44, 0x300c, 'H', 'E', 'Y', '!', ' ', 'H', 'E', 'Y', '!',
  '\t', 'H', 'E', 'Y', '!', 0x300d, 0x306e, 0x30db, 0x30fc, 0x30e0,
  0x30da, 0x30fc, 0x30b8, 0x3002, 0
};
static int test6Results[] = { 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1,
                              5, 1, 4, 1, 5,
                              1, 2, 1, 2, 2 };
static int test6PreResults[] = { 20, 1, 13 };
static int test6PreWrapResults[] = { 1, 1, 1, 1, 1,
                                     1, 1, 1, 1, 1,
                                     5, 1, 4, 1, 5,
                                     1, 2, 1, 2, 2 };
#endif

static SelfTestData tests[] = {
  { test1text,
    { { sizeof(test1Results)/sizeof(int), test1Results, },
      { sizeof(test1PreResults)/sizeof(int), test1PreResults, },
      { sizeof(test1PreWrapResults)/sizeof(int), test1PreWrapResults, } }
  },
  { test2text,
    { { sizeof(test2Results)/sizeof(int), test2Results, },
      { sizeof(test2PreResults)/sizeof(int), test2PreResults, },
      { sizeof(test2PreWrapResults)/sizeof(int), test2PreWrapResults, } }
  },
  { test3text,
    { { sizeof(test3Results)/sizeof(int), test3Results, },
      { sizeof(test3PreResults)/sizeof(int), test3PreResults, },
      { sizeof(test3PreWrapResults)/sizeof(int), test3PreWrapResults, } }
  },
  { test4text,
    { { sizeof(test4Results)/sizeof(int), test4Results, },
      { sizeof(test4PreResults)/sizeof(int), test4PreResults, },
      { sizeof(test4PreWrapResults)/sizeof(int), test4PreWrapResults, } }
  },
  { test5text,
    { { sizeof(test5Results)/sizeof(int), test5Results, },
      { sizeof(test5PreResults)/sizeof(int), test5PreResults, },
      { sizeof(test5PreWrapResults)/sizeof(int), test5PreWrapResults, } }
  },
#if 0
  { test6text,
    { { sizeof(test6Results)/sizeof(int), test6Results, },
      { sizeof(test6PreResults)/sizeof(int), test6PreResults, },
      { sizeof(test6PreWrapResults)/sizeof(int), test6PreWrapResults, } }
  },
#endif
};

#define NUM_TESTS (sizeof(tests) / sizeof(tests[0]))

void
nsTextTransformer::SelfTest(nsPresContext* aPresContext)
{
  PRBool gNoisy = PR_FALSE;
  if (PR_GetEnv("GECKO_TEXT_TRANSFORMER_NOISY_SELF_TEST")) {
    gNoisy = PR_TRUE;
  }

  PRBool error = PR_FALSE;
  PRInt32 testNum = 0;
  SelfTestData* st = tests;
  SelfTestData* last = st + NUM_TESTS;
  for (; st < last; st++) {
    PRUnichar* bp;
    PRInt32 wordLen, contentLen;
    PRBool ws, transformed;

    PRBool isAsciiTest = PR_TRUE;
    const PRUnichar* cp = st->text;
    while (*cp) {
      if (*cp > 255) {
        isAsciiTest = PR_FALSE;
        break;
      }
      cp++;
    }

    nsTextFragment frag;
    frag.SetTo(st->text, nsCRT::strlen(st->text));
    nsTextTransformer tx(aPresContext);

    for (PRInt32 preMode = 0; preMode < NUM_MODES; preMode++) {
      
      if (gNoisy) {
        nsAutoString uc2(st->text);
        printf("%s forwards test: '", isAsciiTest ? "ascii" : "unicode");
        fputs(NS_ConvertUTF16toUTF8(uc2).get(), stdout);
        printf("'\n");
      }
      tx.Init2(&frag, 0, preModeValue[preMode], NS_STYLE_TEXT_TRANSFORM_NONE);

      int* expectedResults = st->modes[preMode].data;
      int resultsLen = st->modes[preMode].length;

#ifdef IBMBIDI
      wordLen = -1;
#endif
      while ((bp = tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &ws, &transformed))) {
        if (gNoisy) {
          nsAutoString tmp(bp, wordLen);
          printf("  '");
          fputs(NS_ConvertUTF16toUTF8(tmp).get(), stdout);
          printf("': ws=%s wordLen=%d (%d) contentLen=%d (offset=%d)\n",
                 ws ? "yes" : "no",
                 wordLen, *expectedResults, contentLen, tx.mOffset);
        }
        if (*expectedResults != wordLen) {
          error = PR_TRUE;
          break;
        }
        expectedResults++;
#ifdef IBMBIDI
        wordLen = -1;
#endif
      }
      if (expectedResults != st->modes[preMode].data + resultsLen) {
        if (st->modes[preMode].data[0] != 0) {
          error = PR_TRUE;
        }
      }

      
      if (gNoisy) {
        nsAutoString uc2(st->text);
        printf("%s backwards test: '", isAsciiTest ? "ascii" : "unicode");
        fputs(NS_ConvertUTF16toUTF8(uc2).get(), stdout);
        printf("'\n");
      }
      tx.Init2(&frag, frag.GetLength(), NS_STYLE_WHITESPACE_NORMAL,
               NS_STYLE_TEXT_TRANSFORM_NONE);
      expectedResults = st->modes[preMode].data + resultsLen;
#ifdef IBMBIDI
      wordLen = -1;
#endif
      while ((bp = tx.GetPrevWord(PR_FALSE, &wordLen, &contentLen, &ws))) {
        --expectedResults;
        if (gNoisy) {
          nsAutoString tmp(bp, wordLen);
          printf("  '");
          fputs(NS_ConvertUTF16toUTF8(tmp).get(), stdout);
          printf("': ws=%s wordLen=%d contentLen=%d (offset=%d)\n",
                 ws ? "yes" : "no",
                 wordLen, contentLen, tx.mOffset);
        }
        if (*expectedResults != wordLen) {
          error = PR_TRUE;
          break;
        }
#ifdef IBMBIDI
        wordLen = -1;
#endif
      }
      if (expectedResults != st->modes[preMode].data) {
        if (st->modes[preMode].data[0] != 0) {
          error = PR_TRUE;
        }
      }

      if (error) {
        fprintf(stderr, "nsTextTransformer: self test %d failed\n", testNum);
      }
      else if (gNoisy) {
        fprintf(stdout, "nsTextTransformer: self test %d succeeded\n", testNum);
      }

      testNum++;
    }
  }
  if (error) {
    NS_ABORT();
  }
}

nsresult
nsTextTransformer::Init2(const nsTextFragment* aFrag,
                         PRInt32 aStartingOffset,
                         PRUint8 aWhiteSpace,
                         PRUint8 aTextTransform)
{
  mFrag = aFrag;

  
  if (aStartingOffset < 0) {
    NS_WARNING("bad starting offset");
    aStartingOffset = 0;
  }
  else if (aStartingOffset > mFrag->GetLength()) {
    NS_WARNING("bad starting offset");
    aStartingOffset = mFrag->GetLength();
  }
  mOffset = aStartingOffset;

  
  if (NS_STYLE_WHITESPACE_PRE == aWhiteSpace) {
    mMode = ePreformatted;
  }
  else if (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == aWhiteSpace) {
    mMode = ePreWrap;
  }
  mTextTransform = aTextTransform;

  return NS_OK;
}
#endif 
