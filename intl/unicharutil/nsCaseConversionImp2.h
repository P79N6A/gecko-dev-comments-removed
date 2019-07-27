




#ifndef nsCaseConversionImp2_h__
#define nsCaseConversionImp2_h__

#include "nscore.h"
#include "nsISupports.h"

#include "nsICaseConversion.h"

class nsCaseConversionImp2 : public nsICaseConversion { 
  NS_DECL_THREADSAFE_ISUPPORTS 

public:
  virtual ~nsCaseConversionImp2() { }

  static nsCaseConversionImp2* GetInstance();

  NS_IMETHOD ToUpper(char16_t aChar, char16_t* aReturn);

  NS_IMETHOD ToLower(char16_t aChar, char16_t* aReturn);

  NS_IMETHOD ToTitle(char16_t aChar, char16_t* aReturn);

  NS_IMETHOD ToUpper(const char16_t* anArray, char16_t* aReturn, uint32_t aLen);

  NS_IMETHOD ToLower(const char16_t* anArray, char16_t* aReturn, uint32_t aLen);

  NS_IMETHOD CaseInsensitiveCompare(const char16_t* aLeft, const char16_t* aRight, uint32_t aLength, int32_t *aResult);
};

#endif
