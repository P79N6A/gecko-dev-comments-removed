



































#ifndef nsICaseConversion_h__
#define nsICaseConversion_h__


#include "nsISupports.h"
#include "nscore.h"


#define NS_ICASECONVERSION_IID \
{ 0x7d3d8e0, 0x9614, 0x11d2, \
    { 0xb3, 0xad, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

class nsICaseConversion : public nsISupports {

public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICASECONVERSION_IID)

  
  NS_IMETHOD ToUpper( PRUnichar aChar, PRUnichar* aReturn) = 0;

  
  NS_IMETHOD ToLower( PRUnichar aChar, PRUnichar* aReturn) = 0;

  
  NS_IMETHOD ToTitle( PRUnichar aChar, PRUnichar* aReturn) = 0;

  
  NS_IMETHOD ToUpper( const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen) = 0;

  
  NS_IMETHOD ToLower( const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen) = 0;

  
  NS_IMETHOD ToTitle( const PRUnichar* anArray, PRUnichar* aReturn, 
                      PRUint32 aLen, PRBool aStartInWordBundary=PR_TRUE) = 0;

  
  
  NS_IMETHOD CaseInsensitiveCompare(const PRUnichar* aLeft, const PRUnichar* aRight, PRUint32 aLength, PRInt32* aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICaseConversion, NS_ICASECONVERSION_IID)

#endif  
