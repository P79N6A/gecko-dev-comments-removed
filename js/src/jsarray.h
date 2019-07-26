





#ifndef jsarray_h___
#define jsarray_h___



#include "jscntxt.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsobj.h"


const unsigned MIN_SPARSE_INDEX = 256;

namespace js {

const uint32_t MAX_ARRAY_INDEX = 4294967294u;
}

inline JSBool
js_IdIsIndex(jsid id, uint32_t *indexp)
{
    if (JSID_IS_INT(id)) {
        int32_t i = JSID_TO_INT(id);
        if (i < 0)
            return JS_FALSE;
        *indexp = (uint32_t)i;
        return JS_TRUE;
    }

    if (JS_UNLIKELY(!JSID_IS_STRING(id)))
        return JS_FALSE;

    return js::StringIsArrayIndex(JSID_TO_ATOM(id), indexp);
}

extern JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj);

extern bool
js_InitContextBusyArrayTable(JSContext *cx);

namespace js {


extern JSObject * JS_FASTCALL
NewDenseEmptyArray(JSContext *cx, RawObject proto = NULL);


extern JSObject * JS_FASTCALL
NewDenseAllocatedArray(JSContext *cx, uint32_t length, RawObject proto = NULL);





extern JSObject * JS_FASTCALL
NewDenseUnallocatedArray(JSContext *cx, uint32_t length, RawObject proto = NULL);


extern JSObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, const Value *vp, RawObject proto = NULL);


extern JSObject *
NewSlowEmptyArray(JSContext *cx);

} 

extern JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, uint32_t *lengthp);

extern JSBool
js_SetLengthProperty(JSContext *cx, js::HandleObject obj, double length);

namespace js {

extern JSBool
array_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue value,
                    PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

extern JSBool
array_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                    MutableHandleValue rval, JSBool strict);







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
js_PrototypeHasIndexedProperties(JSContext *cx, JSObject *obj);




JSBool
js_GetDenseArrayElementValue(JSContext *cx, js::HandleObject obj, jsid id,
                             js::Value *vp);


JSBool
js_Array(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
