






































#ifndef jsbuiltins_h___
#define jsbuiltins_h___

#ifdef JS_TRACER

#include "nanojit.h"

enum JSTNErrType { INFALLIBLE, FAIL_NULL, FAIL_NEG, FAIL_VOID, FAIL_JSVAL };
enum { JSTN_ERRTYPE_MASK = 7, JSTN_MORE = 8 };

#define JSTN_ERRTYPE(jstn)  ((jstn)->flags & JSTN_ERRTYPE_MASK)



























struct JSTraceableNative {
    JSFastNative            native;
    const nanojit::CallInfo *builtin;
    const char              *prefix;
    const char              *argtypes;
    uintN                   flags;  
};







#define JSVAL_ERROR_COOKIE OBJECT_TO_JSVAL((void*)0x10)






#define INT32_ERROR_COOKIE 0xffffabcd


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


#define _JS_TYPEINFO(ctype, size)  (ctype, size)
#define _JS_TYPEINFO_CONTEXT       _JS_TYPEINFO(JSContext *,             _JS_PTR)
#define _JS_TYPEINFO_RUNTIME       _JS_TYPEINFO(JSRuntime *,             _JS_PTR)
#define _JS_TYPEINFO_JSVAL         _JS_TYPEINFO(jsval,                   _JS_I32)
#define _JS_TYPEINFO_BOOL          _JS_TYPEINFO(JSBool,                  _JS_I32)
#define _JS_TYPEINFO_INT32         _JS_TYPEINFO(int32,                   _JS_I32)
#define _JS_TYPEINFO_UINT32        _JS_TYPEINFO(uint32,                  _JS_I32)
#define _JS_TYPEINFO_DOUBLE        _JS_TYPEINFO(jsdouble,                _JS_F64)
#define _JS_TYPEINFO_STRING        _JS_TYPEINFO(JSString *,              _JS_PTR)
#define _JS_TYPEINFO_OBJECT        _JS_TYPEINFO(JSObject *,              _JS_PTR)
#define _JS_TYPEINFO_SCOPEPROP     _JS_TYPEINFO(JSScopeProperty *,       _JS_PTR)
#define _JS_TYPEINFO_PC            _JS_TYPEINFO(jsbytecode *,            _JS_PTR)
#define _JS_TYPEINFO_SIDEEXIT      _JS_TYPEINFO(nanojit::SideExit *,     _JS_PTR)
#define _JS_TYPEINFO_INTERPSTATE   _JS_TYPEINFO(avmplus::InterpState *,  _JS_PTR)
#define _JS_TYPEINFO_FRAGMENT      _JS_TYPEINFO(nanojit::Fragment *,     _JS_PTR)

#define _JS_EXPAND(tokens)  tokens

#define _JS_CTYPE2(ctype, size)    ctype
#define _JS_CTYPE(tyname)          _JS_EXPAND(_JS_CTYPE2 _JS_TYPEINFO_##tyname)
#define _JS_RETSIZE2(ctype, size)  size##_ARGSIZE
#define _JS_RETSIZE(tyname)        _JS_EXPAND(_JS_RETSIZE2 _JS_TYPEINFO_##tyname)
#define _JS_ARGSIZE2(ctype, size)  size##_RETSIZE
#define _JS_ARGSIZE(tyname)        _JS_EXPAND(_JS_ARGSIZE2 _JS_TYPEINFO_##tyname)

#define _JS_static_TN(t)  static t
#define _JS_static_CI     static
#define _JS_extern_TN(t)  extern t
#define _JS_extern_CI
#define _JS_FRIEND_TN(t)  extern JS_FRIEND_API(t)
#define _JS_FRIEND_CI
#define _JS_EXPAND_TN_LINKAGE(linkage, t)  _JS_##linkage##_TN(t)
#define _JS_EXPAND_CI_LINKAGE(linkage)     _JS_##linkage##_CI

#define _JS_CALLINFO(name) name##_ci

#define _JS_DEFINE_CALLINFO(linkage, name, crtype, cargtypes, argtypes, cse, fold)                \
    _JS_EXPAND_TN_LINKAGE(linkage, crtype) FASTCALL name cargtypes;                               \
    _JS_EXPAND_CI_LINKAGE(linkage) const nanojit::CallInfo _JS_CALLINFO(name) =                   \
        { (intptr_t) &name, argtypes, cse, fold, nanojit::ABI_FASTCALL _JS_CI_NAME(name) };







#define JS_DEFINE_CALLINFO_1(linkage, rt, op, at0, cse, fold)                                     \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE(rt), (_JS_CTYPE(at0)),                             \
                        (_JS_ARGSIZE(at0) << 2) | _JS_RETSIZE(rt), cse, fold)
