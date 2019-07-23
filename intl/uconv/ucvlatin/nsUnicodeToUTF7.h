




































#ifndef nsUnicodeToUTF7_h___
#define nsUnicodeToUTF7_h___

#include "nsUCSupport.h"










class nsBasicUTF7Encoder : public nsEncoderSupport
{
public:

  


  nsBasicUTF7Encoder(char aLastChar, char aEscChar);

  NS_IMETHOD FillInfo(PRUint32 *aInfo);

protected:

  PRInt32                   mEncoding;      
  PRUint32                  mEncBits;
  PRInt32                   mEncStep;
  char                      mLastChar;
  char                      mEscChar;

  nsresult ShiftEncoding(PRInt32 aEncoding, char * aDest, 
      PRInt32 * aDestLength);
  nsresult EncodeDirect(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
  nsresult EncodeBase64(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
  char ValueToChar(PRUint32 aValue);
  virtual PRBool DirectEncodable(PRUnichar aChar);

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
      char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD FinishNoBuff(char * aDest, PRInt32 * aDestLength);
  NS_IMETHOD Reset();
};










class nsUnicodeToUTF7 : public nsBasicUTF7Encoder
{
public:

  


  nsUnicodeToUTF7();

protected:

  virtual PRBool DirectEncodable(PRUnichar aChar);
};

#endif 
