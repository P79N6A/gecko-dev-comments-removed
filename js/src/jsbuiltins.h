






































#ifndef jsbuiltins_h___
#define jsbuiltins_h___

#ifdef JS_TRACER

#include "nanojit/nanojit.h"
#include "jstracer.h"

#ifdef THIS
#undef THIS
#endif

enum JSTNErrType { INFALLIBLE, FAIL_STATUS, FAIL_NULL, FAIL_NEG, FAIL_VOID, FAIL_COOKIE };
enum { JSTN_ERRTYPE_MASK = 0x07, JSTN_UNBOX_AFTER = 0x08, JSTN_MORE = 0x10,
       JSTN_CONSTRUCTOR = 0x20 };

#define JSTN_ERRTYPE(jstn)  ((jstn)->flags & JSTN_ERRTYPE_MASK)



























struct JSTraceableNative {
    JSFastNative            native;
    const nanojit::CallInfo *builtin;
    const char              *prefix;
    const char              *argtypes;
    uintN                   flags;  

};







#define JSVAL_ERROR_COOKIE OBJECT_TO_JSVAL((JSObject*)0x10)


#ifdef DEBUG
#define _JS_CI_NAME(op) ,#op
#else
#define _JS_CI_NAME(op)
#endif

#define  _JS_I32_ARGSIZE    nanojit::ARGSIZE_LO
#define  _JS_I32_RETSIZE    nanojit::ARGSIZE_LO
#define  _JS_F64_ARGSIZE    nanojit::ARGSIZE_F
#define  _JS_F64_RETSIZE    nanojit::ARGSIZE_F
#define  _JS_PTR_ARGSIZE    nanojit::ARGSIZE_LO
#if defined AVMPLUS_64BIT
# define _JS_PTR_RETSIZE    nanojit::ARGSIZE_Q
#else
# define _JS_PTR_RETSIZE    nanojit::ARGSIZE_LO
#endif

class ClosureVarInfo;
























































#define _JS_CTYPE(ctype, size, pch, ach, flags)     (ctype, size, pch, ach, flags)
#define _JS_JSVAL_CTYPE(size, pch, ach, flags)  (jsval, size, pch, ach, (flags | JSTN_UNBOX_AFTER))

