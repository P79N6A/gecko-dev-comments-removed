





#include "nsString.h"


  



#if MOZ_STRING_WITH_OBSOLETE_API

#include "nsDependentString.h"
#include "nsDependentSubstring.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsUTF8Utils.h"
#include "prdtoa.h"










inline char
ascii_tolower(char aChar)
{
  if (aChar >= 'A' && aChar <= 'Z')
    return aChar + ('a' - 'A');
  return aChar;
}


















static int32_t
FindChar1(const char* aDest,uint32_t aDestLength,int32_t anOffset,const PRUnichar aChar,int32_t aCount) {

  if(anOffset < 0)
    anOffset=0;

  if(aCount < 0)
    aCount = (int32_t)aDestLength;

  if((aChar < 256) && (0 < aDestLength) && ((uint32_t)anOffset < aDestLength)) {

    
    
    
    if(0<aCount) {

      const char* left= aDest+anOffset;
      const char* last= left+aCount;
      const char* max = aDest+aDestLength;
      const char* end = (last<max) ? last : max;

      int32_t theMax = end-left;
      if(0<theMax) {
        
        unsigned char theChar = (unsigned char) aChar;
        const char* result=(const char*)memchr(left, (int)theChar, theMax);
        
        if(result)
          return result-aDest;
        
      }
    }
  }

  return kNotFound;
}













static int32_t
FindChar2(const PRUnichar* aDest,uint32_t aDestLength,int32_t anOffset,const PRUnichar aChar,int32_t aCount) {

  if(anOffset < 0)
    anOffset=0;

  if(aCount < 0)
    aCount = (int32_t)aDestLength;

  if((0<aDestLength) && ((uint32_t)anOffset < aDestLength)) {
 
    if(0<aCount) {

      const PRUnichar* root = aDest;
      const PRUnichar* left = root+anOffset;
      const PRUnichar* last = left+aCount;
      const PRUnichar* max  = root+aDestLength;
      const PRUnichar* end  = (last<max) ? last : max;

      while(left<end){
        
        if(*left==aChar)
          return (left-root);
        
        ++left;
      }
    }
  }

  return kNotFound;
}














static int32_t
RFindChar1(const char* aDest,uint32_t aDestLength,int32_t anOffset,const PRUnichar aChar,int32_t aCount) {

  if(anOffset < 0)
    anOffset=(int32_t)aDestLength-1;

  if(aCount < 0)
    aCount = int32_t(aDestLength);

  if((aChar<256) && (0 < aDestLength) && ((uint32_t)anOffset < aDestLength)) {

    
    
 
    if(0 < aCount) {

      const char* rightmost = aDest + anOffset;  
      const char* min       = rightmost - aCount + 1;
      const char* leftmost  = (min<aDest) ? aDest: min;

      char theChar=(char)aChar;
      while(leftmost <= rightmost){
        
        if((*rightmost) == theChar)
          return rightmost - aDest;
        
        --rightmost;
      }
    }
  }

  return kNotFound;
}













static int32_t
RFindChar2(const PRUnichar* aDest,uint32_t aDestLength,int32_t anOffset,const PRUnichar aChar,int32_t aCount) {

  if(anOffset < 0)
    anOffset=(int32_t)aDestLength-1;

  if(aCount < 0)
    aCount = int32_t(aDestLength);

  if((0 < aDestLength) && ((uint32_t)anOffset < aDestLength)) {
 
    if(0 < aCount) {

      const PRUnichar* root      = aDest;
      const PRUnichar* rightmost = root + anOffset;  
      const PRUnichar* min       = rightmost - aCount + 1;
      const PRUnichar* leftmost  = (min<root) ? root: min;
      
      while(leftmost <= rightmost){
        
        if((*rightmost) == aChar)
          return rightmost - root;
        
        --rightmost;
      }
    }
  }

  return kNotFound;
}




















