





#ifndef jit_AsmJSLink_h
#define jit_AsmJSLink_h

#include "NamespaceImports.h"

namespace js {

#ifdef JS_ION



extern JSFunction *
NewAsmJSModuleFunction(ExclusiveContext *cx, JSFunction *originalFun, HandleObject moduleObj);


extern bool
IsAsmJSModuleNative(JSNative native);



extern bool
IsAsmJSModule(JSContext *cx, unsigned argc, JS::Value *vp);



extern bool
IsAsmJSModuleLoadedFromCache(JSContext *cx, unsigned argc, Value *vp);



extern bool
IsAsmJSFunction(JSContext *cx, unsigned argc, JS::Value *vp);

#else 

inline bool
IsAsmJSModuleNative(JSNative native)
{
    return false;
}

inline bool
IsAsmJSFunction(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(BooleanValue(false));
    return true;
}

inline bool
IsAsmJSModule(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(BooleanValue(false));
    return true;
}

inline bool
IsAsmJSModuleLoadedFromCache(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(BooleanValue(false));
    return true;
}

#endif 

} 

#endif 
