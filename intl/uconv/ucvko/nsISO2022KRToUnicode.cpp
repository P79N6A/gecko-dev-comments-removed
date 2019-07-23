




































#include "nsISO2022KRToUnicode.h"
#include "nsUCSupport.h"
#include "nsICharsetConverterManager.h"
#include "nsIServiceManager.h"

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

NS_IMETHODIMP nsISO2022KRToUnicode::Convert(const char * aSrc, PRInt32 * aSrcLen, PRUnichar * aDest, PRInt32 * aDestLen)
{
  const unsigned char* srcEnd = (unsigned char*)aSrc + *aSrcLen;
  const unsigned char* src =(unsigned char*) aSrc;
  PRUnichar* destEnd = aDest + *aDestLen;
  PRUnichar* dest = aDest;
  while((src < srcEnd))
  {
    switch(mState)
    {
      case mState_ASCII:
        if(0x1b == *src) {
            mLastLegalState = mState;
            mState = mState_ESC;
        } 
        else if(0x0e == *src) { 
          mState = mState_KSX1001_1992;
        } 
        else if(*src & 0x80) {
          *dest++ = 0xFFFD;
          if(dest >= destEnd)
            goto error1;
        } 
        else {
          *dest++ = (PRUnichar) *src;
          if(dest >= destEnd)
            goto error1;
        }
        break;
          
      case mState_ESC:
        if('$' == *src) {
          mState = mState_ESC_24;
        } 
        else  {
          if((dest+2) >= destEnd)
            goto error1;
          *dest++ = (PRUnichar) 0x1b;
          *dest++ = (0x80 & *src) ? 0xFFFD : (PRUnichar) *src;
          mState =  mLastLegalState;
        }
        break;

      case mState_ESC_24: 
        if(')' == *src) {
          mState = mState_ESC_24_29;
        } 
        else  {
          if((dest+3) >= destEnd)
            goto error1;
          *dest++ = (PRUnichar) 0x1b;
          *dest++ = (PRUnichar) '$';
          *dest++ = (0x80 & *src) ? 0xFFFD : (PRUnichar) *src;
          mState = mLastLegalState;
        }
        break;

      case mState_ESC_24_29: 
        mState = mLastLegalState;
        if('C' == *src) {
          mState = mState_ASCII;
        } 
        else  {
          if((dest+4) >= destEnd)
            goto error1;
          *dest++ = (PRUnichar) 0x1b;
          *dest++ = (PRUnichar) '$';
          *dest++ = (PRUnichar) ')';
          *dest++ = (0x80 & *src) ? 0xFFFD : (PRUnichar) *src;
          mState = mLastLegalState;
        }
        break;

      case mState_KSX1001_1992:
        if (0x20 < (PRUint8) *src  && (PRUint8) *src < 0x7f) {
          mData = (PRUint8) *src;
          mState = mState_KSX1001_1992_2ndbyte;
        } 
        else if (0x0f == *src) { 
          mState = mState_ASCII;
        } 
        else if ((PRUint8) *src == 0x20 || (PRUint8) *src == 0x09) {
          
          mState = mState_KSX1001_1992;
          *dest++ = (PRUnichar) *src;
          if(dest >= destEnd)
          goto error1;
        } 
        else {         
          *dest++ = 0xFFFD;
          if(dest >= destEnd)
             goto error1;
        }
        break;

      case mState_KSX1001_1992_2ndbyte:
        if ( 0x20 < (PRUint8) *src && (PRUint8) *src < 0x7f  ) {
          if (!mEUCKRDecoder) {
            
            nsresult rv;
            nsCOMPtr<nsICharsetConverterManager> ccm = 
                  do_GetService(kCharsetConverterManagerCID, &rv);
            if (NS_SUCCEEDED(rv)) {
              rv = ccm->GetUnicodeDecoderRaw("EUC-KR", &mEUCKRDecoder);
            }
          }

          if (!mEUCKRDecoder) {
           *dest++ = 0xFFFD;
          } 
          else {              
            unsigned char ksx[2];
            PRUnichar uni;
            PRInt32 ksxLen = 2, uniLen = 1;
            
            
            
            ksx[0] = mData | 0x80;
            ksx[1] = *src | 0x80;
            
            mEUCKRDecoder->Convert((const char *)ksx, &ksxLen, &uni, &uniLen);
            *dest++ = uni;
          }
          if(dest >= destEnd)
            goto error1;
          mState = mState_KSX1001_1992;
        } 
        else {        
          if ( 0x0f == *src ) {   
            mState = mState_ASCII;
          } 
          else {
            mState = mState_KSX1001_1992;
          }
          *dest++ = 0xFFFD;
          if(dest >= destEnd)
           goto error1;
        }
        break;

      case mState_ERROR:
        mState = mLastLegalState;
        *dest++ = 0xFFFD;
        if(dest >= destEnd)
          goto error1;
        break;

    } 
    src++;
    if ( *src == 0x0a || *src == 0x0d )   
      mState = mState_ASCII;
   }
   *aDestLen = dest - aDest;
   return NS_OK;

error1:
   *aDestLen = dest-aDest;
   *aSrcLen = src-(unsigned char*)aSrc;
   return NS_OK_UDEC_MOREOUTPUT;
}