static
#ifdef __SUNPRO_CC
inline
#endif 
int32_t
Compare1To1(const char* aStr1,const char* aStr2,uint32_t aCount,bool aIgnoreCase){ 
  int32_t result=0;
  if(aIgnoreCase)
    result=int32_t(PL_strncasecmp(aStr1, aStr2, aCount));
  else 
    result=nsCharTraits<char>::compare(aStr1,aStr2,aCount);

      
      
  if ( result < -1 )
    result = -1;
  else if ( result > 1 )
    result = 1;
  return result;
}










static 
#ifdef __SUNPRO_CC
inline
#endif 
int32_t
Compare2To2(const PRUnichar* aStr1,const PRUnichar* aStr2,uint32_t aCount){
  int32_t result;
  
  if ( aStr1 && aStr2 )
    result = nsCharTraits<PRUnichar>::compare(aStr1, aStr2, aCount);

      
      
      
      
  else if ( !aStr1 && !aStr2 )
    result = 0;
  else if ( aStr1 )
    result = 1;
  else
    result = -1;

      
  if ( result < -1 )
    result = -1;
  else if ( result > 1 )
    result = 1;
  return result;
}











static
#ifdef __SUNPRO_CC
inline
#endif 
int32_t
Compare2To1(const PRUnichar* aStr1,const char* aStr2,uint32_t aCount,bool aIgnoreCase){
  const PRUnichar* s1 = aStr1;
  const char *s2 = aStr2;
  
  if (aStr1 && aStr2) {
    if (aCount != 0) {
      do {

        PRUnichar c1 = *s1++;
        PRUnichar c2 = PRUnichar((unsigned char)*s2++);
        
        if (c1 != c2) {
#ifdef DEBUG
          
          
          
          
          if (aIgnoreCase && c2>=128)
            NS_WARNING("got a non-ASCII string, but we can't do an accurate case conversion!");
#endif

          
          if (aIgnoreCase && c1<128 && c2<128) {

              c1 = ascii_tolower(char(c1));
              c2 = ascii_tolower(char(c2));
            
              if (c1 == c2) continue;
          }

          if (c1 < c2) return -1;
          return 1;
        }
      } while (--aCount);
    }
  }
  return 0;
}











inline int32_t
Compare1To2(const char* aStr1,const PRUnichar* aStr2,uint32_t aCount,bool aIgnoreCase){
  return Compare2To1(aStr2, aStr1, aCount, aIgnoreCase) * -1;
}



















static int32_t
CompressChars1(char* aString,uint32_t aLength,const char* aSet){ 

  char*  from = aString;
  char*  end =  aString + aLength;
  char*  to = from;

    
    
  if(aSet && aString && (0 < aLength)){
    uint32_t aSetLen=strlen(aSet);

    while (from < end) {
      char theChar = *from++;
      
      *to++=theChar; 

      if((kNotFound!=FindChar1(aSet,aSetLen,0,theChar,aSetLen))){
        while (from < end) {
          theChar = *from++;
          if(kNotFound==FindChar1(aSet,aSetLen,0,theChar,aSetLen)){
            *to++ = theChar;
            break;
          }
        } 
      } 
    } 
    *to = 0;
  }
  return to - aString;
}














static int32_t
CompressChars2(PRUnichar* aString,uint32_t aLength,const char* aSet){ 

  PRUnichar*  from = aString;
  PRUnichar*  end =  from + aLength;
  PRUnichar*  to = from;

    
    
  if(aSet && aString && (0 < aLength)){
    uint32_t aSetLen=strlen(aSet);

    while (from < end) {
      PRUnichar theChar = *from++;
      
      *to++=theChar; 

      if((theChar<256) && (kNotFound!=FindChar1(aSet,aSetLen,0,theChar,aSetLen))){
        while (from < end) {
          theChar = *from++;
          if(kNotFound==FindChar1(aSet,aSetLen,0,theChar,aSetLen)){
            *to++ = theChar;
            break;
          }
        } 
      } 
    } 
    *to = 0;
  }
  return to - (PRUnichar*)aString;
}












