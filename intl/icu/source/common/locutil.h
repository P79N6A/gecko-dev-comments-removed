







#ifndef LOCUTIL_H
#define LOCUTIL_H

#include "unicode/utypes.h"
#include "hash.h"

#if !UCONFIG_NO_SERVICE || !UCONFIG_NO_TRANSLITERATION


U_NAMESPACE_BEGIN




class U_COMMON_API LocaleUtility {
public:
  static UnicodeString& canonicalLocaleString(const UnicodeString* id, UnicodeString& result);
  static Locale& initLocaleFromName(const UnicodeString& id, Locale& result);
  static UnicodeString& initNameFromLocale(const Locale& locale, UnicodeString& result);
  static const Hashtable* getAvailableLocaleNames(const UnicodeString& bundleID);
  static UBool isFallbackOf(const UnicodeString& root, const UnicodeString& child);
};

U_NAMESPACE_END


#endif

#endif
