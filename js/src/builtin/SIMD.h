





#ifndef builtin_SIMD_h
#define builtin_SIMD_h

#include "jsapi.h"
#include "jsobj.h"
#include "builtin/TypedObject.h"
#include "vm/GlobalObject.h"







#define FLOAT32X4_NULLARY_FUNCTION_LIST(V)                                                              \
  V(zero, (FuncZero<Float32x4>), 0, 0, Zero)

#define FLOAT32X4_UNARY_FUNCTION_LIST(V)                                                                \
  V(abs, (UnaryFunc<Float32x4, Abs, Float32x4>), 1, 0, Abs)                                             \
  V(fromInt32x4Bits, (FuncConvertBits<Int32x4, Float32x4>), 1, 0, FromInt32x4Bits)                      \
  V(neg, (UnaryFunc<Float32x4, Neg, Float32x4>), 1, 0, Neg)                                             \
  V(not, (CoercedUnaryFunc<Float32x4, Int32x4, Not, Float32x4>), 1, 0, Not)                             \
  V(reciprocal, (UnaryFunc<Float32x4, Rec, Float32x4>), 1, 0, Reciprocal)                               \
  V(reciprocalSqrt, (UnaryFunc<Float32x4, RecSqrt, Float32x4>), 1, 0, ReciprocalSqrt)                   \
  V(splat, (FuncSplat<Float32x4>), 1, 0, Splat)                                                         \
  V(sqrt, (UnaryFunc<Float32x4, Sqrt, Float32x4>), 1, 0, Sqrt)                                          \
  V(fromInt32x4, (FuncConvert<Int32x4, Float32x4> ), 1, 0, FromInt32x4)

#define FLOAT32X4_BINARY_FUNCTION_LIST(V)                                                               \
  V(add, (BinaryFunc<Float32x4, Add, Float32x4>), 2, 0, Add)                                            \
  V(and, (CoercedBinaryFunc<Float32x4, Int32x4, And, Float32x4>), 2, 0, And)                            \
  V(div, (BinaryFunc<Float32x4, Div, Float32x4>), 2, 0, Div)                                            \
  V(equal, (BinaryFunc<Float32x4, Equal, Int32x4>), 2, 0, Equal)                                        \
  V(greaterThan, (BinaryFunc<Float32x4, GreaterThan, Int32x4>), 2, 0, GreaterThan)                      \
  V(greaterThanOrEqual, (BinaryFunc<Float32x4, GreaterThanOrEqual, Int32x4>), 2, 0, GreaterThanOrEqual) \
  V(lessThan, (BinaryFunc<Float32x4, LessThan, Int32x4>), 2, 0, LessThan)                               \
  V(lessThanOrEqual, (BinaryFunc<Float32x4, LessThanOrEqual, Int32x4>), 2, 0, LessThanOrEqual)          \
  V(max, (BinaryFunc<Float32x4, Maximum, Float32x4>), 2, 0, Max)                                        \
  V(min, (BinaryFunc<Float32x4, Minimum, Float32x4>), 2, 0, Min)                                        \
  V(mul, (BinaryFunc<Float32x4, Mul, Float32x4>), 2, 0, Mul)                                            \
  V(notEqual, (BinaryFunc<Float32x4, NotEqual, Int32x4>), 2, 0, NotEqual)                               \
  V(shuffle, FuncShuffle<Float32x4>, 2, 0, Shuffle)                                                     \
  V(or, (CoercedBinaryFunc<Float32x4, Int32x4, Or, Float32x4>), 2, 0, Or)                               \
  V(scale, (FuncWith<Float32x4, Scale>), 2, 0, Scale)                                                   \
  V(sub, (BinaryFunc<Float32x4, Sub, Float32x4>), 2, 0, Sub)                                            \
  V(withX, (FuncWith<Float32x4, WithX>), 2, 0, WithX)                                                   \
  V(withY, (FuncWith<Float32x4, WithY>), 2, 0, WithY)                                                   \
  V(withZ, (FuncWith<Float32x4, WithZ>), 2, 0, WithZ)                                                   \
  V(withW, (FuncWith<Float32x4, WithW>), 2, 0, WithW)                                                   \
  V(xor, (CoercedBinaryFunc<Float32x4, Int32x4, Xor, Float32x4>), 2, 0, Xor)

