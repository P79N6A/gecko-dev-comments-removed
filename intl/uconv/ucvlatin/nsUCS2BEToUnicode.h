




































#ifndef nsUCS2BEToUnicode_h___
#define nsUCS2BEToUnicode_h___

#include "nsISupports.h"
#include "nsUCSupport.h"


class nsUTF16ToUnicodeBase : public nsBasicDecoderSupport
{
protected:
  
  nsUTF16ToUnicodeBase() { Reset();}

public: 
  
  

  NS_IMETHOD GetMaxLength(const char * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
  NS_IMETHOD Reset();

protected:
  PRUint8 mState;
  PRUint8 mData;
};


class nsUTF16BEToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength); 
};


class nsUTF16LEToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength); 
};


class nsUTF16ToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, PRInt32 * aSrcLength,
      PRUnichar * aDest, PRInt32 * aDestLength); 

  NS_IMETHOD Reset();

private:

  enum Endian {kUnknown, kBigEndian, kLittleEndian};
  Endian  mEndian; 
  PRBool  mFoundBOM;
};

#endif
