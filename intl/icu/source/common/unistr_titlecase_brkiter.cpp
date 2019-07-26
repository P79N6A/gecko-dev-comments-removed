
















#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/brkiter.h"
#include "unicode/ubrk.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "ustr_imp.h"

static int32_t U_CALLCONV
unistr_case_internalToTitle(const UCaseMap *csm,
                            UChar *dest, int32_t destCapacity,
                            const UChar *src, int32_t srcLength,
                            UErrorCode *pErrorCode) {
  ubrk_setText(csm->iter, src, srcLength, pErrorCode);
  return ustrcase_internalToTitle(csm, dest, destCapacity, src, srcLength, pErrorCode);
}





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

U_NAMESPACE_BEGIN

UnicodeString &
UnicodeString::toTitle(BreakIterator *titleIter) {
  return toTitle(titleIter, Locale::getDefault(), 0);
}

UnicodeString &
UnicodeString::toTitle(BreakIterator *titleIter, const Locale &locale) {
  return toTitle(titleIter, locale, 0);
}

UnicodeString &
UnicodeString::toTitle(BreakIterator *titleIter, const Locale &locale, uint32_t options) {
  UCaseMap csm=UCASEMAP_INITIALIZER;
  csm.options=options;
  setTempCaseMap(&csm, locale.getName());
  BreakIterator *bi=titleIter;
  if(bi==NULL) {
    UErrorCode errorCode=U_ZERO_ERROR;
    bi=BreakIterator::createWordInstance(locale, errorCode);
    if(U_FAILURE(errorCode)) {
      setToBogus();
      return *this;
    }
  }
  csm.iter=reinterpret_cast<UBreakIterator *>(bi);
  caseMap(&csm, unistr_case_internalToTitle);
  if(titleIter==NULL) {
    delete bi;
  }
  return *this;
}

U_NAMESPACE_END

#endif  
