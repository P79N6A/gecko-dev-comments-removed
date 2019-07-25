







  






int32_t
nsTString_CharT::Find( const nsCString& aString, bool aIgnoreCase, int32_t aOffset, int32_t aCount) const
  {
    
    Find_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    int32_t result = FindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), aIgnoreCase);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

int32_t
nsTString_CharT::Find( const char* aString, bool aIgnoreCase, int32_t aOffset, int32_t aCount) const
  {
    return Find(nsDependentCString(aString), aIgnoreCase, aOffset, aCount);
  }


  






int32_t
nsTString_CharT::RFind( const nsCString& aString, bool aIgnoreCase, int32_t aOffset, int32_t aCount) const
  {
    
    RFind_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    int32_t result = RFindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), aIgnoreCase);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

int32_t
nsTString_CharT::RFind( const char* aString, bool aIgnoreCase, int32_t aOffset, int32_t aCount) const
  {
    return RFind(nsDependentCString(aString), aIgnoreCase, aOffset, aCount);
  }


  



int32_t
nsTString_CharT::RFindChar( PRUnichar aChar, int32_t aOffset, int32_t aCount) const
  {
    return nsBufferRoutines<CharT>::rfind_char(mData, mLength, aOffset, aChar, aCount);
  }


  



int32_t
nsTString_CharT::FindCharInSet( const char* aSet, int32_t aOffset ) const
  {
    if (aOffset < 0)
      aOffset = 0;
    else if (aOffset >= int32_t(mLength))
      return kNotFound;
    
    int32_t result = ::FindCharInSet(mData + aOffset, mLength - aOffset, aSet);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }


  



int32_t
nsTString_CharT::RFindCharInSet( const CharT* aSet, int32_t aOffset ) const
  {
    
    if (aOffset < 0 || aOffset > int32_t(mLength))
      aOffset = mLength;
    else
      ++aOffset;

    return ::RFindCharInSet(mData, aOffset, aSet);
  }


  
  
  
int32_t
nsTString_CharT::ToInteger( nsresult* aErrorCode, uint32_t aRadix ) const
{
  CharT*  cp=mData;
  int32_t theRadix=10; 
  int32_t result=0;
  bool    negate=false;
  CharT   theChar=0;

    
  *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
  
  if(cp) {
  
    
    
    CharT*  endcp=cp+mLength;
    bool    done=false;
    
    while((cp<endcp) && (!done)){
      switch(*cp++) {
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          theRadix=16;
          done=true;
          break;
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
          done=true;
          break;
        case '-': 
          negate=true; 
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
      bool haveValue = false;

      while(cp<endcp){
        int32_t oldresult = result;

        theChar=*cp++;
        if(('0'<=theChar) && (theChar<='9')){
          result = (theRadix * result) + (theChar-'0');
          haveValue = true;
        }
        else if((theChar>='A') && (theChar<='F')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; 
              result=0;
              haveValue = false;
            }
            else {
              *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'A')+10);
            haveValue = true;
          }
        }
        else if((theChar>='a') && (theChar<='f')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; 
              result=0;
              haveValue = false;
            }
            else {
              *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'a')+10);
            haveValue = true;
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

        if (result < oldresult) {
          
          *aErrorCode = NS_ERROR_ILLEGAL_VALUE;
          result = 0;
          break;
        }
      } 
      if(negate)
        result=-result;
    } 
  }
  return result;
}


  



uint32_t
nsTString_CharT::Mid( self_type& aResult, index_type aStartPos, size_type aLengthToCopy ) const
  {
    if (aStartPos == 0 && aLengthToCopy >= mLength)
      aResult = *this;
    else
      aResult = Substring(*this, aStartPos, aLengthToCopy);

    return aResult.mLength;
  }


  



bool
nsTString_CharT::SetCharAt( PRUnichar aChar, uint32_t aIndex )
  {
    if (aIndex >= mLength)
      return false;

    if (!EnsureMutable())
      NS_RUNTIMEABORT("OOM");

    mData[aIndex] = CharT(aChar);
    return true;
  }

 
  



void
nsTString_CharT::StripChars( const char* aSet )
  {
    if (!EnsureMutable())
      NS_RUNTIMEABORT("OOM");

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
    if (!EnsureMutable()) 
      NS_RUNTIMEABORT("OOM");

    for (uint32_t i=0; i<mLength; ++i)
      {
        if (mData[i] == aOldChar)
          mData[i] = aNewChar;
      }
  }

void
nsTString_CharT::ReplaceChar( const char* aSet, char_type aNewChar )
  {
    if (!EnsureMutable()) 
      NS_RUNTIMEABORT("OOM");

    char_type* data = mData;
    uint32_t lenRemaining = mLength;

    while (lenRemaining)
      {
        int32_t i = ::FindCharInSet(data, lenRemaining, aSet);
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

    uint32_t i = 0;
    while (i < mLength)
      {
        int32_t r = FindSubstring(mData + i, mLength - i, aTarget.Data(), aTarget.Length(), false);
        if (r == kNotFound)
          break;

        Replace(i + r, aTarget.Length(), aNewValue);
        i += r + aNewValue.Length();
      }
  }


  



void
nsTString_CharT::Trim( const char* aSet, bool aTrimLeading, bool aTrimTrailing, bool aIgnoreQuotes )
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

    uint32_t setLen = nsCharTraits<char>::length(aSet);

    if (aTrimLeading)
      {
        uint32_t cutStart = start - mData;
        uint32_t cutLength = 0;

          
        for (; start != end; ++start, ++cutLength)
          {
            int32_t pos = FindChar1(aSet, setLen, 0, *start, setLen);
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
        uint32_t cutEnd = end - mData;
        uint32_t cutLength = 0;

          
        --end;
        for (; end >= start; --end, ++cutLength)
          {
            int32_t pos = FindChar1(aSet, setLen, 0, *end, setLen);
            if (pos == kNotFound)
              break;
          }

        if (cutLength)
          Cut(cutEnd - cutLength, cutLength);
      }
  }


  



void
nsTString_CharT::CompressWhitespace( bool aTrimLeading, bool aTrimTrailing )
  {
    const char* set = kWhitespace;

    ReplaceChar(set, ' ');
    Trim(set, aTrimLeading, aTrimTrailing);

      
    mLength = nsBufferRoutines<char_type>::compress_chars(mData, mLength, set);
  }


  



void
nsTString_CharT::AssignWithConversion( const incompatible_char_type* aData, int32_t aLength )
  {
      
      
    if (!aData)
      {
        Truncate();
      }
    else
      {
        if (aLength < 0)
          aLength = nsCharTraits<incompatible_char_type>::length(aData);

        AssignWithConversion(Substring(aData, aLength));
      }
  }