#define _JS_CTYPE_CONTEXT           _JS_CTYPE(JSContext *,            _JS_PTR,"C", "", INFALLIBLE)
#define _JS_CTYPE_RUNTIME           _JS_CTYPE(JSRuntime *,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_THIS              _JS_CTYPE(JSObject *,             _JS_PTR,"T", "", INFALLIBLE)
#define _JS_CTYPE_THIS_DOUBLE       _JS_CTYPE(jsdouble,               _JS_F64,"D", "", INFALLIBLE)
#define _JS_CTYPE_THIS_STRING       _JS_CTYPE(JSString *,             _JS_PTR,"S", "", INFALLIBLE)
#define _JS_CTYPE_CALLEE            _JS_CTYPE(JSObject *,             _JS_PTR,"f","",  INFALLIBLE)
#define _JS_CTYPE_FUNCTION          _JS_CTYPE(JSFunction *,           _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_PC                _JS_CTYPE(jsbytecode *,           _JS_PTR,"P", "", INFALLIBLE)
#define _JS_CTYPE_JSVALPTR          _JS_CTYPE(jsval *,                _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_JSVAL             _JS_JSVAL_CTYPE(                  _JS_PTR, "","v", INFALLIBLE)
#define _JS_CTYPE_JSVAL_RETRY       _JS_JSVAL_CTYPE(                  _JS_PTR, --, --, FAIL_COOKIE)
#define _JS_CTYPE_JSVAL_FAIL        _JS_JSVAL_CTYPE(                  _JS_PTR, --, --, FAIL_STATUS)
#define _JS_CTYPE_JSID              _JS_CTYPE(jsid,                   _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_BOOL              _JS_CTYPE(JSBool,                 _JS_I32, "","i", INFALLIBLE)
#define _JS_CTYPE_BOOL_RETRY        _JS_CTYPE(JSBool,                 _JS_I32, --, --, FAIL_VOID)
#define _JS_CTYPE_BOOL_FAIL         _JS_CTYPE(JSBool,                 _JS_I32, --, --, FAIL_STATUS)
#define _JS_CTYPE_INT32             _JS_CTYPE(int32,                  _JS_I32, "","i", INFALLIBLE)
#define _JS_CTYPE_INT32_RETRY       _JS_CTYPE(int32,                  _JS_I32, --, --, FAIL_NEG)
#define _JS_CTYPE_INT32_FAIL        _JS_CTYPE(int32,                  _JS_I32, --, --, FAIL_STATUS)
#define _JS_CTYPE_UINT32            _JS_CTYPE(uint32,                 _JS_I32, "","i", INFALLIBLE)
#define _JS_CTYPE_UINT32_RETRY      _JS_CTYPE(uint32,                 _JS_I32, --, --, FAIL_NEG)
#define _JS_CTYPE_UINT32_FAIL       _JS_CTYPE(uint32,                 _JS_I32, --, --, FAIL_STATUS)
#define _JS_CTYPE_DOUBLE            _JS_CTYPE(jsdouble,               _JS_F64, "","d", INFALLIBLE)
#define _JS_CTYPE_DOUBLE_FAIL       _JS_CTYPE(jsdouble,               _JS_F64, --, --, FAIL_STATUS)
#define _JS_CTYPE_STRING            _JS_CTYPE(JSString *,             _JS_PTR, "","s", INFALLIBLE)
#define _JS_CTYPE_STRING_RETRY      _JS_CTYPE(JSString *,             _JS_PTR, --, --, FAIL_NULL)
#define _JS_CTYPE_STRING_FAIL       _JS_CTYPE(JSString *,             _JS_PTR, --, --, FAIL_STATUS)
#define _JS_CTYPE_STRINGPTR         _JS_CTYPE(JSString **,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_OBJECT            _JS_CTYPE(JSObject *,             _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_OBJECT_RETRY      _JS_CTYPE(JSObject *,             _JS_PTR, --, --, FAIL_NULL)
#define _JS_CTYPE_OBJECT_FAIL       _JS_CTYPE(JSObject *,             _JS_PTR, --, --, FAIL_STATUS)
#define _JS_CTYPE_CONSTRUCTOR_RETRY _JS_CTYPE(JSObject *,             _JS_PTR, --, --, FAIL_NULL | \
                                                                                  JSTN_CONSTRUCTOR)
#define _JS_CTYPE_REGEXP            _JS_CTYPE(JSObject *,             _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_SCOPEPROP         _JS_CTYPE(JSScopeProperty *,      _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_SIDEEXIT          _JS_CTYPE(SideExit *,             _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_INTERPSTATE       _JS_CTYPE(InterpState *,          _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_FRAGMENT          _JS_CTYPE(nanojit::Fragment *,    _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CLASS             _JS_CTYPE(JSClass *,              _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_DOUBLEPTR         _JS_CTYPE(double *,               _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CVIPTR            _JS_CTYPE(const ClosureVarInfo *, _JS_PTR, --, --, INFALLIBLE)

#define _JS_EXPAND(tokens)  tokens

#define _JS_CTYPE_TYPE2(t,s,p,a,f)      t
#define _JS_CTYPE_TYPE(tyname)          _JS_EXPAND(_JS_CTYPE_TYPE2    _JS_CTYPE_##tyname)
#define _JS_CTYPE_RETSIZE2(t,s,p,a,f)   s##_RETSIZE
#define _JS_CTYPE_RETSIZE(tyname)       _JS_EXPAND(_JS_CTYPE_RETSIZE2 _JS_CTYPE_##tyname)
#define _JS_CTYPE_ARGSIZE2(t,s,p,a,f)   s##_ARGSIZE
#define _JS_CTYPE_ARGSIZE(tyname)       _JS_EXPAND(_JS_CTYPE_ARGSIZE2 _JS_CTYPE_##tyname)
#define _JS_CTYPE_PCH2(t,s,p,a,f)       p
#define _JS_CTYPE_PCH(tyname)           _JS_EXPAND(_JS_CTYPE_PCH2     _JS_CTYPE_##tyname)
#define _JS_CTYPE_ACH2(t,s,p,a,f)       a
#define _JS_CTYPE_ACH(tyname)           _JS_EXPAND(_JS_CTYPE_ACH2     _JS_CTYPE_##tyname)
#define _JS_CTYPE_FLAGS2(t,s,p,a,f)     f
#define _JS_CTYPE_FLAGS(tyname)         _JS_EXPAND(_JS_CTYPE_FLAGS2   _JS_CTYPE_##tyname)

