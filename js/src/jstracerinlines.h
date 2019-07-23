





































#ifndef jstracerinlines_h___
#define jstracerinlines_h___

#define PRIMITIVE(x) interp_##x

#include "jsinterpinlines.h"	

#undef PRIMITIVE

#define RECORD_P(s, a)                                                        \
    JS_BEGIN_MACRO                                                            \
        js_CallRecorder(cx, s, native_pointer_to_jsval(a));                   \
    JS_END_MACRO

#define RECORD_PP(s, a, b)                                                    \
    JS_BEGIN_MACRO                                                            \
        js_CallRecorder(cx, s, native_pointer_to_jsval(a),                    \
                               native_pointer_to_jsval(b));                   \
    JS_END_MACRO

#define RECORD_PV(s, a, b)                                                    \
    JS_BEGIN_MACRO                                                            \
        js_CallRecorder(cx, s, native_pointer_to_jsval(a),                    \
                               b);                                            \
    JS_END_MACRO

static inline void
prim_push_stack(JSContext* cx, JSFrameRegs& regs, jsval& v)
{
    RECORD_PP("track", &v, regs.sp);
    interp_prim_push_stack(cx, regs, v);
    RECORD_P("setSP", regs.sp);
}

static inline void
prim_pop_stack(JSContext* cx, JSFrameRegs& regs, jsval& v)
{
    interp_prim_pop_stack(cx, regs, v);
    RECORD_PP("track", regs.sp, &v);
    RECORD_P("setSP", regs.sp);
}

static inline void
prim_store_stack(JSContext* cx, JSFrameRegs& regs, int n, jsval& v)
{
    RECORD_PP("track", &v, &regs.sp[n]);
    interp_prim_store_stack(cx, regs, n, v);
}

static inline void
prim_fetch_stack(JSContext* cx, JSFrameRegs& regs, int n, jsval& v)
{
    RECORD_PP("track", &regs.sp[n], &v);
    interp_prim_fetch_stack(cx, regs, n, v);
}

static inline void
prim_adjust_stack(JSContext* cx, JSFrameRegs& regs, int n)
{
    interp_prim_adjust_stack(cx, regs, n);
    RECORD_P("setSP", regs.sp);
}

static inline void
prim_generate_constant(JSContext* cx, jsval c, jsval& v)
{
    interp_prim_generate_constant(cx, c, v);
    RECORD_PV("constant", &v, c);
}

static inline void
prim_boolean_to_jsval(JSContext* cx, JSBool& b, jsval& v)
{
    interp_prim_boolean_to_jsval(cx, b, v);
}

static inline void
prim_string_to_jsval(JSContext* cx, JSString*& str, jsval& v)
{
    interp_prim_string_to_jsval(cx, str, v);
}

static inline void
prim_object_to_jsval(JSContext* cx, JSObject*& obj, jsval& v)
{
    interp_prim_object_to_jsval(cx, obj, v);
}

static inline void
prim_id_to_jsval(JSContext* cx, jsid& id, jsval& v)
{
    interp_prim_id_to_jsval(cx, id, v);
}

static inline bool
guard_jsdouble_is_int_and_int_fits_in_jsval(JSContext* cx, jsdouble& d, jsint& i)
{
    return interp_guard_jsdouble_is_int_and_int_fits_in_jsval(cx, d, i);
}

static inline void
prim_int_to_jsval(JSContext* cx, jsint& i, jsval& v)
{
    interp_prim_int_to_jsval(cx, i, v);
}

static inline bool
call_NewDoubleInRootedValue(JSContext* cx, jsdouble& d, jsval* vp)
{
    return interp_call_NewDoubleInRootedValue(cx, d, vp);
}

static inline bool
guard_int_fits_in_jsval(JSContext* cx, jsint& i)
{
    return interp_guard_int_fits_in_jsval(cx, i);
}

static inline void
prim_int_to_double(JSContext* cx, jsint& i, jsdouble& d)
{
    interp_prim_int_to_double(cx, i, d);
}

