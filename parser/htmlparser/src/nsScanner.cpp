







#include "mozilla/DebugOnly.h"

#include "nsScanner.h"
#include "nsDebug.h"
#include "nsReadableUtils.h"
#include "nsIInputStream.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
#include "nsUTF8Utils.h" 
#include "nsCRT.h"
#include "nsParser.h"
#include "nsCharsetSource.h"

#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;


static char16_t sInvalid = UCS2_REPLACEMENT_CHAR;

nsReadEndCondition::nsReadEndCondition(const char16_t* aTerminateChars) :
  mChars(aTerminateChars), mFilter(char16_t(~0)) 
{
  
  
  
  
  
  
  
  const char16_t *current = aTerminateChars;
  char16_t terminalChar = *current;
  while (terminalChar) {
    mFilter &= ~terminalChar;
    ++current;
    terminalChar = *current;
  }
}










nsScanner::nsScanner(const nsAString& anHTMLString)
{
  MOZ_COUNT_CTOR(nsScanner);

  mSlidingBuffer = nullptr;
  mCountRemaining = 0;
  mFirstNonWhitespacePosition = -1;
  if (AppendToBuffer(anHTMLString)) {
    mSlidingBuffer->BeginReading(mCurrentPosition);
  } else {
    
    memset(&mCurrentPosition, 0, sizeof(mCurrentPosition));
    mEndPosition = mCurrentPosition;
  }
  mMarkPosition = mCurrentPosition;
  mIncremental = false;
  mUnicodeDecoder = 0;
  mCharsetSource = kCharsetUninitialized;
  mHasInvalidCharacter = false;
  mReplacementCharacter = char16_t(0x0);
}






nsScanner::nsScanner(nsString& aFilename, bool aCreateStream)
  : mFilename(aFilename)
{
  MOZ_COUNT_CTOR(nsScanner);
  NS_ASSERTION(!aCreateStream, "This is always true.");

  mSlidingBuffer = nullptr;

  
  
  
  
  
  memset(&mCurrentPosition, 0, sizeof(mCurrentPosition));
  mMarkPosition = mCurrentPosition;
  mEndPosition = mCurrentPosition;

  mIncremental = true;
  mFirstNonWhitespacePosition = -1;
  mCountRemaining = 0;

  mUnicodeDecoder = 0;
  mCharsetSource = kCharsetUninitialized;
  mHasInvalidCharacter = false;
  mReplacementCharacter = char16_t(0x0);
  
  SetDocumentCharset(NS_LITERAL_CSTRING("UTF-8"), kCharsetFromDocTypeDefault);
}

nsresult nsScanner::SetDocumentCharset(const nsACString& aCharset , int32_t aSource)
{
  if (aSource < mCharsetSource) 
    return NS_OK;

  mCharsetSource = aSource;

  nsCString charsetName;
  mozilla::DebugOnly<bool> valid =
      EncodingUtils::FindEncodingForLabel(aCharset, charsetName);
  MOZ_ASSERT(valid, "Should never call with a bogus aCharset.");

  if (!mCharset.IsEmpty() && charsetName.Equals(mCharset)) {
    return NS_OK; 
  }

  

  mCharset.Assign(charsetName);

  mUnicodeDecoder = EncodingUtils::DecoderForEncoding(mCharset);
  mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);

  return NS_OK;
}









nsScanner::~nsScanner() {

  delete mSlidingBuffer;

  MOZ_COUNT_DTOR(nsScanner);
}











void nsScanner::RewindToMark(void){
  if (mSlidingBuffer) {
    mCountRemaining += (Distance(mMarkPosition, mCurrentPosition));
    mCurrentPosition = mMarkPosition;
  }
}











int32_t nsScanner::Mark() {
  int32_t distance = 0;
  if (mSlidingBuffer) {
    nsScannerIterator oldStart;
    mSlidingBuffer->BeginReading(oldStart);

    distance = Distance(oldStart, mCurrentPosition);

    mSlidingBuffer->DiscardPrefix(mCurrentPosition);
    mSlidingBuffer->BeginReading(mCurrentPosition);
    mMarkPosition = mCurrentPosition;
  }

  return distance;
}








