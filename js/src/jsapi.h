







































#ifndef jsapi_h___
#define jsapi_h___




#include "mozilla/StandardInteger.h"

#include <stddef.h>
#include <stdio.h>
#include "js-config.h"
#include "jspubtd.h"
#include "jsutil.h"
#include "jsval.h"

#include "js/Utility.h"

#ifdef __cplusplus
#include "jsalloc.h"
#include "js/Vector.h"
#include "mozilla/Attributes.h"
#endif




#define JSVAL_INT_BITS          32
#define JSVAL_INT_MIN           ((int32_t)0x80000000)
#define JSVAL_INT_MAX           ((int32_t)0x7fffffff)



#define JS_Assert MOZ_Assert

#ifdef __cplusplus
namespace JS {





























































template<typename T> class AnchorPermitted;
template<> class AnchorPermitted<JSObject *> { };
template<> class AnchorPermitted<const JSObject *> { };
template<> class AnchorPermitted<JSFunction *> { };
template<> class AnchorPermitted<const JSFunction *> { };
template<> class AnchorPermitted<JSString *> { };
template<> class AnchorPermitted<const JSString *> { };
template<> class AnchorPermitted<Value> { };

template<typename T>
class Anchor: AnchorPermitted<T>
{
  public:
    Anchor() { }
    explicit Anchor(T t) { hold = t; }
    inline ~Anchor();
    T &get() { return hold; }
    const T &get() const { return hold; }
    void set(const T &t) { hold = t; }
    void clear() { hold = 0; }
  private:
    T hold;
    
    Anchor(const Anchor &);
    const Anchor &operator=(const Anchor &);
};

#ifdef __GNUC__
template<typename T>
inline Anchor<T>::~Anchor()
{
    










    asm volatile("":: "g" (hold) : "memory");
}
#else
template<typename T>
inline Anchor<T>::~Anchor()
{
    
























    volatile T sink;
    sink = hold;
}
#endif  












inline void PoisonPtr(uintptr_t *v)
{
#if defined(JSGC_ROOT_ANALYSIS) && defined(DEBUG)
    uint8_t *ptr = (uint8_t *) v + 3;
    *ptr = JS_FREE_PATTERN;
#endif
}

template <typename T>
inline bool IsPoisonedPtr(T *v)
{
#if defined(JSGC_ROOT_ANALYSIS) && defined(DEBUG)
    uint32_t mask = uintptr_t(v) & 0xff000000;
    return mask == uint32_t(JS_FREE_PATTERN << 24);
#else
    return false;
#endif
}


































class Value
{
  public:
    




    

    JS_ALWAYS_INLINE
    void setNull() {
        data.asBits = BUILD_JSVAL(JSVAL_TAG_NULL, 0).asBits;
    }

    JS_ALWAYS_INLINE
    void setUndefined() {
        data.asBits = BUILD_JSVAL(JSVAL_TAG_UNDEFINED, 0).asBits;
    }

    JS_ALWAYS_INLINE
    void setInt32(int32_t i) {
        data = INT32_TO_JSVAL_IMPL(i);
    }

    JS_ALWAYS_INLINE
    int32_t &getInt32Ref() {
        JS_ASSERT(isInt32());
        return data.s.payload.i32;
    }

    JS_ALWAYS_INLINE
    void setDouble(double d) {
        data = DOUBLE_TO_JSVAL_IMPL(d);
    }

    JS_ALWAYS_INLINE
    double &getDoubleRef() {
        JS_ASSERT(isDouble());
        return data.asDouble;
    }

    JS_ALWAYS_INLINE
    void setString(JSString *str) {
        JS_ASSERT(!IsPoisonedPtr(str));
        data = STRING_TO_JSVAL_IMPL(str);
    }

    JS_ALWAYS_INLINE
    void setString(const JS::Anchor<JSString *> &str) {
        setString(str.get());
    }

    JS_ALWAYS_INLINE
    void setObject(JSObject &obj) {
        JS_ASSERT(!IsPoisonedPtr(&obj));
        data = OBJECT_TO_JSVAL_IMPL(&obj);
    }

    JS_ALWAYS_INLINE
    void setBoolean(bool b) {
        data = BOOLEAN_TO_JSVAL_IMPL(b);
    }

    JS_ALWAYS_INLINE
    void setMagic(JSWhyMagic why) {
        data = MAGIC_TO_JSVAL_IMPL(why);
    }

    JS_ALWAYS_INLINE
    bool setNumber(uint32_t ui) {
        if (ui > JSVAL_INT_MAX) {
            setDouble((double)ui);
            return false;
        } else {
            setInt32((int32_t)ui);
            return true;
        }
    }

    JS_ALWAYS_INLINE
    bool setNumber(double d) {
        int32_t i;
        if (JSDOUBLE_IS_INT32(d, &i)) {
            setInt32(i);
            return true;
        } else {
            setDouble(d);
            return false;
        }
    }

    JS_ALWAYS_INLINE
    void setObjectOrNull(JSObject *arg) {
        if (arg)
            setObject(*arg);
        else
            setNull();
    }

    JS_ALWAYS_INLINE
    void swap(Value &rhs) {
        uint64_t tmp = rhs.data.asBits;
        rhs.data.asBits = data.asBits;
        data.asBits = tmp;
    }

    

