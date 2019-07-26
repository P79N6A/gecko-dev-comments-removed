





#ifndef builtin_ParallelArray_h
#define builtin_ParallelArray_h

#include "jsobj.h"

namespace js {

class ParallelArrayObject : public JSObject
{
    static const Class protoClass;
    static const JSFunctionSpec methods[];
    static const uint32_t NumFixedSlots = 4;
    static const uint32_t NumCtors = 4;
    static FixedHeapPtr<PropertyName> ctorNames[NumCtors];

    static bool initProps(JSContext *cx, HandleObject obj);

  public:
    static const Class class_;

    static bool construct(JSContext *cx, unsigned argc, Value *vp);
    static bool constructHelper(JSContext *cx, MutableHandleFunction ctor, CallArgs &args);

    
    
    
    
    
    
    
    
    
    
    static JSObject *newInstance(JSContext *cx, NewObjectKind newKind = GenericObject);

    
    static JSFunction *getConstructor(JSContext *cx, unsigned argc);

    static JSObject *initClass(JSContext *cx, HandleObject obj);
    static bool is(const Value &v);
};

} 

extern JSObject *
js_InitParallelArrayClass(JSContext *cx, js::HandleObject obj);

#endif 
