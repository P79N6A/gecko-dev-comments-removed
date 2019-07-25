




#ifndef nsUCS2BEToUnicode_h___
#define nsUCS2BEToUnicode_h___

#include "nsISupports.h"
#include "nsUCSupport.h"


class nsUTF16ToUnicodeBase : public nsBasicDecoderSupport
{
protected:
  
  nsUTF16ToUnicodeBase() { Reset();}

public: 
  
  

  NS_IMETHOD GetMaxLength(const char * aSrc, int32_t aSrcLength, 
      int32_t * aDestLength);
  NS_IMETHOD Reset();

protected:
  uint8_t mState;
  
  uint8_t mOddByte;
  
  PRUnichar mOddHighSurrogate;
  
  PRUnichar mOddLowSurrogate;
};


class nsUTF16BEToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength,
      PRUnichar * aDest, int32_t * aDestLength); 
};


class nsUTF16LEToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength,
      PRUnichar * aDest, int32_t * aDestLength); 
};


class nsUTF16ToUnicode : public nsUTF16ToUnicodeBase
{
public:

  nsUTF16ToUnicode() { Reset();}
  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength,
      PRUnichar * aDest, int32_t * aDestLength); 

  NS_IMETHOD Reset();

private:

  enum Endian {kUnknown, kBigEndian, kLittleEndian};
  Endian  mEndian; 
  bool    mFoundBOM;
};

#endif
