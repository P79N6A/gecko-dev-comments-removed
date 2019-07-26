








#ifndef jsapi_h___
#define jsapi_h___

#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/RangedPtr.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/ThreadLocal.h"

#include <stddef.h>
#include <stdio.h>

#include "js-config.h"
#include "jsalloc.h"
#include "jspubtd.h"
#include "jsutil.h"

#include "gc/Root.h"
#include "js/Anchor.h"
#include "js/CharacterEncoding.h"
#include "js/HashTable.h"
#include "js/PropertyKey.h"
#include "js/Utility.h"
#include "js/Value.h"
#include "js/Vector.h"



namespace JS {

typedef mozilla::RangedPtr<const jschar> CharPtr;

class StableCharPtr : public CharPtr {
  public:
    StableCharPtr(const StableCharPtr &s) : CharPtr(s) {}
    StableCharPtr(const mozilla::RangedPtr<const jschar> &s) : CharPtr(s) {}
    StableCharPtr(const jschar *s, size_t len) : CharPtr(s, len) {}
    StableCharPtr(const jschar *pos, const jschar *start, size_t len)
      : CharPtr(pos, start, len)
    {}
};

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

#ifdef DEBUG





JS_PUBLIC_API(void)
AssertArgumentsAreSane(JSContext *cx, const Value &v);
#else
inline void AssertArgumentsAreSane(JSContext *cx, const Value &v) {
    
}
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
    static void traceAllWrappers(JSTracer *trc);

  protected:
    AutoGCRooter * const down;

    






    ptrdiff_t tag_;