#define JS_DEFINE_CALLINFO_2(linkage, rt, op, at0, at1, cse, fold)                                \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE(rt), (_JS_CTYPE(at0), _JS_CTYPE(at1)),             \
                        (_JS_ARGSIZE(at0) << 4) | (_JS_ARGSIZE(at1) << 2) | _JS_RETSIZE(rt),      \
                        cse, fold)
#define JS_DEFINE_CALLINFO_3(linkage, rt, op, at0, at1, at2, cse, fold)                           \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE(rt),                                               \
                        (_JS_CTYPE(at0), _JS_CTYPE(at1), _JS_CTYPE(at2)),                         \
                        (_JS_ARGSIZE(at0) << 6) | (_JS_ARGSIZE(at1) << 4) |                       \
                        (_JS_ARGSIZE(at2) << 2) | _JS_RETSIZE(rt),                                \
                        cse, fold)
#define JS_DEFINE_CALLINFO_4(linkage, rt, op, at0, at1, at2, at3, cse, fold)                      \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE(rt),                                               \
                        (_JS_CTYPE(at0), _JS_CTYPE(at1), _JS_CTYPE(at2), _JS_CTYPE(at3)),         \
                        (_JS_ARGSIZE(at0) << 8) | (_JS_ARGSIZE(at1) << 6) |                       \
                        (_JS_ARGSIZE(at2) << 4) | (_JS_ARGSIZE(at3) << 2) | _JS_RETSIZE(rt),      \
                        cse, fold)
#define JS_DEFINE_CALLINFO_5(linkage, rt, op, at0, at1, at2, at3, at4, cse, fold)                 \
    _JS_DEFINE_CALLINFO(linkage, op, _JS_CTYPE(rt),                                               \
                        (_JS_CTYPE(at0), _JS_CTYPE(at1), _JS_CTYPE(at2), _JS_CTYPE(at3),          \
                         _JS_CTYPE(at4)),                                                         \
                        (_JS_ARGSIZE(at0) << 10) | (_JS_ARGSIZE(at1) << 8) |                      \
                        (_JS_ARGSIZE(at2) << 6) | (_JS_ARGSIZE(at3) << 4) |                       \
                        (_JS_ARGSIZE(at4) << 2) | _JS_RETSIZE(rt),                                \
                        cse, fold)

#define JS_DECLARE_CALLINFO(name)  extern const nanojit::CallInfo _JS_CALLINFO(name);

#else

#define JS_DEFINE_CALLINFO_1(linkage, rt, op, at0, cse, fold)
#define JS_DEFINE_CALLINFO_2(linkage, rt, op, at0, at1, cse, fold)
#define JS_DEFINE_CALLINFO_3(linkage, rt, op, at0, at1, at2, cse, fold)
#define JS_DEFINE_CALLINFO_4(linkage, rt, op, at0, at1, at2, at3, cse, fold)
#define JS_DEFINE_CALLINFO_5(linkage, rt, op, at0, at1, at2, at3, at4, cse, fold)
#define JS_DECLARE_CALLINFO(name)

#endif 


JS_DECLARE_CALLINFO(js_Array_dense_setelem)
JS_DECLARE_CALLINFO(js_FastNewArray)
JS_DECLARE_CALLINFO(js_Array_1int)
JS_DECLARE_CALLINFO(js_Array_1str)
JS_DECLARE_CALLINFO(js_Array_2obj)
JS_DECLARE_CALLINFO(js_Array_3num)


JS_DECLARE_CALLINFO(js_FastNewDate)


JS_DECLARE_CALLINFO(js_NumberToString)


JS_DECLARE_CALLINFO(js_ConcatStrings)
JS_DECLARE_CALLINFO(js_String_getelem)
JS_DECLARE_CALLINFO(js_String_p_charCodeAt)
JS_DECLARE_CALLINFO(js_EqualStrings)
JS_DECLARE_CALLINFO(js_CompareStrings)


#define BUILTIN1(linkage, rt, op, at0,                     cse, fold)  JS_DECLARE_CALLINFO(op)
#define BUILTIN2(linkage, rt, op, at0, at1,                cse, fold)  JS_DECLARE_CALLINFO(op)
#define BUILTIN3(linkage, rt, op, at0, at1, at2,           cse, fold)  JS_DECLARE_CALLINFO(op)
#define BUILTIN4(linkage, rt, op, at0, at1, at2, at3,      cse, fold)  JS_DECLARE_CALLINFO(op)
#define BUILTIN5(linkage, rt, op, at0, at1, at2, at3, at4, cse, fold)  JS_DECLARE_CALLINFO(op)
#include "builtins.tbl"
#undef BUILTIN
#undef BUILTIN1
#undef BUILTIN2
#undef BUILTIN3
#undef BUILTIN4
#undef BUILTIN5

#endif 
