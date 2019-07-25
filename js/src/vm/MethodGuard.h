








































#ifndef MethodGuard_h___
#define MethodGuard_h___

#include "jsobj.h"

namespace js {










extern void
ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp);






























inline JSObject *
NonGenericMethodGuard(JSContext *cx, CallArgs args, Native native, Class *clasp, bool *ok);








extern bool
HandleNonGenericMethodClassMismatch(JSContext *cx, CallArgs args, Native native, Class *clasp);








template <typename T>
inline bool
BoxedPrimitiveMethodGuard(JSContext *cx, CallArgs args, Native native, T *v, bool *ok);

} 

#endif 
