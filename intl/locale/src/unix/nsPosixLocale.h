



































#ifndef nsPosixLocale_h__
#define nsPosixLocale_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsIPosixLocale.h"



class nsPosixLocale : public nsIPosixLocale {

  NS_DECL_ISUPPORTS

public:
  
  nsPosixLocale();
  virtual ~nsPosixLocale();

  NS_IMETHOD GetPlatformLocale(const nsAString& locale, nsACString& posixLocale);
  NS_IMETHOD GetXPLocale(const char* posixLocale, nsAString& locale);

protected:
  inline PRBool ParseLocaleString(const char* locale_string, char* language, char* country, char* extra, char separator);

};


#endif
