





#ifndef builtin_Intl_h
#define builtin_Intl_h

#include "NamespaceImports.h"
#include "unicode/utypes.h"










extern JSObject *
js_InitIntlClass(JSContext *cx, js::HandleObject obj);

namespace js {















extern bool
intl_Collator(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_Collator_availableLocales(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_availableCollations(JSContext *cx, unsigned argc, Value *vp);











extern bool
intl_CompareStrings(JSContext *cx, unsigned argc, Value *vp);











extern bool
intl_NumberFormat(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_NumberFormat_availableLocales(JSContext *cx, unsigned argc, Value *vp);








extern bool
intl_numberingSystem(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_FormatNumber(JSContext *cx, unsigned argc, Value *vp);











extern bool
intl_DateTimeFormat(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_DateTimeFormat_availableLocales(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_availableCalendars(JSContext *cx, unsigned argc, Value *vp);









extern bool
intl_patternForSkeleton(JSContext *cx, unsigned argc, Value *vp);










extern bool
intl_FormatDateTime(JSContext *cx, unsigned argc, Value *vp);




inline const UChar *
Char16ToUChar(const char16_t *chars)
{
  return reinterpret_cast<const UChar *>(chars);
}

inline UChar *
Char16ToUChar(char16_t *chars)
{
  return reinterpret_cast<UChar *>(chars);
}

} 

#endif 
