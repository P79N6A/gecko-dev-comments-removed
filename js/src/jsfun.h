






































#ifndef jsfun_h___
#define jsfun_h___



#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

struct JSFunction {
    JSObject     *object;       
    uint16       nargs;         
    uint16       flags;         
    union {
        struct {
            uint16   extra;     
            uint16   spare;     
            JSNative native;    
        } n;
        struct {
            uint16   nvars;     
            uint16   nregexps;  
            JSScript *script;   
        } i;
    } u;
    JSAtom       *atom;         
    JSClass      *clasp;        
};

#define JSFUN_EXPR_CLOSURE   0x4000 /* expression closure: function(x)x*x */
#define JSFUN_INTERPRETED    0x8000 /* use u.i if set, u.n if unset */

#define FUN_INTERPRETED(fun) ((fun)->flags & JSFUN_INTERPRETED)
#define FUN_NATIVE(fun)      (FUN_INTERPRETED(fun) ? NULL : (fun)->u.n.native)
#define FUN_SCRIPT(fun)      (FUN_INTERPRETED(fun) ? (fun)->u.i.script : NULL)

extern JSClass js_ArgumentsClass;
extern JSClass js_CallClass;


extern JS_FRIEND_DATA(JSClass) js_FunctionClass;




#define VALUE_IS_FUNCTION(cx, v)                                              \
    (!JSVAL_IS_PRIMITIVE(v) &&                                                \
     OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_FunctionClass)

extern JSBool
js_fun_toString(JSContext *cx, JSObject *obj, uint32 indent,
                uintN argc, jsval *argv, jsval *rval);

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitArgumentsClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitCallClass(JSContext *cx, JSObject *obj);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, JSNative native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom);

extern JSObject *
js_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent);

extern JSBool
js_LinkFunctionObject(JSContext *cx, JSFunction *fun, JSObject *object);

extern JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, JSNative native,
                  uintN nargs, uintN flags);






#define JSV2F_CONSTRUCT         JSINVOKE_CONSTRUCT
#define JSV2F_ITERATOR          JSINVOKE_ITERATOR
#define JSV2F_SEARCH_STACK      0x10000

extern JSFunction *
js_ValueToFunction(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_ValueToFunctionObject(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_ValueToCallableObject(JSContext *cx, jsval *vp, uintN flags);

extern void
js_ReportIsNotFunction(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp, JSObject *parent);

extern JSBool
js_PutCallObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_GetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_SetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, jsval *vp);

extern JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, jsval *vp);

extern JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_PutArgsObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_XDRFunction(JSXDRState *xdr, JSObject **objp);

JS_END_EXTERN_C

#endif 
