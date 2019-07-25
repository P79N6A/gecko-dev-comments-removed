







































#ifndef jslogic_h__
#define jslogic_h__

#include "MethodJIT.h"

namespace js {
namespace mjit {
namespace stubs {

void * JS_FASTCALL Call(VMFrame &f, uint32 argc);
void * JS_FASTCALL Return(VMFrame &f);

JSObject * JS_FASTCALL BindName(VMFrame &f);
void JS_FASTCALL SetName(VMFrame &f, uint32 index);
void JS_FASTCALL Name(VMFrame &f, uint32 index);
void JS_FASTCALL CallName(VMFrame &f, uint32 index);
void JS_FASTCALL DefFun(VMFrame &f, uint32 index);

void JS_FASTCALL VpInc(VMFrame &f, Value *vp);
void JS_FASTCALL VpDec(VMFrame &f, Value *vp);
void JS_FASTCALL DecVp(VMFrame &f, Value *vp);
void JS_FASTCALL IncVp(VMFrame &f, Value *vp);

void JS_FASTCALL BitAnd(VMFrame &f);

}}} 

extern "C" void *
js_InternalThrow(js::VMFrame &f);

#endif 

