




#ifndef nsUTF7ToUnicode_h___
#define nsUTF7ToUnicode_h___

#include "nsUCSupport.h"










class nsBasicUTF7Decoder : public nsBufferDecoderSupport
{
public:

  


  nsBasicUTF7Decoder(char aLastChar, char aEscChar);

protected:

  int32_t                   mEncoding;      
  uint32_t                  mEncBits;
  int32_t                   mEncStep;
  char                      mLastChar;
  char                      mEscChar;
  bool                      mFreshBase64;

  nsresult DecodeDirect(const char * aSrc, int32_t * aSrcLength, 
      PRUnichar * aDest, int32_t * aDestLength);
  nsresult DecodeBase64(const char * aSrc, int32_t * aSrcLength, 
      PRUnichar * aDest, int32_t * aDestLength);
  uint32_t CharToValue(char aChar);

  
  

  NS_IMETHOD ConvertNoBuff(const char * aSrc, int32_t * aSrcLength, 
      PRUnichar * aDest, int32_t * aDestLength);
  NS_IMETHOD Reset();
};










class nsUTF7ToUnicode : public nsBasicUTF7Decoder 
{
public:

  


  nsUTF7ToUnicode();

};

#endif 
