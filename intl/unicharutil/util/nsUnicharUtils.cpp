







































#include "nsUnicharUtils.h"
#include "nsUnicharUtilCIID.h"

#include "nsCRT.h"
#include "nsICaseConversion.h"
#include "nsServiceManagerUtils.h"
#include "nsXPCOMStrings.h"
#include "casetable.h"

#include <ctype.h>


enum {
  kUpperIdx =0,
  kTitleIdx
};


enum {
  kLowIdx =0,
  kSizeEveryIdx,
  kDiffIdx
};

#define IS_ASCII(u)       ( 0x0000 == ((u) & 0xFF80))
#define IS_ASCII_UPPER(u) ((0x0041 <= (u)) && ( (u) <= 0x005a))
#define IS_ASCII_LOWER(u) ((0x0061 <= (u)) && ( (u) <= 0x007a))
#define IS_ASCII_ALPHA(u) (IS_ASCII_UPPER(u) || IS_ASCII_LOWER(u))
#define IS_ASCII_SPACE(u) ( 0x0020 == (u) )

#define IS_NOCASE_CHAR(u)  (0==(1&(gCaseBlocks[(u)>>13]>>(0x001F&((u)>>8)))))
  


#define CASE_MAP_CACHE_SIZE 0x40
#define CASE_MAP_CACHE_MASK 0x3F

struct nsCompressedMap {
  const PRUnichar *mTable;
  PRUint32 mSize;
  PRUint32 mCache[CASE_MAP_CACHE_SIZE];
  PRUint32 mLastBase;

  PRUnichar Map(PRUnichar aChar)
  {
    
    
    
    
    

    PRUint32 cachedData = mCache[aChar & CASE_MAP_CACHE_MASK];
    if(aChar == ((cachedData >> 16) & 0x0000FFFF))
      return (cachedData & 0x0000FFFF);

    
    
    PRUint32 base = mLastBase; 
    PRUnichar res = 0;
    
    if (( aChar <=  ((mTable[base+kSizeEveryIdx] >> 8) + 
                  mTable[base+kLowIdx])) &&
        ( mTable[base+kLowIdx]  <= aChar )) 
    {
        
        if(((mTable[base+kSizeEveryIdx] & 0x00FF) > 0) && 
          (0 != ((aChar - mTable[base+kLowIdx]) % 
                (mTable[base+kSizeEveryIdx] & 0x00FF))))
        {
          res = aChar;
        } else {
          res = aChar + mTable[base+kDiffIdx];
        }
    } else {
        res = this->Lookup(0, (mSize/2), mSize-1, aChar);
    }

    mCache[aChar & CASE_MAP_CACHE_MASK] =
        (((aChar << 16) & 0xFFFF0000) | (0x0000FFFF & res));
    return res;
  }

  PRUnichar Lookup(PRUint32 l,
                   PRUint32 m,
                   PRUint32 r,
                   PRUnichar aChar)
  {
    PRUint32 base = m*3;
    if ( aChar >  ((mTable[base+kSizeEveryIdx] >> 8) + 
                  mTable[base+kLowIdx])) 
    {
      if( l > m )
        return aChar;
      PRUint32 newm = (m+r+1)/2;
      if(newm == m)
        newm++;
      return this->Lookup(m+1, newm , r, aChar);
      
    } else if ( mTable[base+kLowIdx]  > aChar ) {
      if( r < m )
        return aChar;
      PRUint32 newm = (l+m-1)/2;
      if(newm == m)
        newm++;
      return this->Lookup(l, newm, m-1, aChar);

    } else  {
      if(((mTable[base+kSizeEveryIdx] & 0x00FF) > 0) && 
        (0 != ((aChar - mTable[base+kLowIdx]) % 
                (mTable[base+kSizeEveryIdx] & 0x00FF))))
      {
        return aChar;
      }
      mLastBase = base; 
      return aChar + mTable[base+kDiffIdx];
    }
  }
};

static nsCompressedMap gUpperMap = {
  reinterpret_cast<const PRUnichar*>(&gToUpper[0]),
  gToUpperItems
};

static nsCompressedMap gLowerMap = {
  reinterpret_cast<const PRUnichar*>(&gToLower[0]),
  gToLowerItems
};

void
ToLowerCase(nsAString& aString)
{
  PRUnichar *buf = aString.BeginWriting();
  ToLowerCase(buf, buf, aString.Length());
}

void
ToLowerCase(const nsAString& aSource,
            nsAString& aDest)
{
  const PRUnichar *in;
  PRUnichar *out;
  PRUint32 len = NS_StringGetData(aSource, &in);
  NS_StringGetMutableData(aDest, len, &out);
  NS_ASSERTION(out, "Uh...");
  ToLowerCase(in, out, len);
}

PRUnichar
ToLowerCaseASCII(const PRUnichar aChar)
{
  if (IS_ASCII_UPPER(aChar))
    return aChar + 0x0020;
  return aChar;
}