bool nsScanner::UngetReadable(const nsAString& aBuffer) {
  if (!mSlidingBuffer) {
    return false;
  }

  mSlidingBuffer->UngetReadable(aBuffer,mCurrentPosition);
  mSlidingBuffer->BeginReading(mCurrentPosition); 
  mSlidingBuffer->EndReading(mEndPosition);
 
  uint32_t length = aBuffer.Length();
  mCountRemaining += length; 
  return true;
}








nsresult nsScanner::Append(const nsAString& aBuffer) {
  if (!AppendToBuffer(aBuffer))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}








nsresult nsScanner::Append(const char* aBuffer, uint32_t aLen,
                           nsIRequest *aRequest)
{
  nsresult res = NS_OK;
  if (mUnicodeDecoder) {
    int32_t unicharBufLen = 0;
    mUnicodeDecoder->GetMaxLength(aBuffer, aLen, &unicharBufLen);
    nsScannerString::Buffer* buffer = nsScannerString::AllocBuffer(unicharBufLen + 1);
    NS_ENSURE_TRUE(buffer,NS_ERROR_OUT_OF_MEMORY);
    char16_t *unichars = buffer->DataStart();

    int32_t totalChars = 0;
    int32_t unicharLength = unicharBufLen;
    int32_t errorPos = -1;

    do {
      int32_t srcLength = aLen;
      res = mUnicodeDecoder->Convert(aBuffer, &srcLength, unichars, &unicharLength);

      totalChars += unicharLength;
      
      if(NS_FAILED(res)) {
        
        

        
        
        
        if ((unichars + unicharLength) >= buffer->DataEnd()) {
          NS_ERROR("Unexpected end of destination buffer");
          break;
        }

        if (mReplacementCharacter == 0x0 && errorPos == -1) {
          errorPos = totalChars;
        }
        unichars[unicharLength++] = mReplacementCharacter == 0x0 ?
                                    mUnicodeDecoder->GetCharacterForUnMapped() :
                                    mReplacementCharacter;

        unichars = unichars + unicharLength;
        unicharLength = unicharBufLen - (++totalChars);

        mUnicodeDecoder->Reset();

        if(((uint32_t) (srcLength + 1)) > aLen) {
          srcLength = aLen;
        }
        else {
          ++srcLength;
        }

        aBuffer += srcLength;
        aLen -= srcLength;
      }
    } while (NS_FAILED(res) && (aLen > 0));

    buffer->SetDataLength(totalChars);
    
    
    
    res = NS_OK; 
    if (!AppendToBuffer(buffer, aRequest, errorPos))
      res = NS_ERROR_OUT_OF_MEMORY;
  }
  else {
    NS_WARNING("No decoder found.");
    res = NS_ERROR_FAILURE;
  }

  return res;
}








nsresult nsScanner::GetChar(char16_t& aChar) {
  if (!mSlidingBuffer || mCurrentPosition == mEndPosition) {
    aChar = 0;
    return kEOF;
  }

  aChar = *mCurrentPosition++;
  --mCountRemaining;

  return NS_OK;
}










nsresult nsScanner::Peek(char16_t& aChar, uint32_t aOffset) {
  aChar = 0;

  if (!mSlidingBuffer || mCurrentPosition == mEndPosition) {
    return kEOF;
  }

  if (aOffset > 0) {
    if (mCountRemaining <= aOffset)
      return kEOF;

    nsScannerIterator pos = mCurrentPosition;
    pos.advance(aOffset);
    aChar=*pos;
  }
  else {
    aChar=*mCurrentPosition;
  }

  return NS_OK;
}

nsresult nsScanner::Peek(nsAString& aStr, int32_t aNumChars, int32_t aOffset)
{
  if (!mSlidingBuffer || mCurrentPosition == mEndPosition) {
    return kEOF;
  }

  nsScannerIterator start, end;

  start = mCurrentPosition;

  if ((int32_t)mCountRemaining <= aOffset) {
    return kEOF;
  }

  if (aOffset > 0) {
    start.advance(aOffset);
  }

  if (mCountRemaining < uint32_t(aNumChars + aOffset)) {
    end = mEndPosition;
  }
  else {
    end = start;
    end.advance(aNumChars);
  }

  CopyUnicodeTo(start, end, aStr);

  return NS_OK;
}









