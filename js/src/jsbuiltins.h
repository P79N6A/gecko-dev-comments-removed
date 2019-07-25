






































#ifndef jsbuiltins_h___
#define jsbuiltins_h___

#ifdef JS_TRACER

#include "nanojit/nanojit.h"
#include "jsvalue.h"

#ifdef THIS
#undef THIS
#endif

enum JSTNErrType { INFALLIBLE, FAIL_STATUS, FAIL_NULL, FAIL_NEG, FAIL_NEITHER };
enum { 
    JSTN_ERRTYPE_MASK        = 0x07,
    JSTN_UNBOX_AFTER         = 0x08,
    JSTN_MORE                = 0x10,
    JSTN_CONSTRUCTOR         = 0x20,
    JSTN_RETURN_NULLABLE_STR = 0x40,
    JSTN_RETURN_NULLABLE_OBJ = 0x80
};

#define JSTN_ERRTYPE(jstn)  ((jstn)->flags & JSTN_ERRTYPE_MASK)





























struct JSSpecializedNative {
    const nanojit::CallInfo *builtin;
    const char              *prefix;
    const char              *argtypes;
    uintN                   flags;  

};







struct JSNativeTraceInfo {
    js::Native              native;
    JSSpecializedNative     *specializations;
};


#ifdef DEBUG
#define _JS_CI_NAME(op) ,#op
#else
#define _JS_CI_NAME(op)
#endif

#define _JS_I32_ARGTYPE    nanojit::ARGTYPE_I
#define _JS_I32_RETTYPE    nanojit::ARGTYPE_I
#define _JS_U64_ARGTYPE    nanojit::ARGTYPE_Q
#define _JS_U64_RETTYPE    nanojit::ARGTYPE_Q
#define _JS_F64_ARGTYPE    nanojit::ARGTYPE_D
#define _JS_F64_RETTYPE    nanojit::ARGTYPE_D
#define _JS_PTR_ARGTYPE    nanojit::ARGTYPE_P
#define _JS_PTR_RETTYPE    nanojit::ARGTYPE_P

struct ClosureVarInfo;























































#define _JS_CTYPE(ctype, size, pch, ach, flags)     (ctype, size, pch, ach, flags)

#define _JS_CTYPE_CONTEXT           _JS_CTYPE(JSContext *,            _JS_PTR,"C", "", INFALLIBLE)
#define _JS_CTYPE_RUNTIME           _JS_CTYPE(JSRuntime *,            _JS_PTR,"R", "", INFALLIBLE)
#define _JS_CTYPE_MATHCACHE         _JS_CTYPE(js::MathCache *,        _JS_PTR,"M", "", INFALLIBLE)
#define _JS_CTYPE_THIS              _JS_CTYPE(JSObject *,             _JS_PTR,"T", "", INFALLIBLE)
#define _JS_CTYPE_THIS_DOUBLE       _JS_CTYPE(jsdouble,               _JS_F64,"D", "", INFALLIBLE)
#define _JS_CTYPE_THIS_STRING       _JS_CTYPE(JSString *,             _JS_PTR,"S", "", INFALLIBLE)
#define _JS_CTYPE_CALLEE            _JS_CTYPE(JSObject *,             _JS_PTR,"f", "", INFALLIBLE)
#define _JS_CTYPE_CALLEE_PROTOTYPE  _JS_CTYPE(JSObject *,             _JS_PTR,"p", "", INFALLIBLE)
#define _JS_CTYPE_FUNCTION          _JS_CTYPE(JSFunction *,           _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_PC                _JS_CTYPE(jsbytecode *,           _JS_PTR,"P", "", INFALLIBLE)
#define _JS_CTYPE_VALUEPTR          _JS_CTYPE(js::Value *,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CVALUEPTR         _JS_CTYPE(const js::Value *,      _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_JSID              _JS_CTYPE(jsid,                   _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_JSVAL             _JS_CTYPE(js::Value,              _JS_U64, --, --, INFALLIBLE)
#define _JS_CTYPE_BOOL              _JS_CTYPE(JSBool,                 _JS_I32, "","i", INFALLIBLE)
#define _JS_CTYPE_BOOL_RETRY        _JS_CTYPE(JSBool,                 _JS_I32, --, --, FAIL_NEITHER)
#define _JS_CTYPE_BOOL_FAIL         _JS_CTYPE(JSBool,                 _JS_I32, --, --, FAIL_STATUS)
#define _JS_CTYPE_BOOLPTR           _JS_CTYPE(JSBool *,               _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_INT32             _JS_CTYPE(int32,                  _JS_I32, "","i", INFALLIBLE)
#define _JS_CTYPE_INT32_RETRY       _JS_CTYPE(int32,                  _JS_I32, --, --, FAIL_NEG)
#define _JS_CTYPE_INT32_FAIL        _JS_CTYPE(int32,                  _JS_I32, --, --, FAIL_STATUS)
#define _JS_CTYPE_INT32PTR          _JS_CTYPE(int32 *,                _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_UINT32            _JS_CTYPE(uint32,                 _JS_I32, "","i", INFALLIBLE)
#define _JS_CTYPE_UINT32_RETRY      _JS_CTYPE(uint32,                 _JS_I32, --, --, FAIL_NEG)
#define _JS_CTYPE_UINT32_FAIL       _JS_CTYPE(uint32,                 _JS_I32, --, --, FAIL_STATUS)
#define _JS_CTYPE_DOUBLE            _JS_CTYPE(jsdouble,               _JS_F64, "","d", INFALLIBLE)
#define _JS_CTYPE_DOUBLE_FAIL       _JS_CTYPE(jsdouble,               _JS_F64, --, --, FAIL_STATUS)
#define _JS_CTYPE_STRING            _JS_CTYPE(JSString *,             _JS_PTR, "","s", INFALLIBLE)
#define _JS_CTYPE_STRING_RETRY      _JS_CTYPE(JSString *,             _JS_PTR, --, --, FAIL_NULL)
#define _JS_CTYPE_STRING_FAIL       _JS_CTYPE(JSString *,             _JS_PTR, --, --, FAIL_STATUS)
#define _JS_CTYPE_STRING_OR_NULL_FAIL _JS_CTYPE(JSString *,           _JS_PTR, --, --, FAIL_STATUS | \
                                                                              JSTN_RETURN_NULLABLE_STR)