static int32_t
StripChars1(char* aString,uint32_t aLength,const char* aSet){ 

  

  char*  to   = aString;
  char*  from = aString-1;
  char*  end  = aString + aLength;

  if(aSet && aString && (0 < aLength)){
    uint32_t aSetLen=strlen(aSet);
    while (++from < end) {
      char theChar = *from;
      if(kNotFound==FindChar1(aSet,aSetLen,0,theChar,aSetLen)){
        *to++ = theChar;
      }
    }
    *to = 0;
  }
  return to - (char*)aString;
}













static int32_t
StripChars2(PRUnichar* aString,uint32_t aLength,const char* aSet){ 

  

  PRUnichar*  to   = aString;
  PRUnichar*  from = aString-1;
  PRUnichar*  end  = to + aLength;

  if(aSet && aString && (0 < aLength)){
    uint32_t aSetLen=strlen(aSet);
    while (++from < end) {
      PRUnichar theChar = *from;
      
      
      
      if((255<theChar) || (kNotFound==FindChar1(aSet,aSetLen,0,theChar,aSetLen))){
        *to++ = theChar;
      }
    }
    *to = 0;
  }
  return to - (PRUnichar*)aString;
}



static const char* kWhitespace="\b\t\r\n ";


template <class CharT>
#ifndef __SUNPRO_CC
static
#endif 
CharT
GetFindInSetFilter( const CharT* set)
  {
    CharT filter = ~CharT(0); 
    while (*set) {
      filter &= ~(*set);
      ++set;
    }
    return filter;
  }


template <class CharT> struct nsBufferRoutines {};

template <>
struct nsBufferRoutines<char>
  {
    static
    int32_t compare( const char* a, const char* b, uint32_t max, bool ic )
      {
        return Compare1To1(a, b, max, ic);
      }

    static
    int32_t compare( const char* a, const PRUnichar* b, uint32_t max, bool ic )
      {
        return Compare1To2(a, b, max, ic);
      }

    static
    int32_t find_char( const char* s, uint32_t max, int32_t offset, const PRUnichar c, int32_t count )
      {
        return FindChar1(s, max, offset, c, count);
      }

    static
    int32_t rfind_char( const char* s, uint32_t max, int32_t offset, const PRUnichar c, int32_t count )
      {
        return RFindChar1(s, max, offset, c, count);
      }

    static
    char get_find_in_set_filter( const char* set )
      {
        return GetFindInSetFilter(set);
      }

    static
    int32_t strip_chars( char* s, uint32_t len, const char* set )
      {
        return StripChars1(s, len, set);
      }

    static
    int32_t compress_chars( char* s, uint32_t len, const char* set ) 
      {
        return CompressChars1(s, len, set);
      }
  };

template <>
struct nsBufferRoutines<PRUnichar>
  {
    static
    int32_t compare( const PRUnichar* a, const PRUnichar* b, uint32_t max, bool ic )
      {
        NS_ASSERTION(!ic, "no case-insensitive compare here");
        return Compare2To2(a, b, max);
      }

    static
    int32_t compare( const PRUnichar* a, const char* b, uint32_t max, bool ic )
      {
        return Compare2To1(a, b, max, ic);
      }

    static
    int32_t find_char( const PRUnichar* s, uint32_t max, int32_t offset, const PRUnichar c, int32_t count )
      {
        return FindChar2(s, max, offset, c, count);
      }

    static
    int32_t rfind_char( const PRUnichar* s, uint32_t max, int32_t offset, const PRUnichar c, int32_t count )
      {
        return RFindChar2(s, max, offset, c, count);
      }

    static
    PRUnichar get_find_in_set_filter( const PRUnichar* set )
      {
        return GetFindInSetFilter(set);
      }

    static
    PRUnichar get_find_in_set_filter( const char* set )
      {
        return (~PRUnichar(0)^~char(0)) | GetFindInSetFilter(set);
      }

    static
    int32_t strip_chars( PRUnichar* s, uint32_t max, const char* set )
      {
        return StripChars2(s, max, set);
      }

    static
    int32_t compress_chars( PRUnichar* s, uint32_t len, const char* set ) 
      {
        return CompressChars2(s, len, set);
      }
  };



