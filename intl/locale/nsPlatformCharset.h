



#ifndef nsPlatformCharset_h__
#define nsPlatformCharset_h__

#include "nsIPlatformCharset.h"

class nsPlatformCharset : public nsIPlatformCharset
{
  NS_DECL_ISUPPORTS

public:

  nsPlatformCharset();

  NS_IMETHOD Init();
  NS_IMETHOD GetCharset(nsPlatformCharsetSel selector, nsACString& oResult) MOZ_OVERRIDE;
  NS_IMETHOD GetDefaultCharsetForLocale(const nsAString& localeName, nsACString& oResult) MOZ_OVERRIDE;

private:
  nsCString mCharset;
  nsString mLocale; 

  nsresult MapToCharset(nsAString& inANSICodePage, nsACString& outCharset);
  nsresult InitGetCharset(nsACString& oString);
  nsresult VerifyCharset(nsCString &aCharset);

  virtual ~nsPlatformCharset();
};

#endif 


