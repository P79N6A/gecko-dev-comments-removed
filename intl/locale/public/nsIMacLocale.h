



































#ifndef nsIMacLocale_h__
#define nsIMacLocale_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include <Script.h>


#define NS_IMACLOCALE_IID                \
{  0xe58b24b2, 0xfd1a, 0x11d2,           \
{  0x9e, 0x8e, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b }}  


class nsIMacLocale : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMACLOCALE_IID)

  NS_IMETHOD GetPlatformLocale(const nsAString& locale, short* scriptCode, 
                               short* langCode, short* regionCode) = 0;
  NS_IMETHOD GetXPLocale(short scriptCode, short langCode, short regionCode,
                         nsAString& locale) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMacLocale, NS_IMACLOCALE_IID)

#endif
