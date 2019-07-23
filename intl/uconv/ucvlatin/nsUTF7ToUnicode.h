




































#ifndef nsUTF7ToUnicode_h___
#define nsUTF7ToUnicode_h___

#include "nsUCSupport.h"










class nsBasicUTF7Decoder : public nsBufferDecoderSupport
{
public:

  


  nsBasicUTF7Decoder(char aLastChar, char aEscChar);

protected:

  PRInt32                   mEncoding;      
  PRUint32                  mEncBits;
  PRInt32                   mEncStep;
  char                      mLastChar;
  char                      mEscChar;
  PRBool                    mFreshBase64;

  nsresult DecodeDirect(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  nsresult DecodeBase64(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  PRUint32 CharToValue(char aChar);

  
  

  NS_IMETHOD ConvertNoBuff(const char * aSrc, PRInt32 * aSrcLength, 
      PRUnichar * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
};










class nsUTF7ToUnicode : public nsBasicUTF7Decoder 
{
public:

  


  nsUTF7ToUnicode();

};

#endif 