nsresult nsScanner::SkipWhitespace(int32_t& aNewlinesSkipped) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  char16_t theChar = 0;
  nsresult  result = Peek(theChar);
  
  if (NS_FAILED(result)) {
    return result;
  }
  
  nsScannerIterator current = mCurrentPosition;
  bool      done = false;
  bool      skipped = false;
  
  while (!done && current != mEndPosition) {
    switch(theChar) {
      case '\n':
      case '\r': ++aNewlinesSkipped;
      case ' ' :
      case '\t':
        {
          skipped = true;
          char16_t thePrevChar = theChar;
          theChar = (++current != mEndPosition) ? *current : '\0';
          if ((thePrevChar == '\r' && theChar == '\n') ||
              (thePrevChar == '\n' && theChar == '\r')) {
            theChar = (++current != mEndPosition) ? *current : '\0'; 
          }
        }
        break;
      default:
        done = true;
        break;
    }
  }

  if (skipped) {
    SetPosition(current);
    if (current == mEndPosition) {
      result = kEOF;
    }
  }

  return result;
}








nsresult nsScanner::SkipOver(char16_t aSkipChar){

  if (!mSlidingBuffer) {
    return kEOF;
  }

  char16_t ch=0;
  nsresult   result=NS_OK;

  while(NS_OK==result) {
    result=Peek(ch);
    if(NS_OK == result) {
      if(ch!=aSkipChar) {
        break;
      }
      GetChar(ch);
    } 
    else break;
  } 
  return result;

}

#if 0
void DoErrTest(nsString& aString) {
  int32_t pos=aString.FindChar(0);
  if(kNotFound<pos) {
    if(aString.Length()-1!=pos) {
    }
  }
}

void DoErrTest(nsCString& aString) {
  int32_t pos=aString.FindChar(0);
  if(kNotFound<pos) {
    if(aString.Length()-1!=pos) {
    }
  }
}
#endif







nsresult nsScanner::ReadTagIdentifier(nsScannerSharedSubstring& aString) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  char16_t         theChar=0;
  nsresult          result=Peek(theChar);
  nsScannerIterator current, end;
  bool              found=false;  
  
  current = mCurrentPosition;
  end = mEndPosition;

  
  
  while(current != end && !found) {
    theChar=*current;

    switch(theChar) {
      case '\n':
      case '\r':
      case ' ' :
      case '\t':
      case '\v':
      case '\f':
      case '<':
      case '>':
      case '/':
        found = true;
        break;

      case '\0':
        ReplaceCharacter(current, sInvalid);
        break;

      default:
        break;
    }

    if (!found) {
      ++current;
    }
  }

  
  if (current != mCurrentPosition) {
    AppendUnicodeTo(mCurrentPosition, current, aString);
  }

  SetPosition(current);  
  if (current == end) {
    result = kEOF;
  }

  

  return result;
}








nsresult nsScanner::ReadEntityIdentifier(nsString& aString) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  char16_t         theChar=0;
  nsresult          result=Peek(theChar);
  nsScannerIterator origin, current, end;
  bool              found=false;  

  origin = mCurrentPosition;
  current = mCurrentPosition;
  end = mEndPosition;

  while(current != end) {
 
    theChar=*current;
    if(theChar) {
      found=false;
      switch(theChar) {
        case '_':
        case '-':
        case '.':
          
          found = true;
          break;
        default:
          found = ('a'<=theChar && theChar<='z') ||
                  ('A'<=theChar && theChar<='Z') ||
                  ('0'<=theChar && theChar<='9');
          break;
      }

      if(!found) {
        AppendUnicodeTo(mCurrentPosition, current, aString);
        break;
      }
    }
    ++current;
  }
  
  SetPosition(current);
  if (current == end) {
    AppendUnicodeTo(origin, current, aString);
    return kEOF;
  }

  

  return result;
}







