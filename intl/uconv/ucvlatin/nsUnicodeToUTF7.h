




#ifndef nsUnicodeToUTF7_h___
#define nsUnicodeToUTF7_h___

#include "nsUCSupport.h"










class nsBasicUTF7Encoder : public nsEncoderSupport
{
public:

  


  nsBasicUTF7Encoder(char aLastChar, char aEscChar);

protected:

  int32_t                   mEncoding;      
  uint32_t                  mEncBits;
  int32_t                   mEncStep;
  char                      mLastChar;
  char                      mEscChar;

  nsresult ShiftEncoding(int32_t aEncoding, char * aDest, 
      int32_t * aDestLength);
  nsresult EncodeDirect(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
  nsresult EncodeBase64(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
  char ValueToChar(uint32_t aValue);
  virtual bool DirectEncodable(PRUnichar aChar);

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, int32_t * aSrcLength, 
      char * aDest, int32_t * aDestLength);
  NS_IMETHOD FinishNoBuff(char * aDest, int32_t * aDestLength);
  NS_IMETHOD Reset();
};










class nsUnicodeToUTF7 : public nsBasicUTF7Encoder
{
public:

  


  nsUnicodeToUTF7();

protected:

  virtual bool DirectEncodable(PRUnichar aChar);
};

#endif 
