



































#ifndef nsGBK2312ToUnicode_h___
#define nsGBK2312ToUnicode_h___

#include "nsCOMPtr.h"
#include "nsIUnicodeDecoder.h"
#include "nsUCSupport.h"
#include "gbku.h"











class nsGBKToUnicode : public nsBufferDecoderSupport
{
public:
		  
  


  nsGBKToUnicode() : nsBufferDecoderSupport(1)
  {
    mExtensionDecoder = nsnull;
    m4BytesDecoder = nsnull;
  };

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const char* aSrc, PRInt32 * aSrcLength, PRUnichar *aDest, PRInt32 * aDestLength);

protected:
  nsGBKConvUtil mUtil;
  nsCOMPtr<nsIUnicodeDecoder> mExtensionDecoder;
  nsCOMPtr<nsIUnicodeDecoder> m4BytesDecoder;

  virtual void CreateExtensionDecoder();
  virtual void Create4BytesDecoder();
  PRBool TryExtensionDecoder(const char* aSrc, PRUnichar* aDest);
  PRBool Try4BytesDecoder(const char* aSrc, PRUnichar* aDest);
  virtual PRBool DecodeToSurrogate(const char* aSrc, PRUnichar* aDest);

};


class nsGB18030ToUnicode : public nsGBKToUnicode
{
public:
  nsGB18030ToUnicode() {};
  virtual ~nsGB18030ToUnicode() {};
protected:
  virtual void CreateExtensionDecoder();
  virtual void Create4BytesDecoder();
  virtual PRBool DecodeToSurrogate(const char* aSrc, PRUnichar* aDest);
};

#endif 

