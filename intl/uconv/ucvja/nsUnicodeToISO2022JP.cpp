




































#include "nsUnicodeToISO2022JP.h"
#include "nsIComponentManager.h"
#include "nsUCVJADll.h"
#include "nsUnicodeEncodeHelper.h"




static const PRUint16 g_ufAsciiMapping [] = {
  0x0001, 0x0004, 0x0005, 0x0008, 0x0000, 0x0000, 0x007F, 0x0000
};

#define SIZE_OF_TABLES 5
static const PRUint16 * g_ufMappingTables[SIZE_OF_TABLES] = {
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







nsUnicodeToISO2022JP::nsUnicodeToISO2022JP() 
: nsEncoderSupport(8)
{
  Reset();
}

nsUnicodeToISO2022JP::~nsUnicodeToISO2022JP() 
{
}

nsresult nsUnicodeToISO2022JP::ChangeCharset(PRInt32 aCharset,
                                             char * aDest, 
                                             PRInt32 * aDestLength)
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




NS_IMETHODIMP nsUnicodeToISO2022JP::FillInfo(PRUint32* aInfo)
{
  return nsUnicodeEncodeHelper::FillInfo(aInfo, SIZE_OF_TABLES, 
                                         (uMappingTable **) g_ufMappingTables);

}
NS_IMETHODIMP nsUnicodeToISO2022JP::ConvertNoBuffNoErr(
                                    const PRUnichar * aSrc, 
                                    PRInt32 * aSrcLength, 
                                    char * aDest, 
                                    PRInt32 * aDestLength)
{
  nsresult res = NS_OK;

  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;
  PRInt32 bcr, bcw;
  PRInt32 i;

  while (src < srcEnd) {
    for (i=0; i< SIZE_OF_TABLES ; i++) {
      bcr = 1;
      bcw = destEnd - dest;
      res = nsUnicodeEncodeHelper::ConvertByTable(src, &bcr, dest, &bcw, 
                                      g_ufScanClassIDs[i], nsnull,
                                      (uMappingTable *) g_ufMappingTables[i]);
      if (res != NS_ERROR_UENC_NOMAPPING) break;
    }

    if ( i>=  SIZE_OF_TABLES) {
      res = NS_ERROR_UENC_NOMAPPING;
      src++;
    }
    if (res != NS_OK) break;

    bcw = destEnd - dest;
    res = ChangeCharset(i, dest, &bcw);
    dest += bcw;
    if (res != NS_OK) break;

    bcr = srcEnd - src;
    bcw = destEnd - dest;
    res = nsUnicodeEncodeHelper::ConvertByTable(src, &bcr, dest, &bcw, 
                                      g_ufScanClassIDs[i], nsnull,
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
                                                 PRInt32 * aDestLength)
{
  ChangeCharset(0, aDest, aDestLength);
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToISO2022JP::Reset()
{
  mCharset = 0;
  return nsEncoderSupport::Reset();
}