#define _JS_static_TN(t)  static t
#define _JS_static_CI     static
#define _JS_extern_TN(t)  extern t
#define _JS_extern_CI
#define _JS_FRIEND_TN(t)  extern JS_FRIEND_API(t)
#define _JS_FRIEND_CI
#define _JS_TN_LINKAGE(linkage, t)  _JS_##linkage##_TN(t)
#define _JS_CI_LINKAGE(linkage)     _JS_##linkage##_CI

#define _JS_CALLINFO(name) name##_ci

#if defined(JS_NO_FASTCALL) && defined(NANOJIT_IA32)
#define _JS_DEFINE_CALLINFO(linkage, name, crtype, cargtypes, argtypes, cse, fold)                \
    _JS_TN_LINKAGE(linkage, crtype) name cargtypes;                                               \
    _JS_CI_LINKAGE(linkage) const nanojit::CallInfo _JS_CALLINFO(name) =                          \
        { (intptr_t) &name, argtypes, cse, fold, nanojit::ABI_CDECL _JS_CI_NAME(name) };
#else
#define _JS_DEFINE_CALLINFO(linkage, name, crtype, cargtypes, argtypes, cse, fold)                \
    _JS_TN_LINKAGE(linkage, crtype) FASTCALL name cargtypes;                                      \
    _JS_CI_LINKAGE(linkage) const nanojit::CallInfo _JS_CALLINFO(name) =                          \
        { (intptr_t) &name, argtypes, cse, fold, nanojit::ABI_FASTCALL _JS_CI_NAME(name) };
#endif






























#define JS_DEFINE_CALLINFO_1(linkage, rt, op, at0, cse, fold)                                     \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE_TYPE(rt), (_JS_CTYPE_TYPE(at0)),                   \
                        (_JS_CTYPE_ARGSIZE(at0) << (1*nanojit::ARGSIZE_SHIFT)) |                  \
                        _JS_CTYPE_RETSIZE(rt), cse, fold)
#define JS_DEFINE_CALLINFO_2(linkage, rt, op, at0, at1, cse, fold)                                \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE_TYPE(rt),                                          \
                        (_JS_CTYPE_TYPE(at0), _JS_CTYPE_TYPE(at1)),                               \
                        (_JS_CTYPE_ARGSIZE(at0) << (2*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at1) << (1*nanojit::ARGSIZE_SHIFT)) |                  \
                        _JS_CTYPE_RETSIZE(rt),                                                    \
                        cse, fold)
#define JS_DEFINE_CALLINFO_3(linkage, rt, op, at0, at1, at2, cse, fold)                           \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE_TYPE(rt),                                          \
                        (_JS_CTYPE_TYPE(at0), _JS_CTYPE_TYPE(at1), _JS_CTYPE_TYPE(at2)),          \
                        (_JS_CTYPE_ARGSIZE(at0) << (3*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at1) << (2*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at2) << (1*nanojit::ARGSIZE_SHIFT)) |                  \
                        _JS_CTYPE_RETSIZE(rt),                                                    \
                        cse, fold)
#define JS_DEFINE_CALLINFO_4(linkage, rt, op, at0, at1, at2, at3, cse, fold)                      \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE_TYPE(rt),                                          \
                        (_JS_CTYPE_TYPE(at0), _JS_CTYPE_TYPE(at1), _JS_CTYPE_TYPE(at2),           \
                         _JS_CTYPE_TYPE(at3)),                                                    \
                        (_JS_CTYPE_ARGSIZE(at0) << (4*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at1) << (3*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at2) << (2*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at3) << (1*nanojit::ARGSIZE_SHIFT)) |                  \
                        _JS_CTYPE_RETSIZE(rt),                                                    \
                        cse, fold)
#define JS_DEFINE_CALLINFO_5(linkage, rt, op, at0, at1, at2, at3, at4, cse, fold)                 \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE_TYPE(rt),                                          \
                        (_JS_CTYPE_TYPE(at0), _JS_CTYPE_TYPE(at1), _JS_CTYPE_TYPE(at2),           \
                         _JS_CTYPE_TYPE(at3), _JS_CTYPE_TYPE(at4)),                               \
                        (_JS_CTYPE_ARGSIZE(at0) << (5*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at1) << (4*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at2) << (3*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at3) << (2*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at4) << (1*nanojit::ARGSIZE_SHIFT)) |                  \
                        _JS_CTYPE_RETSIZE(rt),                                                    \
                        cse, fold)