#define _JS_CTYPE_STRINGPTR         _JS_CTYPE(JSString **,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_OBJECT            _JS_CTYPE(JSObject *,             _JS_PTR, "","o", INFALLIBLE)
#define _JS_CTYPE_OBJECT_RETRY      _JS_CTYPE(JSObject *,             _JS_PTR, --, --, FAIL_NULL)
#define _JS_CTYPE_OBJECT_FAIL       _JS_CTYPE(JSObject *,             _JS_PTR, --, --, FAIL_STATUS)
#define _JS_CTYPE_OBJECT_OR_NULL_FAIL _JS_CTYPE(JSObject *,           _JS_PTR, --, --, FAIL_STATUS | \
                                                                              JSTN_RETURN_NULLABLE_OBJ)
#define _JS_CTYPE_OBJECTPTR         _JS_CTYPE(JSObject **,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CONSTRUCTOR_RETRY _JS_CTYPE(JSObject *,             _JS_PTR, --, --, FAIL_NULL | \
                                                                                  JSTN_CONSTRUCTOR)
#define _JS_CTYPE_REGEXP            _JS_CTYPE(JSObject *,             _JS_PTR, "","r", INFALLIBLE)
#define _JS_CTYPE_SHAPE             _JS_CTYPE(js::Shape *,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_TRACERSTATE       _JS_CTYPE(TracerState *,          _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_FRAGMENT          _JS_CTYPE(nanojit::Fragment *,    _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CLASS             _JS_CTYPE(js::Class *,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_DOUBLEPTR         _JS_CTYPE(double *,               _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CHARPTR           _JS_CTYPE(char *,                 _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_CVIPTR            _JS_CTYPE(const ClosureVarInfo *, _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_FRAMEINFO         _JS_CTYPE(FrameInfo *,            _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_PICTABLE          _JS_CTYPE(PICTable *,             _JS_PTR, --, --, INFALLIBLE)
#define _JS_CTYPE_UINTN             _JS_CTYPE(uintN,                  _JS_PTR, --, --, INFALLIBLE)
 













#if JS_BITS_PER_WORD == 32
# define _JS_CTYPE_VALUE            _JS_CTYPE(js::ValueArgType,       _JS_PTR, "","v", INFALLIBLE)
#elif JS_BITS_PER_WORD == 64
# define _JS_CTYPE_VALUE            _JS_CTYPE(js::ValueArgType,       _JS_U64, "","v", INFALLIBLE)
#endif

