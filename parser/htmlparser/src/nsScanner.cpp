







































#include "nsScanner.h"
#include "nsDebug.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsReadableUtils.h"
#include "nsIInputStream.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsUTF8Utils.h" 
#include "nsCRT.h"
#include "nsParser.h"


static PRUnichar sInvalid = UCS2_REPLACEMENT_CHAR;

nsReadEndCondition::nsReadEndCondition(const PRUnichar* aTerminateChars) :
  mChars(aTerminateChars), mFilter(PRUnichar(~0)) 
{
  
  
  
  
  
  
  
  const PRUnichar *current = aTerminateChars;
  PRUnichar terminalChar = *current;
  while (terminalChar) {
    mFilter &= ~terminalChar;
    ++current;
    terminalChar = *current;
  }
}

#ifdef __INCREMENTAL
const int   kBufsize=1;
#else
const int   kBufsize=64;
#endif










nsScanner::nsScanner(const nsAString& anHTMLString, const nsACString& aCharset,
                     PRInt32 aSource)
  : mParser(nsnull)
{
  MOZ_COUNT_CTOR(nsScanner);

  mSlidingBuffer = nsnull;
  mCountRemaining = 0;
  mFirstNonWhitespacePosition = -1;
  if (AppendToBuffer(anHTMLString)) {
    mSlidingBuffer->BeginReading(mCurrentPosition);
  } else {
    
    memset(&mCurrentPosition, 0, sizeof(mCurrentPosition));
    mEndPosition = mCurrentPosition;
  }
  mMarkPosition = mCurrentPosition;
  mIncremental = PR_FALSE;
  mUnicodeDecoder = 0;
  mCharsetSource = kCharsetUninitialized;
  mHasInvalidCharacter = PR_FALSE;
  mReplacementCharacter = PRUnichar(0x0);
}










nsScanner::nsScanner(nsString& aFilename,PRBool aCreateStream,
                     const nsACString& aCharset, PRInt32 aSource)
  : mFilename(aFilename), mParser(nsnull)
{
  MOZ_COUNT_CTOR(nsScanner);
  NS_ASSERTION(!aCreateStream, "This is always true.");

  mSlidingBuffer = nsnull;

  
  
  
  
  
  memset(&mCurrentPosition, 0, sizeof(mCurrentPosition));
  mMarkPosition = mCurrentPosition;
  mEndPosition = mCurrentPosition;

  mIncremental = PR_TRUE;
  mFirstNonWhitespacePosition = -1;
  mCountRemaining = 0;

  mUnicodeDecoder = 0;
  mCharsetSource = kCharsetUninitialized;
  mHasInvalidCharacter = PR_FALSE;
  mReplacementCharacter = PRUnichar(0x0);
  SetDocumentCharset(aCharset, aSource);
}

nsresult nsScanner::SetDocumentCharset(const nsACString& aCharset , PRInt32 aSource)
{
  if (aSource < mCharsetSource) 
    return NS_OK;

  nsICharsetAlias* calias = nsParser::GetCharsetAliasService();
  NS_ASSERTION(calias, "Must have the charset alias service!");

  nsresult res = NS_OK;
  if (!mCharset.IsEmpty())
  {
    PRBool same;
    res = calias->Equals(aCharset, mCharset, &same);
    if(NS_SUCCEEDED(res) && same)
    {
      return NS_OK; 
    }
  }

  
  nsCString charsetName;
  res = calias->GetPreferred(aCharset, charsetName);

  if(NS_FAILED(res) && (mCharsetSource == kCharsetUninitialized))
  {
     
    mCharset.AssignLiteral("ISO-8859-1");
  }
  else
  {
    mCharset.Assign(charsetName);
  }

  mCharsetSource = aSource;

  NS_ASSERTION(nsParser::GetCharsetConverterManager(),
               "Must have the charset converter manager!");

  res = nsParser::GetCharsetConverterManager()->
    GetUnicodeDecoderRaw(mCharset.get(), getter_AddRefs(mUnicodeDecoder));
  if (NS_SUCCEEDED(res) && mUnicodeDecoder)
  {
     
     
     mUnicodeDecoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);
  }

  return res;
}









nsScanner::~nsScanner() {

  if (mSlidingBuffer) {
    delete mSlidingBuffer;
  }

  MOZ_COUNT_DTOR(nsScanner);
}











void nsScanner::RewindToMark(void){
  if (mSlidingBuffer) {
    mCountRemaining += (Distance(mMarkPosition, mCurrentPosition));
    mCurrentPosition = mMarkPosition;
  }
}











