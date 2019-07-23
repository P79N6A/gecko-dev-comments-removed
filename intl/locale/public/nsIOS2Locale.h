




































#ifndef _nsios2locale_h_
#define _nsios2locale_h_

#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include <os2.h>


#define NS_IOS2LOCALE_IID	                    \
{ 0xf25f74f0, 0xfb59, 0x11d3,                     \
{ 0xa9, 0xf2, 0x0, 0x20, 0x35, 0x22, 0xa0, 0x3c }}

#define MAX_LANGUAGE_CODE_LEN 3
#define MAX_COUNTRY_CODE_LEN  3
#define MAX_EXTRA_LEN         65
#define MAX_LOCALE_LEN        128

class nsIOS2Locale : public nsISupports
{
 public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOS2LOCALE_IID)

   NS_IMETHOD GetPlatformLocale(const nsAString& locale, PULONG os2Codepage) = 0;
   NS_IMETHOD GetXPLocale(const char* os2Locale, nsAString& locale)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIOS2Locale, NS_IOS2LOCALE_IID)

#endif
