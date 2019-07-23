




































#ifndef nsIWin32LocaleImpl_h__
#define nsIWin32LocaleImpl_h__


#include "nsISupports.h"
#include "nscore.h"
#include "nsString.h"
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

	typedef LCID (WINAPI*LocaleNameToLCIDPtr)(LPCWSTR lpName, DWORD dwFlags);
	typedef int (WINAPI*LCIDToLocaleNamePtr)(LCID Locale, LPWSTR lpName,
	                                         int cchName, DWORD dwFlags);

	static LocaleNameToLCIDPtr localeNameToLCID;
	static LCIDToLocaleNamePtr lcidToLocaleName;

private:
	static HMODULE sKernelDLL;
};

#endif