void
ToUpperCase(nsAString& aString)
{
  PRUnichar *buf = aString.BeginWriting();
  ToUpperCase(buf, buf, aString.Length());
}

void
ToUpperCase(const nsAString& aSource,
            nsAString& aDest)
{
  const PRUnichar *in;
  PRUnichar *out;
  PRUint32 len = NS_StringGetData(aSource, &in);
  NS_StringGetMutableData(aDest, len, &out);
  NS_ASSERTION(out, "Uh...");
  ToUpperCase(in, out, len);
}

#ifdef MOZILLA_INTERNAL_API

PRInt32
nsCaseInsensitiveStringComparator::operator()(const PRUnichar* lhs,
                                              const PRUnichar* rhs,
                                              PRUint32 aLength) const
{
  return CaseInsensitiveCompare(lhs, rhs, aLength);
}

PRInt32
nsCaseInsensitiveStringComparator::operator()(PRUnichar lhs,
                                              PRUnichar rhs) const
{
  
  if (lhs == rhs)
    return 0;
  
  lhs = ToLowerCase(lhs);
  rhs = ToLowerCase(rhs);
  
  if (lhs == rhs)
    return 0;
  else if (lhs < rhs)
    return -1;
  else
    return 1;
}

PRInt32
nsASCIICaseInsensitiveStringComparator::operator()(const PRUnichar* lhs,
                                                   const PRUnichar* rhs,
                                                   PRUint32 aLength) const
{
  while (aLength) {
    PRUnichar l = *lhs++;
    PRUnichar r = *lhs++;
    if (l != r) {
      l = ToLowerCaseASCII(l);
      r = ToLowerCaseASCII(r);
      
      if (l > r)
        return 1;
      else if (r > l)
        return -1;
    }
    aLength--;
  }

  return 0;
}

PRInt32
nsASCIICaseInsensitiveStringComparator::operator()(PRUnichar lhs,
                                                   PRUnichar rhs) const
{
  
  if (lhs == rhs)
    return 0;
  
  lhs = ToLowerCaseASCII(lhs);
  rhs = ToLowerCaseASCII(rhs);
  
  if (lhs == rhs)
    return 0;
  else if (lhs < rhs)
    return -1;
  else
    return 1;
}


#endif 

PRInt32
CaseInsensitiveCompare(const PRUnichar *a,
                       const PRUnichar *b,
                       PRUint32 len)
{
  NS_ASSERTION(a && b, "Do not pass in invalid pointers!");
  
  if (len) {
    do {
      PRUnichar c1 = *a++;
      PRUnichar c2 = *b++;
      
      if (c1 != c2) {
        c1 = ToLowerCase(c1);
        c2 = ToLowerCase(c2);
        if (c1 != c2) {
          if (c1 < c2) {
            return -1;
          }
          return 1;
        }
      }
    } while (--len != 0);
  }
  return 0;
}

PRUnichar
ToLowerCase(PRUnichar aChar)
{
  if (IS_ASCII(aChar)) {
    if (IS_ASCII_UPPER(aChar))
      return aChar + 0x0020;
    else
      return aChar;
  } else if (IS_NOCASE_CHAR(aChar)) {
     return aChar;
  }

  return gLowerMap.Map(aChar);
}

void
ToLowerCase(const PRUnichar *aIn, PRUnichar *aOut, PRUint32 aLen)
{
  for (PRUint32 i = 0; i < aLen; i++) {
    aOut[i] = ToLowerCase(aIn[i]);
  }
}

PRUnichar
ToUpperCase(PRUnichar aChar)
{
  if (IS_ASCII(aChar)) {
    if (IS_ASCII_LOWER(aChar))
      return aChar - 0x0020;
    else
      return aChar;
  } else if (IS_NOCASE_CHAR(aChar)) {
    return aChar;
  }

  return gUpperMap.Map(aChar);
}

void
ToUpperCase(const PRUnichar *aIn, PRUnichar *aOut, PRUint32 aLen)
{
  for (PRUint32 i = 0; i < aLen; i++) {
    aOut[i] = ToUpperCase(aIn[i]);
  }
}

PRUnichar
ToTitleCase(PRUnichar aChar)
{
  if (IS_ASCII(aChar)) {
    return ToUpperCase(aChar);
  } else if (IS_NOCASE_CHAR(aChar)) {
    return aChar;
  }

  
  
  if (0x01C0 == (aChar & 0xFFC0)) {
    for (PRUint32 i = 0; i < gUpperToTitleItems; i++) {
      if (aChar == gUpperToTitle[(i*2)+kUpperIdx]) {
        return aChar;
      }
    }
  }

  PRUnichar upper = gUpperMap.Map(aChar);
  
  if (0x01C0 == ( upper & 0xFFC0)) {
    for (PRUint32 i = 0 ; i < gUpperToTitleItems; i++) {
      if (upper == gUpperToTitle[(i*2)+kUpperIdx]) {
         return gUpperToTitle[(i*2)+kTitleIdx];
      }
    }
  }

  return upper;
}
