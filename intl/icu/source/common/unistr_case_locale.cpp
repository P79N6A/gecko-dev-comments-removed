
















#include "unicode/utypes.h"
#include "unicode/locid.h"
#include "unicode/unistr.h"
#include "cmemory.h"
#include "ustr_imp.h"

U_NAMESPACE_BEGIN









static inline void
setTempCaseMap(UCaseMap *csm, const char *locale) {
    if(csm->csp==NULL) {
        csm->csp=ucase_getSingleton();
    }
    if(locale!=NULL && locale[0]==0) {
        csm->locale[0]=0;
    } else {
        ustrcase_setTempCaseMapLocale(csm, locale);
    }
}

UnicodeString &
UnicodeString::toLower() {
  return toLower(Locale::getDefault());
}

UnicodeString &
UnicodeString::toLower(const Locale &locale) {
  UCaseMap csm=UCASEMAP_INITIALIZER;
  setTempCaseMap(&csm, locale.getName());
  return caseMap(&csm, ustrcase_internalToLower);
}

UnicodeString &
UnicodeString::toUpper() {
  return toUpper(Locale::getDefault());
}

UnicodeString &
UnicodeString::toUpper(const Locale &locale) {
  UCaseMap csm=UCASEMAP_INITIALIZER;
  setTempCaseMap(&csm, locale.getName());
  return caseMap(&csm, ustrcase_internalToUpper);
}

U_NAMESPACE_END
