



































#ifndef nsPlatformCharset_h__
#define nsPlatformCharset_h__

#include "nsIPlatformCharset.h"

class nsPlatformCharset : public nsIPlatformCharset
{
  NS_DECL_ISUPPORTS

public:

  nsPlatformCharset();
  virtual ~nsPlatformCharset();

  NS_IMETHOD Init();
  NS_IMETHOD GetCharset(nsPlatformCharsetSel selector, nsACString& oResult);
  NS_IMETHOD GetDefaultCharsetForLocale(const nsAString& localeName, nsACString& oResult);

private:
  nsCString mCharset;
  nsString mLocale; 

  nsresult MapToCharset(nsAString& inANSICodePage, nsACString& outCharset);
  nsresult InitGetCharset(nsACString& oString);
  nsresult ConvertLocaleToCharsetUsingDeprecatedConfig(nsAString& locale, nsACString& oResult);
  nsresult VerifyCharset(nsCString &aCharset);
};

#endif 