    JS_ALWAYS_INLINE
    bool isUndefined() const {
        return JSVAL_IS_UNDEFINED_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isNull() const {
        return JSVAL_IS_NULL_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isNullOrUndefined() const {
        return isNull() || isUndefined();
    }

    JS_ALWAYS_INLINE
    bool isInt32() const {
        return JSVAL_IS_INT32_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isInt32(int32_t i32) const {
        return JSVAL_IS_SPECIFIC_INT32_IMPL(data, i32);
    }

    JS_ALWAYS_INLINE
    bool isDouble() const {
        return JSVAL_IS_DOUBLE_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isNumber() const {
        return JSVAL_IS_NUMBER_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isString() const {
        return JSVAL_IS_STRING_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isObject() const {
        return JSVAL_IS_OBJECT_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isPrimitive() const {
        return JSVAL_IS_PRIMITIVE_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isObjectOrNull() const {
        return JSVAL_IS_OBJECT_OR_NULL_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isGCThing() const {
        return JSVAL_IS_GCTHING_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isBoolean() const {
        return JSVAL_IS_BOOLEAN_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isTrue() const {
        return JSVAL_IS_SPECIFIC_BOOLEAN(data, true);
    }

    JS_ALWAYS_INLINE
    bool isFalse() const {
        return JSVAL_IS_SPECIFIC_BOOLEAN(data, false);
    }

    JS_ALWAYS_INLINE
    bool isMagic() const {
        return JSVAL_IS_MAGIC_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool isMagic(JSWhyMagic why) const {
        JS_ASSERT_IF(isMagic(), data.s.payload.why == why);
        return JSVAL_IS_MAGIC_IMPL(data);
    }

    




    JS_ALWAYS_INLINE
    bool isParticularMagic(JSWhyMagic why) const {
        return isMagic() && data.s.payload.why == why;
    }

    JS_ALWAYS_INLINE
    bool isMarkable() const {
        return JSVAL_IS_TRACEABLE_IMPL(data);
    }

    JS_ALWAYS_INLINE
    JSGCTraceKind gcKind() const {
        JS_ASSERT(isMarkable());
        return JSGCTraceKind(JSVAL_TRACE_KIND_IMPL(data));
    }

    JS_ALWAYS_INLINE
    JSWhyMagic whyMagic() const {
        JS_ASSERT(isMagic());
        return data.s.payload.why;
    }

    

    JS_ALWAYS_INLINE
    bool operator==(const Value &rhs) const {
        return data.asBits == rhs.data.asBits;
    }

    JS_ALWAYS_INLINE
    bool operator!=(const Value &rhs) const {
        return data.asBits != rhs.data.asBits;
    }

    friend inline bool SameType(const Value &lhs, const Value &rhs);

    

    JS_ALWAYS_INLINE
    int32_t toInt32() const {
        JS_ASSERT(isInt32());
        return JSVAL_TO_INT32_IMPL(data);
    }

    JS_ALWAYS_INLINE
    double toDouble() const {
        JS_ASSERT(isDouble());
        return data.asDouble;
    }

    JS_ALWAYS_INLINE
    double toNumber() const {
        JS_ASSERT(isNumber());
        return isDouble() ? toDouble() : double(toInt32());
    }

    JS_ALWAYS_INLINE
    JSString *toString() const {
        JS_ASSERT(isString());
        return JSVAL_TO_STRING_IMPL(data);
    }

    JS_ALWAYS_INLINE
    JSObject &toObject() const {
        JS_ASSERT(isObject());
        return *JSVAL_TO_OBJECT_IMPL(data);
    }

    JS_ALWAYS_INLINE
    JSObject *toObjectOrNull() const {
        JS_ASSERT(isObjectOrNull());
        return JSVAL_TO_OBJECT_IMPL(data);
    }

    JS_ALWAYS_INLINE
    void *toGCThing() const {
        JS_ASSERT(isGCThing());
        return JSVAL_TO_GCTHING_IMPL(data);
    }

    JS_ALWAYS_INLINE
    bool toBoolean() const {
        JS_ASSERT(isBoolean());
        return JSVAL_TO_BOOLEAN_IMPL(data);
    }

    JS_ALWAYS_INLINE
    uint32_t payloadAsRawUint32() const {
        JS_ASSERT(!isDouble());
        return data.s.payload.u32;
    }

    JS_ALWAYS_INLINE
    uint64_t asRawBits() const {
        return data.asBits;
    }

    JS_ALWAYS_INLINE
    JSValueType extractNonDoubleType() const {
        return JSVAL_EXTRACT_NON_DOUBLE_TYPE_IMPL(data);
    }

    








    JS_ALWAYS_INLINE
    void setPrivate(void *ptr) {
        data = PRIVATE_PTR_TO_JSVAL_IMPL(ptr);
    }

    JS_ALWAYS_INLINE
    void *toPrivate() const {
        JS_ASSERT(JSVAL_IS_DOUBLE_IMPL(data));
        return JSVAL_TO_PRIVATE_PTR_IMPL(data);
    }

    JS_ALWAYS_INLINE
    void setPrivateUint32(uint32_t ui) {
        data = PRIVATE_UINT32_TO_JSVAL_IMPL(ui);
    }

    JS_ALWAYS_INLINE
    uint32_t toPrivateUint32() const {
        JS_ASSERT(JSVAL_IS_DOUBLE_IMPL(data));
        return JSVAL_TO_PRIVATE_UINT32_IMPL(data);
    }

    JS_ALWAYS_INLINE
    uint32_t &getPrivateUint32Ref() {
        JS_ASSERT(isDouble());
        return data.s.payload.u32;
    }

    





    JS_ALWAYS_INLINE
    void setUnmarkedPtr(void *ptr) {
        data.asPtr = ptr;
    }

    JS_ALWAYS_INLINE
    void *toUnmarkedPtr() const {
        return data.asPtr;
    }

    const size_t *payloadWord() const {
#if JS_BITS_PER_WORD == 32
        return &data.s.payload.word;
#elif JS_BITS_PER_WORD == 64
        return &data.asWord;
#endif
    }

#ifndef _MSC_VER
  



  private:
#endif

    jsval_layout data;

  private:
    void staticAssertions() {
        JS_STATIC_ASSERT(sizeof(JSValueType) == 1);
        JS_STATIC_ASSERT(sizeof(JSValueTag) == 4);
        JS_STATIC_ASSERT(sizeof(JSBool) == 4);
        JS_STATIC_ASSERT(sizeof(JSWhyMagic) <= 4);
        JS_STATIC_ASSERT(sizeof(Value) == 8);
    }

    friend jsval_layout (::JSVAL_TO_IMPL)(Value);
    friend Value (::IMPL_TO_JSVAL)(jsval_layout l);
};

inline bool
IsPoisonedValue(const Value &v)
{
    if (v.isString())
        return IsPoisonedPtr(v.toString());
    if (v.isObject())
        return IsPoisonedPtr(&v.toObject());
    return false;
}



static JS_ALWAYS_INLINE Value
NullValue()
{
    Value v;
    v.setNull();
    return v;
}

static JS_ALWAYS_INLINE Value
UndefinedValue()
{
    Value v;
    v.setUndefined();
    return v;
}

static JS_ALWAYS_INLINE Value
Int32Value(int32_t i32)
{
    Value v;
    v.setInt32(i32);
    return v;
}

static JS_ALWAYS_INLINE Value
DoubleValue(double dbl)
{
    Value v;
    v.setDouble(dbl);
    return v;
}

static JS_ALWAYS_INLINE Value
StringValue(JSString *str)
{
    Value v;
    v.setString(str);
    return v;
}

static JS_ALWAYS_INLINE Value
BooleanValue(bool boo)
{
    Value v;
    v.setBoolean(boo);
    return v;
}

static JS_ALWAYS_INLINE Value
ObjectValue(JSObject &obj)
{
    Value v;
    v.setObject(obj);
    return v;
}

static JS_ALWAYS_INLINE Value
MagicValue(JSWhyMagic why)
{
    Value v;
    v.setMagic(why);
    return v;
}

static JS_ALWAYS_INLINE Value
NumberValue(double dbl)
{
    Value v;
    v.setNumber(dbl);
    return v;
}

static JS_ALWAYS_INLINE Value
ObjectOrNullValue(JSObject *obj)
{
    Value v;
    v.setObjectOrNull(obj);
    return v;
}

static JS_ALWAYS_INLINE Value
PrivateValue(void *ptr)
{
    Value v;
    v.setPrivate(ptr);
    return v;
}

static JS_ALWAYS_INLINE Value
PrivateUint32Value(uint32_t ui)
{
    Value v;
    v.setPrivateUint32(ui);
    return v;
}

JS_ALWAYS_INLINE bool
SameType(const Value &lhs, const Value &rhs)
{
    return JSVAL_SAME_TYPE_IMPL(lhs.data, rhs.data);
}



#ifndef __GNUC__












template<>
inline Anchor<Value>::~Anchor()
{
    volatile uint64_t bits;
    bits = JSVAL_TO_IMPL(hold).asBits;
}

#endif

#if defined JS_THREADSAFE && defined DEBUG

class JS_PUBLIC_API(AutoCheckRequestDepth)
{
    JSContext *cx;
  public:
    AutoCheckRequestDepth(JSContext *cx);
    ~AutoCheckRequestDepth();
};

# define CHECK_REQUEST(cx) \
    JS::AutoCheckRequestDepth _autoCheckRequestDepth(cx)

#else

# define CHECK_REQUEST(cx) \
    ((void) 0)

#endif

class JS_PUBLIC_API(AutoGCRooter) {
  public:
    AutoGCRooter(JSContext *cx, ptrdiff_t tag);

    ~AutoGCRooter() {
        JS_ASSERT(this == *stackTop);
        *stackTop = down;
    }

    
    inline void trace(JSTracer *trc);
    static void traceAll(JSTracer *trc);

  protected:
    AutoGCRooter * const down;

    






    ptrdiff_t tag;

    enum {
        JSVAL =        -1, 
        VALARRAY =     -2, 
        PARSER =       -3, 
        SHAPEVECTOR =  -4, 
        ENUMERATOR =   -5, 
        IDARRAY =      -6, 
        DESCRIPTORS =  -7, 
        NAMESPACES =   -8, 
        XML =          -9, 
        OBJECT =      -10, 
        ID =          -11, 
        VALVECTOR =   -12, 
        DESCRIPTOR =  -13, 
        STRING =      -14, 
        IDVECTOR =    -15, 
        OBJVECTOR =   -16  
    };

  private:
    AutoGCRooter ** const stackTop;

    
    AutoGCRooter(AutoGCRooter &ida) MOZ_DELETE;
    void operator=(AutoGCRooter &ida) MOZ_DELETE;
};

class AutoValueRooter : private AutoGCRooter
{
  public:
    explicit AutoValueRooter(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(NullValue())
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    AutoValueRooter(JSContext *cx, const Value &v
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(v)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    





    void set(Value v) {
        JS_ASSERT(tag == JSVAL);
        val = v;
    }

    const Value &value() const {
        JS_ASSERT(tag == JSVAL);
        return val;
    }

    Value *addr() {
        JS_ASSERT(tag == JSVAL);
        return &val;
    }

    const Value &jsval_value() const {
        JS_ASSERT(tag == JSVAL);
        return val;
    }

    Value *jsval_addr() {
        JS_ASSERT(tag == JSVAL);
        return &val;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Value val;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectRooter : private AutoGCRooter {
  public:
    AutoObjectRooter(JSContext *cx, JSObject *obj = NULL
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, OBJECT), obj(obj)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setObject(JSObject *obj) {
        this->obj = obj;
    }

    JSObject * object() const {
        return obj;
    }

    JSObject ** addr() {
        return &obj;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSObject *obj;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoStringRooter : private AutoGCRooter {
  public:
    AutoStringRooter(JSContext *cx, JSString *str = NULL
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, STRING), str(str)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setString(JSString *str) {
        this->str = str;
    }

    JSString * string() const {
        return str;
    }

    JSString ** addr() {
        return &str;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSString *str;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoArrayRooter : private AutoGCRooter {
  public:
    AutoArrayRooter(JSContext *cx, size_t len, Value *vec
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(vec)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag >= 0);
    }

    void changeLength(size_t newLength) {
        tag = ptrdiff_t(newLength);
        JS_ASSERT(tag >= 0);
    }

    void changeArray(Value *newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    Value *array;

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};


class AutoEnumStateRooter : private AutoGCRooter
{
  public:
    AutoEnumStateRooter(JSContext *cx, JSObject *obj
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ENUMERATOR), obj(obj), stateValue(), context(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
    }

    ~AutoEnumStateRooter();

    friend void AutoGCRooter::trace(JSTracer *trc);

    const Value &state() const { return stateValue; }
    Value *addr() { return &stateValue; }

  protected:
    void trace(JSTracer *trc);

    JSObject *obj;

  private:
    Value stateValue;
    JSContext *context;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<class T>
class AutoVectorRooter : protected AutoGCRooter
{
  public:
    explicit AutoVectorRooter(JSContext *cx, ptrdiff_t tag
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, tag), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool append(const T &v) { return vector.append(v); }

    
    void infallibleAppend(const T &v) { vector.infallibleAppend(v); }

    void popBack() { vector.popBack(); }
    T popCopy() { return vector.popCopy(); }

    bool growBy(size_t inc) {
        size_t oldLength = vector.length();
        if (!vector.growByUninitialized(inc))
            return false;
        makeRangeGCSafe(oldLength);
        return true;
    }

    bool resize(size_t newLength) {
        size_t oldLength = vector.length();
        if (newLength <= oldLength) {
            vector.shrinkBy(oldLength - newLength);
            return true;
        }
        if (!vector.growByUninitialized(newLength - oldLength))
            return false;
        makeRangeGCSafe(oldLength);
        return true;
    }

    void clear() { vector.clear(); }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    T &operator[](size_t i) { return vector[i]; }
    const T &operator[](size_t i) const { return vector[i]; }

    const T *begin() const { return vector.begin(); }
    T *begin() { return vector.begin(); }

    const T *end() const { return vector.end(); }
    T *end() { return vector.end(); }

    const T &back() const { return vector.back(); }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    void makeRangeGCSafe(size_t oldLength) {
        T *t = vector.begin() + oldLength;
        for (size_t i = oldLength; i < vector.length(); ++i, ++t)
            memset(t, 0, sizeof(T));
    }

    typedef js::Vector<T, 8> VectorImpl;
    VectorImpl vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueVector : public AutoVectorRooter<Value>
{
  public:
    explicit AutoValueVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Value>(cx, VALVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdVector : public AutoVectorRooter<jsid>
{
  public:
    explicit AutoIdVector(JSContext *cx
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<jsid>(cx, IDVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

}  








typedef JS::Value jsval;

static JS_ALWAYS_INLINE jsval_layout
JSVAL_TO_IMPL(jsval v)
{
    return v.data;
}

static JS_ALWAYS_INLINE jsval
IMPL_TO_JSVAL(jsval_layout l)
{
    JS::Value v;
    v.data = l;
    return v;
}

#ifdef DEBUG
struct JSValueAlignmentTester { char c; JS::Value v; };
JS_STATIC_ASSERT(sizeof(JSValueAlignmentTester) == 16);
#endif 

#else







typedef union jsval_layout jsval;

static JS_ALWAYS_INLINE jsval_layout
JSVAL_TO_IMPL(jsval v)
{
    return v;
}

static JS_ALWAYS_INLINE jsval
IMPL_TO_JSVAL(jsval_layout l)
{
    return l;
}

#endif

#ifdef DEBUG
typedef struct { char c; jsval_layout l; } JSLayoutAlignmentTester;
JS_STATIC_ASSERT(sizeof(JSLayoutAlignmentTester) == 16);
#endif 

JS_STATIC_ASSERT(sizeof(jsval_layout) == sizeof(jsval));












typedef JSBool
(* JSPropertyOp)(JSContext *cx, JSObject *obj, jsid id, jsval *vp);








typedef JSBool
(* JSStrictPropertyOp)(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

































typedef JSBool
(* JSNewEnumerateOp)(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                     jsval *statep, jsid *idp);





typedef JSBool
(* JSEnumerateOp)(JSContext *cx, JSObject *obj);













typedef JSBool
(* JSResolveOp)(JSContext *cx, JSObject *obj, jsid id);






























typedef JSBool
(* JSNewResolveOp)(JSContext *cx, JSObject *obj, jsid id, unsigned flags,
                   JSObject **objp);





typedef JSBool
(* JSConvertOp)(JSContext *cx, JSObject *obj, JSType type, jsval *vp);




typedef JSType
(* JSTypeOfOp)(JSContext *cx, JSObject *obj);






typedef void
(* JSFinalizeOp)(JSContext *cx, JSObject *obj);




typedef struct JSStringFinalizer JSStringFinalizer;

struct JSStringFinalizer {
    void (*finalize)(const JSStringFinalizer *fin, jschar *chars);
};







typedef JSBool
(* JSCheckAccessOp)(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                    jsval *vp);






typedef JSBool
(* JSHasInstanceOp)(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);


















typedef void
(* JSTraceOp)(JSTracer *trc, JSObject *obj);





typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);

typedef JSBool
(* JSEqualityOp)(JSContext *cx, JSObject *obj, const jsval *v, JSBool *bp);







typedef JSBool
(* JSNative)(JSContext *cx, unsigned argc, jsval *vp);



typedef enum JSContextOp {
    JSCONTEXT_NEW,
    JSCONTEXT_DESTROY
} JSContextOp;














typedef JSBool
(* JSContextCallback)(JSContext *cx, unsigned contextOp);

typedef enum JSGCStatus {
    JSGC_BEGIN,
    JSGC_END
} JSGCStatus;

typedef void
(* JSGCCallback)(JSRuntime *rt, JSGCStatus status);

typedef enum JSFinalizeStatus {
    JSFINALIZE_START,
    JSFINALIZE_END
} JSFinalizeStatus;

typedef void
(* JSFinalizeCallback)(JSContext *cx, JSFinalizeStatus status);





typedef void
(* JSTraceDataOp)(JSTracer *trc, void *data);

typedef JSBool
(* JSOperationCallback)(JSContext *cx);

typedef void
(* JSErrorReporter)(JSContext *cx, const char *message, JSErrorReport *report);

#ifdef MOZ_TRACE_JSCALLS
typedef void
(* JSFunctionCallback)(const JSFunction *fun,
                       const JSScript *scr,
                       const JSContext *cx,
                       int entering);
#endif






typedef enum JSExnType {
    JSEXN_NONE = -1,
      JSEXN_ERR,
        JSEXN_INTERNALERR,
        JSEXN_EVALERR,
        JSEXN_RANGEERR,
        JSEXN_REFERENCEERR,
        JSEXN_SYNTAXERR,
        JSEXN_TYPEERR,
        JSEXN_URIERR,
        JSEXN_LIMIT
} JSExnType;

typedef struct JSErrorFormatString {
    
    const char *format;

    
    uint16_t argCount;

    
    int16_t exnType;
} JSErrorFormatString;

typedef const JSErrorFormatString *
(* JSErrorCallback)(void *userRef, const char *locale,
                    const unsigned errorNumber);

#ifdef va_start
#define JS_ARGUMENT_FORMATTER_DEFINED 1

typedef JSBool
(* JSArgumentFormatter)(JSContext *cx, const char *format, JSBool fromJS,
                        jsval **vpp, va_list *app);
#endif

typedef JSBool
(* JSLocaleToUpperCase)(JSContext *cx, JSString *src, jsval *rval);

typedef JSBool
(* JSLocaleToLowerCase)(JSContext *cx, JSString *src, jsval *rval);

typedef JSBool
(* JSLocaleCompare)(JSContext *cx, JSString *src1, JSString *src2,
                    jsval *rval);

typedef JSBool
(* JSLocaleToUnicode)(JSContext *cx, const char *src, jsval *rval);





typedef void
(* JSDestroyPrincipalsOp)(JSPrincipals *principals);

typedef JSBool
(* JSSubsumePrincipalsOp)(JSPrincipals *principals1, JSPrincipals *principals2);








typedef JSBool
(* JSPrincipalsTranscoder)(JSXDRState *xdr, JSPrincipals **principalsp);









typedef JSPrincipals *
(* JSObjectPrincipalsFinder)(JSObject *obj);





typedef JSBool
(* JSCSPEvalChecker)(JSContext *cx);






typedef JSObject *
(* JSWrapObjectCallback)(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent,
                         unsigned flags);






typedef JSObject *
(* JSPreWrapCallback)(JSContext *cx, JSObject *scope, JSObject *obj, unsigned flags);

typedef enum {
    JSCOMPARTMENT_DESTROY
} JSCompartmentOp;

typedef JSBool
(* JSCompartmentCallback)(JSContext *cx, JSCompartment *compartment, unsigned compartmentOp);










typedef JSObject *(*ReadStructuredCloneOp)(JSContext *cx, JSStructuredCloneReader *r,
                                           uint32_t tag, uint32_t data, void *closure);












typedef JSBool (*WriteStructuredCloneOp)(JSContext *cx, JSStructuredCloneWriter *w,
                                         JSObject *obj, void *closure);






typedef void (*StructuredCloneErrorOp)(JSContext *cx, uint32_t errorid);



JS_BEGIN_EXTERN_C






#ifdef _MSC_VER
# pragma warning(disable:4190)
#endif







extern JS_PUBLIC_DATA(const jsval) JSVAL_NULL;
extern JS_PUBLIC_DATA(const jsval) JSVAL_ZERO;
extern JS_PUBLIC_DATA(const jsval) JSVAL_ONE;
extern JS_PUBLIC_DATA(const jsval) JSVAL_FALSE;
extern JS_PUBLIC_DATA(const jsval) JSVAL_TRUE;
extern JS_PUBLIC_DATA(const jsval) JSVAL_VOID;



static JS_ALWAYS_INLINE JSBool
JSVAL_IS_NULL(jsval v)
{
    return JSVAL_IS_NULL_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_VOID(jsval v)
{
    return JSVAL_IS_UNDEFINED_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_INT(jsval v)
{
    return JSVAL_IS_INT32_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE int32_t
JSVAL_TO_INT(jsval v)
{
    JS_ASSERT(JSVAL_IS_INT(v));
    return JSVAL_TO_INT32_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE jsval
INT_TO_JSVAL(int32_t i)
{
    return IMPL_TO_JSVAL(INT32_TO_JSVAL_IMPL(i));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_DOUBLE(jsval v)
{
    return JSVAL_IS_DOUBLE_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE double
JSVAL_TO_DOUBLE(jsval v)
{
    jsval_layout l;
    JS_ASSERT(JSVAL_IS_DOUBLE(v));
    l = JSVAL_TO_IMPL(v);
    return l.asDouble;
}

static JS_ALWAYS_INLINE jsval
DOUBLE_TO_JSVAL(double d)
{
    



    jsval_layout l;
    if (JS_UNLIKELY(d != d)) {
        l.asBits = 0x7FF8000000000000LL;
    } else {
        l.asDouble = d;
    }
    return IMPL_TO_JSVAL(l);
}

static JS_ALWAYS_INLINE jsval
UINT_TO_JSVAL(uint32_t i)
{
    if (i <= JSVAL_INT_MAX)
        return INT_TO_JSVAL((int32_t)i);
    return DOUBLE_TO_JSVAL((double)i);
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_NUMBER(jsval v)
{
    return JSVAL_IS_NUMBER_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_STRING(jsval v)
{
    return JSVAL_IS_STRING_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSString *
JSVAL_TO_STRING(jsval v)
{
    JS_ASSERT(JSVAL_IS_STRING(v));
    return JSVAL_TO_STRING_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE jsval
STRING_TO_JSVAL(JSString *str)
{
    return IMPL_TO_JSVAL(STRING_TO_JSVAL_IMPL(str));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_OBJECT(jsval v)
{
    return JSVAL_IS_OBJECT_OR_NULL_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSObject *
JSVAL_TO_OBJECT(jsval v)
{
    JS_ASSERT(JSVAL_IS_OBJECT(v));
    return JSVAL_TO_OBJECT_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE jsval
OBJECT_TO_JSVAL(JSObject *obj)
{
    if (obj)
        return IMPL_TO_JSVAL(OBJECT_TO_JSVAL_IMPL(obj));
    return JSVAL_NULL;
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_BOOLEAN(jsval v)
{
    return JSVAL_IS_BOOLEAN_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_TO_BOOLEAN(jsval v)
{
    JS_ASSERT(JSVAL_IS_BOOLEAN(v));
    return JSVAL_TO_BOOLEAN_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE jsval
BOOLEAN_TO_JSVAL(JSBool b)
{
    return IMPL_TO_JSVAL(BOOLEAN_TO_JSVAL_IMPL(b));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_PRIMITIVE(jsval v)
{
    return JSVAL_IS_PRIMITIVE_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE JSBool
JSVAL_IS_GCTHING(jsval v)
{
    return JSVAL_IS_GCTHING_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_GCTHING(jsval v)
{
    JS_ASSERT(JSVAL_IS_GCTHING(v));
    return JSVAL_TO_GCTHING_IMPL(JSVAL_TO_IMPL(v));
}



static JS_ALWAYS_INLINE jsval
PRIVATE_TO_JSVAL(void *ptr)
{
    return IMPL_TO_JSVAL(PRIVATE_PTR_TO_JSVAL_IMPL(ptr));
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_PRIVATE(jsval v)
{
    JS_ASSERT(JSVAL_IS_DOUBLE(v));
    return JSVAL_TO_PRIVATE_PTR_IMPL(JSVAL_TO_IMPL(v));
}















#define JSID_TYPE_STRING                 0x0
#define JSID_TYPE_INT                    0x1
#define JSID_TYPE_VOID                   0x2
#define JSID_TYPE_OBJECT                 0x4
#define JSID_TYPE_DEFAULT_XML_NAMESPACE  0x6
#define JSID_TYPE_MASK                   0x7





#define id iden

static JS_ALWAYS_INLINE JSBool
JSID_IS_STRING(jsid id)
{
    return (JSID_BITS(id) & JSID_TYPE_MASK) == 0;
}

static JS_ALWAYS_INLINE JSString *
JSID_TO_STRING(jsid id)
{
    JS_ASSERT(JSID_IS_STRING(id));
    return (JSString *)JSID_BITS(id);
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_ZERO(jsid id)
{
    return JSID_BITS(id) == 0;
}

JS_PUBLIC_API(JSBool)
JS_StringHasBeenInterned(JSContext *cx, JSString *str);








static JS_ALWAYS_INLINE jsid
INTERNED_STRING_TO_JSID(JSContext *cx, JSString *str)
{
    jsid id;
    JS_ASSERT(str);
    JS_ASSERT(((size_t)str & JSID_TYPE_MASK) == 0);
#ifdef DEBUG
    JS_ASSERT(JS_StringHasBeenInterned(cx, str));
#else
    (void)cx;
#endif
    JSID_BITS(id) = (size_t)str;
    return id;
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_INT(jsid id)
{
    return !!(JSID_BITS(id) & JSID_TYPE_INT);
}

static JS_ALWAYS_INLINE int32_t
JSID_TO_INT(jsid id)
{
    JS_ASSERT(JSID_IS_INT(id));
    return ((int32_t)JSID_BITS(id)) >> 1;
}





#define JSID_INT_MIN  (-(1 << 30))
#define JSID_INT_MAX  ((1 << 30) - 1)

static JS_ALWAYS_INLINE JSBool
INT_FITS_IN_JSID(int32_t i)
{
    return ((uint32_t)(i) - (uint32_t)JSID_INT_MIN <=
            (uint32_t)(JSID_INT_MAX - JSID_INT_MIN));
}

static JS_ALWAYS_INLINE jsid
INT_TO_JSID(int32_t i)
{
    jsid id;
    JS_ASSERT(INT_FITS_IN_JSID(i));
    JSID_BITS(id) = ((i << 1) | JSID_TYPE_INT);
    return id;
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_OBJECT(jsid id)
{
    return (JSID_BITS(id) & JSID_TYPE_MASK) == JSID_TYPE_OBJECT &&
           (size_t)JSID_BITS(id) != JSID_TYPE_OBJECT;
}

static JS_ALWAYS_INLINE JSObject *
JSID_TO_OBJECT(jsid id)
{
    JS_ASSERT(JSID_IS_OBJECT(id));
    return (JSObject *)(JSID_BITS(id) & ~(size_t)JSID_TYPE_MASK);
}

static JS_ALWAYS_INLINE jsid
OBJECT_TO_JSID(JSObject *obj)
{
    jsid id;
    JS_ASSERT(obj != NULL);
    JS_ASSERT(((size_t)obj & JSID_TYPE_MASK) == 0);
    JSID_BITS(id) = ((size_t)obj | JSID_TYPE_OBJECT);
    return id;
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_GCTHING(jsid id)
{
    return JSID_IS_STRING(id) || JSID_IS_OBJECT(id);
}

static JS_ALWAYS_INLINE void *
JSID_TO_GCTHING(jsid id)
{
    return (void *)(JSID_BITS(id) & ~(size_t)JSID_TYPE_MASK);
}






static JS_ALWAYS_INLINE JSBool
JSID_IS_DEFAULT_XML_NAMESPACE(jsid id)
{
    JS_ASSERT_IF(((size_t)JSID_BITS(id) & JSID_TYPE_MASK) == JSID_TYPE_DEFAULT_XML_NAMESPACE,
                 JSID_BITS(id) == JSID_TYPE_DEFAULT_XML_NAMESPACE);
    return ((size_t)JSID_BITS(id) == JSID_TYPE_DEFAULT_XML_NAMESPACE);
}

#ifdef JS_USE_JSID_STRUCT_TYPES
extern JS_PUBLIC_DATA(jsid) JS_DEFAULT_XML_NAMESPACE_ID;
#else
# define JS_DEFAULT_XML_NAMESPACE_ID ((jsid)JSID_TYPE_DEFAULT_XML_NAMESPACE)
#endif








static JS_ALWAYS_INLINE JSBool
JSID_IS_VOID(jsid id)
{
    JS_ASSERT_IF(((size_t)JSID_BITS(id) & JSID_TYPE_MASK) == JSID_TYPE_VOID,
                 JSID_BITS(id) == JSID_TYPE_VOID);
    return ((size_t)JSID_BITS(id) == JSID_TYPE_VOID);
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_EMPTY(jsid id)
{
    return ((size_t)JSID_BITS(id) == JSID_TYPE_OBJECT);
}

#undef id

#ifdef JS_USE_JSID_STRUCT_TYPES
extern JS_PUBLIC_DATA(jsid) JSID_VOID;
extern JS_PUBLIC_DATA(jsid) JSID_EMPTY;
#else
# define JSID_VOID ((jsid)JSID_TYPE_VOID)
# define JSID_EMPTY ((jsid)JSID_TYPE_OBJECT)
#endif





static JS_ALWAYS_INLINE JSBool
JSVAL_IS_UNIVERSAL(jsval v)
{
    return !JSVAL_IS_GCTHING(v);
}

#ifdef __cplusplus

namespace JS {

class AutoIdRooter : private AutoGCRooter
{
  public:
    explicit AutoIdRooter(JSContext *cx, jsid id = INT_TO_JSID(0)
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ID), id_(id)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    jsid id() {
        return id_;
    }

    jsid * addr() {
        return &id_;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    jsid id_;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

#endif




#define JSVAL_LOCK(cx,v)        (JSVAL_IS_GCTHING(v)                          \
                                 ? JS_LockGCThing(cx, JSVAL_TO_GCTHING(v))    \
                                 : JS_TRUE)
#define JSVAL_UNLOCK(cx,v)      (JSVAL_IS_GCTHING(v)                          \
                                 ? JS_UnlockGCThing(cx, JSVAL_TO_GCTHING(v))  \
                                 : JS_TRUE)


#define JSPROP_ENUMERATE        0x01    /* property is visible to for/in loop */
#define JSPROP_READONLY         0x02    /* not settable: assignment is no-op.
                                           This flag is only valid when neither
                                           JSPROP_GETTER nor JSPROP_SETTER is
                                           set. */
#define JSPROP_PERMANENT        0x04    /* property cannot be deleted */
#define JSPROP_GETTER           0x10    /* property holds getter function */
#define JSPROP_SETTER           0x20    /* property holds setter function */
#define JSPROP_SHARED           0x40    /* don't allocate a value slot for this
                                           property; don't copy the property on
                                           set of the same-named property in an
                                           object that delegates to a prototype
                                           containing this property */
#define JSPROP_INDEX            0x80    /* name is actually (int) index */
#define JSPROP_SHORTID          0x100   /* set in JS_DefineProperty attrs
                                           if getters/setters use a shortid */
#define JSPROP_NATIVE_ACCESSORS 0x08    /* set in JSPropertyDescriptor.flags
                                           if getters/setters are JSNatives */


#define JSFUN_LAMBDA            0x08    /* expressed, not declared, function */
#define JSFUN_HEAVYWEIGHT       0x80    /* activation requires a Call object */

#define JSFUN_HEAVYWEIGHT_TEST(f)  ((f) & JSFUN_HEAVYWEIGHT)


#define JSFUN_CONSTRUCTOR     0x0200    /* native that can be called as a ctor
                                           without creating a this object */

#define JSFUN_FLAGS_MASK      0x07f8    /* overlay JSFUN_* attributes --
                                           bits 12-15 are used internally to
                                           flag interpreted functions */

#define JSFUN_STUB_GSOPS      0x1000    /* use JS_PropertyStub getter/setter
                                           instead of defaulting to class gsops
                                           for property holding function */












#define JSFUN_GENERIC_NATIVE    JSFUN_LAMBDA









extern JS_PUBLIC_API(JSBool)
JS_CallOnce(JSCallOnceType *once, JSInitCallback func);


extern JS_PUBLIC_API(int64_t)
JS_Now(void);


extern JS_PUBLIC_API(jsval)
JS_GetNaNValue(JSContext *cx);

extern JS_PUBLIC_API(jsval)
JS_GetNegativeInfinityValue(JSContext *cx);

extern JS_PUBLIC_API(jsval)
JS_GetPositiveInfinityValue(JSContext *cx);

extern JS_PUBLIC_API(jsval)
JS_GetEmptyStringValue(JSContext *cx);

extern JS_PUBLIC_API(JSString *)
JS_GetEmptyString(JSRuntime *rt);

































extern JS_PUBLIC_API(JSBool)
JS_ConvertArguments(JSContext *cx, unsigned argc, jsval *argv, const char *format,
                    ...);

#ifdef va_start
extern JS_PUBLIC_API(JSBool)
JS_ConvertArgumentsVA(JSContext *cx, unsigned argc, jsval *argv,
                      const char *format, va_list ap);
#endif

#ifdef JS_ARGUMENT_FORMATTER_DEFINED








































extern JS_PUBLIC_API(JSBool)
JS_AddArgumentFormatter(JSContext *cx, const char *format,
                        JSArgumentFormatter formatter);

extern JS_PUBLIC_API(void)
JS_RemoveArgumentFormatter(JSContext *cx, const char *format);

#endif 

extern JS_PUBLIC_API(JSBool)
JS_ConvertValue(JSContext *cx, jsval v, JSType type, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_ValueToObject(JSContext *cx, jsval v, JSObject **objp);

extern JS_PUBLIC_API(JSFunction *)
JS_ValueToFunction(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSFunction *)
JS_ValueToConstructor(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSString *)
JS_ValueToString(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSString *)
JS_ValueToSource(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSBool)
JS_ValueToNumber(JSContext *cx, jsval v, double *dp);

extern JS_PUBLIC_API(JSBool)
JS_DoubleIsInt32(double d, int32_t *ip);

extern JS_PUBLIC_API(int32_t)
JS_DoubleToInt32(double d);

extern JS_PUBLIC_API(uint32_t)
JS_DoubleToUint32(double d);





extern JS_PUBLIC_API(JSBool)
JS_ValueToECMAInt32(JSContext *cx, jsval v, int32_t *ip);





extern JS_PUBLIC_API(JSBool)
JS_ValueToECMAUint32(JSContext *cx, jsval v, uint32_t *ip);






extern JS_PUBLIC_API(JSBool)
JS_ValueToInt32(JSContext *cx, jsval v, int32_t *ip);




extern JS_PUBLIC_API(JSBool)
JS_ValueToUint16(JSContext *cx, jsval v, uint16_t *ip);

extern JS_PUBLIC_API(JSBool)
JS_ValueToBoolean(JSContext *cx, jsval v, JSBool *bp);

extern JS_PUBLIC_API(JSType)
JS_TypeOfValue(JSContext *cx, jsval v);

extern JS_PUBLIC_API(const char *)
JS_GetTypeName(JSContext *cx, JSType type);

extern JS_PUBLIC_API(JSBool)
JS_StrictlyEqual(JSContext *cx, jsval v1, jsval v2, JSBool *equal);

extern JS_PUBLIC_API(JSBool)
JS_LooselyEqual(JSContext *cx, jsval v1, jsval v2, JSBool *equal);

extern JS_PUBLIC_API(JSBool)
JS_SameValue(JSContext *cx, jsval v1, jsval v2, JSBool *same);


extern JS_PUBLIC_API(JSBool)
JS_IsBuiltinEvalFunction(JSFunction *fun);


extern JS_PUBLIC_API(JSBool)
JS_IsBuiltinFunctionConstructor(JSFunction *fun);










#define JS_NewRuntime       JS_Init
#define JS_DestroyRuntime   JS_Finish
#define JS_LockRuntime      JS_Lock
#define JS_UnlockRuntime    JS_Unlock

extern JS_PUBLIC_API(JSRuntime *)
JS_NewRuntime(uint32_t maxbytes);


#define JS_CommenceRuntimeShutDown(rt) ((void) 0)

extern JS_PUBLIC_API(void)
JS_DestroyRuntime(JSRuntime *rt);

extern JS_PUBLIC_API(void)
JS_ShutDown(void);

JS_PUBLIC_API(void *)
JS_GetRuntimePrivate(JSRuntime *rt);

extern JS_PUBLIC_API(JSRuntime *)
JS_GetRuntime(JSContext *cx);

JS_PUBLIC_API(void)
JS_SetRuntimePrivate(JSRuntime *rt, void *data);

extern JS_PUBLIC_API(void)
JS_BeginRequest(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_EndRequest(JSContext *cx);


extern JS_PUBLIC_API(void)
JS_YieldRequest(JSContext *cx);

extern JS_PUBLIC_API(unsigned)
JS_SuspendRequest(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_ResumeRequest(JSContext *cx, unsigned saveDepth);

extern JS_PUBLIC_API(JSBool)
JS_IsInRequest(JSRuntime *rt);

extern JS_PUBLIC_API(JSBool)
JS_IsInSuspendedRequest(JSRuntime *rt);

#ifdef __cplusplus
JS_END_EXTERN_C

inline bool
IsPoisonedId(jsid iden)
{
    if (JSID_IS_STRING(iden))
        return JS::IsPoisonedPtr(JSID_TO_STRING(iden));
    if (JSID_IS_OBJECT(iden))
        return JS::IsPoisonedPtr(JSID_TO_OBJECT(iden));
    return false;
}

class JSAutoRequest {
  public:
    JSAutoRequest(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx), mSaveDepth(0) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_BeginRequest(mContext);
    }
    ~JSAutoRequest() {
        JS_EndRequest(mContext);
    }

    void suspend() {
        mSaveDepth = JS_SuspendRequest(mContext);
    }
    void resume() {
        JS_ResumeRequest(mContext, mSaveDepth);
    }

  protected:
    JSContext *mContext;
    unsigned  mSaveDepth;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

#if 0
  private:
    static void *operator new(size_t) CPP_THROW_NEW { return 0; };
    static void operator delete(void *, size_t) { };
#endif
};

class JSAutoSuspendRequest {
  public:
    JSAutoSuspendRequest(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : mContext(cx), mSaveDepth(0) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        if (mContext) {
            mSaveDepth = JS_SuspendRequest(mContext);
        }
    }
    ~JSAutoSuspendRequest() {
        resume();
    }

    void resume() {
        if (mContext) {
            JS_ResumeRequest(mContext, mSaveDepth);
            mContext = 0;
        }
    }

  protected:
    JSContext *mContext;
    unsigned mSaveDepth;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

#if 0
  private:
    static void *operator new(size_t) CPP_THROW_NEW { return 0; };
    static void operator delete(void *, size_t) { };
#endif
};

class JSAutoCheckRequest {
  public:
    JSAutoCheckRequest(JSContext *cx JS_GUARD_OBJECT_NOTIFIER_PARAM) {
#if defined JS_THREADSAFE && defined DEBUG
        mContext = cx;
        JS_ASSERT(JS_IsInRequest(JS_GetRuntime(cx)));
#endif
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~JSAutoCheckRequest() {
#if defined JS_THREADSAFE && defined DEBUG
        JS_ASSERT(JS_IsInRequest(JS_GetRuntime(mContext)));
#endif
    }


  private:
#if defined JS_THREADSAFE && defined DEBUG
    JSContext *mContext;
#endif
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

JS_BEGIN_EXTERN_C
#endif

extern JS_PUBLIC_API(JSContextCallback)
JS_SetContextCallback(JSRuntime *rt, JSContextCallback cxCallback);

extern JS_PUBLIC_API(JSContext *)
JS_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern JS_PUBLIC_API(void)
JS_DestroyContext(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_DestroyContextNoGC(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_DestroyContextMaybeGC(JSContext *cx);

extern JS_PUBLIC_API(void *)
JS_GetContextPrivate(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetContextPrivate(JSContext *cx, void *data);

extern JS_PUBLIC_API(void *)
JS_GetSecondContextPrivate(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetSecondContextPrivate(JSContext *cx, void *data);

extern JS_PUBLIC_API(JSRuntime *)
JS_GetRuntime(JSContext *cx);

extern JS_PUBLIC_API(JSContext *)
JS_ContextIterator(JSRuntime *rt, JSContext **iterp);

extern JS_PUBLIC_API(JSVersion)
JS_GetVersion(JSContext *cx);

extern JS_PUBLIC_API(JSVersion)
JS_SetVersion(JSContext *cx, JSVersion version);

extern JS_PUBLIC_API(const char *)
JS_VersionToString(JSVersion version);

extern JS_PUBLIC_API(JSVersion)
JS_StringToVersion(const char *string);








#define JSOPTION_STRICT         JS_BIT(0)       /* warn on dubious practice */
#define JSOPTION_WERROR         JS_BIT(1)       /* convert warning to error */
#define JSOPTION_VAROBJFIX      JS_BIT(2)       /* make JS_EvaluateScript use
                                                   the last object on its 'obj'
                                                   param's scope chain as the
                                                   ECMA 'variables object' */
#define JSOPTION_PRIVATE_IS_NSISUPPORTS \
                                JS_BIT(3)       /* context private data points
                                                   to an nsISupports subclass */
#define JSOPTION_COMPILE_N_GO   JS_BIT(4)       /* caller of JS_Compile*Script
                                                   promises to execute compiled
                                                   script once only; enables
                                                   compile-time scope chain
                                                   resolution of consts. */
#define JSOPTION_ATLINE         JS_BIT(5)       /* //@line number ["filename"]
                                                   option supported for the
                                                   XUL preprocessor and kindred
                                                   beasts. */
#define JSOPTION_XML            JS_BIT(6)       /* EMCAScript for XML support:
                                                   parse <!-- --> as a token,
                                                   not backward compatible with
                                                   the comment-hiding hack used
                                                   in HTML script tags. */
#define JSOPTION_DONT_REPORT_UNCAUGHT \
                                JS_BIT(8)       /* When returning from the
                                                   outermost API call, prevent
                                                   uncaught exceptions from
                                                   being converted to error
                                                   reports */

#define JSOPTION_RELIMIT        JS_BIT(9)       /* Throw exception on any
                                                   regular expression which
                                                   backtracks more than n^3
                                                   times, where n is length
                                                   of the input string */




#define JSOPTION_NO_SCRIPT_RVAL JS_BIT(12)      /* A promise to the compiler
                                                   that a null rval out-param
                                                   will be passed to each call
                                                   to JS_ExecuteScript. */
#define JSOPTION_UNROOTED_GLOBAL JS_BIT(13)     /* The GC will not root the
                                                   contexts' global objects
                                                   (see JS_GetGlobalObject),
                                                   leaving that up to the
                                                   embedding. */

#define JSOPTION_METHODJIT      JS_BIT(14)      /* Whole-method JIT. */



#define JSOPTION_METHODJIT_ALWAYS \
                                JS_BIT(16)      /* Always whole-method JIT,
                                                   don't tune at run-time. */
#define JSOPTION_PCCOUNT        JS_BIT(17)      

#define JSOPTION_TYPE_INFERENCE JS_BIT(18)      /* Perform type inference. */


#define JSCOMPILEOPTION_MASK    (JSOPTION_XML)

#define JSRUNOPTION_MASK        (JS_BITMASK(19) & ~JSCOMPILEOPTION_MASK)
#define JSALLOPTION_MASK        (JSCOMPILEOPTION_MASK | JSRUNOPTION_MASK)

extern JS_PUBLIC_API(uint32_t)
JS_GetOptions(JSContext *cx);

extern JS_PUBLIC_API(uint32_t)
JS_SetOptions(JSContext *cx, uint32_t options);

extern JS_PUBLIC_API(uint32_t)
JS_ToggleOptions(JSContext *cx, uint32_t options);

extern JS_PUBLIC_API(void)
JS_SetJitHardening(JSRuntime *rt, JSBool enabled);

extern JS_PUBLIC_API(const char *)
JS_GetImplementationVersion(void);

extern JS_PUBLIC_API(JSCompartmentCallback)
JS_SetCompartmentCallback(JSRuntime *rt, JSCompartmentCallback callback);

extern JS_PUBLIC_API(JSWrapObjectCallback)
JS_SetWrapObjectCallbacks(JSRuntime *rt,
                          JSWrapObjectCallback callback,
                          JSPreWrapCallback precallback);

extern JS_PUBLIC_API(JSCrossCompartmentCall *)
JS_EnterCrossCompartmentCall(JSContext *cx, JSObject *target);

extern JS_PUBLIC_API(void)
JS_LeaveCrossCompartmentCall(JSCrossCompartmentCall *call);

extern JS_PUBLIC_API(void)
JS_SetCompartmentPrivate(JSCompartment *compartment, void *data);

extern JS_PUBLIC_API(void *)
JS_GetCompartmentPrivate(JSCompartment *compartment);

extern JS_PUBLIC_API(JSBool)
JS_WrapObject(JSContext *cx, JSObject **objp);

extern JS_PUBLIC_API(JSBool)
JS_WrapValue(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(JSObject *)
JS_TransplantObject(JSContext *cx, JSObject *origobj, JSObject *target);

extern JS_FRIEND_API(JSObject *)
js_TransplantObjectWithWrapper(JSContext *cx,
                               JSObject *origobj,
                               JSObject *origwrapper,
                               JSObject *targetobj,
                               JSObject *targetwrapper);

extern JS_FRIEND_API(JSObject *)
js_TransplantObjectWithWrapper(JSContext *cx,
                               JSObject *origobj,
                               JSObject *origwrapper,
                               JSObject *targetobj,
                               JSObject *targetwrapper);

#ifdef __cplusplus
JS_END_EXTERN_C

class JS_PUBLIC_API(JSAutoEnterCompartment)
{
    







    void* bytes[sizeof(void*) == 4 && MOZ_ALIGNOF(uint64_t) == 8 ? 16 : 13];

    







    enum State {
        STATE_UNENTERED,
        STATE_SAME_COMPARTMENT,
        STATE_OTHER_COMPARTMENT
    } state;

  public:
    JSAutoEnterCompartment() : state(STATE_UNENTERED) {}

    bool enter(JSContext *cx, JSObject *target);

    void enterAndIgnoreErrors(JSContext *cx, JSObject *target);

    bool entered() const { return state != STATE_UNENTERED; }

    ~JSAutoEnterCompartment();
};

JS_BEGIN_EXTERN_C
#endif

typedef void (*JSIterateCompartmentCallback)(JSRuntime *rt, void *data, JSCompartment *compartment);






extern JS_PUBLIC_API(void)
JS_IterateCompartments(JSRuntime *rt, void *data,
                       JSIterateCompartmentCallback compartmentCallback);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalObject(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetGlobalObject(JSContext *cx, JSObject *obj);








extern JS_PUBLIC_API(JSBool)
JS_InitStandardClasses(JSContext *cx, JSObject *obj);














extern JS_PUBLIC_API(JSBool)
JS_ResolveStandardClass(JSContext *cx, JSObject *obj, jsid id,
                        JSBool *resolved);

extern JS_PUBLIC_API(JSBool)
JS_EnumerateStandardClasses(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSIdArray *)
JS_EnumerateResolvedStandardClasses(JSContext *cx, JSObject *obj,
                                    JSIdArray *ida);

extern JS_PUBLIC_API(JSBool)
JS_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);





extern JS_PUBLIC_API(JSObject *)
JS_GetFunctionPrototype(JSContext *cx, JSObject *forObj);





extern JS_PUBLIC_API(JSObject *)
JS_GetObjectPrototype(JSContext *cx, JSObject *forObj);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalForObject(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalForScopeChain(JSContext *cx);




extern JS_PUBLIC_API(JSObject *)
JS_InitReflect(JSContext *cx, JSObject *global);

#ifdef JS_HAS_CTYPES




extern JS_PUBLIC_API(JSBool)
JS_InitCTypesClass(JSContext *cx, JSObject *global);






typedef char *
(* JSCTypesUnicodeToNativeFun)(JSContext *cx, const jschar *source, size_t slen);






struct JSCTypesCallbacks {
    JSCTypesUnicodeToNativeFun unicodeToNative;
};

typedef struct JSCTypesCallbacks JSCTypesCallbacks;







extern JS_PUBLIC_API(void)
JS_SetCTypesCallbacks(JSObject *ctypesObj, JSCTypesCallbacks *callbacks);
#endif

typedef JSBool
(* JSEnumerateDiagnosticMemoryCallback)(void *ptr, size_t length);





extern JS_PUBLIC_API(void)
JS_EnumerateDiagnosticMemoryRegions(JSEnumerateDiagnosticMemoryCallback callback);





























#define JS_CALLEE(cx,vp)        ((vp)[0])
#define JS_THIS(cx,vp)          JS_ComputeThis(cx, vp)
#define JS_THIS_OBJECT(cx,vp)   (JSVAL_TO_OBJECT(JS_THIS(cx,vp)))
#define JS_ARGV(cx,vp)          ((vp) + 2)
#define JS_RVAL(cx,vp)          (*(vp))
#define JS_SET_RVAL(cx,vp,v)    (*(vp) = (v))

extern JS_PUBLIC_API(jsval)
JS_ComputeThis(JSContext *cx, jsval *vp);

#ifdef __cplusplus
#undef JS_THIS
static inline jsval
JS_THIS(JSContext *cx, jsval *vp)
{
    return JSVAL_IS_PRIMITIVE(vp[1]) ? JS_ComputeThis(cx, vp) : vp[1];
}
#endif













#define JS_THIS_VALUE(cx,vp)    ((vp)[1])

extern JS_PUBLIC_API(void)
JS_MallocInCompartment(JSCompartment *comp, size_t nbytes);

extern JS_PUBLIC_API(void)
JS_FreeInCompartment(JSCompartment *comp, size_t nbytes);

extern JS_PUBLIC_API(void *)
JS_malloc(JSContext *cx, size_t nbytes);

extern JS_PUBLIC_API(void *)
JS_realloc(JSContext *cx, void *p, size_t nbytes);

extern JS_PUBLIC_API(void)
JS_free(JSContext *cx, void *p);

extern JS_PUBLIC_API(void)
JS_updateMallocCounter(JSContext *cx, size_t nbytes);

extern JS_PUBLIC_API(char *)
JS_strdup(JSContext *cx, const char *s);

extern JS_PUBLIC_API(JSBool)
JS_NewNumberValue(JSContext *cx, double d, jsval *rval);


























extern JS_PUBLIC_API(JSBool)
JS_AddValueRoot(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_AddStringRoot(JSContext *cx, JSString **rp);

extern JS_PUBLIC_API(JSBool)
JS_AddObjectRoot(JSContext *cx, JSObject **rp);

extern JS_PUBLIC_API(JSBool)
JS_AddGCThingRoot(JSContext *cx, void **rp);

#ifdef NAME_ALL_GC_ROOTS
#define JS_DEFINE_TO_TOKEN(def) #def
#define JS_DEFINE_TO_STRING(def) JS_DEFINE_TO_TOKEN(def)
#define JS_AddValueRoot(cx,vp) JS_AddNamedValueRoot((cx), (vp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#define JS_AddStringRoot(cx,rp) JS_AddNamedStringRoot((cx), (rp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#define JS_AddObjectRoot(cx,rp) JS_AddNamedObjectRoot((cx), (rp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#define JS_AddGCThingRoot(cx,rp) JS_AddNamedGCThingRoot((cx), (rp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#endif

extern JS_PUBLIC_API(JSBool)
JS_AddNamedValueRoot(JSContext *cx, jsval *vp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedStringRoot(JSContext *cx, JSString **rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedObjectRoot(JSContext *cx, JSObject **rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedScriptRoot(JSContext *cx, JSScript **rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedGCThingRoot(JSContext *cx, void **rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_RemoveValueRoot(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_RemoveStringRoot(JSContext *cx, JSString **rp);

extern JS_PUBLIC_API(JSBool)
JS_RemoveObjectRoot(JSContext *cx, JSObject **rp);

extern JS_PUBLIC_API(JSBool)
JS_RemoveScriptRoot(JSContext *cx, JSScript **rp);

extern JS_PUBLIC_API(JSBool)
JS_RemoveGCThingRoot(JSContext *cx, void **rp);



extern JS_FRIEND_API(JSBool)
js_AddRootRT(JSRuntime *rt, jsval *vp, const char *name);

extern JS_FRIEND_API(JSBool)
js_AddGCThingRootRT(JSRuntime *rt, void **rp, const char *name);

extern JS_FRIEND_API(JSBool)
js_RemoveRoot(JSRuntime *rt, void *rp);





extern JS_NEVER_INLINE JS_PUBLIC_API(void)
JS_AnchorPtr(void *p);





#define JS_TYPED_ROOTING_API


#define JS_EnterLocalRootScope(cx) (JS_TRUE)
#define JS_LeaveLocalRootScope(cx) ((void) 0)
#define JS_LeaveLocalRootScopeWithResult(cx, rval) ((void) 0)
#define JS_ForgetLocalRoot(cx, thing) ((void) 0)

typedef enum JSGCRootType {
    JS_GC_ROOT_VALUE_PTR,
    JS_GC_ROOT_GCTHING_PTR
} JSGCRootType;

#ifdef DEBUG
extern JS_PUBLIC_API(void)
JS_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, JSGCRootType type, void *data),
                  void *data);
#endif























#define JS_MAP_GCROOT_NEXT      0       /* continue mapping entries */
#define JS_MAP_GCROOT_STOP      1       /* stop mapping entries */
#define JS_MAP_GCROOT_REMOVE    2       /* remove and free the current entry */

typedef int
(* JSGCRootMapFun)(void *rp, JSGCRootType type, const char *name, void *data);

extern JS_PUBLIC_API(uint32_t)
JS_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data);

extern JS_PUBLIC_API(JSBool)
JS_LockGCThing(JSContext *cx, void *thing);

extern JS_PUBLIC_API(JSBool)
JS_LockGCThingRT(JSRuntime *rt, void *thing);

extern JS_PUBLIC_API(JSBool)
JS_UnlockGCThing(JSContext *cx, void *thing);

extern JS_PUBLIC_API(JSBool)
JS_UnlockGCThingRT(JSRuntime *rt, void *thing);








extern JS_PUBLIC_API(void)
JS_SetExtraGCRootsTracer(JSRuntime *rt, JSTraceDataOp traceOp, void *data);














static JS_ALWAYS_INLINE JSBool
JSVAL_IS_TRACEABLE(jsval v)
{
    return JSVAL_IS_TRACEABLE_IMPL(JSVAL_TO_IMPL(v));
}

static JS_ALWAYS_INLINE void *
JSVAL_TO_TRACEABLE(jsval v)
{
    return JSVAL_TO_GCTHING(v);
}

static JS_ALWAYS_INLINE JSGCTraceKind
JSVAL_TRACE_KIND(jsval v)
{
    JS_ASSERT(JSVAL_IS_GCTHING(v));
    return (JSGCTraceKind) JSVAL_TRACE_KIND_IMPL(JSVAL_TO_IMPL(v));
}

















typedef void
(* JSTraceCallback)(JSTracer *trc, void **thingp, JSGCTraceKind kind);

struct JSTracer {
    JSRuntime           *runtime;
    JSTraceCallback     callback;
    JSTraceNamePrinter  debugPrinter;
    const void          *debugPrintArg;
    size_t              debugPrintIndex;
    JSBool              eagerlyTraceWeakMaps;
};







extern JS_PUBLIC_API(void)
JS_CallTracer(JSTracer *trc, void *thing, JSGCTraceKind kind);

















#ifdef DEBUG
# define JS_SET_TRACING_DETAILS(trc, printer, arg, index)                     \
    JS_BEGIN_MACRO                                                            \
        (trc)->debugPrinter = (printer);                                      \
        (trc)->debugPrintArg = (arg);                                         \
        (trc)->debugPrintIndex = (index);                                     \
    JS_END_MACRO
#else
# define JS_SET_TRACING_DETAILS(trc, printer, arg, index)                     \
    JS_BEGIN_MACRO                                                            \
    JS_END_MACRO
#endif





# define JS_SET_TRACING_INDEX(trc, name, index)                               \
    JS_SET_TRACING_DETAILS(trc, NULL, name, index)




# define JS_SET_TRACING_NAME(trc, name)                                       \
    JS_SET_TRACING_DETAILS(trc, NULL, name, (size_t)-1)





# define JS_CALL_TRACER(trc, thing, kind, name)                               \
    JS_BEGIN_MACRO                                                            \
        JS_SET_TRACING_NAME(trc, name);                                       \
        JS_CallTracer((trc), (thing), (kind));                                \
    JS_END_MACRO





#define JS_CALL_VALUE_TRACER(trc, val, name)                                  \
    JS_BEGIN_MACRO                                                            \
        if (JSVAL_IS_TRACEABLE(val)) {                                        \
            JS_CALL_TRACER((trc), JSVAL_TO_GCTHING(val),                      \
                           JSVAL_TRACE_KIND(val), name);                      \
        }                                                                     \
    JS_END_MACRO

#define JS_CALL_OBJECT_TRACER(trc, object, name)                              \
    JS_BEGIN_MACRO                                                            \
        JSObject *obj_ = (object);                                            \
        JS_ASSERT(obj_);                                                      \
        JS_CALL_TRACER((trc), obj_, JSTRACE_OBJECT, name);                    \
    JS_END_MACRO

#define JS_CALL_STRING_TRACER(trc, string, name)                              \
    JS_BEGIN_MACRO                                                            \
        JSString *str_ = (string);                                            \
        JS_ASSERT(str_);                                                      \
        JS_CALL_TRACER((trc), str_, JSTRACE_STRING, name);                    \
    JS_END_MACRO




extern JS_PUBLIC_API(void)
JS_TracerInit(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

#ifdef DEBUG

extern JS_PUBLIC_API(void)
JS_PrintTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                       void *thing, JSGCTraceKind kind, JSBool includeDetails);

extern JS_PUBLIC_API(const char *)
JS_GetTraceEdgeName(JSTracer *trc, char *buffer, int bufferSize);
















extern JS_PUBLIC_API(JSBool)
JS_DumpHeap(JSRuntime *rt, FILE *fp, void* startThing, JSGCTraceKind kind,
            void *thingToFind, size_t maxDepth, void *thingToIgnore);

#endif




extern JS_PUBLIC_API(void)
JS_GC(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_CompartmentGC(JSContext *cx, JSCompartment *comp);

extern JS_PUBLIC_API(void)
JS_MaybeGC(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetGCCallback(JSRuntime *rt, JSGCCallback cb);

extern JS_PUBLIC_API(void)
JS_SetFinalizeCallback(JSRuntime *rt, JSFinalizeCallback cb);

extern JS_PUBLIC_API(JSBool)
JS_IsGCMarkingTracer(JSTracer *trc);

extern JS_PUBLIC_API(JSBool)
JS_IsAboutToBeFinalized(void *thing);

typedef enum JSGCParamKey {
    
    JSGC_MAX_BYTES          = 0,

    
    JSGC_MAX_MALLOC_BYTES   = 1,

    
    JSGC_BYTES = 3,

    
    JSGC_NUMBER = 4,

    
    JSGC_MAX_CODE_CACHE_BYTES = 5,

    
    JSGC_MODE = 6,

    
    JSGC_UNUSED_CHUNKS = 7,

    
    JSGC_TOTAL_CHUNKS = 8,

    
    JSGC_SLICE_TIME_BUDGET = 9,

    
    JSGC_MARK_STACK_LIMIT = 10
} JSGCParamKey;

typedef enum JSGCMode {
    
    JSGC_MODE_GLOBAL = 0,

    
    JSGC_MODE_COMPARTMENT = 1,

    



    JSGC_MODE_INCREMENTAL = 2
} JSGCMode;

extern JS_PUBLIC_API(void)
JS_SetGCParameter(JSRuntime *rt, JSGCParamKey key, uint32_t value);

extern JS_PUBLIC_API(uint32_t)
JS_GetGCParameter(JSRuntime *rt, JSGCParamKey key);

extern JS_PUBLIC_API(void)
JS_SetGCParameterForThread(JSContext *cx, JSGCParamKey key, uint32_t value);

extern JS_PUBLIC_API(uint32_t)
JS_GetGCParameterForThread(JSContext *cx, JSGCParamKey key);







extern JS_PUBLIC_API(void)
JS_FlushCaches(JSContext *cx);





extern JS_PUBLIC_API(JSString *)
JS_NewExternalString(JSContext *cx, const jschar *chars, size_t length,
                     const JSStringFinalizer *fin);





extern JS_PUBLIC_API(JSBool)
JS_IsExternalString(JSString *str);





extern JS_PUBLIC_API(const JSStringFinalizer *)
JS_GetExternalStringFinalizer(JSString *str);





extern JS_PUBLIC_API(void)
JS_SetNativeStackQuota(JSRuntime *cx, size_t stackSize);






typedef void (*JSClassInternal)();

struct JSClass {
    const char          *name;
    uint32_t            flags;

    
    JSPropertyOp        addProperty;
    JSPropertyOp        delProperty;
    JSPropertyOp        getProperty;
    JSStrictPropertyOp  setProperty;
    JSEnumerateOp       enumerate;
    JSResolveOp         resolve;
    JSConvertOp         convert;
    JSFinalizeOp        finalize;

    
    JSCheckAccessOp     checkAccess;
    JSNative            call;
    JSNative            construct;
    JSHasInstanceOp     hasInstance;
    JSTraceOp           trace;

    void                *reserved[40];
};

#define JSCLASS_HAS_PRIVATE             (1<<0)  /* objects have private slot */
#define JSCLASS_NEW_ENUMERATE           (1<<1)  /* has JSNewEnumerateOp hook */
#define JSCLASS_NEW_RESOLVE             (1<<2)  /* has JSNewResolveOp hook */
#define JSCLASS_PRIVATE_IS_NSISUPPORTS  (1<<3)  /* private is (nsISupports *) */
#define JSCLASS_NEW_RESOLVE_GETS_START  (1<<4)  /* JSNewResolveOp gets starting
                                                   object in prototype chain
                                                   passed in via *objp in/out
                                                   parameter */
#define JSCLASS_IMPLEMENTS_BARRIERS     (1<<5)  

#define JSCLASS_DOCUMENT_OBSERVER       (1<<6)  /* DOM document observer */






#define JSCLASS_RESERVED_SLOTS_SHIFT    8       /* room for 8 flags below */
#define JSCLASS_RESERVED_SLOTS_WIDTH    8       /* and 16 above this field */
#define JSCLASS_RESERVED_SLOTS_MASK     JS_BITMASK(JSCLASS_RESERVED_SLOTS_WIDTH)
#define JSCLASS_HAS_RESERVED_SLOTS(n)   (((n) & JSCLASS_RESERVED_SLOTS_MASK)  \
                                         << JSCLASS_RESERVED_SLOTS_SHIFT)
#define JSCLASS_RESERVED_SLOTS(clasp)   (((clasp)->flags                      \
                                          >> JSCLASS_RESERVED_SLOTS_SHIFT)    \
                                         & JSCLASS_RESERVED_SLOTS_MASK)

#define JSCLASS_HIGH_FLAGS_SHIFT        (JSCLASS_RESERVED_SLOTS_SHIFT +       \
                                         JSCLASS_RESERVED_SLOTS_WIDTH)





#define JSCLASS_FOR_OF_ITERATION        (1<<(JSCLASS_HIGH_FLAGS_SHIFT+0))

#define JSCLASS_IS_ANONYMOUS            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+1))
#define JSCLASS_IS_GLOBAL               (1<<(JSCLASS_HIGH_FLAGS_SHIFT+2))
#define JSCLASS_INTERNAL_FLAG2          (1<<(JSCLASS_HIGH_FLAGS_SHIFT+3))
#define JSCLASS_INTERNAL_FLAG3          (1<<(JSCLASS_HIGH_FLAGS_SHIFT+4))


#define JSCLASS_FREEZE_PROTO            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+5))
#define JSCLASS_FREEZE_CTOR             (1<<(JSCLASS_HIGH_FLAGS_SHIFT+6))

#define JSCLASS_XPCONNECT_GLOBAL        (1<<(JSCLASS_HIGH_FLAGS_SHIFT+7))


#define JSGLOBAL_FLAGS_CLEARED          0x1












#define JSCLASS_GLOBAL_SLOT_COUNT      (JSProto_LIMIT * 3 + 8)
#define JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(n)                                    \
    (JSCLASS_IS_GLOBAL | JSCLASS_HAS_RESERVED_SLOTS(JSCLASS_GLOBAL_SLOT_COUNT + (n)))
#define JSCLASS_GLOBAL_FLAGS                                                  \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(0)
#define JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(clasp)                              \
  (((clasp)->flags & JSCLASS_IS_GLOBAL)                                       \
   && JSCLASS_RESERVED_SLOTS(clasp) >= JSCLASS_GLOBAL_SLOT_COUNT)


#define JSCLASS_CACHED_PROTO_SHIFT      (JSCLASS_HIGH_FLAGS_SHIFT + 8)
#define JSCLASS_CACHED_PROTO_WIDTH      8
#define JSCLASS_CACHED_PROTO_MASK       JS_BITMASK(JSCLASS_CACHED_PROTO_WIDTH)
#define JSCLASS_HAS_CACHED_PROTO(key)   ((key) << JSCLASS_CACHED_PROTO_SHIFT)
#define JSCLASS_CACHED_PROTO_KEY(clasp) ((JSProtoKey)                         \
                                         (((clasp)->flags                     \
                                           >> JSCLASS_CACHED_PROTO_SHIFT)     \
                                          & JSCLASS_CACHED_PROTO_MASK))


#define JSCLASS_NO_INTERNAL_MEMBERS     {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
#define JSCLASS_NO_OPTIONAL_MEMBERS     0,0,0,0,0,JSCLASS_NO_INTERNAL_MEMBERS

extern JS_PUBLIC_API(int)
JS_IdArrayLength(JSContext *cx, JSIdArray *ida);

extern JS_PUBLIC_API(jsid)
JS_IdArrayGet(JSContext *cx, JSIdArray *ida, int index);

extern JS_PUBLIC_API(void)
JS_DestroyIdArray(JSContext *cx, JSIdArray *ida);

#ifdef __cplusplus

namespace JS {

class AutoIdArray : private AutoGCRooter {
  public:
    AutoIdArray(JSContext *cx, JSIdArray *ida JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, IDARRAY), context(cx), idArray(ida)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoIdArray() {
        if (idArray)
            JS_DestroyIdArray(context, idArray);
    }
    bool operator!() {
        return !idArray;
    }
    jsid operator[](size_t i) const {
        JS_ASSERT(idArray);
        JS_ASSERT(i < length());
        return JS_IdArrayGet(context, idArray, i);
    }
    size_t length() const {
        return JS_IdArrayLength(context, idArray);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    JSIdArray *steal() {
        JSIdArray *copy = idArray;
        idArray = NULL;
        return copy;
    }

  protected:
    inline void trace(JSTracer *trc);

  private:
    JSContext *context;
    JSIdArray *idArray;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoIdArray(AutoIdArray &ida) MOZ_DELETE;
    void operator=(AutoIdArray &ida) MOZ_DELETE;
};

} 

#endif 

extern JS_PUBLIC_API(JSBool)
JS_ValueToId(JSContext *cx, jsval v, jsid *idp);

extern JS_PUBLIC_API(JSBool)
JS_IdToValue(JSContext *cx, jsid id, jsval *vp);




#define JSRESOLVE_QUALIFIED     0x01    /* resolve a qualified property id */
#define JSRESOLVE_ASSIGNING     0x02    /* resolve on the left of assignment */
#define JSRESOLVE_DETECTING     0x04    /* 'if (o.p)...' or '(o.p) ?...:...' */
#define JSRESOLVE_DECLARING     0x08    /* var, const, or function prolog op */
#define JSRESOLVE_CLASSNAME     0x10    /* class name used when constructing */
#define JSRESOLVE_WITH          0x20    /* resolve inside a with statement */







extern JS_PUBLIC_API(JSBool)
JS_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_PropertyStub(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_StrictPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_EnumerateStub(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_ResolveStub(JSContext *cx, JSObject *obj, jsid id);

extern JS_PUBLIC_API(JSBool)
JS_ConvertStub(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

extern JS_PUBLIC_API(void)
JS_FinalizeStub(JSContext *cx, JSObject *obj);

struct JSConstDoubleSpec {
    double          dval;
    const char      *name;
    uint8_t         flags;
    uint8_t         spare[3];
};






struct JSPropertySpec {
    const char            *name;
    int8_t                tinyid;
    uint8_t               flags;
    JSPropertyOp          getter;
    JSStrictPropertyOp    setter;
};

struct JSFunctionSpec {
    const char      *name;
    JSNative        call;
    uint16_t        nargs;
    uint16_t        flags;
};





#define JS_FS_END JS_FS(NULL,NULL,0,0)






#define JS_FS(name,call,nargs,flags)                                          \
    {name, call, nargs, flags}
#define JS_FN(name,call,nargs,flags)                                          \
    {name, call, nargs, (flags) | JSFUN_STUB_GSOPS}

extern JS_PUBLIC_API(JSObject *)
JS_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             JSClass *clasp, JSNative constructor, unsigned nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);





extern JS_PUBLIC_API(JSBool)
JS_LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto);

extern JS_PUBLIC_API(JSClass *)
JS_GetClass(JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_InstanceOf(JSContext *cx, JSObject *obj, JSClass *clasp, jsval *argv);

extern JS_PUBLIC_API(JSBool)
JS_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

extern JS_PUBLIC_API(void *)
JS_GetPrivate(JSObject *obj);

extern JS_PUBLIC_API(void)
JS_SetPrivate(JSObject *obj, void *data);

extern JS_PUBLIC_API(void *)
JS_GetInstancePrivate(JSContext *cx, JSObject *obj, JSClass *clasp,
                      jsval *argv);

extern JS_PUBLIC_API(JSObject *)
JS_GetPrototype(JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_SetPrototype(JSContext *cx, JSObject *obj, JSObject *proto);

extern JS_PUBLIC_API(JSObject *)
JS_GetParent(JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_SetParent(JSContext *cx, JSObject *obj, JSObject *parent);

extern JS_PUBLIC_API(JSObject *)
JS_GetConstructor(JSContext *cx, JSObject *proto);






extern JS_PUBLIC_API(JSBool)
JS_GetObjectId(JSContext *cx, JSObject *obj, jsid *idp);

extern JS_PUBLIC_API(JSObject *)
JS_NewGlobalObject(JSContext *cx, JSClass *clasp);

extern JS_PUBLIC_API(JSObject *)
JS_NewCompartmentAndGlobalObject(JSContext *cx, JSClass *clasp, JSPrincipals *principals);

extern JS_PUBLIC_API(JSObject *)
JS_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);


extern JS_PUBLIC_API(JSBool)
JS_IsExtensible(JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_IsNative(JSObject *obj);

extern JS_PUBLIC_API(JSRuntime *)
JS_GetObjectRuntime(JSObject *obj);





extern JS_PUBLIC_API(JSObject *)
JS_NewObjectWithGivenProto(JSContext *cx, JSClass *clasp, JSObject *proto,
                           JSObject *parent);






extern JS_PUBLIC_API(JSBool)
JS_DeepFreezeObject(JSContext *cx, JSObject *obj);




extern JS_PUBLIC_API(JSBool)
JS_FreezeObject(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSObject *)
JS_ConstructObject(JSContext *cx, JSClass *clasp, JSObject *parent);

extern JS_PUBLIC_API(JSObject *)
JS_ConstructObjectWithArguments(JSContext *cx, JSClass *clasp, JSObject *parent,
                                unsigned argc, jsval *argv);

extern JS_PUBLIC_API(JSObject *)
JS_New(JSContext *cx, JSObject *ctor, unsigned argc, jsval *argv);

extern JS_PUBLIC_API(JSObject *)
JS_DefineObject(JSContext *cx, JSObject *obj, const char *name, JSClass *clasp,
                JSObject *proto, unsigned attrs);

extern JS_PUBLIC_API(JSBool)
JS_DefineConstDoubles(JSContext *cx, JSObject *obj, JSConstDoubleSpec *cds);

extern JS_PUBLIC_API(JSBool)
JS_DefineProperties(JSContext *cx, JSObject *obj, JSPropertySpec *ps);

extern JS_PUBLIC_API(JSBool)
JS_DefineProperty(JSContext *cx, JSObject *obj, const char *name, jsval value,
                  JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

extern JS_PUBLIC_API(JSBool)
JS_DefinePropertyById(JSContext *cx, JSObject *obj, jsid id, jsval value,
                      JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

extern JS_PUBLIC_API(JSBool)
JS_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id, jsval descriptor, JSBool *bp);







extern JS_PUBLIC_API(JSBool)
JS_GetPropertyAttributes(JSContext *cx, JSObject *obj, const char *name,
                         unsigned *attrsp, JSBool *foundp);






extern JS_PUBLIC_API(JSBool)
JS_GetPropertyAttrsGetterAndSetter(JSContext *cx, JSObject *obj,
                                   const char *name,
                                   unsigned *attrsp, JSBool *foundp,
                                   JSPropertyOp *getterp,
                                   JSStrictPropertyOp *setterp);

extern JS_PUBLIC_API(JSBool)
JS_GetPropertyAttrsGetterAndSetterById(JSContext *cx, JSObject *obj,
                                       jsid id,
                                       unsigned *attrsp, JSBool *foundp,
                                       JSPropertyOp *getterp,
                                       JSStrictPropertyOp *setterp);







extern JS_PUBLIC_API(JSBool)
JS_SetPropertyAttributes(JSContext *cx, JSObject *obj, const char *name,
                         unsigned attrs, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_DefinePropertyWithTinyId(JSContext *cx, JSObject *obj, const char *name,
                            int8_t tinyid, jsval value,
                            JSPropertyOp getter, JSStrictPropertyOp setter,
                            unsigned attrs);

extern JS_PUBLIC_API(JSBool)
JS_AlreadyHasOwnProperty(JSContext *cx, JSObject *obj, const char *name,
                         JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_AlreadyHasOwnPropertyById(JSContext *cx, JSObject *obj, jsid id,
                             JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_HasProperty(JSContext *cx, JSObject *obj, const char *name, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_HasPropertyById(JSContext *cx, JSObject *obj, jsid id, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_LookupProperty(JSContext *cx, JSObject *obj, const char *name, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_LookupPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, const char *name,
                           unsigned flags, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_LookupPropertyWithFlagsById(JSContext *cx, JSObject *obj, jsid id,
                               unsigned flags, JSObject **objp, jsval *vp);

struct JSPropertyDescriptor {
    JSObject           *obj;
    unsigned              attrs;
    JSPropertyOp       getter;
    JSStrictPropertyOp setter;
    jsval              value;
    unsigned              shortid;
};






extern JS_PUBLIC_API(JSBool)
JS_GetPropertyDescriptorById(JSContext *cx, JSObject *obj, jsid id, unsigned flags,
                             JSPropertyDescriptor *desc);

extern JS_PUBLIC_API(JSBool)
JS_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetProperty(JSContext *cx, JSObject *obj, const char *name, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetPropertyDefault(JSContext *cx, JSObject *obj, const char *name, jsval def, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetPropertyByIdDefault(JSContext *cx, JSObject *obj, jsid id, jsval def, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_ForwardGetPropertyTo(JSContext *cx, JSObject *obj, jsid id, JSObject *onBehalfOf, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetMethodById(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                 jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetMethod(JSContext *cx, JSObject *obj, const char *name, JSObject **objp,
             jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetProperty(JSContext *cx, JSObject *obj, const char *name, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetPropertyById(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_DeleteProperty(JSContext *cx, JSObject *obj, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_DeleteProperty2(JSContext *cx, JSObject *obj, const char *name,
                   jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_DeletePropertyById(JSContext *cx, JSObject *obj, jsid id);

extern JS_PUBLIC_API(JSBool)
JS_DeletePropertyById2(JSContext *cx, JSObject *obj, jsid id, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_DefineUCProperty(JSContext *cx, JSObject *obj,
                    const jschar *name, size_t namelen, jsval value,
                    JSPropertyOp getter, JSStrictPropertyOp setter,
                    unsigned attrs);







extern JS_PUBLIC_API(JSBool)
JS_GetUCPropertyAttributes(JSContext *cx, JSObject *obj,
                           const jschar *name, size_t namelen,
                           unsigned *attrsp, JSBool *foundp);






extern JS_PUBLIC_API(JSBool)
JS_GetUCPropertyAttrsGetterAndSetter(JSContext *cx, JSObject *obj,
                                     const jschar *name, size_t namelen,
                                     unsigned *attrsp, JSBool *foundp,
                                     JSPropertyOp *getterp,
                                     JSStrictPropertyOp *setterp);







extern JS_PUBLIC_API(JSBool)
JS_SetUCPropertyAttributes(JSContext *cx, JSObject *obj,
                           const jschar *name, size_t namelen,
                           unsigned attrs, JSBool *foundp);


extern JS_PUBLIC_API(JSBool)
JS_DefineUCPropertyWithTinyId(JSContext *cx, JSObject *obj,
                              const jschar *name, size_t namelen,
                              int8_t tinyid, jsval value,
                              JSPropertyOp getter, JSStrictPropertyOp setter,
                              unsigned attrs);

extern JS_PUBLIC_API(JSBool)
JS_AlreadyHasOwnUCProperty(JSContext *cx, JSObject *obj, const jschar *name,
                           size_t namelen, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_HasUCProperty(JSContext *cx, JSObject *obj,
                 const jschar *name, size_t namelen,
                 JSBool *vp);

extern JS_PUBLIC_API(JSBool)
JS_LookupUCProperty(JSContext *cx, JSObject *obj,
                    const jschar *name, size_t namelen,
                    jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetUCProperty(JSContext *cx, JSObject *obj,
                 const jschar *name, size_t namelen,
                 jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetUCProperty(JSContext *cx, JSObject *obj,
                 const jschar *name, size_t namelen,
                 jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_DeleteUCProperty2(JSContext *cx, JSObject *obj,
                     const jschar *name, size_t namelen,
                     jsval *rval);

extern JS_PUBLIC_API(JSObject *)
JS_NewArrayObject(JSContext *cx, int length, jsval *vector);

extern JS_PUBLIC_API(JSBool)
JS_IsArrayObject(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_GetArrayLength(JSContext *cx, JSObject *obj, uint32_t *lengthp);

extern JS_PUBLIC_API(JSBool)
JS_SetArrayLength(JSContext *cx, JSObject *obj, uint32_t length);

extern JS_PUBLIC_API(JSBool)
JS_DefineElement(JSContext *cx, JSObject *obj, uint32_t index, jsval value,
                 JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs);

extern JS_PUBLIC_API(JSBool)
JS_AlreadyHasOwnElement(JSContext *cx, JSObject *obj, uint32_t index, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_HasElement(JSContext *cx, JSObject *obj, uint32_t index, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_LookupElement(JSContext *cx, JSObject *obj, uint32_t index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetElement(JSContext *cx, JSObject *obj, uint32_t index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_ForwardGetElementTo(JSContext *cx, JSObject *obj, uint32_t index, JSObject *onBehalfOf,
                       jsval *vp);






extern JS_PUBLIC_API(JSBool)
JS_GetElementIfPresent(JSContext *cx, JSObject *obj, uint32_t index, JSObject *onBehalfOf,
                       jsval *vp, JSBool* present);

extern JS_PUBLIC_API(JSBool)
JS_SetElement(JSContext *cx, JSObject *obj, uint32_t index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_DeleteElement(JSContext *cx, JSObject *obj, uint32_t index);

extern JS_PUBLIC_API(JSBool)
JS_DeleteElement2(JSContext *cx, JSObject *obj, uint32_t index, jsval *rval);

extern JS_PUBLIC_API(void)
JS_ClearScope(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSIdArray *)
JS_Enumerate(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSObject *)
JS_NewPropertyIterator(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSBool)
JS_NextProperty(JSContext *cx, JSObject *iterobj, jsid *idp);





extern JS_PUBLIC_API(JSObject *)
JS_NewElementIterator(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSObject *)
JS_ElementIteratorStub(JSContext *cx, JSObject *obj, JSBool keysonly);

extern JS_PUBLIC_API(JSBool)
JS_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               jsval *vp, unsigned *attrsp);

extern JS_PUBLIC_API(jsval)
JS_GetReservedSlot(JSObject *obj, uint32_t index);

extern JS_PUBLIC_API(void)
JS_SetReservedSlot(JSObject *obj, uint32_t index, jsval v);






struct JSPrincipals {
    
    int refcount;

#ifdef DEBUG
    
    uint32_t    debugToken;
#endif

#ifdef __cplusplus
    void setDebugToken(uint32_t token) {
# ifdef DEBUG
        debugToken = token;
# endif
    }

    



    JS_PUBLIC_API(void) dump();
#endif
};

extern JS_PUBLIC_API(void)
JS_HoldPrincipals(JSPrincipals *principals);

extern JS_PUBLIC_API(void)
JS_DropPrincipals(JSRuntime *rt, JSPrincipals *principals);

struct JSSecurityCallbacks {
    JSCheckAccessOp            checkObjectAccess;
    JSSubsumePrincipalsOp      subsumePrincipals;
    JSPrincipalsTranscoder     principalsTranscoder;
    JSObjectPrincipalsFinder   findObjectPrincipals;
    JSCSPEvalChecker           contentSecurityPolicyAllows;
};

extern JS_PUBLIC_API(void)
JS_SetSecurityCallbacks(JSRuntime *rt, const JSSecurityCallbacks *callbacks);

extern JS_PUBLIC_API(const JSSecurityCallbacks *)
JS_GetSecurityCallbacks(JSRuntime *rt);













extern JS_PUBLIC_API(void)
JS_SetTrustedPrincipals(JSRuntime *rt, JSPrincipals *prin);






extern JS_PUBLIC_API(void)
JS_InitDestroyPrincipalsCallback(JSRuntime *rt, JSDestroyPrincipalsOp destroyPrincipals);






extern JS_PUBLIC_API(JSFunction *)
JS_NewFunction(JSContext *cx, JSNative call, unsigned nargs, unsigned flags,
               JSObject *parent, const char *name);





extern JS_PUBLIC_API(JSFunction *)
JS_NewFunctionById(JSContext *cx, JSNative call, unsigned nargs, unsigned flags,
                   JSObject *parent, jsid id);

extern JS_PUBLIC_API(JSObject *)
JS_GetFunctionObject(JSFunction *fun);







extern JS_PUBLIC_API(JSString *)
JS_GetFunctionId(JSFunction *fun);




extern JS_PUBLIC_API(unsigned)
JS_GetFunctionFlags(JSFunction *fun);




extern JS_PUBLIC_API(uint16_t)
JS_GetFunctionArity(JSFunction *fun);







extern JS_PUBLIC_API(JSBool)
JS_ObjectIsFunction(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_ObjectIsCallable(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_IsNativeFunction(JSObject *funobj, JSNative call);

extern JS_PUBLIC_API(JSBool)
JS_DefineFunctions(JSContext *cx, JSObject *obj, JSFunctionSpec *fs);

extern JS_PUBLIC_API(JSFunction *)
JS_DefineFunction(JSContext *cx, JSObject *obj, const char *name, JSNative call,
                  unsigned nargs, unsigned attrs);

extern JS_PUBLIC_API(JSFunction *)
JS_DefineUCFunction(JSContext *cx, JSObject *obj,
                    const jschar *name, size_t namelen, JSNative call,
                    unsigned nargs, unsigned attrs);

extern JS_PUBLIC_API(JSFunction *)
JS_DefineFunctionById(JSContext *cx, JSObject *obj, jsid id, JSNative call,
                      unsigned nargs, unsigned attrs);

extern JS_PUBLIC_API(JSObject *)
JS_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent);








extern JS_PUBLIC_API(JSBool)
JS_BufferIsCompilableUnit(JSContext *cx, JSBool bytes_are_utf8,
                          JSObject *obj, const char *bytes, size_t length);

extern JS_PUBLIC_API(JSScript *)
JS_CompileScript(JSContext *cx, JSObject *obj,
                 const char *bytes, size_t length,
                 const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileScriptForPrincipals(JSContext *cx, JSObject *obj,
                              JSPrincipals *principals,
                              const char *bytes, size_t length,
                              const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileScriptForPrincipalsVersion(JSContext *cx, JSObject *obj,
                                     JSPrincipals *principals,
                                     const char *bytes, size_t length,
                                     const char *filename, unsigned lineno,
                                     JSVersion version);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScript(JSContext *cx, JSObject *obj,
                   const jschar *chars, size_t length,
                   const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScriptForPrincipals(JSContext *cx, JSObject *obj,
                                JSPrincipals *principals,
                                const jschar *chars, size_t length,
                                const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScriptForPrincipalsVersion(JSContext *cx, JSObject *obj,
                                       JSPrincipals *principals,
                                       const jschar *chars, size_t length,
                                       const char *filename, unsigned lineno,
                                       JSVersion version);




extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScriptForPrincipalsVersionOrigin(JSContext *cx, JSObject *obj,
                                             JSPrincipals *principals,
                                             JSPrincipals *originPrincipals,
                                             const jschar *chars, size_t length,
                                             const char *filename, unsigned lineno,
                                             JSVersion version);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUTF8File(JSContext *cx, JSObject *obj, const char *filename);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUTF8FileHandle(JSContext *cx, JSObject *obj, const char *filename,
                         FILE *fh);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUTF8FileHandleForPrincipals(JSContext *cx, JSObject *obj,
                                      const char *filename, FILE *fh,
                                      JSPrincipals *principals);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUTF8FileHandleForPrincipalsVersion(JSContext *cx, JSObject *obj,
                                             const char *filename, FILE *fh,
                                             JSPrincipals *principals,
                                             JSVersion version);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalFromScript(JSScript *script);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileFunction(JSContext *cx, JSObject *obj, const char *name,
                   unsigned nargs, const char **argnames,
                   const char *bytes, size_t length,
                   const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileFunctionForPrincipals(JSContext *cx, JSObject *obj,
                                JSPrincipals *principals, const char *name,
                                unsigned nargs, const char **argnames,
                                const char *bytes, size_t length,
                                const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileUCFunction(JSContext *cx, JSObject *obj, const char *name,
                     unsigned nargs, const char **argnames,
                     const jschar *chars, size_t length,
                     const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileUCFunctionForPrincipals(JSContext *cx, JSObject *obj,
                                  JSPrincipals *principals, const char *name,
                                  unsigned nargs, const char **argnames,
                                  const jschar *chars, size_t length,
                                  const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileUCFunctionForPrincipalsVersion(JSContext *cx, JSObject *obj,
                                         JSPrincipals *principals, const char *name,
                                         unsigned nargs, const char **argnames,
                                         const jschar *chars, size_t length,
                                         const char *filename, unsigned lineno,
                                         JSVersion version);

extern JS_PUBLIC_API(JSString *)
JS_DecompileScript(JSContext *cx, JSScript *script, const char *name, unsigned indent);





#define JS_DONT_PRETTY_PRINT    ((unsigned)0x8000)

extern JS_PUBLIC_API(JSString *)
JS_DecompileFunction(JSContext *cx, JSFunction *fun, unsigned indent);

extern JS_PUBLIC_API(JSString *)
JS_DecompileFunctionBody(JSContext *cx, JSFunction *fun, unsigned indent);




































extern JS_PUBLIC_API(JSBool)
JS_ExecuteScript(JSContext *cx, JSObject *obj, JSScript *script, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_ExecuteScriptVersion(JSContext *cx, JSObject *obj, JSScript *script, jsval *rval,
                        JSVersion version);





typedef enum JSExecPart { JSEXEC_PROLOG, JSEXEC_MAIN } JSExecPart;

extern JS_PUBLIC_API(JSBool)
JS_EvaluateScript(JSContext *cx, JSObject *obj,
                  const char *bytes, unsigned length,
                  const char *filename, unsigned lineno,
                  jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateScriptForPrincipals(JSContext *cx, JSObject *obj,
                               JSPrincipals *principals,
                               const char *bytes, unsigned length,
                               const char *filename, unsigned lineno,
                               jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateScriptForPrincipalsVersion(JSContext *cx, JSObject *obj,
                                      JSPrincipals *principals,
                                      const char *bytes, unsigned length,
                                      const char *filename, unsigned lineno,
                                      jsval *rval, JSVersion version);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCScript(JSContext *cx, JSObject *obj,
                    const jschar *chars, unsigned length,
                    const char *filename, unsigned lineno,
                    jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCScriptForPrincipals(JSContext *cx, JSObject *obj,
                                 JSPrincipals *principals,
                                 const jschar *chars, unsigned length,
                                 const char *filename, unsigned lineno,
                                 jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCScriptForPrincipalsVersion(JSContext *cx, JSObject *obj,
                                        JSPrincipals *principals,
                                        const jschar *chars, unsigned length,
                                        const char *filename, unsigned lineno,
                                        jsval *rval, JSVersion version);









extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCScriptForPrincipalsVersionOrigin(JSContext *cx, JSObject *obj,
                                              JSPrincipals *principals,
                                              JSPrincipals *originPrincipals,
                                              const jschar *chars, unsigned length,
                                              const char *filename, unsigned lineno,
                                              jsval *rval, JSVersion version);

extern JS_PUBLIC_API(JSBool)
JS_CallFunction(JSContext *cx, JSObject *obj, JSFunction *fun, unsigned argc,
                jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunctionName(JSContext *cx, JSObject *obj, const char *name, unsigned argc,
                    jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunctionValue(JSContext *cx, JSObject *obj, jsval fval, unsigned argc,
                     jsval *argv, jsval *rval);

#ifdef __cplusplus
JS_END_EXTERN_C

namespace JS {

static inline bool
Call(JSContext *cx, JSObject *thisObj, JSFunction *fun, unsigned argc, jsval *argv, jsval *rval) {
    return !!JS_CallFunction(cx, thisObj, fun, argc, argv, rval);
}

static inline bool
Call(JSContext *cx, JSObject *thisObj, const char *name, unsigned argc, jsval *argv, jsval *rval) {
    return !!JS_CallFunctionName(cx, thisObj, name, argc, argv, rval);
}

static inline bool
Call(JSContext *cx, JSObject *thisObj, jsval fun, unsigned argc, jsval *argv, jsval *rval) {
    return !!JS_CallFunctionValue(cx, thisObj, fun, argc, argv, rval);
}

extern JS_PUBLIC_API(bool)
Call(JSContext *cx, jsval thisv, jsval fun, unsigned argc, jsval *argv, jsval *rval);

static inline bool
Call(JSContext *cx, jsval thisv, JSObject *funObj, unsigned argc, jsval *argv, jsval *rval) {
    return Call(cx, thisv, OBJECT_TO_JSVAL(funObj), argc, argv, rval);
}

} 

JS_BEGIN_EXTERN_C
#endif 














extern JS_PUBLIC_API(JSOperationCallback)
JS_SetOperationCallback(JSContext *cx, JSOperationCallback callback);

extern JS_PUBLIC_API(JSOperationCallback)
JS_GetOperationCallback(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_TriggerOperationCallback(JSRuntime *rt);

extern JS_PUBLIC_API(JSBool)
JS_IsRunning(JSContext *cx);












extern JS_PUBLIC_API(JSBool)
JS_SaveFrameChain(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_RestoreFrameChain(JSContext *cx);

#ifdef MOZ_TRACE_JSCALLS









extern JS_PUBLIC_API(void)
JS_SetFunctionCallback(JSContext *cx, JSFunctionCallback fcb);

extern JS_PUBLIC_API(JSFunctionCallback)
JS_GetFunctionCallback(JSContext *cx);
#endif 












extern JS_PUBLIC_API(JSString *)
JS_NewStringCopyN(JSContext *cx, const char *s, size_t n);

extern JS_PUBLIC_API(JSString *)
JS_NewStringCopyZ(JSContext *cx, const char *s);

extern JS_PUBLIC_API(JSString *)
JS_InternJSString(JSContext *cx, JSString *str);

extern JS_PUBLIC_API(JSString *)
JS_InternString(JSContext *cx, const char *s);

extern JS_PUBLIC_API(JSString *)
JS_NewUCString(JSContext *cx, jschar *chars, size_t length);

extern JS_PUBLIC_API(JSString *)
JS_NewUCStringCopyN(JSContext *cx, const jschar *s, size_t n);

extern JS_PUBLIC_API(JSString *)
JS_NewUCStringCopyZ(JSContext *cx, const jschar *s);

extern JS_PUBLIC_API(JSString *)
JS_InternUCStringN(JSContext *cx, const jschar *s, size_t length);

extern JS_PUBLIC_API(JSString *)
JS_InternUCString(JSContext *cx, const jschar *s);

extern JS_PUBLIC_API(JSBool)
JS_CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32_t *result);

extern JS_PUBLIC_API(JSBool)
JS_StringEqualsAscii(JSContext *cx, JSString *str, const char *asciiBytes, JSBool *match);

extern JS_PUBLIC_API(size_t)
JS_PutEscapedString(JSContext *cx, char *buffer, size_t size, JSString *str, char quote);

extern JS_PUBLIC_API(JSBool)
JS_FileEscapedString(FILE *fp, JSString *str, char quote);




































extern JS_PUBLIC_API(size_t)
JS_GetStringLength(JSString *str);

extern JS_PUBLIC_API(const jschar *)
JS_GetStringCharsAndLength(JSContext *cx, JSString *str, size_t *length);

extern JS_PUBLIC_API(const jschar *)
JS_GetInternedStringChars(JSString *str);

extern JS_PUBLIC_API(const jschar *)
JS_GetInternedStringCharsAndLength(JSString *str, size_t *length);

extern JS_PUBLIC_API(const jschar *)
JS_GetStringCharsZ(JSContext *cx, JSString *str);

extern JS_PUBLIC_API(const jschar *)
JS_GetStringCharsZAndLength(JSContext *cx, JSString *str, size_t *length);

extern JS_PUBLIC_API(JSFlatString *)
JS_FlattenString(JSContext *cx, JSString *str);

extern JS_PUBLIC_API(const jschar *)
JS_GetFlatStringChars(JSFlatString *str);

static JS_ALWAYS_INLINE JSFlatString *
JSID_TO_FLAT_STRING(jsid id)
{
    JS_ASSERT(JSID_IS_STRING(id));
    return (JSFlatString *)(JSID_BITS(id));
}

static JS_ALWAYS_INLINE JSFlatString *
JS_ASSERT_STRING_IS_FLAT(JSString *str)
{
    JS_ASSERT(JS_GetFlatStringChars((JSFlatString *)str));
    return (JSFlatString *)str;
}

static JS_ALWAYS_INLINE JSString *
JS_FORGET_STRING_FLATNESS(JSFlatString *fstr)
{
    return (JSString *)fstr;
}





extern JS_PUBLIC_API(JSBool)
JS_FlatStringEqualsAscii(JSFlatString *str, const char *asciiBytes);

extern JS_PUBLIC_API(size_t)
JS_PutEscapedFlatString(char *buffer, size_t size, JSFlatString *str, char quote);





extern JS_PUBLIC_API(JSString *)
JS_NewGrowableString(JSContext *cx, jschar *chars, size_t length);



























extern JS_PUBLIC_API(JSString *)
JS_NewDependentString(JSContext *cx, JSString *str, size_t start,
                      size_t length);





extern JS_PUBLIC_API(JSString *)
JS_ConcatStrings(JSContext *cx, JSString *left, JSString *right);





extern JS_PUBLIC_API(const jschar *)
JS_UndependString(JSContext *cx, JSString *str);





extern JS_PUBLIC_API(JSBool)
JS_MakeStringImmutable(JSContext *cx, JSString *str);





JS_PUBLIC_API(JSBool)
JS_CStringsAreUTF8(void);






JS_PUBLIC_API(void)
JS_SetCStringsAreUTF8(void);
























JS_PUBLIC_API(JSBool)
JS_EncodeCharacters(JSContext *cx, const jschar *src, size_t srclen, char *dst,
                    size_t *dstlenp);

JS_PUBLIC_API(JSBool)
JS_DecodeBytes(JSContext *cx, const char *src, size_t srclen, jschar *dst,
               size_t *dstlenp);

JS_PUBLIC_API(JSBool)
JS_DecodeUTF8(JSContext *cx, const char *src, size_t srclen, jschar *dst,
              size_t *dstlenp);





JS_PUBLIC_API(char *)
JS_EncodeString(JSContext *cx, JSString *str);






JS_PUBLIC_API(size_t)
JS_GetStringEncodingLength(JSContext *cx, JSString *str);














JS_PUBLIC_API(size_t)
JS_EncodeStringToBuffer(JSString *str, char *buffer, size_t length);

#ifdef __cplusplus

class JSAutoByteString {
  public:
    JSAutoByteString(JSContext *cx, JSString *str JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : mBytes(JS_EncodeString(cx, str)) {
        JS_ASSERT(cx);
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JSAutoByteString(JS_GUARD_OBJECT_NOTIFIER_PARAM0)
      : mBytes(NULL) {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~JSAutoByteString() {
        js::UnwantedForeground::free_(mBytes);
    }

    
    void initBytes(char *bytes) {
        JS_ASSERT(!mBytes);
        mBytes = bytes;
    }

    char *encode(JSContext *cx, JSString *str) {
        JS_ASSERT(!mBytes);
        JS_ASSERT(cx);
        mBytes = JS_EncodeString(cx, str);
        return mBytes;
    }

    void clear() {
        js::UnwantedForeground::free_(mBytes);
        mBytes = NULL;
    }

    char *ptr() const {
        return mBytes;
    }

    bool operator!() const {
        return !mBytes;
    }

  private:
    char        *mBytes;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    JSAutoByteString(const JSAutoByteString &another);
    JSAutoByteString &operator=(const JSAutoByteString &another);
};

#endif





typedef JSBool (* JSONWriteCallback)(const jschar *buf, uint32_t len, void *data);




JS_PUBLIC_API(JSBool)
JS_Stringify(JSContext *cx, jsval *vp, JSObject *replacer, jsval space,
             JSONWriteCallback callback, void *data);




JS_PUBLIC_API(JSBool)
JS_ParseJSON(JSContext *cx, const jschar *chars, uint32_t len, jsval *vp);

JS_PUBLIC_API(JSBool)
JS_ParseJSONWithReviver(JSContext *cx, const jschar *chars, uint32_t len, jsval reviver,
                        jsval *vp);






#define JS_STRUCTURED_CLONE_VERSION 1

struct JSStructuredCloneCallbacks {
    ReadStructuredCloneOp read;
    WriteStructuredCloneOp write;
    StructuredCloneErrorOp reportError;
};

JS_PUBLIC_API(JSBool)
JS_ReadStructuredClone(JSContext *cx, const uint64_t *data, size_t nbytes,
                       uint32_t version, jsval *vp,
                       const JSStructuredCloneCallbacks *optionalCallbacks,
                       void *closure);


JS_PUBLIC_API(JSBool)
JS_WriteStructuredClone(JSContext *cx, jsval v, uint64_t **datap, size_t *nbytesp,
                        const JSStructuredCloneCallbacks *optionalCallbacks,
                        void *closure);

JS_PUBLIC_API(JSBool)
JS_StructuredClone(JSContext *cx, jsval v, jsval *vp,
                   const JSStructuredCloneCallbacks *optionalCallbacks,
                   void *closure);

#ifdef __cplusplus
JS_END_EXTERN_C


class JS_PUBLIC_API(JSAutoStructuredCloneBuffer) {
    uint64_t *data_;
    size_t nbytes_;
    uint32_t version_;

  public:
    JSAutoStructuredCloneBuffer()
        : data_(NULL), nbytes_(0), version_(JS_STRUCTURED_CLONE_VERSION) {}

    ~JSAutoStructuredCloneBuffer() { clear(); }

    uint64_t *data() const { return data_; }
    size_t nbytes() const { return nbytes_; }

    void clear();

    
    bool copy(const uint64_t *data, size_t nbytes, uint32_t version=JS_STRUCTURED_CLONE_VERSION);

    




    void adopt(uint64_t *data, size_t nbytes, uint32_t version=JS_STRUCTURED_CLONE_VERSION);

    




    void steal(uint64_t **datap, size_t *nbytesp, uint32_t *versionp=NULL);

    bool read(JSContext *cx, jsval *vp,
              const JSStructuredCloneCallbacks *optionalCallbacks=NULL,
              void *closure=NULL) const;

    bool write(JSContext *cx, jsval v,
               const JSStructuredCloneCallbacks *optionalCallbacks=NULL,
               void *closure=NULL);

    


    void swap(JSAutoStructuredCloneBuffer &other);

  private:
    
    JSAutoStructuredCloneBuffer(const JSAutoStructuredCloneBuffer &other);
    JSAutoStructuredCloneBuffer &operator=(const JSAutoStructuredCloneBuffer &other);
};

JS_BEGIN_EXTERN_C
#endif




#define JS_SCTAG_USER_MIN  ((uint32_t) 0xFFFF8000)
#define JS_SCTAG_USER_MAX  ((uint32_t) 0xFFFFFFFF)

#define JS_SCERR_RECURSION 0

JS_PUBLIC_API(void)
JS_SetStructuredCloneCallbacks(JSRuntime *rt, const JSStructuredCloneCallbacks *callbacks);

JS_PUBLIC_API(JSBool)
JS_ReadUint32Pair(JSStructuredCloneReader *r, uint32_t *p1, uint32_t *p2);

JS_PUBLIC_API(JSBool)
JS_ReadBytes(JSStructuredCloneReader *r, void *p, size_t len);

JS_PUBLIC_API(JSBool)
JS_WriteUint32Pair(JSStructuredCloneWriter *w, uint32_t tag, uint32_t data);

JS_PUBLIC_API(JSBool)
JS_WriteBytes(JSStructuredCloneWriter *w, const void *p, size_t len);






struct JSLocaleCallbacks {
    JSLocaleToUpperCase     localeToUpperCase;
    JSLocaleToLowerCase     localeToLowerCase;
    JSLocaleCompare         localeCompare;
    JSLocaleToUnicode       localeToUnicode;
    JSErrorCallback         localeGetErrorMessage;
};





extern JS_PUBLIC_API(void)
JS_SetLocaleCallbacks(JSContext *cx, JSLocaleCallbacks *callbacks);





extern JS_PUBLIC_API(JSLocaleCallbacks *)
JS_GetLocaleCallbacks(JSContext *cx);












extern JS_PUBLIC_API(void)
JS_ReportError(JSContext *cx, const char *format, ...);




extern JS_PUBLIC_API(void)
JS_ReportErrorNumber(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const unsigned errorNumber, ...);




extern JS_PUBLIC_API(void)
JS_ReportErrorNumberUC(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const unsigned errorNumber, ...);







extern JS_PUBLIC_API(JSBool)
JS_ReportWarning(JSContext *cx, const char *format, ...);

extern JS_PUBLIC_API(JSBool)
JS_ReportErrorFlagsAndNumber(JSContext *cx, unsigned flags,
                             JSErrorCallback errorCallback, void *userRef,
                             const unsigned errorNumber, ...);

extern JS_PUBLIC_API(JSBool)
JS_ReportErrorFlagsAndNumberUC(JSContext *cx, unsigned flags,
                               JSErrorCallback errorCallback, void *userRef,
                               const unsigned errorNumber, ...);




extern JS_PUBLIC_API(void)
JS_ReportOutOfMemory(JSContext *cx);




extern JS_PUBLIC_API(void)
JS_ReportAllocationOverflow(JSContext *cx);

struct JSErrorReport {
    const char      *filename;      
    JSPrincipals    *originPrincipals; 
    unsigned           lineno;         
    const char      *linebuf;       
    const char      *tokenptr;      
    const jschar    *uclinebuf;     
    const jschar    *uctokenptr;    
    unsigned           flags;          
    unsigned           errorNumber;    
    const jschar    *ucmessage;     
    const jschar    **messageArgs;  
};




#define JSREPORT_ERROR      0x0     /* pseudo-flag for default case */
#define JSREPORT_WARNING    0x1     /* reported via JS_ReportWarning */
#define JSREPORT_EXCEPTION  0x2     /* exception was thrown */
#define JSREPORT_STRICT     0x4     /* error or warning due to strict option */








#define JSREPORT_STRICT_MODE_ERROR 0x8








#define JSREPORT_IS_WARNING(flags)      (((flags) & JSREPORT_WARNING) != 0)
#define JSREPORT_IS_EXCEPTION(flags)    (((flags) & JSREPORT_EXCEPTION) != 0)
#define JSREPORT_IS_STRICT(flags)       (((flags) & JSREPORT_STRICT) != 0)
#define JSREPORT_IS_STRICT_MODE_ERROR(flags) (((flags) &                      \
                                              JSREPORT_STRICT_MODE_ERROR) != 0)
extern JS_PUBLIC_API(JSErrorReporter)
JS_GetErrorReporter(JSContext *cx);

extern JS_PUBLIC_API(JSErrorReporter)
JS_SetErrorReporter(JSContext *cx, JSErrorReporter er);







extern JS_PUBLIC_API(JSObject *)
JS_NewDateObject(JSContext *cx, int year, int mon, int mday, int hour, int min, int sec);

extern JS_PUBLIC_API(JSObject *)
JS_NewDateObjectMsec(JSContext *cx, double msec);




extern JS_PUBLIC_API(JSBool)
JS_ObjectIsDate(JSContext *cx, JSObject *obj);






#define JSREG_FOLD      0x01    /* fold uppercase to lowercase */
#define JSREG_GLOB      0x02    /* global exec, creates array of matches */
#define JSREG_MULTILINE 0x04    /* treat ^ and $ as begin and end of line */
#define JSREG_STICKY    0x08    /* only match starting at lastIndex */

extern JS_PUBLIC_API(JSObject *)
JS_NewRegExpObject(JSContext *cx, JSObject *obj, char *bytes, size_t length, unsigned flags);

extern JS_PUBLIC_API(JSObject *)
JS_NewUCRegExpObject(JSContext *cx, JSObject *obj, jschar *chars, size_t length, unsigned flags);

extern JS_PUBLIC_API(void)
JS_SetRegExpInput(JSContext *cx, JSObject *obj, JSString *input, JSBool multiline);

extern JS_PUBLIC_API(void)
JS_ClearRegExpStatics(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_ExecuteRegExp(JSContext *cx, JSObject *obj, JSObject *reobj, jschar *chars, size_t length,
                 size_t *indexp, JSBool test, jsval *rval);



extern JS_PUBLIC_API(JSObject *)
JS_NewRegExpObjectNoStatics(JSContext *cx, char *bytes, size_t length, unsigned flags);

extern JS_PUBLIC_API(JSObject *)
JS_NewUCRegExpObjectNoStatics(JSContext *cx, jschar *chars, size_t length, unsigned flags);

extern JS_PUBLIC_API(JSBool)
JS_ExecuteRegExpNoStatics(JSContext *cx, JSObject *reobj, jschar *chars, size_t length,
                          size_t *indexp, JSBool test, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_ObjectIsRegExp(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(unsigned)
JS_GetRegExpFlags(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSString *)
JS_GetRegExpSource(JSContext *cx, JSObject *obj);



extern JS_PUBLIC_API(JSBool)
JS_IsExceptionPending(JSContext *cx);

extern JS_PUBLIC_API(JSBool)
JS_GetPendingException(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(void)
JS_SetPendingException(JSContext *cx, jsval v);

extern JS_PUBLIC_API(void)
JS_ClearPendingException(JSContext *cx);

extern JS_PUBLIC_API(JSBool)
JS_ReportPendingException(JSContext *cx);












extern JS_PUBLIC_API(JSExceptionState *)
JS_SaveExceptionState(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_RestoreExceptionState(JSContext *cx, JSExceptionState *state);

extern JS_PUBLIC_API(void)
JS_DropExceptionState(JSContext *cx, JSExceptionState *state);








extern JS_PUBLIC_API(JSErrorReport *)
JS_ErrorFromException(JSContext *cx, jsval v);





extern JS_PUBLIC_API(JSBool)
JS_ThrowReportedError(JSContext *cx, const char *message,
                      JSErrorReport *reportp);




extern JS_PUBLIC_API(JSBool)
JS_ThrowStopIteration(JSContext *cx);

extern JS_PUBLIC_API(intptr_t)
JS_GetCurrentThread();

















extern JS_PUBLIC_API(void)
JS_AbortIfWrongThread(JSRuntime *rt);

extern JS_PUBLIC_API(void)
JS_ClearRuntimeThread(JSRuntime *rt);

extern JS_PUBLIC_API(void)
JS_SetRuntimeThread(JSRuntime *rt);

#ifdef __cplusplus
JS_END_EXTERN_C

class JSAutoSetRuntimeThread
{
    JSRuntime *runtime;

  public:
    JSAutoSetRuntimeThread(JSRuntime *runtime) : runtime(runtime) {
        JS_SetRuntimeThread(runtime);
    }

    ~JSAutoSetRuntimeThread() {
        JS_ClearRuntimeThread(runtime);
    }
};

JS_BEGIN_EXTERN_C
#endif










static JS_ALWAYS_INLINE JSBool
JS_IsConstructing(JSContext *cx, const jsval *vp)
{
#ifdef DEBUG
    JSObject *callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
    if (JS_ObjectIsFunction(cx, callee)) {
        JSFunction *fun = JS_ValueToFunction(cx, JS_CALLEE(cx, vp));
        JS_ASSERT((JS_GetFunctionFlags(fun) & JSFUN_CONSTRUCTOR) != 0);
    } else {
        JS_ASSERT(JS_GetClass(callee)->construct != NULL);
    }
#else
    (void)cx;
#endif

    return JSVAL_IS_MAGIC_IMPL(JSVAL_TO_IMPL(vp[1]));
}






extern JS_PUBLIC_API(JSObject *)
JS_NewObjectForConstructor(JSContext *cx, const jsval *vp);



#ifdef DEBUG
#define JS_GC_ZEAL 1
#endif

#ifdef JS_GC_ZEAL
#define JS_DEFAULT_ZEAL_FREQ 100

extern JS_PUBLIC_API(void)
JS_SetGCZeal(JSContext *cx, uint8_t zeal, uint32_t frequency, JSBool compartment);

extern JS_PUBLIC_API(void)
JS_ScheduleGC(JSContext *cx, uint32_t count, JSBool compartment);
#endif




extern JS_PUBLIC_API(JSBool)
JS_IndexToId(JSContext *cx, uint32_t index, jsid *id);




extern JS_PUBLIC_API(JSBool)
JS_IsIdentifier(JSContext *cx, JSString *str, JSBool *isIdentifier);

JS_END_EXTERN_C

#endif 