#define _JS_EXPAND(tokens)  tokens

#define _JS_CTYPE_TYPE2(t,s,p,a,f)      t
#define _JS_CTYPE_TYPE(tyname)          _JS_EXPAND(_JS_CTYPE_TYPE2    _JS_CTYPE_##tyname)
#define _JS_CTYPE_RETTYPE2(t,s,p,a,f)   s##_RETTYPE
#define _JS_CTYPE_RETTYPE(tyname)       _JS_EXPAND(_JS_CTYPE_RETTYPE2 _JS_CTYPE_##tyname)
#define _JS_CTYPE_ARGTYPE2(t,s,p,a,f)   s##_ARGTYPE
#define _JS_CTYPE_ARGTYPE(tyname)       _JS_EXPAND(_JS_CTYPE_ARGTYPE2 _JS_CTYPE_##tyname)
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
#define _JS_DEFINE_CALLINFO(linkage, name, crtype, cargtypes, argtypes, isPure, storeAccSet)      \
    _JS_TN_LINKAGE(linkage, crtype) name cargtypes;                                               \
    _JS_CI_LINKAGE(linkage) const nanojit::CallInfo _JS_CALLINFO(name) =                          \
        { (intptr_t) &name, argtypes, nanojit::ABI_CDECL, isPure, storeAccSet _JS_CI_NAME(name) };\
    JS_STATIC_ASSERT_IF(isPure, (storeAccSet) == nanojit::ACCSET_NONE);

#else
#define _JS_DEFINE_CALLINFO(linkage, name, crtype, cargtypes, argtypes, isPure, storeAccSet)      \
    _JS_TN_LINKAGE(linkage, crtype) FASTCALL name cargtypes;                                      \
    _JS_CI_LINKAGE(linkage) const nanojit::CallInfo _JS_CALLINFO(name) =                          \
        { (intptr_t) &name, argtypes, nanojit::ABI_FASTCALL, isPure, storeAccSet _JS_CI_NAME(name) }; \
    JS_STATIC_ASSERT_IF(isPure, (storeAccSet) == nanojit::ACCSET_NONE);
#endif




































