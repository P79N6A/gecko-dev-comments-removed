








































#ifndef jsinterpinlines_h___
#define jsinterpinlines_h___

#include "jsinterp.h"
#include "jslock.h"
#include "jsscope.h"

inline bool
js_MatchPropertyCacheShape(JSContext *cx, JSObject *obj, uint32 shape)
{
    return CX_OWNS_OBJECT_TITLE(cx, obj) && OBJ_SHAPE(obj) == shape;
}


#endif 
