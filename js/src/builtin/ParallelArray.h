






#ifndef ParallelArray_h__
#define ParallelArray_h__

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

#include "ion/Ion.h"
#include "vm/ForkJoin.h"
#include "vm/ThreadPool.h"

namespace js {

class ParallelArrayObject : public JSObject
{
    static Class protoClass;
    static JSFunctionSpec methods[];
    static const uint32_t NumFixedSlots = 4;
    static const uint32_t NumCtors = 4;
    static FixedHeapPtr<PropertyName> ctorNames[NumCtors];

    static bool initProps(JSContext *cx, HandleObject obj);

  public:
    static Class class_;

    static JSBool construct(JSContext *cx, unsigned argc, Value *vp);
    static JSBool constructHelper(JSContext *cx, MutableHandleFunction ctor, CallArgs &args);

    
    
    
    
    
    
    
    
    
    
    static JSObject *newInstance(JSContext *cx);

    
    static JSFunction *getConstructor(JSContext *cx, unsigned argc);

    static JSObject *initClass(JSContext *cx, HandleObject obj);
    static bool is(const Value &v);
};

} 

extern JSObject *
js_InitParallelArrayClass(JSContext *cx, js::HandleObject obj);

#endif 