    enum {
        JSVAL =        -1, 
        VALARRAY =     -2, 
        PARSER =       -3, 
        SHAPEVECTOR =  -4, 
        IDARRAY =      -6, 
        DESCRIPTORS =  -7, 
        
        
        OBJECT =      -10, 
        ID =          -11, 
        VALVECTOR =   -12, 
        DESCRIPTOR =  -13, 
        STRING =      -14, 
        IDVECTOR =    -15, 
        OBJVECTOR =   -16, 
        STRINGVECTOR =-17, 
        SCRIPTVECTOR =-18, 
        PROPDESC =    -19, 
        SHAPERANGE =  -20, 
        STACKSHAPE =  -21, 
        STACKBASESHAPE=-22,
        GETTERSETTER =-24, 
        REGEXPSTATICS=-25, 
        NAMEVECTOR =  -26, 
        HASHABLEVALUE=-27,
        IONMASM =     -28, 
        IONALLOC =    -29, 
        WRAPVECTOR =  -30, 
        WRAPPER =     -31, 
        OBJOBJHASHMAP=-32, 
        OBJU32HASHMAP=-33, 
        OBJHASHSET =  -34  
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
                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(NullValue())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    AutoValueRooter(JSContext *cx, const Value &v
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(v)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    





    void set(Value v) {
        JS_ASSERT(tag_ == JSVAL);
        val = v;
    }

    const Value &value() const {
        JS_ASSERT(tag_ == JSVAL);
        return val;
    }

    Value *addr() {
        JS_ASSERT(tag_ == JSVAL);
        return &val;
    }

    const Value &jsval_value() const {
        JS_ASSERT(tag_ == JSVAL);
        return val;
    }

    Value *jsval_addr() {
        JS_ASSERT(tag_ == JSVAL);
        return &val;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Value val;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectRooter : private AutoGCRooter
{
  public:
    AutoObjectRooter(JSContext *cx, JSObject *obj = NULL
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, OBJECT), obj_(obj)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setObject(JSObject *obj) {
        obj_ = obj;
    }

    JSObject * object() const {
        return obj_;
    }

    JSObject ** addr() {
        return &obj_;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSObject *obj_;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoStringRooter : private AutoGCRooter {
  public:
    AutoStringRooter(JSContext *cx, JSString *str = NULL
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, STRING), str_(str)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setString(JSString *str) {
        str_ = str;
    }

    JSString * string() const {
        return str_;
    }

    JSString ** addr() {
        return &str_;
    }

    JSString * const * addr() const {
        return &str_;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSString *str_;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoArrayRooter : private AutoGCRooter {
  public:
    AutoArrayRooter(JSContext *cx, size_t len, Value *vec
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(vec), skip(cx, array, len)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag_ >= 0);
    }

    void changeLength(size_t newLength) {
        tag_ = ptrdiff_t(newLength);
        JS_ASSERT(tag_ >= 0);
    }

    void changeArray(Value *newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    Value *array;

    MutableHandleValue handleAt(size_t i)
    {
        JS_ASSERT(i < size_t(tag_));
        return MutableHandleValue::fromMarkedLocation(&array[i]);
    }
    HandleValue handleAt(size_t i) const
    {
        JS_ASSERT(i < size_t(tag_));
        return HandleValue::fromMarkedLocation(&array[i]);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    js::SkipRoot skip;
};

template<class T>
class AutoVectorRooter : protected AutoGCRooter
{
  public:
    explicit AutoVectorRooter(JSContext *cx, ptrdiff_t tag
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, tag), vector(cx), vectorRoot(cx, &vector)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    typedef T ElementType;

    size_t length() const { return vector.length(); }
    bool empty() const { return vector.empty(); }

    bool append(const T &v) { return vector.append(v); }
    bool append(const AutoVectorRooter<T> &other) {
        return vector.append(other.vector);
    }

    
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

    JS::MutableHandle<T> handleAt(size_t i) { return JS::MutableHandle<T>::fromMarkedLocation(&vector[i]); }
    JS::Handle<T> handleAt(size_t i) const { return JS::Handle<T>::fromMarkedLocation(&vector[i]); }

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

    
    js::SkipRoot vectorRoot;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<class Key, class Value>
class AutoHashMapRooter : protected AutoGCRooter
{
  private:
    typedef js::HashMap<Key, Value> HashMapImpl;

  public:
    explicit AutoHashMapRooter(JSContext *cx, ptrdiff_t tag
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, tag), map(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    typedef Key KeyType;
    typedef Value ValueType;
    typedef typename HashMapImpl::Lookup Lookup;
    typedef typename HashMapImpl::Ptr Ptr;
    typedef typename HashMapImpl::AddPtr AddPtr;

    bool init(uint32_t len = 16) {
        return map.init(len);
    }
    bool initialized() const {
        return map.initialized();
    }
    Ptr lookup(const Lookup &l) const {
        return map.lookup(l);
    }
    void remove(Ptr p) {
        map.remove(p);
    }
    AddPtr lookupForAdd(const Lookup &l) const {
        return map.lookupForAdd(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool add(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        return map.add(p, k, v);
    }

    bool add(AddPtr &p, const Key &k) {
        return map.add(p, k);
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr &p, const KeyInput &k, const ValueInput &v) {
        return map.relookupOrAdd(p, k, v);
    }

    typedef typename HashMapImpl::Range Range;
    Range all() const {
        return map.all();
    }

    typedef typename HashMapImpl::Enum Enum;

    void clear() {
        map.clear();
    }

    void finish() {
        map.finish();
    }

    bool empty() const {
        return map.empty();
    }

    uint32_t count() const {
        return map.count();
    }

    size_t capacity() const {
        return map.capacity();
    }

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return map.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return map.sizeOfIncludingThis(mallocSizeOf);
    }

    unsigned generation() const {
        return map.generation();
    }

    

    bool has(const Lookup &l) const {
        return map.has(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool put(const KeyInput &k, const ValueInput &v) {
        return map.put(k, v);
    }

    template<typename KeyInput, typename ValueInput>
    bool putNew(const KeyInput &k, const ValueInput &v) {
        return map.putNew(k, v);
    }

    Ptr lookupWithDefault(const Key &k, const Value &defaultValue) {
        return map.lookupWithDefault(k, defaultValue);
    }

    void remove(const Lookup &l) {
        map.remove(l);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    AutoHashMapRooter(const AutoHashMapRooter &hmr) MOZ_DELETE;
    AutoHashMapRooter &operator=(const AutoHashMapRooter &hmr) MOZ_DELETE;

    HashMapImpl map;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<class T>
class AutoHashSetRooter : protected AutoGCRooter
{
  private:
    typedef js::HashSet<T> HashSetImpl;

  public:
    explicit AutoHashSetRooter(JSContext *cx, ptrdiff_t tag
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, tag), set(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    typedef typename HashSetImpl::Lookup Lookup;
    typedef typename HashSetImpl::Ptr Ptr;
    typedef typename HashSetImpl::AddPtr AddPtr;

    bool init(uint32_t len = 16) {
        return set.init(len);
    }
    bool initialized() const {
        return set.initialized();
    }
    Ptr lookup(const Lookup &l) const {
        return set.lookup(l);
    }
    void remove(Ptr p) {
        set.remove(p);
    }
    AddPtr lookupForAdd(const Lookup &l) const {
        return set.lookupForAdd(l);
    }

    bool add(AddPtr &p, const T &t) {
        return set.add(p, t);
    }

    bool relookupOrAdd(AddPtr &p, const Lookup &l, const T &t) {
        return set.relookupOrAdd(p, l, t);
    }

    typedef typename HashSetImpl::Range Range;
    Range all() const {
        return set.all();
    }

    typedef typename HashSetImpl::Enum Enum;

    void clear() {
        set.clear();
    }

    void finish() {
        set.finish();
    }

    bool empty() const {
        return set.empty();
    }

    uint32_t count() const {
        return set.count();
    }

    size_t capacity() const {
        return set.capacity();
    }

    size_t sizeOfExcludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return set.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf) const {
        return set.sizeOfIncludingThis(mallocSizeOf);
    }

    unsigned generation() const {
        return set.generation();
    }

    

    bool has(const Lookup &l) const {
        return set.has(l);
    }

    bool put(const T &t) {
        return set.put(t);
    }

    bool putNew(const T &t) {
        return set.putNew(t);
    }

    void remove(const Lookup &l) {
        set.remove(l);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    AutoHashSetRooter(const AutoHashSetRooter &hmr) MOZ_DELETE;
    AutoHashSetRooter &operator=(const AutoHashSetRooter &hmr) MOZ_DELETE;

    HashSetImpl set;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueVector : public AutoVectorRooter<Value>
{
  public:
    explicit AutoValueVector(JSContext *cx
                             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Value>(cx, VALVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdVector : public AutoVectorRooter<jsid>
{
  public:
    explicit AutoIdVector(JSContext *cx
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<jsid>(cx, IDVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoScriptVector : public AutoVectorRooter<JSScript *>
{
  public:
    explicit AutoScriptVector(JSContext *cx
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSScript *>(cx, SCRIPTVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class CallReceiver
{
  protected:
#ifdef DEBUG
    mutable bool usedRval_;
    void setUsedRval() const { usedRval_ = true; }
    void clearUsedRval() const { usedRval_ = false; }
#else
    void setUsedRval() const {}
    void clearUsedRval() const {}
#endif
    Value *argv_;
  public:
    friend CallReceiver CallReceiverFromVp(Value *);
    friend CallReceiver CallReceiverFromArgv(Value *);
    Value *base() const { return argv_ - 2; }
    JSObject &callee() const { JS_ASSERT(!usedRval_); return argv_[-2].toObject(); }

    JS::HandleValue calleev() const {
        JS_ASSERT(!usedRval_);
        return JS::HandleValue::fromMarkedLocation(&argv_[-2]);
    }

    JS::HandleValue thisv() const {
        return JS::HandleValue::fromMarkedLocation(&argv_[-1]);
    }

    JS::MutableHandleValue mutableThisv() const {
        return JS::MutableHandleValue::fromMarkedLocation(&argv_[-1]);
    }

    JS::MutableHandleValue rval() const {
        setUsedRval();
        return JS::MutableHandleValue::fromMarkedLocation(&argv_[-2]);
    }

    Value *spAfterCall() const {
        setUsedRval();
        return argv_ - 1;
    }

    void setCallee(Value aCalleev) const {
        clearUsedRval();
        argv_[-2] = aCalleev;
    }

    void setThis(Value aThisv) const {
        argv_[-1] = aThisv;
    }
};

JS_ALWAYS_INLINE CallReceiver
CallReceiverFromArgv(Value *argv)
{
    CallReceiver receiver;
    receiver.clearUsedRval();
    receiver.argv_ = argv;
    return receiver;
}

JS_ALWAYS_INLINE CallReceiver
CallReceiverFromVp(Value *vp)
{
    return CallReceiverFromArgv(vp + 2);
}



class CallArgs : public CallReceiver
{
  protected:
    unsigned argc_;
  public:
    friend CallArgs CallArgsFromVp(unsigned, Value *);
    friend CallArgs CallArgsFromArgv(unsigned, Value *);
    friend CallArgs CallArgsFromSp(unsigned, Value *);
    Value &operator[](unsigned i) const { JS_ASSERT(i < argc_); return argv_[i]; }
    MutableHandleValue handleAt(unsigned i)
    {
        JS_ASSERT(i < argc_);
        return MutableHandleValue::fromMarkedLocation(&argv_[i]);
    }
    HandleValue handleAt(unsigned i) const
    {
        JS_ASSERT(i < argc_);
        return HandleValue::fromMarkedLocation(&argv_[i]);
    }
    Value get(unsigned i) const
    {
        return i < length() ? argv_[i] : UndefinedValue();
    }
    Value *array() const { return argv_; }
    unsigned length() const { return argc_; }
    Value *end() const { return argv_ + argc_; }
    bool hasDefined(unsigned i) const { return i < argc_ && !argv_[i].isUndefined(); }
};

JS_ALWAYS_INLINE CallArgs
CallArgsFromArgv(unsigned argc, Value *argv)
{
    CallArgs args;
    args.clearUsedRval();
    args.argv_ = argv;
    args.argc_ = argc;
    return args;
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromVp(unsigned argc, Value *vp)
{
    return CallArgsFromArgv(argc, vp + 2);
}

JS_ALWAYS_INLINE CallArgs
CallArgsFromSp(unsigned argc, Value *sp)
{
    return CallArgsFromArgv(argc, sp - argc);
}


typedef bool (*IsAcceptableThis)(const Value &v);





typedef bool (*NativeImpl)(JSContext *cx, CallArgs args);

namespace detail {


extern JS_PUBLIC_API(bool)
CallMethodIfWrapped(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);

} 

































































template<IsAcceptableThis Test, NativeImpl Impl>
JS_ALWAYS_INLINE bool
CallNonGenericMethod(JSContext *cx, CallArgs args)
{
    const Value &thisv = args.thisv();
    if (Test(thisv))
        return Impl(cx, args);

    return detail::CallMethodIfWrapped(cx, Test, Impl, args);
}

JS_ALWAYS_INLINE bool
CallNonGenericMethod(JSContext *cx, IsAcceptableThis Test, NativeImpl Impl, CallArgs args)
{
    const Value &thisv = args.thisv();
    if (Test(thisv))
        return Impl(cx, args);

    return detail::CallMethodIfWrapped(cx, Test, Impl, args);
}

}  



typedef JS::Handle<JSObject*> JSHandleObject;
typedef JS::Handle<JSString*> JSHandleString;
typedef JS::Handle<JS::Value> JSHandleValue;
typedef JS::Handle<jsid> JSHandleId;

typedef JS::MutableHandle<JSObject*>   JSMutableHandleObject;
typedef JS::MutableHandle<JSFunction*> JSMutableHandleFunction;
typedef JS::MutableHandle<JSScript*>   JSMutableHandleScript;
typedef JS::MutableHandle<JSString*>   JSMutableHandleString;
typedef JS::MutableHandle<JS::Value>   JSMutableHandleValue;
typedef JS::MutableHandle<jsid>        JSMutableHandleId;

typedef js::RawObject   JSRawObject;
typedef js::RawFunction JSRawFunction;
typedef js::RawScript   JSRawScript;
typedef js::RawString   JSRawString;
typedef js::RawId       JSRawId;
typedef js::RawValue    JSRawValue;










typedef JSBool
(* JSPropertyOp)(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp);








typedef JSBool
(* JSStrictPropertyOp)(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

































typedef JSBool
(* JSNewEnumerateOp)(JSContext *cx, JSHandleObject obj, JSIterateOp enum_op,
                     JSMutableHandleValue statep, JSMutableHandleId idp);





typedef JSBool
(* JSEnumerateOp)(JSContext *cx, JSHandleObject obj);













typedef JSBool
(* JSResolveOp)(JSContext *cx, JSHandleObject obj, JSHandleId id);













typedef JSBool
(* JSNewResolveOp)(JSContext *cx, JSHandleObject obj, JSHandleId id, unsigned flags,
                   JSMutableHandleObject objp);





typedef JSBool
(* JSConvertOp)(JSContext *cx, JSHandleObject obj, JSType type, JSMutableHandleValue vp);

typedef struct JSFreeOp JSFreeOp;

struct JSFreeOp {
  private:
    JSRuntime   *runtime_;

  protected:
    JSFreeOp(JSRuntime *rt)
      : runtime_(rt) { }

  public:
    JSRuntime *runtime() const {
        return runtime_;
    }
};






typedef void
(* JSFinalizeOp)(JSFreeOp *fop, JSObject *obj);




typedef struct JSStringFinalizer JSStringFinalizer;

struct JSStringFinalizer {
    void (*finalize)(const JSStringFinalizer *fin, jschar *chars);
};







typedef JSBool
(* JSCheckAccessOp)(JSContext *cx, JSHandleObject obj, JSHandleId id, JSAccessMode mode,
                    JSMutableHandleValue vp);






typedef JSBool
(* JSHasInstanceOp)(JSContext *cx, JSHandleObject obj, JSMutableHandleValue vp, JSBool *bp);


















typedef void
(* JSTraceOp)(JSTracer *trc, JSRawObject obj);





typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);

typedef JSRawObject
(* JSWeakmapKeyDelegateOp)(JSRawObject obj);







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
    




    JSFINALIZE_GROUP_START,

    





    JSFINALIZE_GROUP_END,

    


    JSFINALIZE_COLLECTION_END
} JSFinalizeStatus;

typedef void
(* JSFinalizeCallback)(JSFreeOp *fop, JSFinalizeStatus status, JSBool isCompartment);





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
(* JSCSPEvalChecker)(JSContext *cx);










typedef JSObject *
(* JSWrapObjectCallback)(JSContext *cx, JSObject *existing, JSObject *obj,
                         JSObject *proto, JSObject *parent,
                         unsigned flags);






typedef JSObject *
(* JSPreWrapCallback)(JSContext *cx, JSObject *scope, JSObject *obj, unsigned flags);











typedef JSObject *
(* JSSameCompartmentWrapObjectCallback)(JSContext *cx, JSObject *obj);

typedef void
(* JSDestroyCompartmentCallback)(JSFreeOp *fop, JSCompartment *compartment);

typedef void
(* JSCompartmentNameCallback)(JSRuntime *rt, JSCompartment *compartment,
                              char *buf, size_t bufsize);










typedef JSObject *(*ReadStructuredCloneOp)(JSContext *cx, JSStructuredCloneReader *r,
                                           uint32_t tag, uint32_t data, void *closure);












typedef JSBool (*WriteStructuredCloneOp)(JSContext *cx, JSStructuredCloneWriter *w,
                                         JSObject *obj, void *closure);






typedef void (*StructuredCloneErrorOp)(JSContext *cx, uint32_t errorid);









extern JS_PUBLIC_DATA(const jsval) JSVAL_NULL;
extern JS_PUBLIC_DATA(const jsval) JSVAL_ZERO;
extern JS_PUBLIC_DATA(const jsval) JSVAL_ONE;
extern JS_PUBLIC_DATA(const jsval) JSVAL_FALSE;
extern JS_PUBLIC_DATA(const jsval) JSVAL_TRUE;
extern JS_PUBLIC_DATA(const jsval) JSVAL_VOID;

static JS_ALWAYS_INLINE jsval
JS_NumberValue(double d)
{
    int32_t i;
    d = JS_CANONICALIZE_NAN(d);
    if (MOZ_DOUBLE_IS_INT32(d, &i))
        return INT_TO_JSVAL(i);
    return DOUBLE_TO_JSVAL(d);
}













#define JSID_TYPE_STRING                 0x0
#define JSID_TYPE_INT                    0x1
#define JSID_TYPE_VOID                   0x2
#define JSID_TYPE_OBJECT                 0x4
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








JS_PUBLIC_API(jsid)
INTERNED_STRING_TO_JSID(JSContext *cx, JSString *str);

static JS_ALWAYS_INLINE JSBool
JSID_IS_INT(jsid id)
{
    return !!(JSID_BITS(id) & JSID_TYPE_INT);
}

static JS_ALWAYS_INLINE int32_t
JSID_TO_INT(jsid id)
{
    JS_ASSERT(JSID_IS_INT(id));
    return ((uint32_t)JSID_BITS(id)) >> 1;
}

#define JSID_INT_MIN  0
#define JSID_INT_MAX  INT32_MAX

static JS_ALWAYS_INLINE JSBool
INT_FITS_IN_JSID(int32_t i)
{
    return i >= 0;
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
OBJECT_TO_JSID(JSRawObject obj)
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
JSID_IS_VOID(const js::RawId id)
{
    JS_ASSERT_IF(((size_t)JSID_BITS(id) & JSID_TYPE_MASK) == JSID_TYPE_VOID,
                 JSID_BITS(id) == JSID_TYPE_VOID);
    return ((size_t)JSID_BITS(id) == JSID_TYPE_VOID);
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_EMPTY(const js::RawId id)
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

namespace JS {

class AutoIdRooter : private AutoGCRooter
{
  public:
    explicit AutoIdRooter(JSContext *cx, jsid aId = INT_TO_JSID(0)
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ID), id_(aId)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
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
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 




#define JSPROP_ENUMERATE        0x01    /* property is visible to for/in loop */
#define JSPROP_READONLY         0x02    /* not settable: assignment is no-op.
                                           This flag is only valid when neither
                                           JSPROP_GETTER nor JSPROP_SETTER is
                                           set. */
#define JSPROP_PERMANENT        0x04    /* property cannot be deleted */
#define JSPROP_NATIVE_ACCESSORS 0x08    /* set in JSPropertyDescriptor.flags
                                           if getters/setters are JSNatives */
#define JSPROP_GETTER           0x10    /* property holds getter function */
#define JSPROP_SETTER           0x20    /* property holds setter function */
#define JSPROP_SHARED           0x40    /* don't allocate a value slot for this
                                           property; don't copy the property on
                                           set of the same-named property in an
                                           object that delegates to a prototype
                                           containing this property */
#define JSPROP_INDEX            0x80    /* name is actually (int) index */
#define JSPROP_SHORTID         0x100    /* set in JS_DefineProperty attrs
                                           if getters/setters use a shortid */

#define JSFUN_STUB_GSOPS       0x200    /* use JS_PropertyStub getter/setter
                                           instead of defaulting to class gsops
                                           for property holding function */

#define JSFUN_CONSTRUCTOR      0x400    /* native that can be called as a ctor */












#define JSFUN_GENERIC_NATIVE   0x800

#define JSFUN_FLAGS_MASK       0xe00    /* | of all the JSFUN_* flags */









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

namespace js {



extern JS_PUBLIC_API(bool)
ToNumberSlow(JSContext *cx, JS::Value v, double *dp);




extern JS_PUBLIC_API(bool)
ToBooleanSlow(const JS::Value &v);
} 

namespace JS {


JS_ALWAYS_INLINE bool
ToNumber(JSContext *cx, const Value &v, double *out)
{
    AssertArgumentsAreSane(cx, v);
    {
        js::SkipRoot root(cx, &v);
        js::MaybeCheckStackRoots(cx);
    }

    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    return js::ToNumberSlow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToBoolean(const Value &v)
{
    if (v.isBoolean())
        return v.toBoolean();
    if (v.isInt32())
        return v.toInt32() != 0;
    if (v.isNullOrUndefined())
        return false;
    if (v.isDouble()) {
        double d = v.toDouble();
        return !MOZ_DOUBLE_IS_NaN(d) && d != 0;
    }

    
    return js::ToBooleanSlow(v);
}

} 

extern JS_PUBLIC_API(JSBool)
JS_DoubleIsInt32(double d, int32_t *ip);

extern JS_PUBLIC_API(int32_t)
JS_DoubleToInt32(double d);

extern JS_PUBLIC_API(uint32_t)
JS_DoubleToUint32(double d);





extern JS_PUBLIC_API(JSBool)
JS_ValueToECMAInt32(JSContext *cx, jsval v, int32_t *ip);





extern JS_PUBLIC_API(JSBool)
JS_ValueToInt64(JSContext *cx, jsval v, int64_t *ip);





extern JS_PUBLIC_API(JSBool)
JS_ValueToUint64(JSContext *cx, jsval v, uint64_t *ip);

namespace js {

extern JS_PUBLIC_API(bool)
ToUint16Slow(JSContext *cx, const JS::Value &v, uint16_t *out);


extern JS_PUBLIC_API(bool)
ToInt32Slow(JSContext *cx, const JS::Value &v, int32_t *out);


extern JS_PUBLIC_API(bool)
ToUint32Slow(JSContext *cx, const JS::Value &v, uint32_t *out);


extern JS_PUBLIC_API(bool)
ToInt64Slow(JSContext *cx, const JS::Value &v, int64_t *out);


extern JS_PUBLIC_API(bool)
ToUint64Slow(JSContext *cx, const JS::Value &v, uint64_t *out);
} 

namespace JS {

JS_ALWAYS_INLINE bool
ToUint16(JSContext *cx, const js::Value &v, uint16_t *out)
{
    AssertArgumentsAreSane(cx, v);
    {
        js::SkipRoot skip(cx, &v);
        js::MaybeCheckStackRoots(cx);
    }

    if (v.isInt32()) {
        *out = uint16_t(v.toInt32());
        return true;
    }
    return js::ToUint16Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    AssertArgumentsAreSane(cx, v);
    {
        js::SkipRoot root(cx, &v);
        js::MaybeCheckStackRoots(cx);
    }

    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    return js::ToInt32Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToUint32(JSContext *cx, const js::Value &v, uint32_t *out)
{
    AssertArgumentsAreSane(cx, v);
    {
        js::SkipRoot root(cx, &v);
        js::MaybeCheckStackRoots(cx);
    }

    if (v.isInt32()) {
        *out = uint32_t(v.toInt32());
        return true;
    }
    return js::ToUint32Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToInt64(JSContext *cx, const js::Value &v, int64_t *out)
{
    AssertArgumentsAreSane(cx, v);
    {
        js::SkipRoot skip(cx, &v);
        js::MaybeCheckStackRoots(cx);
    }

    if (v.isInt32()) {
        *out = int64_t(v.toInt32());
        return true;
    }

    return js::ToInt64Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToUint64(JSContext *cx, const js::Value &v, uint64_t *out)
{
    AssertArgumentsAreSane(cx, v);
    {
        js::SkipRoot skip(cx, &v);
        js::MaybeCheckStackRoots(cx);
    }

    if (v.isInt32()) {
        
        *out = uint64_t(int64_t(v.toInt32()));
        return true;
    }

    return js::ToUint64Slow(cx, v, out);
}


} 





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











typedef enum JSUseHelperThreads
{
    JS_NO_HELPER_THREADS,
    JS_USE_HELPER_THREADS
} JSUseHelperThreads;

extern JS_PUBLIC_API(JSRuntime *)
JS_NewRuntime(uint32_t maxbytes, JSUseHelperThreads useHelperThreads);

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

extern JS_PUBLIC_API(JSBool)
JS_IsInRequest(JSRuntime *rt);

namespace JS {

inline bool
IsPoisonedId(jsid iden)
{
    if (JSID_IS_STRING(iden))
        return JS::IsPoisonedPtr(JSID_TO_STRING(iden));
    if (JSID_IS_OBJECT(iden))
        return JS::IsPoisonedPtr(JSID_TO_OBJECT(iden));
    return false;
}

} 

namespace js {

template <> struct RootMethods<jsid>
{
    static jsid initial() { return JSID_VOID; }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(jsid id) { return IsPoisonedId(id); }
};

} 

class JSAutoRequest
{
  public:
    JSAutoRequest(JSContext *cx
                  MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        JS_BeginRequest(mContext);
    }
    ~JSAutoRequest() {
        JS_EndRequest(mContext);
    }

  protected:
    JSContext *mContext;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

#if 0
  private:
    static void *operator new(size_t) CPP_THROW_NEW { return 0; };
    static void operator delete(void *, size_t) { };
#endif
};

class JSAutoCheckRequest
{
  public:
    JSAutoCheckRequest(JSContext *cx
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
#if defined JS_THREADSAFE && defined DEBUG
        mContext = cx;
        JS_ASSERT(JS_IsInRequest(JS_GetRuntime(cx)));
#endif
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
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
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

extern JS_PUBLIC_API(JSContextCallback)
JS_SetContextCallback(JSRuntime *rt, JSContextCallback cxCallback);

extern JS_PUBLIC_API(JSContext *)
JS_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern JS_PUBLIC_API(void)
JS_DestroyContext(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_DestroyContextNoGC(JSContext *cx);

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





#define JSOPTION_DONT_REPORT_UNCAUGHT                                   \
                                JS_BIT(8)       /* When returning from the
                                                   outermost API call, prevent
                                                   uncaught exceptions from
                                                   being converted to error
                                                   reports */







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
#define JSOPTION_STRICT_MODE    JS_BIT(19)      /* Provides a way to force
                                                   strict mode for all code
                                                   without requiring
                                                   "use strict" annotations. */

#define JSOPTION_ION            JS_BIT(20)      /* IonMonkey */

#define JSOPTION_MASK           JS_BITMASK(21)

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

extern JS_PUBLIC_API(void)
JS_SetDestroyCompartmentCallback(JSRuntime *rt, JSDestroyCompartmentCallback callback);

extern JS_PUBLIC_API(void)
JS_SetCompartmentNameCallback(JSRuntime *rt, JSCompartmentNameCallback callback);

extern JS_PUBLIC_API(JSWrapObjectCallback)
JS_SetWrapObjectCallbacks(JSRuntime *rt,
                          JSWrapObjectCallback callback,
                          JSSameCompartmentWrapObjectCallback sccallback,
                          JSPreWrapCallback precallback);

extern JS_PUBLIC_API(void)
JS_SetCompartmentPrivate(JSCompartment *compartment, void *data);

extern JS_PUBLIC_API(void *)
JS_GetCompartmentPrivate(JSCompartment *compartment);

extern JS_PUBLIC_API(JSBool)
JS_WrapObject(JSContext *cx, JSObject **objp);

extern JS_PUBLIC_API(JSBool)
JS_WrapValue(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_WrapId(JSContext *cx, jsid *idp);

extern JS_PUBLIC_API(JSObject *)
JS_TransplantObject(JSContext *cx, JSObject *origobj, JSObject *target);

extern JS_FRIEND_API(JSObject *)
js_TransplantObjectWithWrapper(JSContext *cx,
                               JSObject *origobj,
                               JSObject *origwrapper,
                               JSObject *targetobj,
                               JSObject *targetwrapper);

extern JS_PUBLIC_API(JSBool)
JS_RefreshCrossCompartmentWrappers(JSContext *cx, JSObject *ob);




































class JS_PUBLIC_API(JSAutoCompartment)
{
    JSContext *cx_;
    JSCompartment *oldCompartment_;
  public:
    JSAutoCompartment(JSContext *cx, JSRawObject target);
    JSAutoCompartment(JSContext *cx, JSScript *target);
    JSAutoCompartment(JSContext *cx, JSString *target);
    ~JSAutoCompartment();
};


extern JS_PUBLIC_API(JSCompartment *)
JS_EnterCompartment(JSContext *cx, JSRawObject target);

extern JS_PUBLIC_API(void)
JS_LeaveCompartment(JSContext *cx, JSCompartment *oldCompartment);

typedef void (*JSIterateCompartmentCallback)(JSRuntime *rt, void *data, JSCompartment *compartment);






extern JS_PUBLIC_API(void)
JS_IterateCompartments(JSRuntime *rt, void *data,
                       JSIterateCompartmentCallback compartmentCallback);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalObject(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetGlobalObject(JSContext *cx, JSRawObject obj);








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
JS_GetClassObject(JSContext *cx, JSRawObject obj, JSProtoKey key, JSObject **objp);

extern JS_PUBLIC_API(JSBool)
JS_GetClassPrototype(JSContext *cx, JSProtoKey key, JSObject **objp);

extern JS_PUBLIC_API(JSProtoKey)
JS_IdentifyClassPrototype(JSContext *cx, JSObject *obj);





extern JS_PUBLIC_API(JSObject *)
JS_GetFunctionPrototype(JSContext *cx, JSRawObject forObj);





extern JS_PUBLIC_API(JSObject *)
JS_GetObjectPrototype(JSContext *cx, JSRawObject forObj);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalForObject(JSContext *cx, JSRawObject obj);

extern JS_PUBLIC_API(JSBool)
JS_IsGlobalObject(JSRawObject obj);





extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalForCompartmentOrNull(JSContext *cx, JSCompartment *c);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalForScopeChain(JSContext *cx);

extern JS_PUBLIC_API(JSObject *)
JS_GetScriptedGlobal(JSContext *cx);




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
JS_SetCTypesCallbacks(JSRawObject ctypesObj, JSCTypesCallbacks *callbacks);
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

#undef JS_THIS
static JS_ALWAYS_INLINE jsval
JS_THIS(JSContext *cx, jsval *vp)
{
    return JSVAL_IS_PRIMITIVE(vp[1]) ? JS_ComputeThis(cx, vp) : vp[1];
}













#define JS_THIS_VALUE(cx,vp)    ((vp)[1])

extern JS_PUBLIC_API(void *)
JS_malloc(JSContext *cx, size_t nbytes);

extern JS_PUBLIC_API(void *)
JS_realloc(JSContext *cx, void *p, size_t nbytes);






extern JS_PUBLIC_API(void)
JS_free(JSContext *cx, void *p);





extern JS_PUBLIC_API(void)
JS_freeop(JSFreeOp *fop, void *p);

extern JS_PUBLIC_API(JSFreeOp *)
JS_GetDefaultFreeOp(JSRuntime *rt);

extern JS_PUBLIC_API(void)
JS_updateMallocCounter(JSContext *cx, size_t nbytes);

extern JS_PUBLIC_API(char *)
JS_strdup(JSContext *cx, const char *s);


extern JS_PUBLIC_API(char *)
JS_strdup(JSRuntime *rt, const char *s);



























extern JS_PUBLIC_API(JSBool)
JS_AddValueRoot(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_AddStringRoot(JSContext *cx, JSString **rp);

extern JS_PUBLIC_API(JSBool)
JS_AddObjectRoot(JSContext *cx, JSObject **rp);

#ifdef NAME_ALL_GC_ROOTS
#define JS_DEFINE_TO_TOKEN(def) #def
#define JS_DEFINE_TO_STRING(def) JS_DEFINE_TO_TOKEN(def)
#define JS_AddValueRoot(cx,vp) JS_AddNamedValueRoot((cx), (vp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#define JS_AddStringRoot(cx,rp) JS_AddNamedStringRoot((cx), (rp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#define JS_AddObjectRoot(cx,rp) JS_AddNamedObjectRoot((cx), (rp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#endif

extern JS_PUBLIC_API(JSBool)
JS_AddNamedValueRoot(JSContext *cx, jsval *vp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedValueRootRT(JSRuntime *rt, jsval *vp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedStringRoot(JSContext *cx, JSString **rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedObjectRoot(JSContext *cx, JSObject **rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedScriptRoot(JSContext *cx, JSScript **rp, const char *name);

extern JS_PUBLIC_API(void)
JS_RemoveValueRoot(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(void)
JS_RemoveStringRoot(JSContext *cx, JSString **rp);

extern JS_PUBLIC_API(void)
JS_RemoveObjectRoot(JSContext *cx, JSObject **rp);

extern JS_PUBLIC_API(void)
JS_RemoveScriptRoot(JSContext *cx, JSScript **rp);

extern JS_PUBLIC_API(void)
JS_RemoveValueRootRT(JSRuntime *rt, jsval *vp);

extern JS_PUBLIC_API(void)
JS_RemoveStringRootRT(JSRuntime *rt, JSString **rp);

extern JS_PUBLIC_API(void)
JS_RemoveObjectRootRT(JSRuntime *rt, JSObject **rp);

extern JS_PUBLIC_API(void)
JS_RemoveScriptRootRT(JSRuntime *rt, JSScript **rp);



extern JS_FRIEND_API(void)
js_RemoveRoot(JSRuntime *rt, void *rp);





extern JS_NEVER_INLINE JS_PUBLIC_API(void)
JS_AnchorPtr(void *p);

extern JS_PUBLIC_API(JSBool)
JS_LockGCThingRT(JSRuntime *rt, void *gcthing);

extern JS_PUBLIC_API(JSBool)
JS_UnlockGCThingRT(JSRuntime *rt, void *gcthing);








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
#ifdef JS_GC_ZEAL
    void                *realLocation;
#endif
};







extern JS_PUBLIC_API(void)
JS_CallTracer(JSTracer *trc, void *thing, JSGCTraceKind kind);

















# define JS_SET_TRACING_DETAILS(trc, printer, arg, index)                     \
    JS_BEGIN_MACRO                                                            \
        (trc)->debugPrinter = (printer);                                      \
        (trc)->debugPrintArg = (arg);                                         \
        (trc)->debugPrintIndex = (index);                                     \
    JS_END_MACRO









#ifdef JS_GC_ZEAL
# define JS_SET_TRACING_LOCATION(trc, location)                               \
    JS_BEGIN_MACRO                                                            \
        if (!(trc)->realLocation || !(location))                              \
            (trc)->realLocation = (location);                                 \
    JS_END_MACRO
# define JS_UNSET_TRACING_LOCATION(trc)                                       \
    JS_BEGIN_MACRO                                                            \
        (trc)->realLocation = NULL;                                           \
    JS_END_MACRO
#else
# define JS_SET_TRACING_LOCATION(trc, location)                               \
    JS_BEGIN_MACRO                                                            \
    JS_END_MACRO
# define JS_UNSET_TRACING_LOCATION(trc)                                       \
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

#define JS_CALL_SCRIPT_TRACER(trc, script, name)                              \
    JS_BEGIN_MACRO                                                            \
        JSScript *script_ = (script);                                         \
        JS_ASSERT(script_);                                                   \
        JS_CALL_TRACER((trc), script_, JSTRACE_SCRIPT, name);                 \
    JS_END_MACRO




extern JS_PUBLIC_API(void)
JS_TracerInit(JSTracer *trc, JSRuntime *rt, JSTraceCallback callback);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                     void *thing, JSGCTraceKind kind, JSBool includeDetails);

extern JS_PUBLIC_API(const char *)
JS_GetTraceEdgeName(JSTracer *trc, char *buffer, int bufferSize);

#ifdef DEBUG
















extern JS_PUBLIC_API(JSBool)
JS_DumpHeap(JSRuntime *rt, FILE *fp, void* startThing, JSGCTraceKind kind,
            void *thingToFind, size_t maxDepth, void *thingToIgnore);

#endif




extern JS_PUBLIC_API(void)
JS_GC(JSRuntime *rt);

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

    
    JSGC_MARK_STACK_LIMIT = 10,

    



    JSGC_HIGH_FREQUENCY_TIME_LIMIT = 11,

    
    JSGC_HIGH_FREQUENCY_LOW_LIMIT = 12,

    
    JSGC_HIGH_FREQUENCY_HIGH_LIMIT = 13,

    
    JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX = 14,

    
    JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN = 15,

    
    JSGC_LOW_FREQUENCY_HEAP_GROWTH = 16,

    



    JSGC_DYNAMIC_HEAP_GROWTH = 17,

    
    JSGC_DYNAMIC_MARK_SLICE = 18,

    
    JSGC_ANALYSIS_PURGE_TRIGGER = 19,

    
    JSGC_ALLOCATION_THRESHOLD = 20,

    
    JSGC_ENABLE_GENERATIONAL = 21
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
    JSHasInstanceOp     hasInstance;
    JSNative            construct;
    JSTraceOp           trace;

    void                *reserved[40];
};

#define JSCLASS_HAS_PRIVATE             (1<<0)  /* objects have private slot */
#define JSCLASS_NEW_ENUMERATE           (1<<1)  /* has JSNewEnumerateOp hook */
#define JSCLASS_NEW_RESOLVE             (1<<2)  /* has JSNewResolveOp hook */
#define JSCLASS_PRIVATE_IS_NSISUPPORTS  (1<<3)  /* private is (nsISupports *) */
#define JSCLASS_IS_DOMJSCLASS           (1<<4)  /* objects are DOM */
#define JSCLASS_IMPLEMENTS_BARRIERS     (1<<5)  /* Correctly implements GC read
                                                   and write barriers */
#define JSCLASS_EMULATES_UNDEFINED      (1<<6)  


#define JSCLASS_USERBIT1                (1<<7)  /* Reserved for embeddings. */






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

#define JSCLASS_IS_ANONYMOUS            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+0))
#define JSCLASS_IS_GLOBAL               (1<<(JSCLASS_HIGH_FLAGS_SHIFT+1))
#define JSCLASS_INTERNAL_FLAG2          (1<<(JSCLASS_HIGH_FLAGS_SHIFT+2))
#define JSCLASS_INTERNAL_FLAG3          (1<<(JSCLASS_HIGH_FLAGS_SHIFT+3))


#define JSCLASS_FREEZE_PROTO            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+4))
#define JSCLASS_FREEZE_CTOR             (1<<(JSCLASS_HIGH_FLAGS_SHIFT+5))


#define JSCLASS_USERBIT2                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+6))
#define JSCLASS_USERBIT3                (1<<(JSCLASS_HIGH_FLAGS_SHIFT+7))

#define JSCLASS_BACKGROUND_FINALIZE     (1<<(JSCLASS_HIGH_FLAGS_SHIFT+8))







#define JSGLOBAL_FLAGS_CLEARED          0x1












#define JSCLASS_GLOBAL_SLOT_COUNT      (JSProto_LIMIT * 3 + 26)
#define JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(n)                                    \
    (JSCLASS_IS_GLOBAL | JSCLASS_HAS_RESERVED_SLOTS(JSCLASS_GLOBAL_SLOT_COUNT + (n)))
#define JSCLASS_GLOBAL_FLAGS                                                  \
    JSCLASS_GLOBAL_FLAGS_WITH_SLOTS(0)
#define JSCLASS_HAS_GLOBAL_FLAG_AND_SLOTS(clasp)                              \
  (((clasp)->flags & JSCLASS_IS_GLOBAL)                                       \
   && JSCLASS_RESERVED_SLOTS(clasp) >= JSCLASS_GLOBAL_SLOT_COUNT)


#define JSCLASS_CACHED_PROTO_SHIFT      (JSCLASS_HIGH_FLAGS_SHIFT + 10)
#define JSCLASS_CACHED_PROTO_WIDTH      6
#define JSCLASS_CACHED_PROTO_MASK       JS_BITMASK(JSCLASS_CACHED_PROTO_WIDTH)
#define JSCLASS_HAS_CACHED_PROTO(key)   (uint32_t(key) << JSCLASS_CACHED_PROTO_SHIFT)
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

namespace JS {

class AutoIdArray : private AutoGCRooter
{
  public:
    AutoIdArray(JSContext *cx, JSIdArray *ida
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, IDARRAY), context(cx), idArray(ida)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
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
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoIdArray(AutoIdArray &ida) MOZ_DELETE;
    void operator=(AutoIdArray &ida) MOZ_DELETE;
};

} 

extern JS_PUBLIC_API(JSBool)
JS_ValueToId(JSContext *cx, jsval v, jsid *idp);

extern JS_PUBLIC_API(JSBool)
JS_IdToValue(JSContext *cx, jsid id, jsval *vp);




#define JSRESOLVE_ASSIGNING     0x01    /* resolve on the left of assignment */







extern JS_PUBLIC_API(JSBool)
JS_DefaultValue(JSContext *cx, JSObject *obj, JSType hint, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_PropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSMutableHandleValue vp);

extern JS_PUBLIC_API(JSBool)
JS_StrictPropertyStub(JSContext *cx, JSHandleObject obj, JSHandleId id, JSBool strict, JSMutableHandleValue vp);

extern JS_PUBLIC_API(JSBool)
JS_EnumerateStub(JSContext *cx, JSHandleObject obj);

extern JS_PUBLIC_API(JSBool)
JS_ResolveStub(JSContext *cx, JSHandleObject obj, JSHandleId id);

extern JS_PUBLIC_API(JSBool)
JS_ConvertStub(JSContext *cx, JSHandleObject obj, JSType type, JSMutableHandleValue vp);

struct JSConstDoubleSpec {
    double          dval;
    const char      *name;
    uint8_t         flags;
    uint8_t         spare[3];
};

typedef struct JSJitInfo JSJitInfo;






typedef struct JSStrictPropertyOpWrapper {
    JSStrictPropertyOp  op;
    const JSJitInfo     *info;
} JSStrictPropertyOpWrapper;

typedef struct JSPropertyOpWrapper {
    JSPropertyOp        op;
    const JSJitInfo     *info;
} JSPropertyOpWrapper;




typedef struct JSNativeWrapper {
    JSNative        op;
    const JSJitInfo *info;
} JSNativeWrapper;





#define JSOP_WRAPPER(op) {op, NULL}
#define JSOP_NULLWRAPPER JSOP_WRAPPER(NULL)






struct JSPropertySpec {
    const char                  *name;
    int8_t                      tinyid;
    uint8_t                     flags;
    JSPropertyOpWrapper         getter;
    JSStrictPropertyOpWrapper   setter;
};






struct JSFunctionSpec {
    const char      *name;
    JSNativeWrapper call;
    uint16_t        nargs;
    uint16_t        flags;
    const char      *selfHostedName;
};





#define JS_FS_END JS_FS(NULL,NULL,0,0)






#define JS_FS(name,call,nargs,flags)                                          \
    {name, JSOP_WRAPPER(call), nargs, flags}
#define JS_FN(name,call,nargs,flags)                                          \
    {name, JSOP_WRAPPER(call), nargs, (flags) | JSFUN_STUB_GSOPS}
#define JS_FNINFO(name,call,info,nargs,flags)                                 \
    {name,{call,info},nargs,flags}

extern JS_PUBLIC_API(JSObject *)
JS_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             JSClass *clasp, JSNative constructor, unsigned nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);





extern JS_PUBLIC_API(JSBool)
JS_LinkConstructorAndPrototype(JSContext *cx, JSObject *ctor, JSObject *proto);

extern JS_PUBLIC_API(JSClass *)
JS_GetClass(JSRawObject obj);

extern JS_PUBLIC_API(JSBool)
JS_InstanceOf(JSContext *cx, JSObject *obj, JSClass *clasp, jsval *argv);

extern JS_PUBLIC_API(JSBool)
JS_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

extern JS_PUBLIC_API(void *)
JS_GetPrivate(JSRawObject obj);

extern JS_PUBLIC_API(void)
JS_SetPrivate(JSRawObject obj, void *data);

extern JS_PUBLIC_API(void *)
JS_GetInstancePrivate(JSContext *cx, JSObject *obj, JSClass *clasp,
                      jsval *argv);

extern JS_PUBLIC_API(JSBool)
JS_GetPrototype(JSContext *cx, JSObject *obj, JSObject **protop);

extern JS_PUBLIC_API(JSBool)
JS_SetPrototype(JSContext *cx, JSObject *obj, JSObject *proto);

extern JS_PUBLIC_API(JSObject *)
JS_GetParent(JSRawObject obj);

extern JS_PUBLIC_API(JSBool)
JS_SetParent(JSContext *cx, JSObject *obj, JSObject *parent);

extern JS_PUBLIC_API(JSObject *)
JS_GetConstructor(JSContext *cx, JSObject *proto);






extern JS_PUBLIC_API(JSBool)
JS_GetObjectId(JSContext *cx, JSRawObject obj, jsid *idp);

extern JS_PUBLIC_API(JSObject *)
JS_NewGlobalObject(JSContext *cx, JSClass *clasp, JSPrincipals *principals);

extern JS_PUBLIC_API(JSObject *)
JS_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);


extern JS_PUBLIC_API(JSBool)
JS_IsExtensible(JSRawObject obj);

extern JS_PUBLIC_API(JSBool)
JS_IsNative(JSRawObject obj);

extern JS_PUBLIC_API(JSRuntime *)
JS_GetObjectRuntime(JSRawObject obj);





extern JS_PUBLIC_API(JSObject *)
JS_NewObjectWithGivenProto(JSContext *cx, JSClass *clasp, JSObject *proto,
                           JSObject *parent);






extern JS_PUBLIC_API(JSBool)
JS_DeepFreezeObject(JSContext *cx, JSObject *obj);




extern JS_PUBLIC_API(JSBool)
JS_FreezeObject(JSContext *cx, JSObject *obj);

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
    unsigned           attrs;
    unsigned           shortid;
    JSPropertyOp       getter;
    JSStrictPropertyOp setter;
    jsval              value;

    JSPropertyDescriptor() : obj(NULL), attrs(0), shortid(0), getter(NULL),
                             setter(NULL), value(JSVAL_VOID)
    {}
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





JS_PUBLIC_API(void)
JS_ClearNonGlobalObject(JSContext *cx, JSObject *objArg);





JS_PUBLIC_API(void)
JS_SetAllNonReservedSlotsToUndefined(JSContext *cx, JSObject *objArg);







extern JS_PUBLIC_API(JSObject *)
JS_NewArrayBufferWithContents(JSContext *cx, void *contents);










extern JS_PUBLIC_API(JSBool)
JS_StealArrayBufferContents(JSContext *cx, JSObject *obj, void **contents,
                            uint8_t **data);










extern JS_PUBLIC_API(JSBool)
JS_AllocateArrayBufferContents(JSContext *cx, uint32_t nbytes, void **contents, uint8_t **data);


extern JS_PUBLIC_API(JSIdArray *)
JS_Enumerate(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSObject *)
JS_NewPropertyIterator(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSBool)
JS_NextProperty(JSContext *cx, JSObject *iterobj, jsid *idp);







extern JS_PUBLIC_API(JSBool)
JS_ArrayIterator(JSContext *cx, unsigned argc, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               jsval *vp, unsigned *attrsp);

extern JS_PUBLIC_API(jsval)
JS_GetReservedSlot(JSRawObject obj, uint32_t index);

extern JS_PUBLIC_API(void)
JS_SetReservedSlot(JSRawObject obj, uint32_t index, jsval v);






struct JSPrincipals {
    
    int refcount;

#ifdef DEBUG
    
    uint32_t    debugToken;
#endif

    void setDebugToken(uint32_t token) {
# ifdef DEBUG
        debugToken = token;
# endif
    }

    



    JS_PUBLIC_API(void) dump();
};

extern JS_PUBLIC_API(void)
JS_HoldPrincipals(JSPrincipals *principals);

extern JS_PUBLIC_API(void)
JS_DropPrincipals(JSRuntime *rt, JSPrincipals *principals);

struct JSSecurityCallbacks {
    JSCheckAccessOp            checkObjectAccess;
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








extern JS_PUBLIC_API(JSString *)
JS_GetFunctionDisplayId(JSFunction *fun);




extern JS_PUBLIC_API(uint16_t)
JS_GetFunctionArity(JSFunction *fun);







extern JS_PUBLIC_API(JSBool)
JS_ObjectIsFunction(JSContext *cx, JSRawObject obj);

extern JS_PUBLIC_API(JSBool)
JS_ObjectIsCallable(JSContext *cx, JSRawObject obj);

extern JS_PUBLIC_API(JSBool)
JS_IsNativeFunction(JSRawObject funobj, JSNative call);


extern JS_PUBLIC_API(JSBool)
JS_IsConstructor(JSFunction *fun);






extern JS_PUBLIC_API(JSObject*)
JS_BindCallable(JSContext *cx, JSObject *callable, JSRawObject newThis);

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
JS_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSRawObject parent);








extern JS_PUBLIC_API(JSBool)
JS_BufferIsCompilableUnit(JSContext *cx, JSObject *obj, const char *utf8, size_t length);

extern JS_PUBLIC_API(JSScript *)
JS_CompileScript(JSContext *cx, JSObject *obj,
                 const char *ascii, size_t length,
                 const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileScriptForPrincipals(JSContext *cx, JSObject *obj,
                              JSPrincipals *principals,
                              const char *ascii, size_t length,
                              const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScript(JSContext *cx, JSObject *obj,
                   const jschar *chars, size_t length,
                   const char *filename, unsigned lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScriptForPrincipals(JSContext *cx, JSObject *obj,
                                JSPrincipals *principals,
                                const jschar *chars, size_t length,
                                const char *filename, unsigned lineno);

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

namespace JS {


struct JS_PUBLIC_API(CompileOptions) {
    JSPrincipals *principals;
    JSPrincipals *originPrincipals;
    JSVersion version;
    bool versionSet;
    bool utf8;
    const char *filename;
    unsigned lineno;
    bool compileAndGo;
    bool noScriptRval;
    bool selfHostingMode;
    bool userBit;
    enum SourcePolicy {
        NO_SOURCE,
        LAZY_SOURCE,
        SAVE_SOURCE
    } sourcePolicy;

    explicit CompileOptions(JSContext *cx);
    CompileOptions &setPrincipals(JSPrincipals *p) { principals = p; return *this; }
    CompileOptions &setOriginPrincipals(JSPrincipals *p) { originPrincipals = p; return *this; }
    CompileOptions &setVersion(JSVersion v) { version = v; versionSet = true; return *this; }
    CompileOptions &setUTF8(bool u) { utf8 = u; return *this; }
    CompileOptions &setFileAndLine(const char *f, unsigned l) {
        filename = f; lineno = l; return *this;
    }
    CompileOptions &setCompileAndGo(bool cng) { compileAndGo = cng; return *this; }
    CompileOptions &setNoScriptRval(bool nsr) { noScriptRval = nsr; return *this; }
    CompileOptions &setSelfHostingMode(bool shm) { selfHostingMode = shm; return *this; }
    CompileOptions &setUserBit(bool bit) { userBit = bit; return *this; }
    CompileOptions &setSourcePolicy(SourcePolicy sp) { sourcePolicy = sp; return *this; }
};

extern JS_PUBLIC_API(JSScript *)
Compile(JSContext *cx, JSHandleObject obj, CompileOptions options,
        const char *bytes, size_t length);

extern JS_PUBLIC_API(JSScript *)
Compile(JSContext *cx, JSHandleObject obj, CompileOptions options,
        const jschar *chars, size_t length);

extern JS_PUBLIC_API(JSScript *)
Compile(JSContext *cx, JSHandleObject obj, CompileOptions options, FILE *file);

extern JS_PUBLIC_API(JSScript *)
Compile(JSContext *cx, JSHandleObject obj, CompileOptions options, const char *filename);

extern JS_PUBLIC_API(JSFunction *)
CompileFunction(JSContext *cx, JSHandleObject obj, CompileOptions options,
                const char *name, unsigned nargs, const char **argnames,
                const char *bytes, size_t length);

extern JS_PUBLIC_API(JSFunction *)
CompileFunction(JSContext *cx, JSHandleObject obj, CompileOptions options,
                const char *name, unsigned nargs, const char **argnames,
                const jschar *chars, size_t length);

} 

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

namespace JS {

extern JS_PUBLIC_API(bool)
Evaluate(JSContext *cx, JSHandleObject obj, CompileOptions options,
         const jschar *chars, size_t length, jsval *rval);

extern JS_PUBLIC_API(bool)
Evaluate(JSContext *cx, JSHandleObject obj, CompileOptions options,
         const char *bytes, size_t length, jsval *rval);

extern JS_PUBLIC_API(bool)
Evaluate(JSContext *cx, JSHandleObject obj, CompileOptions options,
         const char *filename, jsval *rval);

} 

extern JS_PUBLIC_API(JSBool)
JS_CallFunction(JSContext *cx, JSObject *obj, JSFunction *fun, unsigned argc,
                jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunctionName(JSContext *cx, JSObject *obj, const char *name, unsigned argc,
                    jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunctionValue(JSContext *cx, JSObject *obj, jsval fval, unsigned argc,
                     jsval *argv, jsval *rval);

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
JS_InternStringN(JSContext *cx, const char *s, size_t length);

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














JS_PUBLIC_API(JSBool)
JS_DecodeBytes(JSContext *cx, const char *src, size_t srclen, jschar *dst,
               size_t *dstlenp);





JS_PUBLIC_API(char *)
JS_EncodeString(JSContext *cx, JSRawString str);




JS_PUBLIC_API(char *)
JS_EncodeStringToUTF8(JSContext *cx, JSRawString str);






JS_PUBLIC_API(size_t)
JS_GetStringEncodingLength(JSContext *cx, JSString *str);









JS_PUBLIC_API(size_t)
JS_EncodeStringToBuffer(JSContext *cx, JSString *str, char *buffer, size_t length);

class JSAutoByteString
{
  public:
    JSAutoByteString(JSContext *cx, JSString *str
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mBytes(JS_EncodeString(cx, str))
    {
        JS_ASSERT(cx);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JSAutoByteString(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
      : mBytes(NULL)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~JSAutoByteString() {
        js_free(mBytes);
    }

    
    void initBytes(char *bytes) {
        JS_ASSERT(!mBytes);
        mBytes = bytes;
    }

    char *encodeLatin1(JSContext *cx, JSString *str) {
        JS_ASSERT(!mBytes);
        JS_ASSERT(cx);
        mBytes = JS_EncodeString(cx, str);
        return mBytes;
    }

    char *encodeUtf8(JSContext *cx, JSString *str) {
        JS_ASSERT(!mBytes);
        JS_ASSERT(cx);
        mBytes = JS_EncodeStringToUTF8(cx, str);
        return mBytes;
    }

    void clear() {
        js_free(mBytes);
        mBytes = NULL;
    }

    char *ptr() const {
        return mBytes;
    }

    bool operator!() const {
        return !mBytes;
    }

    size_t length() const {
        if (!mBytes)
            return 0;
        return strlen(mBytes);
    }

  private:
    char        *mBytes;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    JSAutoByteString(const JSAutoByteString &another);
    JSAutoByteString &operator=(const JSAutoByteString &another);
};





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
JS_ReadStructuredClone(JSContext *cx, uint64_t *data, size_t nbytes,
                       uint32_t version, jsval *vp,
                       const JSStructuredCloneCallbacks *optionalCallbacks,
                       void *closure);



JS_PUBLIC_API(JSBool)
JS_WriteStructuredClone(JSContext *cx, jsval v, uint64_t **datap, size_t *nbytesp,
                        const JSStructuredCloneCallbacks *optionalCallbacks,
                        void *closure, jsval transferable);

JS_PUBLIC_API(JSBool)
JS_ClearStructuredClone(const uint64_t *data, size_t nbytes);

JS_PUBLIC_API(JSBool)
JS_StructuredCloneHasTransferables(const uint64_t *data, size_t nbytes,
                                   JSBool *hasTransferable);

JS_PUBLIC_API(JSBool)
JS_StructuredClone(JSContext *cx, jsval v, jsval *vp,
                   const JSStructuredCloneCallbacks *optionalCallbacks,
                   void *closure);


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
              void *closure=NULL);

    bool write(JSContext *cx, jsval v,
               const JSStructuredCloneCallbacks *optionalCallbacks=NULL,
               void *closure=NULL);

    bool write(JSContext *cx, jsval v,
               jsval transferable,
               const JSStructuredCloneCallbacks *optionalCallbacks=NULL,
               void *closure=NULL);

    


    void swap(JSAutoStructuredCloneBuffer &other);

  private:
    
    JSAutoStructuredCloneBuffer(const JSAutoStructuredCloneBuffer &other);
    JSAutoStructuredCloneBuffer &operator=(const JSAutoStructuredCloneBuffer &other);
};




#define JS_SCTAG_USER_MIN  ((uint32_t) 0xFFFF8000)
#define JS_SCTAG_USER_MAX  ((uint32_t) 0xFFFFFFFF)

#define JS_SCERR_RECURSION 0
#define JS_SCERR_TRANSFERABLE 1

JS_PUBLIC_API(void)
JS_SetStructuredCloneCallbacks(JSRuntime *rt, const JSStructuredCloneCallbacks *callbacks);

JS_PUBLIC_API(JSBool)
JS_ReadUint32Pair(JSStructuredCloneReader *r, uint32_t *p1, uint32_t *p2);

JS_PUBLIC_API(JSBool)
JS_ReadBytes(JSStructuredCloneReader *r, void *p, size_t len);

JS_PUBLIC_API(JSBool)
JS_ReadTypedArray(JSStructuredCloneReader *r, jsval *vp);

JS_PUBLIC_API(JSBool)
JS_WriteUint32Pair(JSStructuredCloneWriter *w, uint32_t tag, uint32_t data);

JS_PUBLIC_API(JSBool)
JS_WriteBytes(JSStructuredCloneWriter *w, const void *p, size_t len);

JS_PUBLIC_API(JSBool)
JS_WriteTypedArray(JSStructuredCloneWriter *w, jsval v);










extern JS_PUBLIC_API(JSBool)
JS_SetDefaultLocale(JSRuntime *rt, const char *locale);




extern JS_PUBLIC_API(void)
JS_ResetDefaultLocale(JSRuntime *rt);




struct JSLocaleCallbacks {
    JSLocaleToUpperCase     localeToUpperCase;
    JSLocaleToLowerCase     localeToLowerCase;
    JSLocaleCompare         localeCompare;
    JSLocaleToUnicode       localeToUnicode;
    JSErrorCallback         localeGetErrorMessage;
};





extern JS_PUBLIC_API(void)
JS_SetLocaleCallbacks(JSRuntime *rt, JSLocaleCallbacks *callbacks);





extern JS_PUBLIC_API(JSLocaleCallbacks *)
JS_GetLocaleCallbacks(JSRuntime *rt);












extern JS_PUBLIC_API(void)
JS_ReportError(JSContext *cx, const char *format, ...);




extern JS_PUBLIC_API(void)
JS_ReportErrorNumber(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const unsigned errorNumber, ...);

#ifdef va_start
extern JS_PUBLIC_API(void)
JS_ReportErrorNumberVA(JSContext *cx, JSErrorCallback errorCallback,
                       void *userRef, const unsigned errorNumber, va_list ap);
#endif




extern JS_PUBLIC_API(void)
JS_ReportErrorNumberUC(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const unsigned errorNumber, ...);

extern JS_PUBLIC_API(void)
JS_ReportErrorNumberUCArray(JSContext *cx, JSErrorCallback errorCallback,
                            void *userRef, const unsigned errorNumber,
                            const jschar **args);







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
    unsigned        lineno;         
    const char      *linebuf;       
    const char      *tokenptr;      
    const jschar    *uclinebuf;     
    const jschar    *uctokenptr;    
    unsigned        flags;          
    unsigned        errorNumber;    
    const jschar    *ucmessage;     
    const jschar    **messageArgs;  
    int16_t         exnType;        
    unsigned        column;         
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
JS_ObjectIsDate(JSContext *cx, JSRawObject obj);





extern JS_PUBLIC_API(void)
JS_ClearDateCaches(JSContext *cx);






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

class JSAutoSetRuntimeThread
{
    JSRuntime *runtime_;

  public:
    JSAutoSetRuntimeThread(JSRuntime *runtime) : runtime_(runtime) {
        JS_SetRuntimeThread(runtime_);
    }

    ~JSAutoSetRuntimeThread() {
        JS_ClearRuntimeThread(runtime_);
    }
};










static JS_ALWAYS_INLINE JSBool
JS_IsConstructing(JSContext *cx, const jsval *vp)
{
#ifdef DEBUG
    JSObject *callee = JSVAL_TO_OBJECT(JS_CALLEE(cx, vp));
    if (JS_ObjectIsFunction(cx, callee)) {
        JSFunction *fun = JS_ValueToFunction(cx, JS_CALLEE(cx, vp));
        JS_ASSERT(JS_IsConstructor(fun));
    } else {
        JS_ASSERT(JS_GetClass(callee)->construct != NULL);
    }
#else
    (void)cx;
#endif

    return JSVAL_IS_MAGIC_IMPL(JSVAL_TO_IMPL(vp[1]));
}






extern JS_PUBLIC_API(JSObject *)
JS_NewObjectForConstructor(JSContext *cx, JSClass *clasp, const jsval *vp);



#ifdef JS_GC_ZEAL
#define JS_DEFAULT_ZEAL_FREQ 100

extern JS_PUBLIC_API(void)
JS_SetGCZeal(JSContext *cx, uint8_t zeal, uint32_t frequency);

extern JS_PUBLIC_API(void)
JS_ScheduleGC(JSContext *cx, uint32_t count);
#endif

extern JS_PUBLIC_API(void)
JS_SetParallelCompilationEnabled(JSContext *cx, bool enabled);




extern JS_PUBLIC_API(JSBool)
JS_IndexToId(JSContext *cx, uint32_t index, jsid *id);






extern JS_PUBLIC_API(JSBool)
JS_CharsToId(JSContext* cx, JS::TwoByteChars chars, jsid *idp);




extern JS_PUBLIC_API(JSBool)
JS_IsIdentifier(JSContext *cx, JSString *str, JSBool *isIdentifier);





extern JS_PUBLIC_API(JSBool)
JS_DescribeScriptedCaller(JSContext *cx, JSScript **script, unsigned *lineno);






extern JS_PUBLIC_API(void *)
JS_EncodeScript(JSContext *cx, JSScript *script, uint32_t *lengthp);

extern JS_PUBLIC_API(void *)
JS_EncodeInterpretedFunction(JSContext *cx, JSRawObject funobj, uint32_t *lengthp);

extern JS_PUBLIC_API(JSScript *)
JS_DecodeScript(JSContext *cx, const void *data, uint32_t length,
                JSPrincipals *principals, JSPrincipals *originPrincipals);

extern JS_PUBLIC_API(JSObject *)
JS_DecodeInterpretedFunction(JSContext *cx, const void *data, uint32_t length,
                             JSPrincipals *principals, JSPrincipals *originPrincipals);

#endif
