




































#ifndef nsIWin32LocaleImpl_h__
#define nsIWin32LocaleImpl_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
#include "nsILocale.h"
#include "nsIWin32Locale.h"
#include <windows.h>

class nsIWin32LocaleImpl: public nsIWin32Locale
{

	NS_DECL_ISUPPORTS

public:

	nsIWin32LocaleImpl(void);
	~nsIWin32LocaleImpl(void);

	NS_IMETHOD GetPlatformLocale(const nsAString& locale, LCID* winLCID);
	NS_IMETHOD GetXPLocale(LCID winLCID, nsAString& locale);

protected:
	inline PRBool	ParseLocaleString(const char* locale_string, char* language, char* country, char* region);

};

#endif
