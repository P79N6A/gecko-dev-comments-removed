






































#include "nsUCConstructors.h"
#include "nsUnicodeToLangBoxArabic16.h"

#include "nsISupports.h"



static const unsigned char uni2lbox [] =
{
  0x6B, 
  0x90, 
  0x6C, 
  0x3F, 
  0x6D, 
  0x3F, 
  0x6E, 
  0x93, 
  0x6F, 
  0x94, 
  0x70, 
  0x95, 
  0x71, 
  0x97, 
  0x72, 
  0x96, 
  0x41, 
  0x42, 
  0xA1, 
  0x43, 
  0xA2, 
  0x44, 
  0xA3, 
  0x45, 
  0xA4, 
  0x46, 
  0xF9, 
  0xF8, 
  0xA0, 
  0x47, 
  0xA5, 
  0x48, 
  0xAE, 
  0xAC, 
  0xAD, 
  0x49, 
  0xB1, 
  0x4A, 
  0xB4, 
  0xB2, 
  0xB3, 
  0x4B, 
  0xB7, 
  0xB5, 
  0xB6, 
  0x4C, 
  0xBA, 
  0xB8, 
  0xB9, 
  0x4D, 
  0xBD, 
  0xBB, 
  0xBC, 
  0x4E, 
  0xC0, 
  0xBE, 
  0xBF, 
  0x4F, 
  0xA6, 
  0x50, 
  0xA7, 
  0x51, 
  0xA8, 
  0x52, 
  0xA9, 
  0x53, 
  0xC3, 
  0xC1, 
  0xC2, 
  0x54, 
  0xC6, 
  0xC4, 
  0xC5, 
  0x55, 
  0xC9, 
  0xC7, 
  0xC8, 
  0x56, 
  0xCC, 
  0xCA, 
  0xCB, 
  0x57, 
  0xCF, 
  0xCD, 
  0xCE, 
  0x58, 
  0xD2, 
  0xD0, 
  0xD1, 
  0x59, 
  0xD5, 
  0xD3, 
  0xD4, 
  0x5A, 
  0xD8, 
  0xD6, 
  0xD7, 
  0x61, 
  0xDB, 
  0xD9, 
  0xDA, 
  0x62, 
  0xDE, 
  0xDC, 
  0xDD, 
  0x63, 
  0xE1, 
  0xDF, 
  0xE0, 
  0x64, 
  0xE4, 
  0xE2, 
  0xE3, 
  0x65, 
  0xE7, 
  0xE5, 
  0xE6, 
  0x66, 
  0xEA, 
  0xE8, 
  0xE9, 
  0x67, 
  0xED, 
  0xEB, 
  0xEC, 
  0x68, 
  0xAA, 
  0x69, 
  0xAB, 
  0x6A, 
  0xF0, 
  0xEE, 
  0xEF, 
  0x76, 
  0xFA, 
  0x77, 
  0xFC, 
  0x78, 
  0xFB, 
  0x79, 
  0xFD  
 };

NS_IMETHODIMP nsUnicodeToLangBoxArabic16::Convert(
      const PRUnichar * aSrc, PRInt32 * aSrcLength,
      char * aDest, PRInt32 * aDestLength)
{
   char* dest = aDest;
   PRInt32 inlen = 0;

   while (inlen < *aSrcLength) {
     PRUnichar aChar = aSrc[inlen];
     
     if (((aChar >= 0x0020) && (aChar <= 0x0027)) ||
          (aChar == 0x2A) ||
          (aChar == 0x2B) ||
         ((aChar >= 0x002D) && (aChar <= 0x002F)) ||
          (aChar == 0x003A) ||
         ((aChar >= 0x003C) && (aChar <= 0x003E)) ||
          (aChar == 0x40) ||
          (aChar == 0x5C) ||
          (aChar == 0x5E) ||
          (aChar == 0x5F) ||
          (aChar == 0x7C) ||
          (aChar == 0x7E)) {
       *dest++ = (char) aChar;
       
       
       
     } else if (0x0028 == aChar) {
       *dest++ = 0x29;
     } else if (0x0029 == aChar) {
       *dest++ = 0x28;
     } else if (0x005B == aChar) {
       *dest++ = 0x5D;
     } else if (0x005D == aChar) {
       *dest++ = 0x5B;
     } else if (0x007B == aChar) {
       *dest++ = 0x7D;
     } else if (0x007D == aChar) {
       *dest++ = 0x7B;
     } else if (0x060C == aChar) {
       
       *dest++ = 0x2C;
     } else if (0x061B == aChar) {
       
       *dest++ = 0x3B;
     } else if (0x061F == aChar) {
       
       *dest++ = 0x3F;
     } else if (0x0640 == aChar) {
       
       *dest++ = 0x60;
     } else if ((aChar >= 0x0660) && (aChar <=0x0669)) {
       
       *dest++ = (char)(aChar - 0x0660 + 0x30);
     } else if ((aChar>=0xFE70) && (aChar <= 0xFEFC)) {
       
       *dest++ = uni2lbox[aChar-0xFE70];
     } else {
       
     }
     inlen++;
   }

    *aDestLength = dest - aDest;
    return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic16::GetMaxLength(
const PRUnichar * aSrc, PRInt32 aSrcLength,
                           PRInt32 * aDestLength)
{
  *aDestLength = aSrcLength;
  return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic16::Finish(
      char * aDest, PRInt32 * aDestLength)
{
   *aDestLength=0;
   return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic16::Reset()
{
   return NS_OK;
}

NS_IMETHODIMP nsUnicodeToLangBoxArabic16::SetOutputErrorBehavior(
      PRInt32 aBehavior,
      nsIUnicharEncoder * aEncoder, PRUnichar aChar)
{
   return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsUnicodeToLangBoxArabic16::FillInfo(PRUint32* aInfo)
{
   PRUnichar i;

   




   for(i=0x0000; i <= 0x007F; i++)
     CLEAR_REPRESENTABLE(aInfo, i);

   
   for(i=0x0020; i <= 0x002B; i++)
     SET_REPRESENTABLE(aInfo, i);
   for(i=0x002D; i <= 0x002F; i++)
     SET_REPRESENTABLE(aInfo, i);
   SET_REPRESENTABLE(aInfo, 0x003A);
   for(i=0x003C; i <= 0x003E; i++)
     SET_REPRESENTABLE(aInfo, i);
   SET_REPRESENTABLE(aInfo, 0x0040);
   for(i=0x005B; i <= 0x005F; i++)
     SET_REPRESENTABLE(aInfo, i);
   for(i=0x007B; i <= 0x007E;i++)
     SET_REPRESENTABLE(aInfo, i);

   
   SET_REPRESENTABLE(aInfo, 0x060c);
   SET_REPRESENTABLE(aInfo, 0x061b);
   SET_REPRESENTABLE(aInfo, 0x061f);
   SET_REPRESENTABLE(aInfo, 0x0640);
   for(i=0x0660; i<=0x0669; i++)
      SET_REPRESENTABLE(aInfo, i);

   
   for(i=0xFE70; i <= 0xFE72; i++)
     SET_REPRESENTABLE(aInfo, i);
   SET_REPRESENTABLE(aInfo, 0xFE74);
   for(i=0xFE76; i <= 0xFEFC; i++)
     SET_REPRESENTABLE(aInfo, i);

   return NS_OK;
}