template <class L, class R>
#ifndef __SUNPRO_CC
static
#endif 
int32_t
FindSubstring( const L* big, uint32_t bigLen,
               const R* little, uint32_t littleLen,
               bool ignoreCase )
  {
    if (littleLen > bigLen)
      return kNotFound;

    int32_t i, max = int32_t(bigLen - littleLen);
    for (i=0; i<=max; ++i, ++big)
      {
        if (nsBufferRoutines<L>::compare(big, little, littleLen, ignoreCase) == 0)
          return i;
      }

    return kNotFound;
  }

template <class L, class R>
#ifndef __SUNPRO_CC
static
#endif 
int32_t
RFindSubstring( const L* big, uint32_t bigLen,
                const R* little, uint32_t littleLen,
                bool ignoreCase )
  {
    if (littleLen > bigLen)
      return kNotFound;

    int32_t i, max = int32_t(bigLen - littleLen);

    const L* iter = big + max;
    for (i=max; iter >= big; --i, --iter)
      {
        if (nsBufferRoutines<L>::compare(iter, little, littleLen, ignoreCase) == 0)
          return i;
      }

    return kNotFound;
  }

template <class CharT, class SetCharT>
#ifndef __SUNPRO_CC
static
#endif 
int32_t
FindCharInSet( const CharT* data, uint32_t dataLen, const SetCharT* set )
  {
    CharT filter = nsBufferRoutines<CharT>::get_find_in_set_filter(set);

    const CharT* end = data + dataLen; 
    for (const CharT* iter = data; iter < end; ++iter)
      {
        CharT currentChar = *iter;
        if (currentChar & filter)
          continue; 

        
        const SetCharT* charInSet = set;
        CharT setChar = CharT(*charInSet);
        while (setChar)
          {
            if (setChar == currentChar)
              return iter - data; 

            setChar = CharT(*(++charInSet));
          }
      }
    return kNotFound;
  }

template <class CharT, class SetCharT>
#ifndef __SUNPRO_CC
static
#endif 
int32_t
RFindCharInSet( const CharT* data, uint32_t dataLen, const SetCharT* set )
  {
    CharT filter = nsBufferRoutines<CharT>::get_find_in_set_filter(set);

    for (const CharT* iter = data + dataLen - 1; iter >= data; --iter)
      {
        CharT currentChar = *iter;
        if (currentChar & filter)
          continue; 

        
        const CharT* charInSet = set;
        CharT setChar = *charInSet;
        while (setChar)
          {
            if (setChar == currentChar)
              return iter - data; 

            setChar = *(++charInSet);
          }
      }
    return kNotFound;
  }

  





 
static void
Find_ComputeSearchRange( uint32_t bigLen, uint32_t littleLen, int32_t& offset, int32_t& count )
  {
    

    if (offset < 0)
      {
        offset = 0;
      }
    else if (uint32_t(offset) > bigLen)
      {
        count = 0;
        return;
      }

    int32_t maxCount = bigLen - offset;
    if (count < 0 || count > maxCount)
      {
        count = maxCount;
      } 
    else
      {
        count += littleLen;
        if (count > maxCount)
          count = maxCount;
      }
  }

  






















 
