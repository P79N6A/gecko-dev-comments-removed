







































#ifndef jsdbgapiinlines_h___
#define jsdbgapiinlines_h___

#include "jsdbgapi.h"
#include "jscntxt.h"

#if defined(JS_HAS_OBJ_WATCHPOINT) && defined(__cplusplus)

extern bool
js_SlowPathUpdateWatchpointsForShape(JSContext *cx, JSObject *obj, const js::Shape *newShape);






static inline bool
js_UpdateWatchpointsForShape(JSContext *cx, JSObject *obj, const js::Shape *newShape)
{
    if (JS_CLIST_IS_EMPTY(&cx->runtime->watchPointList))
        return true;

    return js_SlowPathUpdateWatchpointsForShape(cx, obj, newShape);
}

#endif 

#endif 