static inline bool
guard_uint_fits_in_jsval(JSContext* cx, uint32& u)
{
    return interp_guard_uint_fits_in_jsval(cx, u);
}

static inline void
prim_uint_to_jsval(JSContext* cx, uint32& u, jsval& v)
{
    interp_prim_uint_to_jsval(cx, u, v);
}

static inline void
prim_uint_to_double(JSContext* cx, uint32& u, jsdouble& d)
{
    interp_prim_uint_to_double(cx, u, d);
}

static inline bool
guard_jsval_is_int(JSContext* cx, jsval& v)
{
    return interp_guard_jsval_is_int(cx, v);
}

static inline void
prim_jsval_to_int(JSContext* cx, jsval& v, jsint& i)
{
    interp_prim_jsval_to_int(cx, v, i);
}

static inline bool
guard_jsval_is_double(JSContext* cx, jsval& v)
{
    return interp_guard_jsval_is_double(cx, v);
}

static inline void
prim_jsval_to_double(JSContext* cx, jsval& v, jsdouble& d)
{
    interp_prim_jsval_to_double(cx, v, d);
}

static inline void
call_ValueToNumber(JSContext* cx, jsval* vp, jsdouble& d)
{
    interp_call_ValueToNumber(cx, vp, d);
}

static inline bool
guard_jsval_is_null(JSContext* cx, jsval& v)
{
    return interp_guard_jsval_is_null(cx, v);
}

static inline void
call_ValueToECMAInt32(JSContext* cx, jsval* vp, jsint& i)
{
    interp_call_ValueToECMAInt32(cx, vp, i);
}

static inline void
prim_int_to_uint(JSContext* cx, jsint& i, uint32& u)
{
    interp_prim_int_to_uint(cx, i, u);
}

static inline void
call_ValueToECMAUint32(JSContext* cx, jsval* vp, uint32& u)
{
    interp_call_ValueToECMAUint32(cx, vp, u);
}

static inline void
prim_generate_boolean_constant(JSContext* cx, JSBool c, JSBool& b)
{
    interp_prim_generate_boolean_constant(cx, c, b);
}

static inline bool
guard_jsval_is_boolean(JSContext* cx, jsval& v)
{
    return interp_guard_jsval_is_boolean(cx, v);
}

static inline void
prim_jsval_to_boolean(JSContext* cx, jsval& v, JSBool& b)
{
    interp_prim_jsval_to_boolean(cx, v, b);
}

static inline void
call_ValueToBoolean(JSContext* cx, jsval& v, JSBool& b)
{
    interp_call_ValueToBoolean(cx, v, b);
}

static inline bool
guard_jsval_is_primitive(JSContext* cx, jsval& v)
{
    return interp_guard_jsval_is_primitive(cx, v);
}

static inline void
prim_jsval_to_object(JSContext* cx, jsval& v, JSObject*& obj)
{
    interp_prim_jsval_to_object(cx, v, obj);
}

static inline bool
guard_obj_is_null(JSContext* cx, JSObject*& obj)
{
    return interp_guard_obj_is_null(cx, obj);
}

static inline void
call_ValueToNonNullObject(JSContext* cx, jsval& v, JSObject*& obj)
{
    interp_call_ValueToNonNullObject(cx, v, obj);
}

static inline bool
call_obj_default_value(JSContext* cx, JSObject*& obj, JSType hint,
                                  jsval* vp)
{
    return interp_call_obj_default_value(cx, obj, hint, vp);
}

static inline void
prim_dadd(JSContext* cx, jsdouble& a, jsdouble& b, jsdouble& r)
{
    interp_prim_dadd(cx, a, b, r);
}

static inline void
prim_dsub(JSContext* cx, jsdouble& a, jsdouble& b, jsdouble& r)
{
    interp_prim_dsub(cx, a, b, r);
}

static inline void
prim_dmul(JSContext* cx, jsdouble& a, jsdouble& b, jsdouble& r)
{
    interp_prim_dmul(cx, a, b, r);
}