#define JS_DEFINE_CALLINFO_1(linkage, rt, op, at0, isPure, storeAccSet)                           \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0)),                                                    \
                        nanojit::CallInfo::typeSig1(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_2(linkage, rt, op, at0, at1, isPure, storeAccSet)                      \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1)),                                                    \
                        nanojit::CallInfo::typeSig2(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_3(linkage, rt, op, at0, at1, at2, isPure, storeAccSet)                 \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1),                                                     \
                         _JS_CTYPE_TYPE(at2)),                                                    \
                        nanojit::CallInfo::typeSig3(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1),                       \
                                                    _JS_CTYPE_ARGTYPE(at2)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_4(linkage, rt, op, at0, at1, at2, at3, isPure, storeAccSet)            \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1),                                                     \
                         _JS_CTYPE_TYPE(at2),                                                     \
                         _JS_CTYPE_TYPE(at3)),                                                    \
                        nanojit::CallInfo::typeSig4(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1),                       \
                                                    _JS_CTYPE_ARGTYPE(at2),                       \
                                                    _JS_CTYPE_ARGTYPE(at3)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_5(linkage, rt, op, at0, at1, at2, at3, at4, isPure, storeAccSet)       \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1),                                                     \
                         _JS_CTYPE_TYPE(at2),                                                     \
                         _JS_CTYPE_TYPE(at3),                                                     \
                         _JS_CTYPE_TYPE(at4)),                                                    \
                        nanojit::CallInfo::typeSig5(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1),                       \
                                                    _JS_CTYPE_ARGTYPE(at2),                       \
                                                    _JS_CTYPE_ARGTYPE(at3),                       \
                                                    _JS_CTYPE_ARGTYPE(at4)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_6(linkage, rt, op, at0, at1, at2, at3, at4, at5, isPure, storeAccSet)  \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1),                                                     \
                         _JS_CTYPE_TYPE(at2),                                                     \
                         _JS_CTYPE_TYPE(at3),                                                     \
                         _JS_CTYPE_TYPE(at4),                                                     \
                         _JS_CTYPE_TYPE(at5)),                                                    \
                        nanojit::CallInfo::typeSig6(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1),                       \
                                                    _JS_CTYPE_ARGTYPE(at2),                       \
                                                    _JS_CTYPE_ARGTYPE(at3),                       \
                                                    _JS_CTYPE_ARGTYPE(at4),                       \
                                                    _JS_CTYPE_ARGTYPE(at5)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_7(linkage, rt, op, at0, at1, at2, at3, at4, at5, at6, isPure,          \
                             storeAccSet)                                                         \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1),                                                     \
                         _JS_CTYPE_TYPE(at2),                                                     \
                         _JS_CTYPE_TYPE(at3),                                                     \
                         _JS_CTYPE_TYPE(at4),                                                     \
                         _JS_CTYPE_TYPE(at5),                                                     \
                         _JS_CTYPE_TYPE(at6)),                                                    \
                        nanojit::CallInfo::typeSig7(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1),                       \
                                                    _JS_CTYPE_ARGTYPE(at2),                       \
                                                    _JS_CTYPE_ARGTYPE(at3),                       \
                                                    _JS_CTYPE_ARGTYPE(at4),                       \
                                                    _JS_CTYPE_ARGTYPE(at5),                       \
                                                    _JS_CTYPE_ARGTYPE(at6)),                      \
                        isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_8(linkage, rt, op, at0, at1, at2, at3, at4, at5, at6, at7, isPure,     \
                             storeAccSet)                                                         \
    _JS_DEFINE_CALLINFO(linkage, op,                                                              \
                        _JS_CTYPE_TYPE(rt),                                                       \
                        (_JS_CTYPE_TYPE(at0),                                                     \
                         _JS_CTYPE_TYPE(at1),                                                     \
                         _JS_CTYPE_TYPE(at2),                                                     \
                         _JS_CTYPE_TYPE(at3),                                                     \
                         _JS_CTYPE_TYPE(at4),                                                     \
                         _JS_CTYPE_TYPE(at5),                                                     \
                         _JS_CTYPE_TYPE(at6),                                                     \
                         _JS_CTYPE_TYPE(at7)),                                                    \
                        nanojit::CallInfo::typeSig8(_JS_CTYPE_RETTYPE(rt),                        \
                                                    _JS_CTYPE_ARGTYPE(at0),                       \
                                                    _JS_CTYPE_ARGTYPE(at1),                       \
                                                    _JS_CTYPE_ARGTYPE(at2),                       \
                                                    _JS_CTYPE_ARGTYPE(at3),                       \
                                                    _JS_CTYPE_ARGTYPE(at4),                       \
                                                    _JS_CTYPE_ARGTYPE(at5),                       \
                                                    _JS_CTYPE_ARGTYPE(at6),                       \
                                                    _JS_CTYPE_ARGTYPE(at7)),                      \
                        isPure, storeAccSet)

#define JS_DECLARE_CALLINFO(name)  extern const nanojit::CallInfo _JS_CALLINFO(name);

#define _JS_TN_INIT_HELPER_n(n, args)  _JS_TN_INIT_HELPER_##n args

#define _JS_TN_INIT_HELPER_1(linkage, rt, op, at0, isPure, storeAccSet)                           \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at0),                                                                           \
    _JS_CTYPE_ACH(at0),                                                                           \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_2(linkage, rt, op, at0, at1, isPure, storeAccSet)                      \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                                        \
    _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                                        \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_3(linkage, rt, op, at0, at1, at2, isPure, storeAccSet)                 \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at2) _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                     \
    _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                     \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_4(linkage, rt, op, at0, at1, at2, at3, isPure, storeAccSet)            \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2) _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                  \
    _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                  \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_5(linkage, rt, op, at0, at1, at2, at3, at4, isPure, storeAccSet)       \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at4) _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2) _JS_CTYPE_PCH(at1)                   \
        _JS_CTYPE_PCH(at0),                                                                       \
    _JS_CTYPE_ACH(at4) _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1)                   \
        _JS_CTYPE_ACH(at0),                                                                       \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_6(linkage, rt, op, at0, at1, at2, at3, at4, at5, isPure, storeAccSet) \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at5) _JS_CTYPE_PCH(at4) _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2)                   \
        _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                                    \
    _JS_CTYPE_ACH(at5) _JS_CTYPE_ACH(at4) _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2)                   \
        _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                                    \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_7(linkage, rt, op, at0, at1, at2, at3, at4, at5, at6, isPure, storeAccSet) \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at6) _JS_CTYPE_PCH(at5) _JS_CTYPE_PCH(at4) _JS_CTYPE_PCH(at3)                   \
        _JS_CTYPE_PCH(at2)  _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),                                \
    _JS_CTYPE_ACH(at6) _JS_CTYPE_ACH(at5) _JS_CTYPE_ACH(at4) _JS_CTYPE_ACH(at3)                   \
        _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),                                 \
    _JS_CTYPE_FLAGS(rt)