#define JS_DEFINE_CALLINFO_6(linkage, rt, op, at0, at1, at2, at3, at4, at5, cse, fold)            \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE_TYPE(rt),                                          \
                        (_JS_CTYPE_TYPE(at0), _JS_CTYPE_TYPE(at1), _JS_CTYPE_TYPE(at2),           \
                         _JS_CTYPE_TYPE(at3), _JS_CTYPE_TYPE(at4), _JS_CTYPE_TYPE(at5)),          \
                        (_JS_CTYPE_ARGSIZE(at0) << (6*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at1) << (5*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at2) << (4*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at3) << (3*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at4) << (2*nanojit::ARGSIZE_SHIFT)) |                  \
                        (_JS_CTYPE_ARGSIZE(at5) << (1*nanojit::ARGSIZE_SHIFT)) |                  \
                        _JS_CTYPE_RETSIZE(rt), cse, fold)

#define JS_DECLARE_CALLINFO(name)  extern const nanojit::CallInfo _JS_CALLINFO(name);

#define _JS_TN_INIT_HELPER_n(n, args)  _JS_TN_INIT_HELPER_##n args

#define _JS_TN_INIT_HELPER_1(linkage, rt, op, at0, cse, fold)                                     \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at0),                                                                           \
    _JS_CTYPE_ACH(at0),                                                                           \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_2(linkage, rt, op, at0, at1, cse, fold)                                \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                                        \
    _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                                        \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_3(linkage, rt, op, at0, at1, at2, cse, fold)                           \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at2) _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                     \
    _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                     \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_4(linkage, rt, op, at0, at1, at2, at3, cse, fold)                      \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2) _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                  \
    _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                  \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_5(linkage, rt, op, at0, at1, at2, at3, at4, cse, fold)                 \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at4) _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2) _JS_CTYPE_PCH(at1)                   \
        _JS_CTYPE_PCH(at0),                                                                       \
    _JS_CTYPE_ACH(at4) _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1)                   \
        _JS_CTYPE_ACH(at0),                                                                       \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_6(linkage, rt, op, at0, at1, at2, at3, at4, at5, cse, fold)            \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at5) _JS_CTYPE_PCH(at4) _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2)                   \
        _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                                    \
    _JS_CTYPE_ACH(at5) _JS_CTYPE_ACH(at4) _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2)                   \
        _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                                    \
    _JS_CTYPE_FLAGS(rt)

#define JS_DEFINE_TRCINFO_1(name, tn0)                                                            \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    JSTraceableNative name##_trcinfo[] = {                                                        \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn0 }                                          \
    };

#define JS_DEFINE_TRCINFO_2(name, tn0, tn1)                                                       \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    _JS_DEFINE_CALLINFO_n tn1                                                                     \
    JSTraceableNative name##_trcinfo[] = {                                                        \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn0 | JSTN_MORE },                             \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn1 }                                          \
    };

#define JS_DEFINE_TRCINFO_3(name, tn0, tn1, tn2)                                                  \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    _JS_DEFINE_CALLINFO_n tn1                                                                     \
    _JS_DEFINE_CALLINFO_n tn2                                                                     \
    JSTraceableNative name##_trcinfo[] = {                                                        \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn0 | JSTN_MORE },                             \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn1 | JSTN_MORE },                             \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn2 }                                          \
    };

#define JS_DEFINE_TRCINFO_4(name, tn0, tn1, tn2, tn3)                                             \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    _JS_DEFINE_CALLINFO_n tn1                                                                     \
    _JS_DEFINE_CALLINFO_n tn2                                                                     \
    _JS_DEFINE_CALLINFO_n tn3                                                                     \
    JSTraceableNative name##_trcinfo[] = {                                                        \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn0 | JSTN_MORE },                             \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn1 | JSTN_MORE },                             \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn2 | JSTN_MORE },                             \
        { (JSFastNative)name, _JS_TN_INIT_HELPER_n tn3 }                                          \
    };

#define _JS_DEFINE_CALLINFO_n(n, args)  JS_DEFINE_CALLINFO_##n args

