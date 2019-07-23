




































 







#ifndef nsUnicodeToGBK_h___
#define nsUnicodeToGBK_h___

#include "nsUCSupport.h"
#include "nsCOMPtr.h"
#include "nsIUnicodeEncoder.h"
#include "gbku.h"



class nsUnicodeToGBK: public nsEncoderSupport
{
public:

  


  nsUnicodeToGBK(PRUint32 aMaxLengthFactor = 2);
  virtual ~nsUnicodeToGBK() {};

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const PRUnichar * aSrc, 
                            PRInt32 * aSrcLength, 
                            char * aDest, 
                            PRInt32 * aDestLength);

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                                char * aDest, PRInt32 * aDestLength)
  {
    return NS_OK;
  };  

  NS_IMETHOD FillInfo(PRUint32 *aInfo);

  virtual void CreateExtensionEncoder();
  virtual void Create4BytesEncoder();

  nsCOMPtr<nsIUnicodeEncoder> mExtensionEncoder;
  nsCOMPtr<nsIUnicodeEncoder> m4BytesEncoder;
protected:
  PRUnichar mSurrogateHigh;
  nsGBKConvUtil mUtil;
  PRBool TryExtensionEncoder(PRUnichar aChar, char* aDest, PRInt32* aOutLen);
  PRBool Try4BytesEncoder(PRUnichar aChar, char* aDest, PRInt32* aOutLen);
  virtual PRBool EncodeSurrogate(PRUnichar aSurrogateHigh, PRUnichar aSurrogateLow, char* aDest);
};

class nsUnicodeToGB18030: public nsUnicodeToGBK
{
public:
  nsUnicodeToGB18030() : nsUnicodeToGBK(4) {};
  virtual ~nsUnicodeToGB18030() {};
protected:
  virtual void CreateExtensionEncoder();
  virtual void Create4BytesEncoder();
  virtual PRBool EncodeSurrogate(PRUnichar aSurrogateHigh, PRUnichar aSurrogateLow, char* aDest);
};

#ifdef MOZ_EXTRA_X11CONVERTERS
class nsUnicodeToGB18030Font0: public nsUnicodeToGB18030
{
public:
  nsUnicodeToGB18030Font0() {};
  virtual ~nsUnicodeToGB18030Font0() {};
protected:
  virtual void Create4BytesEncoder();
  NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength, 
      PRInt32 * aDestLength);
  NS_IMETHOD FillInfo(PRUint32 *aInfo);
};
class nsUnicodeToGB18030Font1 : public nsTableEncoderSupport
{
public: 
  nsUnicodeToGB18030Font1();
  virtual ~nsUnicodeToGB18030Font1() {};
protected: 
  NS_IMETHOD FillInfo(PRUint32 *aInfo);
};
#endif 

#endif 