#define _JS_TN_INIT_HELPER_8(linkage, rt, op, at0, at1, at2, at3, at4, at5, at6, at7, isPure, storeAccSet) \
    &_JS_CALLINFO(op),                                                                            \
    _JS_CTYPE_PCH(at7) _JS_CTYPE_PCH(at6) _JS_CTYPE_PCH(at5) _JS_CTYPE_PCH(at4)                   \
        _JS_CTYPE_PCH(at3) _JS_CTYPE_PCH(at2)  _JS_CTYPE_PCH(at1) _JS_CTYPE_PCH(at0),             \
    _JS_CTYPE_ACH(at7) _JS_CTYPE_ACH(at6) _JS_CTYPE_ACH(at5) _JS_CTYPE_ACH(at4)                   \
        _JS_CTYPE_ACH(at3) _JS_CTYPE_ACH(at2) _JS_CTYPE_ACH(at1) _JS_CTYPE_ACH(at0),              \
    _JS_CTYPE_FLAGS(rt)

#define JS_DEFINE_TRCINFO_1(name, tn0)                                                            \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    JSSpecializedNative name##_sns[] = {                                                          \
        { _JS_TN_INIT_HELPER_n tn0 }                                                              \
    };                                                                                            \
    JSNativeTraceInfo name##_trcinfo = { JS_VALUEIFY_NATIVE(name), name##_sns };

#define JS_DEFINE_TRCINFO_2(name, tn0, tn1)                                                       \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    _JS_DEFINE_CALLINFO_n tn1                                                                     \
    JSSpecializedNative name##_sns[] = {                                                          \
        { _JS_TN_INIT_HELPER_n tn0 | JSTN_MORE },                                                 \
        { _JS_TN_INIT_HELPER_n tn1 }                                                              \
    };                                                                                            \
    JSNativeTraceInfo name##_trcinfo = { JS_VALUEIFY_NATIVE(name), name##_sns };

#define JS_DEFINE_TRCINFO_3(name, tn0, tn1, tn2)                                                  \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    _JS_DEFINE_CALLINFO_n tn1                                                                     \
    _JS_DEFINE_CALLINFO_n tn2                                                                     \
    JSSpecializedNative name##_sns[] = {                                                          \
        { _JS_TN_INIT_HELPER_n tn0 | JSTN_MORE },                                                 \
        { _JS_TN_INIT_HELPER_n tn1 | JSTN_MORE },                                                 \
        { _JS_TN_INIT_HELPER_n tn2 }                                                              \
    };                                                                                            \
    JSNativeTraceInfo name##_trcinfo = { JS_VALUEIFY_NATIVE(name), name##_sns };

#define JS_DEFINE_TRCINFO_4(name, tn0, tn1, tn2, tn3)                                             \
    _JS_DEFINE_CALLINFO_n tn0                                                                     \
    _JS_DEFINE_CALLINFO_n tn1                                                                     \
    _JS_DEFINE_CALLINFO_n tn2                                                                     \
    _JS_DEFINE_CALLINFO_n tn3                                                                     \
    JSSpecializedNative name##_sns[] = {                                                          \
        { _JS_TN_INIT_HELPER_n tn0 | JSTN_MORE },                                                 \
        { _JS_TN_INIT_HELPER_n tn1 | JSTN_MORE },                                                 \
        { _JS_TN_INIT_HELPER_n tn2 | JSTN_MORE },                                                 \
        { _JS_TN_INIT_HELPER_n tn3 }                                                              \
    };                                                                                            \
    JSNativeTraceInfo name##_trcinfo = { JS_VALUEIFY_NATIVE(name), name##_sns };

