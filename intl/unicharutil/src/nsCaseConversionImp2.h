




































#ifndef nsCaseConversionImp2_h__
#define nsCaseConversionImp2_h__

#include "nscore.h"
#include "nsISupports.h"

#include "nsICaseConversion.h"

class nsCaseConversionImp2 : public nsICaseConversion { 
  NS_DECL_ISUPPORTS 

public:
  virtual ~nsCaseConversionImp2() { }

  static nsCaseConversionImp2* GetInstance();

  NS_IMETHOD ToUpper(PRUnichar aChar, PRUnichar* aReturn);

  NS_IMETHOD ToLower(PRUnichar aChar, PRUnichar* aReturn);

  NS_IMETHOD ToTitle(PRUnichar aChar, PRUnichar* aReturn);

  NS_IMETHOD ToUpper(const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen);

  NS_IMETHOD ToLower(const PRUnichar* anArray, PRUnichar* aReturn, PRUint32 aLen);

  NS_IMETHOD ToTitle(const PRUnichar* anArray, PRUnichar* aReturn, 
                     PRUint32 aLen, PRBool aStartInWordBoundary = PR_TRUE);
   
  NS_IMETHOD CaseInsensitiveCompare(const PRUnichar* aLeft, const PRUnichar* aRight, PRUint32 aLength, PRInt32 *aResult);
};

extern nsCaseConversionImp2* gCaseConv;

#endif
