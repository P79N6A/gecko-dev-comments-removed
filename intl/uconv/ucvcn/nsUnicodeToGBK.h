




 







#ifndef nsUnicodeToGBK_h___
#define nsUnicodeToGBK_h___

#include "nsUCSupport.h"
#include "nsCOMPtr.h"
#include "nsIUnicodeEncoder.h"
#include "nsGBKConvUtil.h"



class nsUnicodeToGBK: public nsEncoderSupport
{
public:

  


  explicit nsUnicodeToGBK(uint32_t aMaxLengthFactor = 2);
  virtual ~nsUnicodeToGBK() {}

protected:

  
  
  NS_IMETHOD ConvertNoBuffNoErr(const char16_t * aSrc,
                                int32_t * aSrcLength,
                                char * aDest,
                                int32_t * aDestLength);

  virtual void CreateExtensionEncoder();
  virtual void Create4BytesEncoder();

  nsCOMPtr<nsIUnicodeEncoder> mExtensionEncoder;
  nsCOMPtr<nsIUnicodeEncoder> m4BytesEncoder;
protected:
  char16_t mSurrogateHigh;
  nsGBKConvUtil mUtil;
  nsresult TryExtensionEncoder(char16_t aChar, char* aDest, int32_t* aOutLen);
  nsresult Try4BytesEncoder(char16_t aChar, char* aDest, int32_t* aOutLen);
  virtual nsresult EncodeSurrogate(char16_t aSurrogateHigh,
                                   char16_t aSurrogateLow, char* aDest,
                                   int32_t aDestLength, int32_t aBufferLength);
};

class nsUnicodeToGB18030: public nsUnicodeToGBK
{
public:
  nsUnicodeToGB18030() : nsUnicodeToGBK(4) {}
  virtual ~nsUnicodeToGB18030() {}
protected:
  virtual void CreateExtensionEncoder();
  virtual void Create4BytesEncoder();
  virtual nsresult EncodeSurrogate(char16_t aSurrogateHigh,
                                   char16_t aSurrogateLow, char* aDest,
                                   int32_t aDestLength, int32_t aBufferLength);
};

#endif 

