







































  






PRInt32
nsTString_CharT::Find( const nsCString& aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    
    Find_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = FindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), aIgnoreCase);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

PRInt32
nsTString_CharT::Find( const char* aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    return Find(nsDependentCString(aString), aIgnoreCase, aOffset, aCount);
  }


  






PRInt32
nsTString_CharT::RFind( const nsCString& aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    
    RFind_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = RFindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), aIgnoreCase);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

PRInt32
nsTString_CharT::RFind( const char* aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    return RFind(nsDependentCString(aString), aIgnoreCase, aOffset, aCount);
  }


  



PRInt32
nsTString_CharT::RFindChar( PRUnichar aChar, PRInt32 aOffset, PRInt32 aCount) const
  {
    return nsBufferRoutines<CharT>::rfind_char(mData, mLength, aOffset, aChar, aCount);
  }


  



PRInt32
nsTString_CharT::FindCharInSet( const char* aSet, PRInt32 aOffset ) const
  {
    if (aOffset < 0)
      aOffset = 0;
    else if (aOffset >= PRInt32(mLength))
      return kNotFound;
    
    PRInt32 result = ::FindCharInSet(mData + aOffset, mLength - aOffset, aSet);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }


  



PRInt32
nsTString_CharT::RFindCharInSet( const CharT* aSet, PRInt32 aOffset ) const
  {
    
    if (aOffset < 0 || aOffset > PRInt32(mLength))
      aOffset = mLength;
    else
      ++aOffset;

    return ::RFindCharInSet(mData, aOffset, aSet);
  }


  
  
  
PRInt32
nsTString_CharT::ToInteger( PRInt32* aErrorCode, PRUint32 aRadix ) const
{
  CharT*  cp=mData;
  PRInt32 theRadix=10; 
  PRInt32 result=0;
  PRBool  negate=PR_FALSE;
  CharT   theChar=0;

    
  *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
  
  if(cp) {
  
    
    
    CharT*  endcp=cp+mLength;
    PRBool  done=PR_FALSE;
    
    while((cp<endcp) && (!done)){
      switch(*cp++) {
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          theRadix=16;
          done=PR_TRUE;
          break;
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
          done=PR_TRUE;
          break;
        case '-': 
          negate=PR_TRUE; 
          break;
        case 'X': case 'x': 
          theRadix=16;
          break; 
        default:
          break;
      } 
    }

    if (done) {

        
      *aErrorCode = NS_OK;

      if (aRadix!=kAutoDetect) theRadix = aRadix; 

        
      CharT* first=--cp;  
      PRBool haveValue = PR_FALSE;

      while(cp<endcp){
        theChar=*cp++;
        if(('0'<=theChar) && (theChar<='9')){
          result = (theRadix * result) + (theChar-'0');
          haveValue = PR_TRUE;
        }
        else if((theChar>='A') && (theChar<='F')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; 
              result=0;
              haveValue = PR_FALSE;
            }
            else {
              *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'A')+10);
            haveValue = PR_TRUE;
          }
        }
        else if((theChar>='a') && (theChar<='f')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; 
              result=0;
              haveValue = PR_FALSE;
            }
            else {
              *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'a')+10);
            haveValue = PR_TRUE;
          }
        }
        else if((('X'==theChar) || ('x'==theChar)) && (!haveValue || result == 0)) {
          continue;
        }
        else if((('#'==theChar) || ('+'==theChar)) && !haveValue) {
          continue;
        }
        else {
          
          break;
        }
      } 
      if(negate)
        result=-result;
    } 
  }
  return result;
}


  



PRUint32
nsTString_CharT::Mid( self_type& aResult, index_type aStartPos, size_type aLengthToCopy ) const
  {
    if (aStartPos == 0 && aLengthToCopy >= mLength)
      aResult = *this;
    else
      aResult = Substring(*this, aStartPos, aLengthToCopy);

    return aResult.mLength;
  }


  



PRBool
nsTString_CharT::SetCharAt( PRUnichar aChar, PRUint32 aIndex )
  {
    if (aIndex >= mLength)
      return PR_FALSE;

    EnsureMutable();

    mData[aIndex] = CharT(aChar);
    return PR_TRUE;
  }

 
  