static void
RFind_ComputeSearchRange( uint32_t bigLen, uint32_t littleLen, int32_t& offset, int32_t& count )
  {
    if (littleLen > bigLen)
      {
        offset = 0;
        count = 0;
        return;
      }

    if (offset < 0)
      offset = bigLen - littleLen;
    if (count < 0)
      count = offset + 1;

    int32_t start = offset - count + 1;
    if (start < 0)
      start = 0;

    count = offset + littleLen - start;
    offset = start;
  }



  
#include "string-template-def-unichar.h"
#include "nsTStringObsolete.cpp"
#include "string-template-undef.h"

  
#include "string-template-def-char.h"
#include "nsTStringObsolete.cpp"
#include "string-template-undef.h"





int32_t
nsString::Find( const nsAFlatString& aString, int32_t aOffset, int32_t aCount ) const
  {
    
    Find_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    int32_t result = FindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), false);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

int32_t
nsString::Find( const PRUnichar* aString, int32_t aOffset, int32_t aCount ) const
  {
    return Find(nsDependentString(aString), aOffset, aCount);
  }

int32_t
nsString::RFind( const nsAFlatString& aString, int32_t aOffset, int32_t aCount ) const
  {
    
    RFind_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    int32_t result = RFindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), false);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

int32_t
nsString::RFind( const PRUnichar* aString, int32_t aOffset, int32_t aCount ) const
  {
    return RFind(nsDependentString(aString), aOffset, aCount);
  }

int32_t
nsString::FindCharInSet( const PRUnichar* aSet, int32_t aOffset ) const
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
nsCString::Compare( const char* aString, bool aIgnoreCase, int32_t aCount ) const
  {
    uint32_t strLen = char_traits::length(aString);

    int32_t maxCount = int32_t(XPCOM_MIN(mLength, strLen));

    int32_t compareCount;
    if (aCount < 0 || aCount > maxCount)
      compareCount = maxCount;
    else
      compareCount = aCount;

    int32_t result =
        nsBufferRoutines<char>::compare(mData, aString, compareCount, aIgnoreCase);

    if (result == 0 &&
          (aCount < 0 || strLen < uint32_t(aCount) || mLength < uint32_t(aCount)))
      {
        
        
        

        if (mLength != strLen)
          result = (mLength < strLen) ? -1 : 1;
      }
    return result;
  }

bool
nsString::EqualsIgnoreCase( const char* aString, int32_t aCount ) const
  {
    uint32_t strLen = nsCharTraits<char>::length(aString);

    int32_t maxCount = int32_t(XPCOM_MIN(mLength, strLen));

    int32_t compareCount;
    if (aCount < 0 || aCount > maxCount)
      compareCount = maxCount;
    else
      compareCount = aCount;

    int32_t result =
        nsBufferRoutines<PRUnichar>::compare(mData, aString, compareCount, true);

    if (result == 0 &&
          (aCount < 0 || strLen < uint32_t(aCount) || mLength < uint32_t(aCount)))
      {
        
        
        

        if (mLength != strLen)
          result = 1; 
      }
    return result == 0;
  }


  



double
nsCString::ToDouble(nsresult* aErrorCode) const
  {
    double res = 0.0;
    if (mLength > 0)
      {
        char *conv_stopped;
        const char *str = mData;
        
        res = PR_strtod(str, &conv_stopped);
        if (conv_stopped == str+mLength)
          *aErrorCode = NS_OK;
        else 
          *aErrorCode = NS_ERROR_ILLEGAL_VALUE;
      }
    else
      {
        
        *aErrorCode = NS_ERROR_ILLEGAL_VALUE;
      }
    return res;
  }

double
nsString::ToDouble(nsresult* aErrorCode) const
  {
    return NS_LossyConvertUTF16toASCII(*this).ToDouble(aErrorCode);
  }


  



void
nsCString::AssignWithConversion( const nsAString& aData )
  {
    LossyCopyUTF16toASCII(aData, *this);
  }

void
nsString::AssignWithConversion( const nsACString& aData )
  {
    CopyASCIItoUTF16(aData, *this);
  }

#endif 
