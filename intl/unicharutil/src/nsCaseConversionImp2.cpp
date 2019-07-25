





































#include "pratom.h"
#include "nsUUDll.h"
#include "nsCaseConversionImp2.h"
#include "casetable.h"


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

nsCaseConversionImp2* gCaseConv = nsnull;

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

nsCaseConversionImp2* nsCaseConversionImp2::GetInstance()
{
  if (!gCaseConv)
    NS_NEWXPCOM(gCaseConv, nsCaseConversionImp2);
  return gCaseConv;
}

NS_IMETHODIMP_(nsrefcnt) nsCaseConversionImp2::AddRef(void)
{
  return (nsrefcnt)1;
}

NS_IMETHODIMP_(nsrefcnt) nsCaseConversionImp2::Release(void)
{
  return (nsrefcnt)1;
}

NS_IMPL_THREADSAFE_QUERY_INTERFACE1(nsCaseConversionImp2, nsICaseConversion)

nsresult nsCaseConversionImp2::ToUpper(
  PRUnichar aChar, PRUnichar* aReturn
)
{
  if( IS_ASCII(aChar)) 
  {
     if(IS_ASCII_LOWER(aChar))
        *aReturn = aChar - 0x0020;
     else
        *aReturn = aChar;
  } 
  else if( IS_NOCASE_CHAR(aChar)) 
  {
    *aReturn = aChar;
  } 
  else 
  {
    *aReturn = gUpperMap.Map(aChar);
  }
  return NS_OK;
}


static PRUnichar FastToLower(
  PRUnichar aChar
)
{
  if( IS_ASCII(aChar)) 
  {
     if(IS_ASCII_UPPER(aChar))
        return aChar + 0x0020;
     else
        return aChar;
  }
  else if( IS_NOCASE_CHAR(aChar)) 
  {
     return aChar;
  }

  return gLowerMap.Map(aChar);
}

nsresult nsCaseConversionImp2::ToLower(
  PRUnichar aChar, PRUnichar* aReturn
)
{
  *aReturn = FastToLower(aChar);
  return NS_OK;
}

nsresult nsCaseConversionImp2::ToTitle(
  PRUnichar aChar, PRUnichar* aReturn
)
{
  if( IS_ASCII(aChar)) 
  {
    return this->ToUpper(aChar, aReturn);
  }
  else if( IS_NOCASE_CHAR(aChar)) 
  {
    *aReturn = aChar;
  } 
  else
  {
    
    
    if( 0x01C0 == ( aChar & 0xFFC0)) 
    {
      for(PRUint32 i = 0 ; i < gUpperToTitleItems; i++) {
        if ( aChar == gUpperToTitle[(i*2)+kUpperIdx]) {
          *aReturn = aChar;
          return NS_OK;
        }
      }
    }

    PRUnichar upper = gUpperMap.Map(aChar);
    
    if( 0x01C0 == ( upper & 0xFFC0)) 
    {
      for(PRUint32 i = 0 ; i < gUpperToTitleItems; i++) {
        if ( upper == gUpperToTitle[(i*2)+kUpperIdx]) {
           *aReturn = gUpperToTitle[(i*2)+kTitleIdx];
           return NS_OK;
        }
      }
    }
    *aReturn = upper;
  }
  return NS_OK;
}

nsresult nsCaseConversionImp2::ToUpper(
  const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen
)
{
  PRUint32 i;
  for(i=0;i<aLen;i++) 
  {
    PRUnichar aChar = anArray[i];
    if( IS_ASCII(aChar)) 
    {
       if(IS_ASCII_LOWER(aChar))
          aReturn[i] = aChar - 0x0020;
       else
          aReturn[i] = aChar;
    }
    else if( IS_NOCASE_CHAR(aChar)) 
    {
          aReturn[i] = aChar;
    } 
    else 
    {
      aReturn[i] = gUpperMap.Map(aChar);
    }
  }
  return NS_OK;
}

nsresult nsCaseConversionImp2::ToLower(
  const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen
)
{
  PRUint32 i;
  for(i=0;i<aLen;i++) 
    aReturn[i] = FastToLower(anArray[i]);
  return NS_OK;
}



nsresult nsCaseConversionImp2::ToTitle(
  const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen,
  PRBool aStartInWordBoundary
)
{
  if(0 == aLen)
    return NS_OK;

  
  
  
  
  
  
  
  
  
  
  PRBool bLastIsSpace =  IS_ASCII_SPACE(anArray[0]);
  if(aStartInWordBoundary)
  {
     this->ToTitle(anArray[0], &aReturn[0]);
  }

  PRUint32 i;
  for(i=1;i<aLen;i++)
  {
    if(bLastIsSpace)
    {
      this->ToTitle(anArray[i], &aReturn[i]);
    }
    else
    {
      aReturn[i] = anArray[i];
    }

    bLastIsSpace = IS_ASCII_SPACE(aReturn[i]);
  }
  return NS_OK;
}


NS_IMETHODIMP
nsCaseConversionImp2::CaseInsensitiveCompare(const PRUnichar *aLeft,
                                             const PRUnichar *aRight,
                                             PRUint32 aCount, PRInt32* aResult)
{
  if (!aLeft || !aRight)
    return NS_ERROR_INVALID_POINTER;

  
  *aResult = 0;
  
  if (aCount) {
    do {
      PRUnichar c1 = *aLeft++;
      PRUnichar c2 = *aRight++;
      
      if (c1 != c2) {
        c1 = FastToLower(c1);
        c2 = FastToLower(c2);
        if (c1 != c2) {
          if (c1 < c2) {
            *aResult = -1;
            return NS_OK;
          }
          *aResult = 1;
          return NS_OK;
        }
      }
    } while (--aCount != 0);
  }
  return NS_OK;
}
