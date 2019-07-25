



































#ifndef nsWin32Locale_h__
#define nsWin32Locale_h__

#include "nscore.h"
#include "nsString.h"
#include <windows.h>


class nsWin32Locale {
public: 
  static nsresult    GetPlatformLocale(const nsAString& locale, LCID* winLCID); 
  static void        GetXPLocale(LCID winLCID, nsAString& locale);

private:
  
  nsWin32Locale(void) {}

  typedef LCID (WINAPI*LocaleNameToLCIDPtr)(LPCWSTR lpName, DWORD dwFlags);
  typedef int (WINAPI*LCIDToLocaleNamePtr)(LCID Locale, LPWSTR lpName,
                                           int cchName, DWORD dwFlags);

  static LocaleNameToLCIDPtr localeNameToLCID;
  static LCIDToLocaleNamePtr lcidToLocaleName;

  static void initFunctionPointers ();
};

#endif
