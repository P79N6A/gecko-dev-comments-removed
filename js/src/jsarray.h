





#ifndef jsarray_h___
#define jsarray_h___



#include "jscntxt.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsobj.h"

namespace js {

const uint32_t MAX_ARRAY_INDEX = 4294967294u;
}

inline JSBool
js_IdIsIndex(jsid id, uint32_t *indexp)
{
    if (JSID_IS_INT(id)) {
        int32_t i = JSID_TO_INT(id);
        JS_ASSERT(i >= 0);
        *indexp = (uint32_t)i;
        return true;
    }

    if (JS_UNLIKELY(!JSID_IS_STRING(id)))
        return false;

    return js::StringIsArrayIndex(JSID_TO_ATOM(id), indexp);
}

extern JSObject *
js_InitArrayClass(JSContext *cx, js::HandleObject obj);

extern bool
js_InitContextBusyArrayTable(JSContext *cx);

namespace js {


extern JSObject * JS_FASTCALL
NewDenseEmptyArray(JSContext *cx, RawObject proto = NULL,
                   NewObjectKind newKind = GenericObject);


extern JSObject * JS_FASTCALL
NewDenseAllocatedArray(JSContext *cx, uint32_t length, RawObject proto = NULL,
                       NewObjectKind newKind = GenericObject);





extern JSObject * JS_FASTCALL
NewDenseUnallocatedArray(JSContext *cx, uint32_t length, RawObject proto = NULL,
                         NewObjectKind newKind = GenericObject);


extern JSObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, HandleObject src, uint32_t elementOffset, RawObject proto = NULL);


extern JSObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, const Value *values, RawObject proto = NULL,
                    NewObjectKind newKind = GenericObject);


extern RawShape
GetDenseArrayShape(JSContext *cx, HandleObject globalObj);

extern JSBool
GetLengthProperty(JSContext *cx, HandleObject obj, uint32_t *lengthp);

extern JSBool
SetLengthProperty(JSContext *cx, HandleObject obj, double length);

extern JSBool
ObjectMayHaveExtraIndexedProperties(JSObject *obj);







extern bool
GetElements(JSContext *cx, HandleObject aobj, uint32_t length, js::Value *vp);



extern JSBool
array_sort(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
array_push(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
array_pop(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
array_concat(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
array_concat_dense(JSContext *cx, HandleObject obj1, HandleObject obj2, HandleObject result);

extern void
ArrayShiftMoveElements(JSObject *obj);

extern JSBool
array_shift(JSContext *cx, unsigned argc, js::Value *vp);

} 

#ifdef DEBUG
extern JSBool
js_ArrayInfo(JSContext *cx, unsigned argc, js::Value *vp);
#endif








extern JSBool
js_NewbornArrayPush(JSContext *cx, js::HandleObject obj, const js::Value &v);


JSBool
js_Array(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