#define _JS_DEFINE_CALLINFO_n(n, args)  JS_DEFINE_CALLINFO_##n args

jsdouble FASTCALL
js_StringToNumber(JSContext* cx, JSString* str, JSBool *ok);


extern JS_FRIEND_API(void)
js_SetTraceableNativeFailed(JSContext *cx);

extern jsdouble FASTCALL
js_dmod(jsdouble a, jsdouble b);

#else

#define JS_DEFINE_CALLINFO_1(linkage, rt, op, at0, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_2(linkage, rt, op, at0, at1, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_3(linkage, rt, op, at0, at1, at2, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_4(linkage, rt, op, at0, at1, at2, at3, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_5(linkage, rt, op, at0, at1, at2, at3, at4, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_6(linkage, rt, op, at0, at1, at2, at3, at4, at5, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_7(linkage, rt, op, at0, at1, at2, at3, at4, at5, at6, isPure, storeAccSet)
#define JS_DEFINE_CALLINFO_8(linkage, rt, op, at0, at1, at2, at3, at4, at5, at6, at7, isPure, storeAccSet)
#define JS_DECLARE_CALLINFO(name)
#define JS_DEFINE_TRCINFO_1(name, tn0)
#define JS_DEFINE_TRCINFO_2(name, tn0, tn1)
#define JS_DEFINE_TRCINFO_3(name, tn0, tn1, tn2)
#define JS_DEFINE_TRCINFO_4(name, tn0, tn1, tn2, tn3)

#endif 


namespace js {
JS_DECLARE_CALLINFO(NewDenseEmptyArray)
JS_DECLARE_CALLINFO(NewDenseAllocatedArray)
JS_DECLARE_CALLINFO(NewDenseUnallocatedArray)
}
JS_DECLARE_CALLINFO(js_ArrayCompPush_tn)
JS_DECLARE_CALLINFO(js_EnsureDenseArrayCapacity)


JS_DECLARE_CALLINFO(js_UnboxNumberAsDouble)
JS_DECLARE_CALLINFO(js_UnboxNumberAsInt32)
JS_DECLARE_CALLINFO(js_dmod)
JS_DECLARE_CALLINFO(js_imod)
JS_DECLARE_CALLINFO(js_DoubleToInt32)
JS_DECLARE_CALLINFO(js_DoubleToUint32)
JS_DECLARE_CALLINFO(js_StringToNumber)
JS_DECLARE_CALLINFO(js_StringToInt32)
JS_DECLARE_CALLINFO(js_AddProperty)
JS_DECLARE_CALLINFO(js_AddAtomProperty)
JS_DECLARE_CALLINFO(js_HasNamedProperty)
JS_DECLARE_CALLINFO(js_HasNamedPropertyInt32)
JS_DECLARE_CALLINFO(js_TypeOfObject)
JS_DECLARE_CALLINFO(js_BooleanIntToString)
JS_DECLARE_CALLINFO(js_NewNullClosure)


JS_DECLARE_CALLINFO(js_AllocFlatClosure)
JS_DECLARE_CALLINFO(js_PutArgumentsOnTrace)
JS_DECLARE_CALLINFO(js_PutCallObjectOnTrace)
JS_DECLARE_CALLINFO(js_SetCallVar)
JS_DECLARE_CALLINFO(js_SetCallArg)
JS_DECLARE_CALLINFO(js_CloneFunctionObject)
JS_DECLARE_CALLINFO(js_CreateCallObjectOnTrace)
JS_DECLARE_CALLINFO(js_NewArgumentsOnTrace)


JS_DECLARE_CALLINFO(js_NumberToString)


JS_DECLARE_CALLINFO(js_Object_tn)
JS_DECLARE_CALLINFO(js_CreateThisFromTrace)
JS_DECLARE_CALLINFO(js_InitializerObject)


JS_DECLARE_CALLINFO(js_CloneRegExpObject)


JS_DECLARE_CALLINFO(js_String_tn)
JS_DECLARE_CALLINFO(js_CompareStringsOnTrace)
JS_DECLARE_CALLINFO(js_ConcatStrings)
JS_DECLARE_CALLINFO(js_EqualStringsOnTrace)
JS_DECLARE_CALLINFO(js_FlattenOnTrace)


JS_DECLARE_CALLINFO(js_TypedArray_uint8_clamp_double)

#endif 
