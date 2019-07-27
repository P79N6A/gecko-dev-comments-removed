




#ifndef nsUTF16ToUnicode_h_
#define nsUTF16ToUnicode_h_

#include "nsISupports.h"
#include "nsUCSupport.h"


class nsUTF16ToUnicodeBase : public nsBasicDecoderSupport
{
protected:
  
  nsUTF16ToUnicodeBase() { Reset();}

  nsresult UTF16ConvertToUnicode(const char * aSrc,
                                 int32_t * aSrcLength, char16_t * aDest,
                                 int32_t * aDestLength, bool aSwapBytes);

public:
  
  

  MOZ_WARN_UNUSED_RESULT NS_IMETHOD GetMaxLength(const char * aSrc,
                                                 int32_t aSrcLength,
                                                 int32_t * aDestLength) override;
  NS_IMETHOD Reset();

protected:
  uint8_t mState;
  
  uint8_t mOddByte;
  
  char16_t mOddHighSurrogate;
  
  char16_t mOddLowSurrogate;
};


class nsUTF16BEToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength,
      char16_t * aDest, int32_t * aDestLength);
};


class nsUTF16LEToUnicode : public nsUTF16ToUnicodeBase
{
public:

  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength,
      char16_t * aDest, int32_t * aDestLength);
};


class nsUTF16ToUnicode : public nsUTF16ToUnicodeBase
{
public:

  nsUTF16ToUnicode() { Reset();}
  NS_IMETHOD Convert(const char * aSrc, int32_t * aSrcLength,
      char16_t * aDest, int32_t * aDestLength);

  NS_IMETHOD Reset();

private:

  enum Endian {kUnknown, kBigEndian, kLittleEndian};
  Endian  mEndian;
  bool    mFoundBOM;
};

#endif