nsresult nsScanner::ReadNumber(nsString& aString,int32_t aBase) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  NS_ASSERTION(aBase == 10 || aBase == 16,"base value not supported");

  char16_t         theChar=0;
  nsresult          result=Peek(theChar);
  nsScannerIterator origin, current, end;

  origin = mCurrentPosition;
  current = origin;
  end = mEndPosition;

  bool done = false;
  while(current != end) {
    theChar=*current;
    if(theChar) {
      done = (theChar < '0' || theChar > '9') && 
             ((aBase == 16)? (theChar < 'A' || theChar > 'F') &&
                             (theChar < 'a' || theChar > 'f')
                             :true);
      if(done) {
        AppendUnicodeTo(origin, current, aString);
        break;
      }
    }
    ++current;
  }

  SetPosition(current);
  if (current == end) {
    AppendUnicodeTo(origin, current, aString);
    return kEOF;
  }

  

  return result;
}









nsresult nsScanner::ReadWhitespace(nsScannerSharedSubstring& aString,
                                   int32_t& aNewlinesSkipped,
                                   bool& aHaveCR) {

  aHaveCR = false;

  if (!mSlidingBuffer) {
    return kEOF;
  }

  char16_t theChar = 0;
  nsresult  result = Peek(theChar);
  
  if (NS_FAILED(result)) {
    return result;
  }
  
  nsScannerIterator origin, current, end;
  bool done = false;  

  origin = mCurrentPosition;
  current = origin;
  end = mEndPosition;

  bool haveCR = false;

  while(!done && current != end) {
    switch(theChar) {
      case '\n':
      case '\r':
        {
          ++aNewlinesSkipped;
          char16_t thePrevChar = theChar;
          theChar = (++current != end) ? *current : '\0';
          if ((thePrevChar == '\r' && theChar == '\n') ||
              (thePrevChar == '\n' && theChar == '\r')) {
            theChar = (++current != end) ? *current : '\0'; 
            haveCR = true;
          } else if (thePrevChar == '\r') {
            
            AppendUnicodeTo(origin, current, aString);
            aString.writable().Append(char16_t('\n'));
            origin = current;
            haveCR = true;
          }
        }
        break;
      case ' ' :
      case '\t':
        theChar = (++current != end) ? *current : '\0';
        break;
      default:
        done = true;
        AppendUnicodeTo(origin, current, aString);
        break;
    }
  }

  SetPosition(current);
  if (current == end) {
    AppendUnicodeTo(origin, current, aString);
    result = kEOF;
  }

  aHaveCR = haveCR;
  return result;
}



nsresult nsScanner::ReadWhitespace(nsScannerIterator& aStart, 
                                   nsScannerIterator& aEnd,
                                   int32_t& aNewlinesSkipped) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  char16_t theChar = 0;
  nsresult  result = Peek(theChar);
  
  if (NS_FAILED(result)) {
    return result;
  }
  
  nsScannerIterator origin, current, end;
  bool done = false;  

  origin = mCurrentPosition;
  current = origin;
  end = mEndPosition;

  while(!done && current != end) {
    switch(theChar) {
      case '\n':
      case '\r': ++aNewlinesSkipped;
      case ' ' :
      case '\t':
        {
          char16_t thePrevChar = theChar;
          theChar = (++current != end) ? *current : '\0';
          if ((thePrevChar == '\r' && theChar == '\n') ||
              (thePrevChar == '\n' && theChar == '\r')) {
            theChar = (++current != end) ? *current : '\0'; 
          }
        }
        break;
      default:
        done = true;
        aStart = origin;
        aEnd = current;
        break;
    }
  }

  SetPosition(current);
  if (current == end) {
    aStart = origin;
    aEnd = current;
    result = kEOF;
  }

  return result;
}











