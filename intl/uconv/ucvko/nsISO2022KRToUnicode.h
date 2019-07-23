



































#ifndef nsISO2022KRToUnicode_h__
#define nsISO2022KRToUnicode_h__
#include "nsISupports.h"
#include "nsUCSupport.h"


 
class nsISO2022KRToUnicode : public nsBasicDecoderSupport
{
public:
  nsISO2022KRToUnicode()
  { 
    mState = mState_ASCII;
    mLastLegalState = mState_ASCII;
    mData = 0;
    mEUCKRDecoder = nsnull;
  };

  virtual ~nsISO2022KRToUnicode()
  {
    NS_IF_RELEASE(mEUCKRDecoder);
  };

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
     PRUnichar * aDest, PRInt32 * aDestLength) ;
  
  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength,
     PRInt32 * aDestLength) 
  {
    *aDestLength = aSrcLength;
    return NS_OK;
  };

  NS_IMETHOD Reset()
  {
    mState = mState_ASCII;
    mLastLegalState = mState_ASCII;
    return NS_OK;
  };

private:
  enum {
    mState_ASCII,
    mState_ESC,
    mState_ESC_24,
    mState_ESC_24_29,
    mState_KSX1001_1992,
    mState_KSX1001_1992_2ndbyte,
    mState_ERROR
  } mState, mLastLegalState;

  PRUint8 mData;

  nsIUnicodeDecoder *mEUCKRDecoder;
};
#endif 