static inline bool
prim_ddiv(JSContext* cx, JSRuntime* rt, JSFrameRegs& regs, int n,
                     jsdouble& a, jsdouble& b)
{
    return interp_prim_ddiv(cx, rt, regs, n, a, b);
}

static inline bool
prim_dmod(JSContext* cx, JSRuntime* rt, JSFrameRegs& regs, int n,
                     jsdouble& a, jsdouble& b)
{
    return interp_prim_dmod(cx, rt, regs, n, a, b);
}

static inline void
prim_ior(JSContext* cx, jsint& a, jsint& b, jsint& r)
{
    interp_prim_ior(cx, a, b, r);
}

static inline void
prim_ixor(JSContext* cx, jsint& a, jsint& b, jsint& r)
{
    interp_prim_ixor(cx, a, b, r);
}

static inline void
prim_iand(JSContext* cx, jsint& a, jsint& b, jsint& r)
{
    interp_prim_iand(cx, a, b, r);
}

static inline void
prim_ilsh(JSContext* cx, jsint& a, jsint& b, jsint& r)
{
    interp_prim_ilsh(cx, a, b, r);
}

static inline void
prim_irsh(JSContext* cx, jsint& a, jsint& b, jsint& r)
{
    interp_prim_irsh(cx, a, b, r);
}

static inline void
prim_ursh(JSContext* cx, uint32& a, jsint& b, uint32& r)
{
    interp_prim_ursh(cx, a, b, r);
}

static inline bool
guard_boolean_is_true(JSContext* cx, JSBool& cond)
{
    return interp_guard_boolean_is_true(cx, cond);
}

static inline void
prim_icmp_lt(JSContext* cx, jsint& a, jsint& b, JSBool& r)
{
    interp_prim_icmp_lt(cx, a, b, r);
}

static inline void
prim_icmp_le(JSContext* cx, jsint& a, jsint& b, JSBool& r)
{
    interp_prim_icmp_le(cx, a, b, r);
}

static inline void
prim_icmp_gt(JSContext* cx, jsint& a, jsint& b, JSBool& r)
{
    interp_prim_icmp_gt(cx, a, b, r);
}

static inline void
prim_icmp_ge(JSContext* cx, jsint& a, jsint& b, JSBool& r)
{
    interp_prim_icmp_ge(cx, a, b, r);
}

static inline void
prim_dcmp_lt(JSContext* cx, bool ifnan, jsdouble& a, jsdouble& b, JSBool& r)
{
    interp_prim_dcmp_lt(cx, ifnan, a, b, r);
}

static inline void
prim_dcmp_le(JSContext* cx, bool ifnan, jsdouble& a, jsdouble& b, JSBool& r)
{
    interp_prim_dcmp_le(cx, ifnan, a, b, r);
}

static inline void
prim_dcmp_gt(JSContext* cx, bool ifnan, jsdouble& a, jsdouble& b, JSBool& r)
{
    interp_prim_dcmp_gt(cx, ifnan, a, b, r);
}

static inline void
prim_dcmp_ge(JSContext* cx, bool ifnan, jsdouble& a, jsdouble& b, JSBool& r)
{
    interp_prim_dcmp_ge(cx, ifnan, a, b, r);
}

static inline void
prim_generate_int_constant(JSContext* cx, jsint c, jsint& v)
{
    interp_prim_generate_int_constant(cx, c, v);
}

static inline void
prim_jsval_to_string(JSContext* cx, jsval& v, JSString*& s)
{
    interp_prim_jsval_to_string(cx, v, s);
}

static inline void
call_CompareStrings(JSContext* cx, JSString*& a, JSString*& b, jsint& r)
{
    interp_call_CompareStrings(cx, a, b, r);
}

static inline bool
guard_both_jsvals_are_int(JSContext* cx, jsval& a, jsval& b)
{
    return interp_guard_both_jsvals_are_int(cx, a, b);
}

static inline bool
guard_both_jsvals_are_string(JSContext* cx, jsval& a, jsval& b)
{
    return interp_guard_both_jsvals_are_string(cx, a, b);
}

#endif 
