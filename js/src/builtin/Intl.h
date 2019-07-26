





#ifndef Intl_h___
#define Intl_h___

#include "js/RootingAPI.h"

struct JSContext;
class JSObject;










extern JSObject *
js_InitIntlClass(JSContext *cx, js::HandleObject obj);


namespace js {
















extern JSBool
intl_Collator_availableLocales(JSContext *cx, unsigned argc, Value *vp);









extern JSBool
intl_availableCollations(JSContext *cx, unsigned argc, Value *vp);











extern JSBool
intl_CompareStrings(JSContext *cx, unsigned argc, Value *vp);












extern JSBool
intl_NumberFormat_availableLocales(JSContext *cx, unsigned argc, Value *vp);








extern JSBool
intl_numberingSystem(JSContext *cx, unsigned argc, Value *vp);









extern JSBool
intl_FormatNumber(JSContext *cx, unsigned argc, Value *vp);












extern JSBool
intl_DateTimeFormat_availableLocales(JSContext *cx, unsigned argc, Value *vp);









extern JSBool
intl_availableCalendars(JSContext *cx, unsigned argc, Value *vp);









extern JSBool
intl_patternForSkeleton(JSContext *cx, unsigned argc, Value *vp);










extern JSBool
intl_FormatDateTime(JSContext *cx, unsigned argc, Value *vp);

} 

#endif 