#define FLOAT32X4_TERNARY_FUNCTION_LIST(V)                                                              \
  V(clamp, Float32x4Clamp, 3, 0, Clamp)                                                                 \
  V(shuffleMix, FuncShuffle<Float32x4>, 3, 0, ShuffleMix)

#define FLOAT32X4_FUNCTION_LIST(V)                                                                      \
  FLOAT32X4_NULLARY_FUNCTION_LIST(V)                                                                    \
  FLOAT32X4_UNARY_FUNCTION_LIST(V)                                                                      \
  FLOAT32X4_BINARY_FUNCTION_LIST(V)                                                                     \
  FLOAT32X4_TERNARY_FUNCTION_LIST(V)

#define INT32X4_NULLARY_FUNCTION_LIST(V)                                                                \
  V(zero, (FuncZero<Int32x4>), 0, 0, Zero)

#define INT32X4_UNARY_FUNCTION_LIST(V)                                                                  \
  V(fromFloat32x4Bits, (FuncConvertBits<Float32x4, Int32x4>), 1, 0, FromFloat32x4Bits)                  \
  V(neg, (UnaryFunc<Int32x4, Neg, Int32x4>), 1, 0, Neg)                                                 \
  V(not, (UnaryFunc<Int32x4, Not, Int32x4>), 1, 0, Not)                                                 \
  V(splat, (FuncSplat<Int32x4>), 0, 0, Splat)                                                           \
  V(fromFloat32x4, (FuncConvert<Float32x4, Int32x4>), 1, 0, FromFloat32x4)

#define INT32X4_BINARY_FUNCTION_LIST(V)                                                                 \
  V(add, (BinaryFunc<Int32x4, Add, Int32x4>), 2, 0, Add)                                                \
  V(and, (BinaryFunc<Int32x4, And, Int32x4>), 2, 0, And)                                                \
  V(equal, (BinaryFunc<Int32x4, Equal, Int32x4>), 2, 0, Equal)                                          \
  V(greaterThan, (BinaryFunc<Int32x4, GreaterThan, Int32x4>), 2, 0, GreaterThan)                        \
  V(lessThan, (BinaryFunc<Int32x4, LessThan, Int32x4>), 2, 0, LessThan)                                 \
  V(mul, (BinaryFunc<Int32x4, Mul, Int32x4>), 2, 0, Mul)                                                \
  V(or, (BinaryFunc<Int32x4, Or, Int32x4>), 2, 0, Or)                                                   \
  V(sub, (BinaryFunc<Int32x4, Sub, Int32x4>), 2, 0, Sub)                                                \
  V(shiftLeft, (Int32x4BinaryScalar<ShiftLeft>), 2, 0, ShiftLeft)                                       \
  V(shiftRight, (Int32x4BinaryScalar<ShiftRight>), 2, 0, ShiftRight)                                    \
  V(shiftRightLogical, (Int32x4BinaryScalar<ShiftRightLogical>), 2, 0, ShiftRightLogical)               \
  V(shuffle, FuncShuffle<Int32x4>, 2, 0, Shuffle)                                                       \
  V(withFlagX, (FuncWith<Int32x4, WithFlagX>), 2, 0, WithFlagX)                                         \
  V(withFlagY, (FuncWith<Int32x4, WithFlagY>), 2, 0, WithFlagY)                                         \
  V(withFlagZ, (FuncWith<Int32x4, WithFlagZ>), 2, 0, WithFlagZ)                                         \
  V(withFlagW, (FuncWith<Int32x4, WithFlagW>), 2, 0, WithFlagW)                                         \
  V(withX, (FuncWith<Int32x4, WithX>), 2, 0, WithX)                                                     \
  V(withY, (FuncWith<Int32x4, WithY>), 2, 0, WithY)                                                     \
  V(withZ, (FuncWith<Int32x4, WithZ>), 2, 0, WithZ)                                                     \
  V(withW, (FuncWith<Int32x4, WithW>), 2, 0, WithW)                                                     \
  V(xor, (BinaryFunc<Int32x4, Xor, Int32x4>), 2, 0, Xor)