PRInt32 nsScanner::Mark() {
  PRInt32 distance = 0;
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








PRBool nsScanner::UngetReadable(const nsAString& aBuffer) {
  if (!mSlidingBuffer) {
    return PR_FALSE;
  }

  mSlidingBuffer->UngetReadable(aBuffer,mCurrentPosition);
  mSlidingBuffer->BeginReading(mCurrentPosition); 
  mSlidingBuffer->EndReading(mEndPosition);
 
  PRUint32 length = aBuffer.Length();
  mCountRemaining += length; 
  return PR_TRUE;
}








nsresult nsScanner::Append(const nsAString& aBuffer) {
  if (!AppendToBuffer(aBuffer))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}








nsresult nsScanner::Append(const char* aBuffer, PRUint32 aLen,
                           nsIRequest *aRequest)
{
  nsresult res=NS_OK;
  PRUnichar *unichars, *start;
  if (mUnicodeDecoder) {
    PRInt32 unicharBufLen = 0;
    mUnicodeDecoder->GetMaxLength(aBuffer, aLen, &unicharBufLen);
    nsScannerString::Buffer* buffer = nsScannerString::AllocBuffer(unicharBufLen + 1);
    NS_ENSURE_TRUE(buffer,NS_ERROR_OUT_OF_MEMORY);
    start = unichars = buffer->DataStart();

    PRInt32 totalChars = 0;
    PRInt32 unicharLength = unicharBufLen;
    PRInt32 errorPos = -1;

    do {
      PRInt32 srcLength = aLen;
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

        if(((PRUint32) (srcLength + 1)) > aLen) {
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








nsresult nsScanner::GetChar(PRUnichar& aChar) {
  if (!mSlidingBuffer || mCurrentPosition == mEndPosition) {
    aChar = 0;
    return kEOF;
  }

  aChar = *mCurrentPosition++;
  --mCountRemaining;

  return NS_OK;
}










nsresult nsScanner::Peek(PRUnichar& aChar, PRUint32 aOffset) {
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

nsresult nsScanner::Peek(nsAString& aStr, PRInt32 aNumChars, PRInt32 aOffset)
{
  if (!mSlidingBuffer || mCurrentPosition == mEndPosition) {
    return kEOF;
  }

  nsScannerIterator start, end;

  start = mCurrentPosition;

  if ((PRInt32)mCountRemaining <= aOffset) {
    return kEOF;
  }

  if (aOffset > 0) {
    start.advance(aOffset);
  }

  if (mCountRemaining < PRUint32(aNumChars + aOffset)) {
    end = mEndPosition;
  }
  else {
    end = start;
    end.advance(aNumChars);
  }

  CopyUnicodeTo(start, end, aStr);

  return NS_OK;
}









nsresult nsScanner::SkipWhitespace(PRInt32& aNewlinesSkipped) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  PRUnichar theChar = 0;
  nsresult  result = Peek(theChar);
  
  if (NS_FAILED(result)) {
    return result;
  }
  
  nsScannerIterator current = mCurrentPosition;
  PRBool    done = PR_FALSE;
  PRBool    skipped = PR_FALSE;
  
  while (!done && current != mEndPosition) {
    switch(theChar) {
      case '\n':
      case '\r': ++aNewlinesSkipped;
      case ' ' :
      case '\t':
        {
          skipped = PR_TRUE;
          PRUnichar thePrevChar = theChar;
          theChar = (++current != mEndPosition) ? *current : '\0';
          if ((thePrevChar == '\r' && theChar == '\n') ||
              (thePrevChar == '\n' && theChar == '\r')) {
            theChar = (++current != mEndPosition) ? *current : '\0'; 
          }
        }
        break;
      default:
        done = PR_TRUE;
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








nsresult nsScanner::SkipOver(PRUnichar aSkipChar){

  if (!mSlidingBuffer) {
    return kEOF;
  }

  PRUnichar ch=0;
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
  PRInt32 pos=aString.FindChar(0);
  if(kNotFound<pos) {
    if(aString.Length()-1!=pos) {
    }
  }
}

void DoErrTest(nsCString& aString) {
  PRInt32 pos=aString.FindChar(0);
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

  PRUnichar         theChar=0;
  nsresult          result=Peek(theChar);
  nsScannerIterator current, end;
  PRBool            found=PR_FALSE;  
  
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
        found = PR_TRUE;
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

  PRUnichar         theChar=0;
  nsresult          result=Peek(theChar);
  nsScannerIterator origin, current, end;
  PRBool            found=PR_FALSE;  

  origin = mCurrentPosition;
  current = mCurrentPosition;
  end = mEndPosition;

  while(current != end) {
 
    theChar=*current;
    if(theChar) {
      found=PR_FALSE;
      switch(theChar) {
        case '_':
        case '-':
        case '.':
          
          found = PR_TRUE;
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







nsresult nsScanner::ReadNumber(nsString& aString,PRInt32 aBase) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  NS_ASSERTION(aBase == 10 || aBase == 16,"base value not supported");

  PRUnichar         theChar=0;
  nsresult          result=Peek(theChar);
  nsScannerIterator origin, current, end;

  origin = mCurrentPosition;
  current = origin;
  end = mEndPosition;

  PRBool done = PR_FALSE;
  while(current != end) {
    theChar=*current;
    if(theChar) {
      done = (theChar < '0' || theChar > '9') && 
             ((aBase == 16)? (theChar < 'A' || theChar > 'F') &&
                             (theChar < 'a' || theChar > 'f')
                             :PR_TRUE);
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
                                   PRInt32& aNewlinesSkipped,
                                   PRBool& aHaveCR) {

  aHaveCR = PR_FALSE;

  if (!mSlidingBuffer) {
    return kEOF;
  }

  PRUnichar theChar = 0;
  nsresult  result = Peek(theChar);
  
  if (NS_FAILED(result)) {
    return result;
  }
  
  nsScannerIterator origin, current, end;
  PRBool done = PR_FALSE;  

  origin = mCurrentPosition;
  current = origin;
  end = mEndPosition;

  PRBool haveCR = PR_FALSE;

  while(!done && current != end) {
    switch(theChar) {
      case '\n':
      case '\r':
        {
          ++aNewlinesSkipped;
          PRUnichar thePrevChar = theChar;
          theChar = (++current != end) ? *current : '\0';
          if ((thePrevChar == '\r' && theChar == '\n') ||
              (thePrevChar == '\n' && theChar == '\r')) {
            theChar = (++current != end) ? *current : '\0'; 
            haveCR = PR_TRUE;
          } else if (thePrevChar == '\r') {
            
            AppendUnicodeTo(origin, current, aString);
            aString.writable().Append(PRUnichar('\n'));
            origin = current;
            haveCR = PR_TRUE;
          }
        }
        break;
      case ' ' :
      case '\t':
        theChar = (++current != end) ? *current : '\0';
        break;
      default:
        done = PR_TRUE;
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
                                   PRInt32& aNewlinesSkipped) {

  if (!mSlidingBuffer) {
    return kEOF;
  }

  PRUnichar theChar = 0;
  nsresult  result = Peek(theChar);
  
  if (NS_FAILED(result)) {
    return result;
  }
  
  nsScannerIterator origin, current, end;
  PRBool done = PR_FALSE;  

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
          PRUnichar thePrevChar = theChar;
          theChar = (++current != end) ? *current : '\0';
          if ((thePrevChar == '\r' && theChar == '\n') ||
              (thePrevChar == '\n' && theChar == '\r')) {
            theChar = (++current != end) ? *current : '\0'; 
          }
        }
        break;
      default:
        done = PR_TRUE;
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
                              PRBool addTerminal)
{  
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;
  const PRUnichar* setstart = aEndCondition.mChars;
  const PRUnichar* setcurrent;

  origin = mCurrentPosition;
  current = origin;

  PRUnichar         theChar=0;
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
                              PRBool addTerminal)
{  
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;
  const PRUnichar* setstart = aEndCondition.mChars;
  const PRUnichar* setcurrent;

  origin = mCurrentPosition;
  current = origin;

  PRUnichar         theChar=0;
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
                              PRBool addTerminal)
{
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;
  const PRUnichar* setstart = aEndCondition.mChars;
  const PRUnichar* setcurrent;

  origin = mCurrentPosition;
  current = origin;

  PRUnichar         theChar=0;
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
                              PRUnichar aTerminalChar,
                              PRBool addTerminal)
{
  if (!mSlidingBuffer) {
    return kEOF;
  }

  nsScannerIterator origin, current;

  origin = mCurrentPosition;
  current = origin;

  PRUnichar theChar;
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
 
void nsScanner::SetPosition(nsScannerIterator& aPosition, PRBool aTerminate, PRBool aReverse)
{
  if (mSlidingBuffer) {
#ifdef DEBUG
    PRUint32 origRemaining = mCountRemaining;
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
                                 PRUnichar aChar)
{
  if (mSlidingBuffer) {
    mSlidingBuffer->ReplaceCharacter(aPosition, aChar);
  }
}

PRBool nsScanner::AppendToBuffer(nsScannerString::Buffer* aBuf,
                                 nsIRequest *aRequest,
                                 PRInt32 aErrorPos)
{
  if (nsParser::sParserDataListeners && mParser &&
      NS_FAILED(mParser->DataAdded(Substring(aBuf->DataStart(),
                                             aBuf->DataEnd()), aRequest))) {
    

    return mSlidingBuffer != nsnull;
  }

  if (!mSlidingBuffer) {
    mSlidingBuffer = new nsScannerString(aBuf);
    if (!mSlidingBuffer)
      return PR_FALSE;
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
    mHasInvalidCharacter = PR_TRUE;
    mFirstInvalidPosition = mCurrentPosition;
    mFirstInvalidPosition.advance(aErrorPos);
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
  return PR_TRUE;
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

void nsScanner::OverrideReplacementCharacter(PRUnichar aReplacementCharacter)
{
  mReplacementCharacter = aReplacementCharacter;

  if (mHasInvalidCharacter) {
    ReplaceCharacter(mFirstInvalidPosition, mReplacementCharacter);
  }
}

