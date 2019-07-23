



































#ifndef nsIWin32Locale_h__
#define nsIWin32Locale_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include <windows.h>


#define  NS_IWIN32LOCALE_IID              \
{  0xd92d57c2, 0xba1d, 0x11d2,            \
{  0xaf, 0xc, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b }}


class nsIWin32Locale : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IWIN32LOCALE_IID)

  NS_IMETHOD GetPlatformLocale(const nsAString& locale, LCID* winLCID) = 0;
  NS_IMETHOD GetXPLocale(LCID winLCID, nsAString& locale) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIWin32Locale, NS_IWIN32LOCALE_IID)

#endif
