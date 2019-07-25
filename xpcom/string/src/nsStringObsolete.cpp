





































#include "nsString.h"


  



#if MOZ_STRING_WITH_OBSOLETE_API

#include "nsDependentString.h"
#include "nsDependentSubstring.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsUTF8Utils.h"
#include "prdtoa.h"
#include "prprf.h"










inline char
ascii_tolower(char aChar)
{
  if (aChar >= 'A' && aChar <= 'Z')
    return aChar + ('a' - 'A');
  return aChar;
}


















static PRInt32
FindChar1(const char* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRInt32 aCount) {

  if(anOffset < 0)
    anOffset=0;

  if(aCount < 0)
    aCount = (PRInt32)aDestLength;

  if((aChar < 256) && (0 < aDestLength) && ((PRUint32)anOffset < aDestLength)) {

    
    
    
    if(0<aCount) {

      const char* left= aDest+anOffset;
      const char* last= left+aCount;
      const char* max = aDest+aDestLength;
      const char* end = (last<max) ? last : max;

      PRInt32 theMax = end-left;
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













static PRInt32
FindChar2(const PRUnichar* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRInt32 aCount) {

  if(anOffset < 0)
    anOffset=0;

  if(aCount < 0)
    aCount = (PRInt32)aDestLength;

  if((0<aDestLength) && ((PRUint32)anOffset < aDestLength)) {
 
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














static PRInt32
RFindChar1(const char* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRInt32 aCount) {

  if(anOffset < 0)
    anOffset=(PRInt32)aDestLength-1;

  if(aCount < 0)
    aCount = PRInt32(aDestLength);

  if((aChar<256) && (0 < aDestLength) && ((PRUint32)anOffset < aDestLength)) {

    
    
 
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













static PRInt32
RFindChar2(const PRUnichar* aDest,PRUint32 aDestLength,PRInt32 anOffset,const PRUnichar aChar,PRInt32 aCount) {

  if(anOffset < 0)
    anOffset=(PRInt32)aDestLength-1;

  if(aCount < 0)
    aCount = PRInt32(aDestLength);

  if((0 < aDestLength) && ((PRUint32)anOffset < aDestLength)) {
 
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
PRInt32
Compare1To1(const char* aStr1,const char* aStr2,PRUint32 aCount,PRBool aIgnoreCase){ 
  PRInt32 result=0;
  if(aIgnoreCase)
    result=PRInt32(PL_strncasecmp(aStr1, aStr2, aCount));
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
PRInt32
Compare2To2(const PRUnichar* aStr1,const PRUnichar* aStr2,PRUint32 aCount){
  PRInt32 result;
  
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
PRInt32
Compare2To1(const PRUnichar* aStr1,const char* aStr2,PRUint32 aCount,PRBool aIgnoreCase){
  const PRUnichar* s1 = aStr1;
  const char *s2 = aStr2;
  
  if (aStr1 && aStr2) {
    if (aCount != 0) {
      do {

        PRUnichar c1 = *s1++;
        PRUnichar c2 = PRUnichar((unsigned char)*s2++);
        
        if (c1 != c2) {
#ifdef NS_DEBUG
          
          
          
          
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











inline PRInt32
Compare1To2(const char* aStr1,const PRUnichar* aStr2,PRUint32 aCount,PRBool aIgnoreCase){
  return Compare2To1(aStr2, aStr1, aCount, aIgnoreCase) * -1;
}



















static PRInt32
CompressChars1(char* aString,PRUint32 aLength,const char* aSet){ 

  char*  from = aString;
  char*  end =  aString + aLength;
  char*  to = from;

    
    
  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);

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














static PRInt32
CompressChars2(PRUnichar* aString,PRUint32 aLength,const char* aSet){ 

  PRUnichar*  from = aString;
  PRUnichar*  end =  from + aLength;
  PRUnichar*  to = from;

    
    
  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);

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












static PRInt32
StripChars1(char* aString,PRUint32 aLength,const char* aSet){ 

  

  char*  to   = aString;
  char*  from = aString-1;
  char*  end  = aString + aLength;

  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);
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













static PRInt32
StripChars2(PRUnichar* aString,PRUint32 aLength,const char* aSet){ 

  

  PRUnichar*  to   = aString;
  PRUnichar*  from = aString-1;
  PRUnichar*  end  = to + aLength;

  if(aSet && aString && (0 < aLength)){
    PRUint32 aSetLen=strlen(aSet);
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

NS_SPECIALIZE_TEMPLATE
struct nsBufferRoutines<char>
  {
    static
    PRInt32 compare( const char* a, const char* b, PRUint32 max, PRBool ic )
      {
        return Compare1To1(a, b, max, ic);
      }

    static
    PRInt32 compare( const char* a, const PRUnichar* b, PRUint32 max, PRBool ic )
      {
        return Compare1To2(a, b, max, ic);
      }

    static
    PRInt32 find_char( const char* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return FindChar1(s, max, offset, c, count);
      }

    static
    PRInt32 rfind_char( const char* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return RFindChar1(s, max, offset, c, count);
      }

    static
    char get_find_in_set_filter( const char* set )
      {
        return GetFindInSetFilter(set);
      }

    static
    PRInt32 strip_chars( char* s, PRUint32 len, const char* set )
      {
        return StripChars1(s, len, set);
      }

    static
    PRInt32 compress_chars( char* s, PRUint32 len, const char* set ) 
      {
        return CompressChars1(s, len, set);
      }
  };

NS_SPECIALIZE_TEMPLATE
struct nsBufferRoutines<PRUnichar>
  {
    static
    PRInt32 compare( const PRUnichar* a, const PRUnichar* b, PRUint32 max, PRBool ic )
      {
        NS_ASSERTION(!ic, "no case-insensitive compare here");
        return Compare2To2(a, b, max);
      }

    static
    PRInt32 compare( const PRUnichar* a, const char* b, PRUint32 max, PRBool ic )
      {
        return Compare2To1(a, b, max, ic);
      }

    static
    PRInt32 find_char( const PRUnichar* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return FindChar2(s, max, offset, c, count);
      }

    static
    PRInt32 rfind_char( const PRUnichar* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
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
    PRInt32 strip_chars( PRUnichar* s, PRUint32 max, const char* set )
      {
        return StripChars2(s, max, set);
      }

    static
    PRInt32 compress_chars( PRUnichar* s, PRUint32 len, const char* set ) 
      {
        return CompressChars2(s, len, set);
      }
  };



template <class L, class R>
#ifndef __SUNPRO_CC
static
#endif 
PRInt32
FindSubstring( const L* big, PRUint32 bigLen,
               const R* little, PRUint32 littleLen,
               PRBool ignoreCase )
  {
    if (littleLen > bigLen)
      return kNotFound;

    PRInt32 i, max = PRInt32(bigLen - littleLen);
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
PRInt32
RFindSubstring( const L* big, PRUint32 bigLen,
                const R* little, PRUint32 littleLen,
                PRBool ignoreCase )
  {
    if (littleLen > bigLen)
      return kNotFound;

    PRInt32 i, max = PRInt32(bigLen - littleLen);

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
PRInt32
FindCharInSet( const CharT* data, PRUint32 dataLen, const SetCharT* set )
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
PRInt32
RFindCharInSet( const CharT* data, PRUint32 dataLen, const SetCharT* set )
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







void 
Modified_cnvtf(char *buf, int bufsz, int prcsn, double fval)
{
  PRIntn decpt, sign, numdigits;
  char *num, *nump;
  char *bufp = buf;
  char *endnum;

  
  num = (char*)malloc(bufsz);
  if (num == NULL) {
    buf[0] = '\0';
    return;
  }
  if (PR_dtoa(fval, 2, prcsn, &decpt, &sign, &endnum, num, bufsz)
      == PR_FAILURE) {
    buf[0] = '\0';
    goto done;
  }
  numdigits = endnum - num;
  nump = num;

  





  if (sign && fval < 0.0f) {
    *bufp++ = '-';
  }

  if (decpt == 9999) {
    while ((*bufp++ = *nump++) != 0) {} 
    goto done;
  }

  if (decpt > (prcsn+1) || decpt < -(prcsn-1) || decpt < -5) {
    *bufp++ = *nump++;
    if (numdigits != 1) {
      *bufp++ = '.';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = 'e';
    PR_snprintf(bufp, bufsz - (bufp - buf), "%+d", decpt-1);
  }
  else if (decpt >= 0) {
    if (decpt == 0) {
      *bufp++ = '0';
    }
    else {
      while (decpt--) {
        if (*nump != '\0') {
          *bufp++ = *nump++;
        }
        else {
          *bufp++ = '0';
        }
      }
    }
    if (*nump != '\0') {
      *bufp++ = '.';
      while (*nump != '\0') {
        *bufp++ = *nump++;
      }
    }
    *bufp++ = '\0';
  }
  else if (decpt < 0) {
    *bufp++ = '0';
    *bufp++ = '.';
    while (decpt++) {
      *bufp++ = '0';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = '\0';
  }
done:
  free(num);
}

  





 
static void
Find_ComputeSearchRange( PRUint32 bigLen, PRUint32 littleLen, PRInt32& offset, PRInt32& count )
  {
    

    if (offset < 0)
      {
        offset = 0;
      }
    else if (PRUint32(offset) > bigLen)
      {
        count = 0;
        return;
      }

    PRInt32 maxCount = bigLen - offset;
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
RFind_ComputeSearchRange( PRUint32 bigLen, PRUint32 littleLen, PRInt32& offset, PRInt32& count )
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

    PRInt32 start = offset - count + 1;
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





PRInt32
nsString::Find( const nsAFlatString& aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    
    Find_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = FindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), PR_FALSE);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

PRInt32
nsString::Find( const PRUnichar* aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    return Find(nsDependentString(aString), aOffset, aCount);
  }

PRInt32
nsString::RFind( const nsAFlatString& aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    
    RFind_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = RFindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), PR_FALSE);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

PRInt32
nsString::RFind( const PRUnichar* aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    return RFind(nsDependentString(aString), aOffset, aCount);
  }

PRInt32
nsString::FindCharInSet( const PRUnichar* aSet, PRInt32 aOffset ) const
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
nsCString::Compare( const char* aString, PRBool aIgnoreCase, PRInt32 aCount ) const
  {
    PRUint32 strLen = char_traits::length(aString);

    PRInt32 maxCount = PRInt32(NS_MIN(mLength, strLen));

    PRInt32 compareCount;
    if (aCount < 0 || aCount > maxCount)
      compareCount = maxCount;
    else
      compareCount = aCount;

    PRInt32 result =
        nsBufferRoutines<char>::compare(mData, aString, compareCount, aIgnoreCase);

    if (result == 0 &&
          (aCount < 0 || strLen < PRUint32(aCount) || mLength < PRUint32(aCount)))
      {
        
        
        

        if (mLength != strLen)
          result = (mLength < strLen) ? -1 : 1;
      }
    return result;
  }

PRBool
nsString::EqualsIgnoreCase( const char* aString, PRInt32 aCount ) const
  {
    PRUint32 strLen = nsCharTraits<char>::length(aString);

    PRInt32 maxCount = PRInt32(NS_MIN(mLength, strLen));

    PRInt32 compareCount;
    if (aCount < 0 || aCount > maxCount)
      compareCount = maxCount;
    else
      compareCount = aCount;

    PRInt32 result =
        nsBufferRoutines<PRUnichar>::compare(mData, aString, compareCount, PR_TRUE);

    if (result == 0 &&
          (aCount < 0 || strLen < PRUint32(aCount) || mLength < PRUint32(aCount)))
      {
        
        
        

        if (mLength != strLen)
          result = 1; 
      }
    return result == 0;
  }

  



float
nsCString::ToFloat(PRInt32* aErrorCode) const
  {
    float res = 0.0f;
    if (mLength > 0)
      {
        char *conv_stopped;
        const char *str = mData;
        
        res = (float)PR_strtod(str, &conv_stopped);
        if (conv_stopped == str+mLength)
          *aErrorCode = (PRInt32) NS_OK;
        else 
          *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
      }
    else
      {
        
        *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
      }
    return res;
  }

float
nsString::ToFloat(PRInt32* aErrorCode) const
  {
    return NS_LossyConvertUTF16toASCII(*this).ToFloat(aErrorCode);
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


  



void
nsCString::AppendWithConversion( const nsAString& aData )
  {
    LossyAppendUTF16toASCII(aData, *this);
  }

void
nsString::AppendWithConversion( const nsACString& aData )
  {
    AppendASCIItoUTF16(aData, *this);
  }


  



void
nsCString::AppendInt( PRInt32 aInteger, PRInt32 aRadix )
  {
    char buf[20];
    const char* fmt;
    switch (aRadix) {
      case 8:
        fmt = "%o";
        break;
      case 10:
        fmt = "%d";
        break;
      default:
        NS_ASSERTION(aRadix == 16, "Invalid radix!");
        fmt = "%x";
    }
    PR_snprintf(buf, sizeof(buf), fmt, aInteger);
    Append(buf);
  }

void
nsString::AppendInt( PRInt32 aInteger, PRInt32 aRadix )
  {
    char buf[20];
    const char* fmt;
    switch (aRadix) {
      case 8:
        fmt = "%o";
        break;
      case 10:
        fmt = "%d";
        break;
      default:
        NS_ASSERTION(aRadix == 16, "Invalid radix!");
        fmt = "%x";
    }
    PR_snprintf(buf, sizeof(buf), fmt, aInteger);
    AppendASCIItoUTF16(buf, *this);
  }

void
nsCString::AppendInt( PRInt64 aInteger, PRInt32 aRadix )
  {
    char buf[30];
    const char* fmt;
    switch (aRadix) {
      case 8:
        fmt = "%llo";
        break;
      case 10:
        fmt = "%lld";
        break;
      default:
        NS_ASSERTION(aRadix == 16, "Invalid radix!");
        fmt = "%llx";
    }
    PR_snprintf(buf, sizeof(buf), fmt, aInteger);
    Append(buf);
  }

void
nsString::AppendInt( PRInt64 aInteger, PRInt32 aRadix )
  {
    char buf[30];
    const char* fmt;
    switch (aRadix) {
      case 8:
        fmt = "%llo";
        break;
      case 10:
        fmt = "%lld";
        break;
      default:
        NS_ASSERTION(aRadix == 16, "Invalid radix!");
        fmt = "%llx";
    }
    PR_snprintf(buf, sizeof(buf), fmt, aInteger);
    AppendASCIItoUTF16(buf, *this);
  }

  



void
nsCString::AppendFloat( float aFloat )
  {
    char buf[40];
    
    
    Modified_cnvtf(buf, sizeof(buf), 6, aFloat);
    Append(buf);
  }

void
nsString::AppendFloat( float aFloat )
  {
    char buf[40];
    
    
    Modified_cnvtf(buf, sizeof(buf), 6, aFloat);
    AppendWithConversion(buf);
  }

void
nsCString::AppendFloat( double aFloat )
  {
    char buf[40];
    
    
    Modified_cnvtf(buf, sizeof(buf), 15, aFloat);
    Append(buf);
  }

void
nsString::AppendFloat( double aFloat )
  {
    char buf[40];
    
    
    Modified_cnvtf(buf, sizeof(buf), 15, aFloat);
    AppendWithConversion(buf);
  }

#endif 
