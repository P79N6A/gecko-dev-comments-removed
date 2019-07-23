



































#ifndef nsMacLocale_h__
#define nsMacLocale_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsIMacLocale.h"



class nsMacLocale : public nsIMacLocale {

  NS_DECL_ISUPPORTS

public:
  
  nsMacLocale();
  virtual ~nsMacLocale();

  NS_IMETHOD GetPlatformLocale(const nsAString& locale,short* scriptCode, short* langCode, short* regionCode);
  NS_IMETHOD GetXPLocale(short scriptCode, short langCode, short regionCode, nsAString& locale);
  
};


#endif
