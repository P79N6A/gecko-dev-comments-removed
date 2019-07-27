





#ifndef asmjs_AsmJSLink_h
#define asmjs_AsmJSLink_h

#include "NamespaceImports.h"

namespace js {



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

} 

#endif 
