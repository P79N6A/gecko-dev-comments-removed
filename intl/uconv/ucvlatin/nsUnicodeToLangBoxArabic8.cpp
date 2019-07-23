





































#include "nsUCConstructors.h"
#include "nsUnicodeToLangBoxArabic8.h"

#include "nsISupports.h"

static const unsigned char uni2lbox [] = 
 {
  0xC1,   
  0xC2 , 
  0xC2 , 
  0xC3 , 
  0xC3 , 
  0xC4 , 
  0xC4 , 
  0xC5 , 
  0xC5 , 
  0x9F , 
  0xC6 , 
  0xC0 , 
  0xC0 , 
  0xC7 ,
  0xC7 , 
  0xC8 , 
  0xC8 , 
  0xEB , 
  0xEB , 
  0xC9 , 
  0x8E , 
  0xCA , 
  0xCA , 
  0xEC , 
  0xEC , 
  0xCB , 
  0xCB , 
  0xED , 
  0xED , 
  0xCC , 
  0xCC , 
  0xEE , 
  0xEE , 
  0xCD , 
  0xCD , 
  0xEF , 
  0xEF , 
  0xCE ,
  0xCE , 
  0xF0 , 
  0xF0 , 
  0xCF , 
  0xCF , 
  0xD0 , 
  0xD0 , 
  0xD1 ,
  0xD1 , 
  0xD2 , 
  0xD2 , 
  0xD3 , 
  0x8F , 
  0xF1 , 
  0xF1 , 
  0xD4 , 
  0x90 , 
  0xF2 , 
  0xF2 , 
  0xD5 , 
  0x91 , 
  0xF3 , 
  0xF3 , 
  0xD6 ,
  0x92 , 
  0xF4 , 
  0xF4 , 
  0xD7 ,
  0xD7 , 
  0x93 , 
  0x93 , 
  0xD8 , 
  0xD8 , 
  0x94 , 
  0x94 , 
  0xD9 , 
  0x96 , 
  0xF5 , 
  0x95 , 
  0xDA ,
  0x98 , 
  0xF6 , 
  0x97 , 
  0xE1 , 
  0xE1 , 
  0xF7 , 
  0x99 , 
  0xE2 , 
  0xE2 , 
  0xF8 , 
  0x9A , 
  0xE3 , 
  0xE3 , 
  0xF9 , 
  0x9B , 
  0xE4 , 
  0xE4 , 
  0xFA , 
  0xFA , 
  0xE5 , 
  0xE5 , 
  0xFB , 
  0xFB , 
  0xE6 , 
  0xE6 , 
  0xFC , 
  0xFC , 
  0xE7 , 
  0x9D , 
  0xFD , 
  0x9C , 
  0xE8 , 
  0xE8 , 
  0x8D , 
  0xE9 , 
  0x9E , 
  0xEA , 
  0xFE , 
  0xFE  
 };























static const unsigned char lboxAlefs[] =
{
  0xA2,
  0xA3,
  0xA4,
  0xA1
};

static const unsigned char lboxLams[] = 
{
  0xA5,
  0xA6
};

NS_IMETHODIMP nsUnicodeToLangBoxArabic8::Convert(
      const PRUnichar * aSrc, PRInt32 * aSrcLength,
      char * aDest, PRInt32 * aDestLength)
{
   char* dest = aDest;
   PRInt32 inlen = 0;

   while (inlen < *aSrcLength) {
     PRUnichar aChar = aSrc[inlen];

     if((aChar >= 0x0660) && (aChar <=0x0669)) { 
       *dest++ = (char)(aChar - 0x0660 + 0xB0);
     } else if ((aChar >= 0x064B) && (aChar <= 0x0652)) {
       *dest++ = (char)(aChar - 0x64B + 0xA8);
     } else if(0x060C == aChar) {
       *dest++ = (char)0xBA;
     } else if(0x061B == aChar) {
       *dest++ = (char)0xBB;
     } else if(0x061F == aChar) {
       *dest++ = (char)0xBF;
     } else if(0x0640 == aChar) {
       *dest++ = (char)0xE0;
     } else if ((aChar>=0xFE80) && (aChar <= 0xFEF4)) {
       *dest++ = uni2lbox[aChar-0xFE80];
     } else if ((aChar >=0xFEF5) && (aChar <= 0xFEFC)) {
       PRUint8 lamAlefType = aChar - 0xFEF5;       
       PRUint8 alefType = (lamAlefType & 6) >> 1;  
       PRUint8 lamType = lamAlefType & 1;          

       *dest++ = lboxAlefs[alefType];
       *dest++ = lboxLams[lamType];
     } else if ((aChar >=0x0001) && (aChar <= 0x007F)) {
       *dest++ = (char) (aChar & 0x7F);
     } else {
       
     }
     inlen++;
   }

    *aDestLength = dest - aDest;
    return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic8::GetMaxLength(
const PRUnichar * aSrc, PRInt32 aSrcLength, 
                           PRInt32 * aDestLength) 
{
  *aDestLength = 2*aSrcLength;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic8::Finish(
      char * aDest, PRInt32 * aDestLength)
{
   *aDestLength=0;
   return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic8::Reset()
{
   return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic8::SetOutputErrorBehavior(
      PRInt32 aBehavior,
      nsIUnicharEncoder * aEncoder, PRUnichar aChar)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsUnicodeToLangBoxArabic8::FillInfo(PRUint32* aInfo)
{
   PRUnichar i;

   SET_REPRESENTABLE(aInfo, 0x060c);      
   SET_REPRESENTABLE(aInfo, 0x061b);      
   SET_REPRESENTABLE(aInfo, 0x061f);      
   for(i=0x0621;i<=0x063a;i++)
      SET_REPRESENTABLE(aInfo, i);      
   for(i=0x0640;i<=0x0652;i++)
      SET_REPRESENTABLE(aInfo, i);      
   for(i=0x0660;i<=0x0669;i++)
      SET_REPRESENTABLE(aInfo, i);      

   
   for(i=0xFE80; i < 0xFEFD;i++)
     SET_REPRESENTABLE(aInfo, i);

   
   for(i=0x0000; i < 0x007f;i++)
     SET_REPRESENTABLE(aInfo, i);

   return NS_OK;
}
