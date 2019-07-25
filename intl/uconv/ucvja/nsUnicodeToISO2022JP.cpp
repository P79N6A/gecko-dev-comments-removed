




#include "nsUnicodeToISO2022JP.h"
#include "nsIComponentManager.h"
#include "nsUCVJADll.h"
#include "nsUnicodeEncodeHelper.h"






static const PRUnichar gBasicMapping[0x40] =
{

0xff60,0x3002,0x300c,0x300d,0x3001,0x30fb,0x30f2,0x30a1,

0x30a3,0x30a5,0x30a7,0x30a9,0x30e3,0x30e5,0x30e7,0x30c3,

0x30fc,0x30a2,0x30a4,0x30a6,0x30a8,0x30aa,0x30ab,0x30ad,

0x30af,0x30b1,0x30b3,0x30b5,0x30b7,0x30b9,0x30bb,0x30bd,

0x30bf,0x30c1,0x30c4,0x30c6,0x30c8,0x30ca,0x30cb,0x30cc,

0x30cd,0x30ce,0x30cf,0x30d2,0x30d5,0x30d8,0x30db,0x30de,

0x30df,0x30e0,0x30e1,0x30e2,0x30e4,0x30e6,0x30e8,0x30e9,

0x30ea,0x30eb,0x30ec,0x30ed,0x30ef,0x30f3,0x309b,0x309c
};


#define NEED_TO_CHECK_NIGORI(u) (((0xff76<=(u))&&((u)<=0xff84))||((0xff8a<=(u))&&((u)<=0xff8e)))


#define NEED_TO_CHECK_MARU(u) ((0xff8a<=(u))&&((u)<=0xff8e))


#define IS_HANKAKU(u) ((0xff61 <= (u)) && ((u) <= 0xff9f))
#define IS_NIGORI(u) (0xff9e == (u))
#define IS_MARU(u)   (0xff9f == (u))
#define NIGORI_MODIFIER 1
#define MARU_MODIFIER   2

static const uint16_t g_ufAsciiMapping [] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0x007F, 0x0000
};

#define SIZE_OF_TABLES 5
static const uint16_t * g_ufMappingTables[SIZE_OF_TABLES] = {
  g_ufAsciiMapping,             
  g_uf0201GLMapping,            
  g_uf0208Mapping,              
  g_uf0208extMapping,           
  g_uf0208Mapping,              
};

static const uScanClassID g_ufScanClassIDs[SIZE_OF_TABLES] = {
  u1ByteCharset,                
  u1ByteCharset,                
  u2BytesCharset,               
  u2BytesCharset,               
  u2BytesCharset,               
};
#define JIS_X_208_INDEX 2







nsUnicodeToISO2022JP::nsUnicodeToISO2022JP() 
: nsEncoderSupport(8)
{
  Reset();
}

nsUnicodeToISO2022JP::~nsUnicodeToISO2022JP() 
{
}

nsresult nsUnicodeToISO2022JP::ChangeCharset(int32_t aCharset,
                                             char * aDest, 
                                             int32_t * aDestLength)
{
  
  
  
  if(((2 == aCharset) && ( 3 == mCharset)) ||
     ((3 == aCharset) && ( 2 == mCharset)) )
  {
    mCharset = aCharset;
  }

  if(aCharset == mCharset) 
  {
    *aDestLength = 0;
    return NS_OK;
  } 
  
  if (*aDestLength < 3) {
    *aDestLength = 0;
    return NS_OK_UENC_MOREOUTPUT;
  }

  switch (aCharset) {
    case 0: 
      aDest[0] = 0x1b;
      aDest[1] = '(';
      aDest[2] = 'B';
      break;
    case 1: 
      aDest[0] = 0x1b;
      aDest[1] = '(';
      aDest[2] = 'J';
      break;
    case 2: 
    case 3: 
            
      aDest[0] = 0x1b;
      aDest[1] = '$';
      aDest[2] = 'B';
      break;
    case 4: 
            
      aDest[0] = 0x1b;
      aDest[1] = '$';
      aDest[2] = '@';
      break;
  }

  mCharset = aCharset;
  *aDestLength = 3;
  return NS_OK;
}