jsdouble FASTCALL
js_StringToNumber(JSContext* cx, JSString* str);

jsdouble FASTCALL
js_BooleanOrUndefinedToNumber(JSContext* cx, int32 unboxed);


extern JS_FRIEND_API(void)
js_SetTraceableNativeFailed(JSContext *cx);

extern jsdouble FASTCALL
js_dmod(jsdouble a, jsdouble b);

#else

#define JS_DEFINE_CALLINFO_1(linkage, rt, op, at0, cse, fold)
#define JS_DEFINE_CALLINFO_2(linkage, rt, op, at0, at1, cse, fold)
#define JS_DEFINE_CALLINFO_3(linkage, rt, op, at0, at1, at2, cse, fold)
#define JS_DEFINE_CALLINFO_4(linkage, rt, op, at0, at1, at2, at3, cse, fold)
#define JS_DEFINE_CALLINFO_5(linkage, rt, op, at0, at1, at2, at3, at4, cse, fold)
#define JS_DECLARE_CALLINFO(name)
#define JS_DEFINE_TRCINFO_1(name, tn0)
#define JS_DEFINE_TRCINFO_2(name, tn0, tn1)
#define JS_DEFINE_TRCINFO_3(name, tn0, tn1, tn2)
#define JS_DEFINE_TRCINFO_4(name, tn0, tn1, tn2, tn3)

#endif 


JS_DECLARE_CALLINFO(js_NewNativeObject)


JS_DECLARE_CALLINFO(js_Array_dense_setelem)
JS_DECLARE_CALLINFO(js_Array_dense_setelem_int)
JS_DECLARE_CALLINFO(js_Array_dense_setelem_double)
JS_DECLARE_CALLINFO(js_NewEmptyArray)
JS_DECLARE_CALLINFO(js_NewUninitializedArray)
JS_DECLARE_CALLINFO(js_ArrayCompPush)


JS_DECLARE_CALLINFO(js_AllocFlatClosure)
JS_DECLARE_CALLINFO(js_PutArguments)


JS_DECLARE_CALLINFO(js_SetCallVar)
JS_DECLARE_CALLINFO(js_SetCallArg)


JS_DECLARE_CALLINFO(js_NumberToString)


JS_DECLARE_CALLINFO(js_String_tn)
JS_DECLARE_CALLINFO(js_CompareStrings)
JS_DECLARE_CALLINFO(js_ConcatStrings)
JS_DECLARE_CALLINFO(js_EqualStrings)
JS_DECLARE_CALLINFO(js_String_getelem)
JS_DECLARE_CALLINFO(js_String_p_charCodeAt)
JS_DECLARE_CALLINFO(js_String_p_charCodeAt0)
JS_DECLARE_CALLINFO(js_String_p_charCodeAt0_int)
JS_DECLARE_CALLINFO(js_String_p_charCodeAt_int)


JS_DECLARE_CALLINFO(js_BoxDouble)
JS_DECLARE_CALLINFO(js_BoxInt32)
JS_DECLARE_CALLINFO(js_UnboxDouble)
JS_DECLARE_CALLINFO(js_UnboxInt32)
JS_DECLARE_CALLINFO(js_dmod)
JS_DECLARE_CALLINFO(js_imod)
JS_DECLARE_CALLINFO(js_DoubleToInt32)
JS_DECLARE_CALLINFO(js_DoubleToUint32)

JS_DECLARE_CALLINFO(js_StringToNumber)
JS_DECLARE_CALLINFO(js_StringToInt32)
JS_DECLARE_CALLINFO(js_CloseIterator)
JS_DECLARE_CALLINFO(js_CallTree)
JS_DECLARE_CALLINFO(js_AddProperty)
JS_DECLARE_CALLINFO(js_HasNamedProperty)
JS_DECLARE_CALLINFO(js_HasNamedPropertyInt32)
JS_DECLARE_CALLINFO(js_TypeOfObject)
JS_DECLARE_CALLINFO(js_TypeOfBoolean)
JS_DECLARE_CALLINFO(js_BooleanOrUndefinedToNumber)
JS_DECLARE_CALLINFO(js_BooleanOrUndefinedToString)
JS_DECLARE_CALLINFO(js_Arguments)
JS_DECLARE_CALLINFO(js_NewNullClosure)
JS_DECLARE_CALLINFO(js_ConcatN)

#endif 