nsresult nsScanner::ReadUntil(nsAString& aString,
                              const nsReadEndCondition& aEndCondition,
                              bool addTerminal)
{  
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;
  const char16_t* setstart = aEndCondition.mChars;
  const char16_t* setcurrent;

  origin = mCurrentPosition;
  current = origin;

  char16_t         theChar=0;
  nsresult          result=Peek(theChar);

  if (NS_FAILED(result)) {
    return result;
  }
  
  while (current != mEndPosition) {
    theChar = *current;
    if (theChar == '\0') {
      ReplaceCharacter(current, sInvalid);
      theChar = sInvalid;
    }

    
    
    if(!(theChar & aEndCondition.mFilter)) {
      

      setcurrent = setstart;
      while (*setcurrent) {
        if (*setcurrent == theChar) {
          if(addTerminal)
            ++current;
          AppendUnicodeTo(origin, current, aString);
          SetPosition(current);

          

          return NS_OK;
        }
        ++setcurrent;
      }
    }
    
    ++current;
  }

  
  
  SetPosition(current);
  AppendUnicodeTo(origin, current, aString);
  return kEOF;
}

nsresult nsScanner::ReadUntil(nsScannerSharedSubstring& aString,
                              const nsReadEndCondition& aEndCondition,
                              bool addTerminal)
{  
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;
  const char16_t* setstart = aEndCondition.mChars;
  const char16_t* setcurrent;

  origin = mCurrentPosition;
  current = origin;

  char16_t         theChar=0;
  nsresult          result=Peek(theChar);

  if (NS_FAILED(result)) {
    return result;
  }
  
  while (current != mEndPosition) {
    theChar = *current;
    if (theChar == '\0') {
      ReplaceCharacter(current, sInvalid);
      theChar = sInvalid;
    }

    
    
    if(!(theChar & aEndCondition.mFilter)) {
      

      setcurrent = setstart;
      while (*setcurrent) {
        if (*setcurrent == theChar) {
          if(addTerminal)
            ++current;
          AppendUnicodeTo(origin, current, aString);
          SetPosition(current);

          

          return NS_OK;
        }
        ++setcurrent;
      }
    }
    
    ++current;
  }

  
  
  SetPosition(current);
  AppendUnicodeTo(origin, current, aString);
  return kEOF;
}

nsresult nsScanner::ReadUntil(nsScannerIterator& aStart, 
                              nsScannerIterator& aEnd,
                              const nsReadEndCondition &aEndCondition,
                              bool addTerminal)
{
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;
  const char16_t* setstart = aEndCondition.mChars;
  const char16_t* setcurrent;

  origin = mCurrentPosition;
  current = origin;

  char16_t         theChar=0;
  nsresult          result=Peek(theChar);
  
  if (NS_FAILED(result)) {
    aStart = aEnd = current;
    return result;
  }
  
  while (current != mEndPosition) {
    theChar = *current;
    if (theChar == '\0') {
      ReplaceCharacter(current, sInvalid);
      theChar = sInvalid;
    }

    
    
    if(!(theChar & aEndCondition.mFilter)) {
      
      setcurrent = setstart;
      while (*setcurrent) {
        if (*setcurrent == theChar) {
          if(addTerminal)
            ++current;
          aStart = origin;
          aEnd = current;
          SetPosition(current);

          return NS_OK;
        }
        ++setcurrent;
      }
    }

    ++current;
  }

  
  
  SetPosition(current);
  aStart = origin;
  aEnd = current;
  return kEOF;
}








nsresult nsScanner::ReadUntil(nsAString& aString,
                              char16_t aTerminalChar,
                              bool addTerminal)
{
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;

  origin = mCurrentPosition;
  current = origin;

  char16_t theChar;
  nsresult result = Peek(theChar);

  if (NS_FAILED(result)) {
    return result;
  }

  while (current != mEndPosition) {
    theChar = *current;
    if (theChar == '\0') {
      ReplaceCharacter(current, sInvalid);
      theChar = sInvalid;
    }

    if (aTerminalChar == theChar) {
      if(addTerminal)
        ++current;
      AppendUnicodeTo(origin, current, aString);
      SetPosition(current);
      return NS_OK;
    }
    ++current;
  }

  
  
  AppendUnicodeTo(origin, current, aString);
  SetPosition(current);
  return kEOF;

}