nsresult nsUnicodeToISO2022JP::ConvertHankaku(const PRUnichar * aSrc,
                                              int32_t * aSrcLength,
                                              char * aDest,
                                              int32_t * aDestLength)
{
  nsresult res = NS_OK;

  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;
  PRUnichar srcChar, tempChar;
  int32_t bcr, bcw;

  bcw = destEnd - dest;
  res = ChangeCharset(JIS_X_208_INDEX, dest, &bcw);
  dest += bcw;
  if (res != NS_OK) {
    return res;
  }

  while (src < srcEnd) {
    srcChar = *src;
    if (!IS_HANKAKU(srcChar)) {
      break;
    }
    ++src;
    tempChar = gBasicMapping[(srcChar) - 0xff60];

    if (src < srcEnd) {
      
      
      if (NEED_TO_CHECK_NIGORI(srcChar) && IS_NIGORI(*src)) {
        tempChar += NIGORI_MODIFIER;
        ++src;
      } else if (NEED_TO_CHECK_MARU(srcChar) && IS_MARU(*src)) {
        tempChar += MARU_MODIFIER;
        ++src;
      }
    }
    bcr = 1;
    bcw = destEnd - dest;
    res = nsUnicodeEncodeHelper::ConvertByTable(
             &tempChar, &bcr, dest, &bcw, g_ufScanClassIDs[JIS_X_208_INDEX],
             nullptr, (uMappingTable *) g_ufMappingTables[JIS_X_208_INDEX]);
    dest += bcw;
    if (res != NS_OK)
      break;
  }
  *aDestLength = dest - aDest;
  *aSrcLength = src - aSrc;
  return res;
}




NS_IMETHODIMP nsUnicodeToISO2022JP::ConvertNoBuffNoErr(
                                    const PRUnichar * aSrc, 
                                    int32_t * aSrcLength, 
                                    char * aDest, 
                                    int32_t * aDestLength)
{
  nsresult res = NS_OK;

  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;
  int32_t bcr, bcw;
  int32_t i;

  while (src < srcEnd) {
    for (i=0; i< SIZE_OF_TABLES ; i++) {
      bcr = 1;
      bcw = destEnd - dest;
      res = nsUnicodeEncodeHelper::ConvertByTable(src, &bcr, dest, &bcw, 
                                      g_ufScanClassIDs[i], nullptr,
                                      (uMappingTable *) g_ufMappingTables[i]);
      if (res != NS_ERROR_UENC_NOMAPPING) break;
    }

    if ( i>=  SIZE_OF_TABLES) {
      if (IS_HANKAKU(*src)) {
        bcr = srcEnd - src;
        bcw = destEnd - dest;
        res = ConvertHankaku(src, &bcr, dest, &bcw);
        dest += bcw;
        src += bcr;
        if (res == NS_OK) continue;
      } else {
        res = NS_ERROR_UENC_NOMAPPING;
        src++;
      }
    }
    if (res != NS_OK) break;

    bcw = destEnd - dest;
    res = ChangeCharset(i, dest, &bcw);
    dest += bcw;
    if (res != NS_OK) break;

    bcr = srcEnd - src;
    bcw = destEnd - dest;
    res = nsUnicodeEncodeHelper::ConvertByTable(src, &bcr, dest, &bcw, 
                                      g_ufScanClassIDs[i], nullptr,
                                      (uMappingTable *) g_ufMappingTables[i]);
    src += bcr;
    dest += bcw;

    if ((res != NS_OK) && (res != NS_ERROR_UENC_NOMAPPING)) break;
    if (res == NS_ERROR_UENC_NOMAPPING) src--;
  }

  *aSrcLength = src - aSrc;
  *aDestLength  = dest - aDest;
  return res;
}

NS_IMETHODIMP nsUnicodeToISO2022JP::FinishNoBuff(char * aDest, 
                                                 int32_t * aDestLength)
{
  ChangeCharset(0, aDest, aDestLength);
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToISO2022JP::Reset()
{
  mCharset = 0;
  return nsEncoderSupport::Reset();
}
