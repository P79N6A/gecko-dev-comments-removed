



#ifndef nsGBKToUnicode_h___
#define nsGBKToUnicode_h___

#include "nsCOMPtr.h"
#include "nsIUnicodeDecoder.h"
#include "nsUCSupport.h"
#include "nsGBKConvUtil.h"











class nsGB18030ToUnicode : public nsBufferDecoderSupport
{
public:
		  
  


  nsGB18030ToUnicode() : nsBufferDecoderSupport(1)
  {
    mExtensionDecoder = nullptr;
    m4BytesDecoder = nullptr;
  }

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const char* aSrc, int32_t * aSrcLength, char16_t *aDest, int32_t * aDestLength);

protected:
  nsGBKConvUtil mUtil;
  nsCOMPtr<nsIUnicodeDecoder> mExtensionDecoder;
  nsCOMPtr<nsIUnicodeDecoder> m4BytesDecoder;

  void CreateExtensionDecoder();
  void Create4BytesDecoder();
  bool TryExtensionDecoder(const char* aSrc, char16_t* aDest);
  bool Try4BytesDecoder(const char* aSrc, char16_t* aDest);
  bool DecodeToSurrogate(const char* aSrc, char16_t* aDest);

};

#endif 

