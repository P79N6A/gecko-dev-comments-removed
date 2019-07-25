







































#ifndef jslogic_h__
#define jslogic_h__

#include "MethodJIT.h"

namespace js {
namespace mjit {
namespace stubs {

void JS_FASTCALL This(VMFrame &f);
JSObject * JS_FASTCALL NewInitArray(VMFrame &f);
JSObject * JS_FASTCALL NewInitObject(VMFrame &f, uint32 empty);
JSObject * JS_FASTCALL NewArray(VMFrame &f, uint32 len);
void JS_FASTCALL InitElem(VMFrame &f, uint32 last);
void JS_FASTCALL InitProp(VMFrame &f, JSAtom *atom);
void JS_FASTCALL EndInit(VMFrame &f);

void * JS_FASTCALL Call(VMFrame &f, uint32 argc);
void * JS_FASTCALL New(VMFrame &f, uint32 argc);
void * JS_FASTCALL Return(VMFrame &f);

JSObject * JS_FASTCALL BindName(VMFrame &f);
void JS_FASTCALL SetName(VMFrame &f, uint32 index);
void JS_FASTCALL Name(VMFrame &f, uint32 index);
void JS_FASTCALL GetElem(VMFrame &f);
void JS_FASTCALL SetElem(VMFrame &f);
void JS_FASTCALL CallName(VMFrame &f, uint32 index);
void JS_FASTCALL GetUpvar(VMFrame &f, uint32 index);
void JS_FASTCALL NameInc(VMFrame &f, JSAtom *atom);
void JS_FASTCALL NameDec(VMFrame &f, JSAtom *atom);
void JS_FASTCALL IncName(VMFrame &f, JSAtom *atom);
void JS_FASTCALL DecName(VMFrame &f, JSAtom *atom);

void JS_FASTCALL DefFun(VMFrame &f, uint32 index);
JSObject * JS_FASTCALL DefLocalFun(VMFrame &f, JSFunction *fun);
JSObject * JS_FASTCALL RegExp(VMFrame &f, JSObject *regex);
JSObject * JS_FASTCALL Lambda(VMFrame &f, JSFunction *fun);

void JS_FASTCALL VpInc(VMFrame &f, Value *vp);
void JS_FASTCALL VpDec(VMFrame &f, Value *vp);
void JS_FASTCALL DecVp(VMFrame &f, Value *vp);
void JS_FASTCALL IncVp(VMFrame &f, Value *vp);

JSBool JS_FASTCALL LessThan(VMFrame &f);
JSBool JS_FASTCALL LessEqual(VMFrame &f);
JSBool JS_FASTCALL GreaterThan(VMFrame &f);
JSBool JS_FASTCALL GreaterEqual(VMFrame &f);
JSBool JS_FASTCALL Equal(VMFrame &f);
JSBool JS_FASTCALL NotEqual(VMFrame &f);

void JS_FASTCALL BitOr(VMFrame &f);
void JS_FASTCALL BitXor(VMFrame &f);
void JS_FASTCALL BitAnd(VMFrame &f);
void JS_FASTCALL BitNot(VMFrame &f);
void JS_FASTCALL Lsh(VMFrame &f);
void JS_FASTCALL Rsh(VMFrame &f);
void JS_FASTCALL Ursh(VMFrame &f);
void JS_FASTCALL Add(VMFrame &f);
void JS_FASTCALL Sub(VMFrame &f);
void JS_FASTCALL Mul(VMFrame &f);
void JS_FASTCALL Div(VMFrame &f);
void JS_FASTCALL Mod(VMFrame &f);
void JS_FASTCALL Neg(VMFrame &f);
void JS_FASTCALL ObjToStr(VMFrame &f);
void JS_FASTCALL Not(VMFrame &f);
JSBool JS_FASTCALL StrictEq(VMFrame &f);
JSBool JS_FASTCALL StrictNe(VMFrame &f);

void JS_FASTCALL Iter(VMFrame &f, uint32 flags);
void JS_FASTCALL IterNext(VMFrame &f);
JSBool JS_FASTCALL IterMore(VMFrame &f);
void JS_FASTCALL EndIter(VMFrame &f);

JSBool JS_FASTCALL ValueToBoolean(VMFrame &f);
JSString * JS_FASTCALL TypeOf(VMFrame &f);

}}} 

extern "C" void *
js_InternalThrow(js::VMFrame &f);

#endif 