void
nsTString_CharT::StripChars( const char* aSet )
  {
    EnsureMutable();
    mLength = nsBufferRoutines<CharT>::strip_chars(mData, mLength, aSet);
  }

void
nsTString_CharT::StripWhitespace()
  {
    StripChars(kWhitespace);
  }


  



void
nsTString_CharT::ReplaceChar( char_type aOldChar, char_type aNewChar )
  {
    EnsureMutable(); 

    for (PRUint32 i=0; i<mLength; ++i)
      {
        if (mData[i] == aOldChar)
          mData[i] = aNewChar;
      }
  }

void
nsTString_CharT::ReplaceChar( const char* aSet, char_type aNewChar )
  {
    EnsureMutable(); 

    char_type* data = mData;
    PRUint32 lenRemaining = mLength;

    while (lenRemaining)
      {
        PRInt32 i = ::FindCharInSet(data, lenRemaining, aSet);
        if (i == kNotFound)
          break;

        data[i++] = aNewChar;
        data += i;
        lenRemaining -= i;
      }
  }

void
nsTString_CharT::ReplaceSubstring( const char_type* aTarget, const char_type* aNewValue )
  {
    ReplaceSubstring(nsTDependentString_CharT(aTarget),
                     nsTDependentString_CharT(aNewValue));
  }

void
nsTString_CharT::ReplaceSubstring( const self_type& aTarget, const self_type& aNewValue )
  {
    if (aTarget.Length() == 0)
      return;

    PRUint32 i = 0;
    while (i < mLength)
      {
        PRInt32 r = FindSubstring(mData + i, mLength - i, aTarget.Data(), aTarget.Length(), PR_FALSE);
        if (r == kNotFound)
          break;

        Replace(i + r, aTarget.Length(), aNewValue);
        i += r + aNewValue.Length();
      }
  }


  



void
nsTString_CharT::Trim( const char* aSet, PRBool aTrimLeading, PRBool aTrimTrailing, PRBool aIgnoreQuotes )
  {
      
    if (!aSet)
      return;

    char_type* start = mData;
    char_type* end   = mData + mLength;

      
    if (aIgnoreQuotes && mLength > 2 && mData[0] == mData[mLength - 1] &&
          (mData[0] == '\'' || mData[0] == '"'))
      {
        ++start;
        --end;
      }

    PRUint32 setLen = nsCharTraits<char>::length(aSet);

    if (aTrimLeading)
      {
        PRUint32 cutStart = start - mData;
        PRUint32 cutLength = 0;

          
        for (; start != end; ++start, ++cutLength)
          {
            PRInt32 pos = FindChar1(aSet, setLen, 0, *start, setLen);
            if (pos == kNotFound)
              break;
          }

        if (cutLength)
          {
            Cut(cutStart, cutLength);

              
            start = mData + cutStart;
            end   = mData + mLength - cutStart;
          }
      }

    if (aTrimTrailing)
      {
        PRUint32 cutEnd = end - mData;
        PRUint32 cutLength = 0;

          
        --end;
        for (; end >= start; --end, ++cutLength)
          {
            PRInt32 pos = FindChar1(aSet, setLen, 0, *end, setLen);
            if (pos == kNotFound)
              break;
          }

        if (cutLength)
          Cut(cutEnd - cutLength, cutLength);
      }
  }


  



void
nsTString_CharT::CompressWhitespace( PRBool aTrimLeading, PRBool aTrimTrailing )
  {
    const char* set = kWhitespace;

    ReplaceChar(set, ' ');
    Trim(set, aTrimLeading, aTrimTrailing);

      
    mLength = nsBufferRoutines<char_type>::compress_chars(mData, mLength, set);
  }


  



void
nsTString_CharT::AssignWithConversion( const incompatible_char_type* aData, PRInt32 aLength )
  {
      
      
    if (!aData)
      {
        Truncate();
      }
    else
      {
        if (aLength < 0)
          aLength = nsCharTraits<incompatible_char_type>::length(aData);

        AssignWithConversion(Substring(aData, aData + aLength));
      }
  }


  



void
nsTString_CharT::AppendWithConversion( const incompatible_char_type* aData, PRInt32 aLength )
  {
      
      
    if (aData)
      {
        if (aLength < 0)
          aLength = nsCharTraits<incompatible_char_type>::length(aData);

        AppendWithConversion(Substring(aData, aData + aLength));
      }
  }