#define INT32X4_TERNARY_FUNCTION_LIST(V)                                                                \
  V(select, Int32x4Select, 3, 0, Select)                                                                \
  V(shuffleMix, FuncShuffle<Int32x4>, 3, 0, ShuffleMix)

#define INT32X4_QUARTERNARY_FUNCTION_LIST(V)                                                            \
  V(bool, Int32x4Bool, 4, 0, Bool)

#define INT32X4_FUNCTION_LIST(V)                                                                        \
  INT32X4_NULLARY_FUNCTION_LIST(V)                                                                      \
  INT32X4_UNARY_FUNCTION_LIST(V)                                                                        \
  INT32X4_BINARY_FUNCTION_LIST(V)                                                                       \
  INT32X4_TERNARY_FUNCTION_LIST(V)                                                                      \
  INT32X4_QUARTERNARY_FUNCTION_LIST(V)

namespace js {

class SIMDObject : public JSObject
{
  public:
    static const Class class_;
    static JSObject* initClass(JSContext *cx, Handle<GlobalObject *> global);
    static bool toString(JSContext *cx, unsigned int argc, jsval *vp);
};



struct Float32x4 {
    typedef float Elem;
    static const unsigned lanes = 4;
    static const X4TypeDescr::Type type = X4TypeDescr::TYPE_FLOAT32;

    static TypeDescr &GetTypeDescr(GlobalObject &global) {
        return global.float32x4TypeDescr().as<TypeDescr>();
    }
    static Elem toType(Elem a) {
        return a;
    }
    static bool toType(JSContext *cx, JS::HandleValue v, Elem *out) {
        *out = v.toNumber();
        return true;
    }
    static void setReturn(CallArgs &args, Elem value) {
        args.rval().setDouble(JS::CanonicalizeNaN(value));
    }
};

struct Int32x4 {
    typedef int32_t Elem;
    static const unsigned lanes = 4;
    static const X4TypeDescr::Type type = X4TypeDescr::TYPE_INT32;

    static TypeDescr &GetTypeDescr(GlobalObject &global) {
        return global.int32x4TypeDescr().as<TypeDescr>();
    }
    static Elem toType(Elem a) {
        return ToInt32(a);
    }
    static bool toType(JSContext *cx, JS::HandleValue v, Elem *out) {
        return ToInt32(cx, v, out);
    }
    static void setReturn(CallArgs &args, Elem value) {
        args.rval().setInt32(value);
    }
};

template<typename V>
JSObject *CreateSimd(JSContext *cx, typename V::Elem *data);

#define DECLARE_SIMD_FLOAT32X4_FUNCTION(Name, Func, Operands, Flags, MIRId)                                       \
extern bool                                                                                                       \
simd_float32x4_##Name(JSContext *cx, unsigned argc, Value *vp);
FLOAT32X4_FUNCTION_LIST(DECLARE_SIMD_FLOAT32X4_FUNCTION)
#undef DECLARE_SIMD_FLOAT32X4_FUNCTION

#define DECLARE_SIMD_INT32x4_FUNCTION(Name, Func, Operands, Flags, MIRId)                                         \
extern bool                                                                                                       \
simd_int32x4_##Name(JSContext *cx, unsigned argc, Value *vp);
INT32X4_FUNCTION_LIST(DECLARE_SIMD_INT32x4_FUNCTION)
#undef DECLARE_SIMD_INT32x4_FUNCTION

}  

JSObject *
js_InitSIMDClass(JSContext *cx, js::HandleObject obj);

#endif 
