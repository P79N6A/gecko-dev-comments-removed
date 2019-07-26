





#ifndef jit_AsmJSLink_h
#define jit_AsmJSLink_h

#include "NamespaceImports.h"

class JSAtom;

namespace js {

class AsmJSActivation;
class AsmJSModule;
namespace jit { class CallSite; }


class AsmJSFrameIterator
{
    const AsmJSModule *module_;
    const jit::CallSite *callsite_;
    uint8_t *sp_;

    void settle(uint8_t *returnAddress);

  public:
    explicit AsmJSFrameIterator(const AsmJSActivation *activation);
    void operator++();
    bool done() const { return !module_; }
    JSAtom *functionDisplayAtom() const;
    unsigned computeLine(uint32_t *column) const;
};

#ifdef JS_ION



extern JSFunction *
NewAsmJSModuleFunction(ExclusiveContext *cx, JSFunction *originalFun, HandleObject moduleObj);


extern bool
IsAsmJSModuleNative(JSNative native);



extern bool
IsAsmJSModule(JSContext *cx, unsigned argc, JS::Value *vp);
extern bool
IsAsmJSModule(HandleFunction fun);

extern JSString*
AsmJSModuleToString(JSContext *cx, HandleFunction fun, bool addParenToLambda);



extern bool
IsAsmJSModuleLoadedFromCache(JSContext *cx, unsigned argc, Value *vp);



extern bool
IsAsmJSFunction(JSContext *cx, unsigned argc, JS::Value *vp);
extern bool
IsAsmJSFunction(HandleFunction fun);

extern JSString *
AsmJSFunctionToString(JSContext *cx, HandleFunction fun);

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
IsAsmJSFunction(HandleFunction fun)
{
    return false;
}

inline JSString *
AsmJSFunctionToString(JSContext *cx, HandleFunction fun)
{
    return nullptr;
}

inline bool
IsAsmJSModule(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().set(BooleanValue(false));
    return true;
}

inline bool
IsAsmJSModule(HandleFunction fun)
{
    return false;
}

inline JSString*
AsmJSModuleToString(JSContext *cx, HandleFunction fun, bool addParenToLambda)
{
    return nullptr;
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
