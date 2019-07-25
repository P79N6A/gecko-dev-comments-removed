







































#ifndef jsinterp_h___
#define jsinterp_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsvalue.h"

#include "vm/Stack.h"

namespace js {

extern JSObject *
GetBlockChain(JSContext *cx, StackFrame *fp);

extern JSObject *
GetBlockChainFast(JSContext *cx, StackFrame *fp, JSOp op, size_t oplen);

extern JSObject *
GetScopeChain(JSContext *cx);








extern JSObject *
GetScopeChain(JSContext *cx, StackFrame *fp);

extern JSObject *
GetScopeChainFast(JSContext *cx, StackFrame *fp, JSOp op, size_t oplen);





void
ReportIncompatibleMethod(JSContext *cx, Value *vp, Class *clasp);









template <typename T>
bool GetPrimitiveThis(JSContext *cx, Value *vp, T *v);







inline bool
ScriptPrologue(JSContext *cx, StackFrame *fp, JSScript *script);

inline bool
ScriptEpilogue(JSContext *cx, StackFrame *fp, bool ok);








inline bool
ScriptPrologueOrGeneratorResume(JSContext *cx, StackFrame *fp);

inline bool
ScriptEpilogueOrGeneratorYield(JSContext *cx, StackFrame *fp, bool ok);



extern void
ScriptDebugPrologue(JSContext *cx, StackFrame *fp);

extern bool
ScriptDebugEpilogue(JSContext *cx, StackFrame *fp, bool ok);







extern bool
BoxNonStrictThis(JSContext *cx, const CallReceiver &call);







inline bool
ComputeThis(JSContext *cx, StackFrame *fp);





enum ConstructOption {
    INVOKE_NORMAL = 0,
    INVOKE_CONSTRUCTOR = StackFrame::CONSTRUCTING
};
JS_STATIC_ASSERT(INVOKE_CONSTRUCTOR != INVOKE_NORMAL);

static inline uintN
ToReportFlags(ConstructOption option)
{
    return (uintN)option;
}

static inline uint32
ToFrameFlags(ConstructOption option)
{
    return (uintN)option;
}









extern JS_REQUIRES_STACK bool
Invoke(JSContext *cx, const CallArgs &args, ConstructOption option = INVOKE_NORMAL);

























class InvokeSessionGuard;






extern bool
ExternalInvoke(JSContext *cx, const Value &thisv, const Value &fval,
               uintN argc, Value *argv, Value *rval);

extern bool
ExternalGetOrSet(JSContext *cx, JSObject *obj, jsid id, const Value &fval,
                 JSAccessMode mode, uintN argc, Value *argv, Value *rval);









extern JS_REQUIRES_STACK bool
InvokeConstructor(JSContext *cx, const CallArgs &args);

extern JS_REQUIRES_STACK bool
InvokeConstructorWithGivenThis(JSContext *cx, JSObject *thisobj, const Value &fval,
                               uintN argc, Value *argv, Value *rval);

extern bool
ExternalInvokeConstructor(JSContext *cx, const Value &fval, uintN argc, Value *argv,
                          Value *rval);





extern JS_FORCES_STACK bool
Execute(JSContext *cx, JSObject &chain, JSScript *script,
        StackFrame *prev, uintN flags, Value *result);


enum InterpMode
{
    JSINTERP_NORMAL    = 0, 
    JSINTERP_RECORD    = 1, 
    JSINTERP_SAFEPOINT = 2, 
    JSINTERP_PROFILE   = 3  
};





extern JS_REQUIRES_STACK JS_NEVER_INLINE bool
Interpret(JSContext *cx, StackFrame *stopFp, InterpMode mode = JSINTERP_NORMAL);

extern JS_REQUIRES_STACK bool
RunScript(JSContext *cx, JSScript *script, StackFrame *fp);

extern bool
CheckRedeclaration(JSContext *cx, JSObject *obj, jsid id, uintN attrs);

extern bool
StrictlyEqual(JSContext *cx, const Value &lval, const Value &rval, JSBool *equal);

extern bool
LooselyEqual(JSContext *cx, const Value &lval, const Value &rval, JSBool *equal);


extern bool
SameValue(JSContext *cx, const Value &v1, const Value &v2, JSBool *same);

extern JSType
TypeOfValue(JSContext *cx, const Value &v);

extern JSBool
HasInstance(JSContext *cx, JSObject *obj, const js::Value *v, JSBool *bp);

extern bool
ValueToId(JSContext *cx, const Value &v, jsid *idp);








extern const js::Value &
GetUpvar(JSContext *cx, uintN level, js::UpvarCookie cookie);

} 












#ifndef JS_LONE_INTERPRET
# ifdef _MSC_VER
#  define JS_LONE_INTERPRET 0
# else
#  define JS_LONE_INTERPRET 1
# endif
#endif

#if !JS_LONE_INTERPRET
# define JS_STATIC_INTERPRET    static
#else
# define JS_STATIC_INTERPRET

extern JS_REQUIRES_STACK JSBool
js_EnterWith(JSContext *cx, jsint stackIndex, JSOp op, size_t oplen);

extern JS_REQUIRES_STACK void
js_LeaveWith(JSContext *cx);







extern JSBool
js_DoIncDec(JSContext *cx, const JSCodeSpec *cs, js::Value *vp, js::Value *vp2);





extern JS_REQUIRES_STACK void
js_LogOpcode(JSContext *cx);




extern void
js_MeterOpcodePair(JSOp op1, JSOp op2);

extern void
js_MeterSlotOpcode(JSOp op, uint32 slot);

#endif 




extern JS_REQUIRES_STACK JSBool
js_UnwindScope(JSContext *cx, jsint stackDepth, JSBool normalUnwind);

extern JSBool
js_OnUnknownMethod(JSContext *cx, js::Value *vp);

extern JS_REQUIRES_STACK js::Class *
js_IsActiveWithOrBlock(JSContext *cx, JSObject *obj, int stackDepth);

#endif 