void nsScanner::BindSubstring(nsScannerSubstring& aSubstring, const nsScannerIterator& aStart, const nsScannerIterator& aEnd)
{
  aSubstring.Rebind(*mSlidingBuffer, aStart, aEnd);
}

void nsScanner::CurrentPosition(nsScannerIterator& aPosition)
{
  aPosition = mCurrentPosition;
}

void nsScanner::EndReading(nsScannerIterator& aPosition)
{
  aPosition = mEndPosition;
}
 
void nsScanner::SetPosition(nsScannerIterator& aPosition, bool aTerminate, bool aReverse)
{
  if (mSlidingBuffer) {
#ifdef DEBUG
    uint32_t origRemaining = mCountRemaining;
#endif

    if (aReverse) {
      mCountRemaining += (Distance(aPosition, mCurrentPosition));
    }
    else {
      mCountRemaining -= (Distance(mCurrentPosition, aPosition));
    }

    NS_ASSERTION((mCountRemaining >= origRemaining && aReverse) ||
                 (mCountRemaining <= origRemaining && !aReverse),
                 "Improper use of nsScanner::SetPosition. Make sure to set the"
                 " aReverse parameter correctly");

    mCurrentPosition = aPosition;
    if (aTerminate && (mCurrentPosition == mEndPosition)) {
      mMarkPosition = mCurrentPosition;
      mSlidingBuffer->DiscardPrefix(mCurrentPosition);
    }
  }
}

void nsScanner::ReplaceCharacter(nsScannerIterator& aPosition,
                                 char16_t aChar)
{
  if (mSlidingBuffer) {
    mSlidingBuffer->ReplaceCharacter(aPosition, aChar);
  }
}

bool nsScanner::AppendToBuffer(nsScannerString::Buffer* aBuf,
                                 nsIRequest *aRequest,
                                 int32_t aErrorPos)
{
  uint32_t countRemaining = mCountRemaining;
  if (!mSlidingBuffer) {
    mSlidingBuffer = new nsScannerString(aBuf);
    if (!mSlidingBuffer)
      return false;
    mSlidingBuffer->BeginReading(mCurrentPosition);
    mMarkPosition = mCurrentPosition;
    mSlidingBuffer->EndReading(mEndPosition);
    mCountRemaining = aBuf->DataLength();
  }
  else {
    mSlidingBuffer->AppendBuffer(aBuf);
    if (mCurrentPosition == mEndPosition) {
      mSlidingBuffer->BeginReading(mCurrentPosition);
    }
    mSlidingBuffer->EndReading(mEndPosition);
    mCountRemaining += aBuf->DataLength();
  }

  if (aErrorPos != -1 && !mHasInvalidCharacter) {
    mHasInvalidCharacter = true;
    mFirstInvalidPosition = mCurrentPosition;
    mFirstInvalidPosition.advance(countRemaining + aErrorPos);
  }

  if (mFirstNonWhitespacePosition == -1) {
    nsScannerIterator iter(mCurrentPosition);
    nsScannerIterator end(mEndPosition);

    while (iter != end) {
      if (!nsCRT::IsAsciiSpace(*iter)) {
        mFirstNonWhitespacePosition = Distance(mCurrentPosition, iter);

        break;
      }

      ++iter;
    }
  }
  return true;
}









void nsScanner::CopyUnusedData(nsString& aCopyBuffer) {
  if (!mSlidingBuffer) {
    aCopyBuffer.Truncate();
    return;
  }

  nsScannerIterator start, end;
  start = mCurrentPosition;
  end = mEndPosition;

  CopyUnicodeTo(start, end, aCopyBuffer);
}









nsString& nsScanner::GetFilename(void) {
  return mFilename;
}










void nsScanner::SelfTest(void) {
#ifdef _DEBUG
#endif
}

void nsScanner::OverrideReplacementCharacter(char16_t aReplacementCharacter)
{
  mReplacementCharacter = aReplacementCharacter;

  if (mHasInvalidCharacter) {
    ReplaceCharacter(mFirstInvalidPosition, mReplacementCharacter);
  }
}

