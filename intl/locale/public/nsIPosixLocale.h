




































#ifndef nsIPosixLocale_h__
#define nsIPosixLocale_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"


#define	NS_IPOSIXLOCALE_IID	  \
{   0xa434957c, \
    0x6514, \
    0x447c, \
    {0x99, 0x1b, 0x21, 0x17, 0xb6, 0x33, 0x35, 0x9c} }

#define MAX_LANGUAGE_CODE_LEN 3
#define MAX_COUNTRY_CODE_LEN  3
#define MAX_LOCALE_LEN        128
#define MAX_EXTRA_LEN         65

class nsIPosixLocale : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPOSIXLOCALE_IID)

  NS_IMETHOD GetPlatformLocale(const nsAString& locale, nsACString& posixLocale) = 0;
  NS_IMETHOD GetXPLocale(const char* posixLocale, nsAString& locale) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPosixLocale, NS_IPOSIXLOCALE_IID)

#endif
