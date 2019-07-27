







#ifndef jsapi_h
#define jsapi_h

#include "mozilla/FloatingPoint.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Range.h"
#include "mozilla/RangedPtr.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "jsalloc.h"
#include "jspubtd.h"

#include "js/CallArgs.h"
#include "js/Class.h"
#include "js/HashTable.h"
#include "js/Id.h"
#include "js/Principals.h"
#include "js/RootingAPI.h"
#include "js/TracingAPI.h"
#include "js/Utility.h"
#include "js/Value.h"
#include "js/Vector.h"



namespace JS {

class Latin1CharsZ;
class TwoByteChars;

#ifdef JS_DEBUG

class JS_PUBLIC_API(AutoCheckRequestDepth)
{
    JSContext* cx;
  public:
    explicit AutoCheckRequestDepth(JSContext* cx);
    explicit AutoCheckRequestDepth(js::ContextFriendFields* cx);
    ~AutoCheckRequestDepth();
};

# define CHECK_REQUEST(cx) \
    JS::AutoCheckRequestDepth _autoCheckRequestDepth(cx)

#else

# define CHECK_REQUEST(cx) \
    ((void) 0)

#endif 


template <size_t N>
class AutoValueArray : public AutoGCRooter
{
    const size_t length_;
    Value elements_[N];

  public:
    explicit AutoValueArray(JSContext* cx
                            MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, VALARRAY), length_(N)
    {
        
        mozilla::PodArrayZero(elements_);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    unsigned length() const { return length_; }
    const Value* begin() const { return elements_; }
    Value* begin() { return elements_; }

    HandleValue operator[](unsigned i) const {
        MOZ_ASSERT(i < N);
        return HandleValue::fromMarkedLocation(&elements_[i]);
    }
    MutableHandleValue operator[](unsigned i) {
        MOZ_ASSERT(i < N);
        return MutableHandleValue::fromMarkedLocation(&elements_[i]);
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<class T>
class AutoVectorRooter : protected AutoGCRooter
{
    typedef js::Vector<T, 8> VectorImpl;
    VectorImpl vector;

  public:
    explicit AutoVectorRooter(JSContext* cx, ptrdiff_t tag
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, tag), vector(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    explicit AutoVectorRooter(js::ContextFriendFields* cx, ptrdiff_t tag
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, tag), vector(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    typedef T ElementType;
    typedef typename VectorImpl::Range Range;

    size_t length() const { return vector.length(); }
    bool empty() const { return vector.empty(); }

    bool append(const T& v) { return vector.append(v); }
    bool appendN(const T& v, size_t len) { return vector.appendN(v, len); }
    bool append(const T* ptr, size_t len) { return vector.append(ptr, len); }
    bool appendAll(const AutoVectorRooter<T>& other) {
        return vector.appendAll(other.vector);
    }

    bool insert(T* p, const T& val) { return vector.insert(p, val); }

    
    void infallibleAppend(const T& v) { vector.infallibleAppend(v); }

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

    JS::MutableHandle<T> operator[](size_t i) {
        return JS::MutableHandle<T>::fromMarkedLocation(&vector[i]);
    }
    JS::Handle<T> operator[](size_t i) const {
        return JS::Handle<T>::fromMarkedLocation(&vector[i]);
    }

    const T* begin() const { return vector.begin(); }
    T* begin() { return vector.begin(); }

    const T* end() const { return vector.end(); }
    T* end() { return vector.end(); }

    Range all() { return vector.all(); }

    const T& back() const { return vector.back(); }

    friend void AutoGCRooter::trace(JSTracer* trc);

  private:
    void makeRangeGCSafe(size_t oldLength) {
        T* t = vector.begin() + oldLength;
        for (size_t i = oldLength; i < vector.length(); ++i, ++t)
            memset(t, 0, sizeof(T));
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<class Key, class Value>
class AutoHashMapRooter : protected AutoGCRooter
{
  private:
    typedef js::HashMap<Key, Value> HashMapImpl;

  public:
    explicit AutoHashMapRooter(JSContext* cx, ptrdiff_t tag
                               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, tag), map(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    typedef Key KeyType;
    typedef Value ValueType;
    typedef typename HashMapImpl::Entry Entry;
    typedef typename HashMapImpl::Lookup Lookup;
    typedef typename HashMapImpl::Ptr Ptr;
    typedef typename HashMapImpl::AddPtr AddPtr;

    bool init(uint32_t len = 16) {
        return map.init(len);
    }
    bool initialized() const {
        return map.initialized();
    }
    Ptr lookup(const Lookup& l) const {
        return map.lookup(l);
    }
    void remove(Ptr p) {
        map.remove(p);
    }
    AddPtr lookupForAdd(const Lookup& l) const {
        return map.lookupForAdd(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool add(AddPtr& p, const KeyInput& k, const ValueInput& v) {
        return map.add(p, k, v);
    }

    bool add(AddPtr& p, const Key& k) {
        return map.add(p, k);
    }

    template<typename KeyInput, typename ValueInput>
    bool relookupOrAdd(AddPtr& p, const KeyInput& k, const ValueInput& v) {
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

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return map.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return map.sizeOfIncludingThis(mallocSizeOf);
    }

    uint32_t generation() const {
        return map.generation();
    }

    

    bool has(const Lookup& l) const {
        return map.has(l);
    }

    template<typename KeyInput, typename ValueInput>
    bool put(const KeyInput& k, const ValueInput& v) {
        return map.put(k, v);
    }

    template<typename KeyInput, typename ValueInput>
    bool putNew(const KeyInput& k, const ValueInput& v) {
        return map.putNew(k, v);
    }

    Ptr lookupWithDefault(const Key& k, const Value& defaultValue) {
        return map.lookupWithDefault(k, defaultValue);
    }

    void remove(const Lookup& l) {
        map.remove(l);
    }

    friend void AutoGCRooter::trace(JSTracer* trc);

  private:
    AutoHashMapRooter(const AutoHashMapRooter& hmr) = delete;
    AutoHashMapRooter& operator=(const AutoHashMapRooter& hmr) = delete;

    HashMapImpl map;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<class T>
class AutoHashSetRooter : protected AutoGCRooter
{
  private:
    typedef js::HashSet<T> HashSetImpl;

  public:
    explicit AutoHashSetRooter(JSContext* cx, ptrdiff_t tag
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
    Ptr lookup(const Lookup& l) const {
        return set.lookup(l);
    }
    void remove(Ptr p) {
        set.remove(p);
    }
    AddPtr lookupForAdd(const Lookup& l) const {
        return set.lookupForAdd(l);
    }

    bool add(AddPtr& p, const T& t) {
        return set.add(p, t);
    }

    bool relookupOrAdd(AddPtr& p, const Lookup& l, const T& t) {
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

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return set.sizeOfExcludingThis(mallocSizeOf);
    }
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return set.sizeOfIncludingThis(mallocSizeOf);
    }

    uint32_t generation() const {
        return set.generation();
    }

    

    bool has(const Lookup& l) const {
        return set.has(l);
    }

    bool put(const T& t) {
        return set.put(t);
    }

    bool putNew(const T& t) {
        return set.putNew(t);
    }

    void remove(const Lookup& l) {
        set.remove(l);
    }

    friend void AutoGCRooter::trace(JSTracer* trc);

  private:
    AutoHashSetRooter(const AutoHashSetRooter& hmr) = delete;
    AutoHashSetRooter& operator=(const AutoHashSetRooter& hmr) = delete;

    HashSetImpl set;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class MOZ_STACK_CLASS AutoValueVector : public AutoVectorRooter<Value>
{
  public:
    explicit AutoValueVector(JSContext* cx
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
    explicit AutoIdVector(JSContext* cx
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<jsid>(cx, IDVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectVector : public AutoVectorRooter<JSObject*>
{
  public:
    explicit AutoObjectVector(JSContext* cx
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSObject*>(cx, OBJVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoFunctionVector : public AutoVectorRooter<JSFunction*>
{
  public:
    explicit AutoFunctionVector(JSContext* cx
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSFunction*>(cx, FUNVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    explicit AutoFunctionVector(js::ContextFriendFields* cx
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSFunction*>(cx, FUNVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoScriptVector : public AutoVectorRooter<JSScript*>
{
  public:
    explicit AutoScriptVector(JSContext* cx
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<JSScript*>(cx, SCRIPTVECTOR)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};




class JS_PUBLIC_API(CustomAutoRooter) : private AutoGCRooter
{
  public:
    template <typename CX>
    explicit CustomAutoRooter(CX* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, CUSTOM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer* trc);

  protected:
    
    virtual void trace(JSTracer* trc) = 0;

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};









template <class T>
class RootedGeneric : private CustomAutoRooter
{
  public:
    template <typename CX>
    explicit RootedGeneric(CX* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : CustomAutoRooter(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    template <typename CX>
    explicit RootedGeneric(CX* cx, const T& initial MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : CustomAutoRooter(cx), value(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    operator const T&() const { return value; }
    T operator->() const { return value; }

  private:
    virtual void trace(JSTracer* trc) { value->trace(trc); }

    T value;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


class HandleValueArray
{
    const size_t length_;
    const Value * const elements_;

    HandleValueArray(size_t len, const Value* elements) : length_(len), elements_(elements) {}

  public:
    explicit HandleValueArray(const RootedValue& value) : length_(1), elements_(value.address()) {}

    MOZ_IMPLICIT HandleValueArray(const AutoValueVector& values)
      : length_(values.length()), elements_(values.begin()) {}

    template <size_t N>
    HandleValueArray(const AutoValueArray<N>& values) : length_(N), elements_(values.begin()) {}

    
    MOZ_IMPLICIT HandleValueArray(const JS::CallArgs& args) : length_(args.length()), elements_(args.array()) {}

    
    static HandleValueArray fromMarkedLocation(size_t len, const Value* elements) {
        return HandleValueArray(len, elements);
    }

    static HandleValueArray subarray(const HandleValueArray& values, size_t startIndex, size_t len) {
        MOZ_ASSERT(startIndex + len <= values.length());
        return HandleValueArray(len, values.begin() + startIndex);
    }

    static HandleValueArray empty() {
        return HandleValueArray(0, nullptr);
    }

    size_t length() const { return length_; }
    const Value* begin() const { return elements_; }

    HandleValue operator[](size_t i) const {
        MOZ_ASSERT(i < length_);
        return HandleValue::fromMarkedLocation(&elements_[i]);
    }
};

}  



struct JSFreeOp {
  private:
    JSRuntime*  runtime_;

  protected:
    explicit JSFreeOp(JSRuntime* rt)
      : runtime_(rt) { }

  public:
    JSRuntime* runtime() const {
        return runtime_;
    }
};





typedef enum JSContextOp {
    JSCONTEXT_NEW,
    JSCONTEXT_DESTROY
} JSContextOp;














typedef bool
(* JSContextCallback)(JSContext* cx, unsigned contextOp, void* data);

typedef enum JSGCStatus {
    JSGC_BEGIN,
    JSGC_END
} JSGCStatus;

typedef void
(* JSGCCallback)(JSRuntime* rt, JSGCStatus status, void* data);

typedef enum JSFinalizeStatus {
    




    JSFINALIZE_GROUP_START,

    





    JSFINALIZE_GROUP_END,

    


    JSFINALIZE_COLLECTION_END
} JSFinalizeStatus;

typedef void
(* JSFinalizeCallback)(JSFreeOp* fop, JSFinalizeStatus status, bool isCompartment, void* data);

typedef void
(* JSWeakPointerCallback)(JSRuntime* rt, void* data);

typedef bool
(* JSInterruptCallback)(JSContext* cx);

typedef void
(* JSErrorReporter)(JSContext* cx, const char* message, JSErrorReport* report);






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
    
    const char* format;

    
    uint16_t argCount;

    
    int16_t exnType;
} JSErrorFormatString;

typedef const JSErrorFormatString*
(* JSErrorCallback)(void* userRef, const unsigned errorNumber);

typedef bool
(* JSLocaleToUpperCase)(JSContext* cx, JS::HandleString src, JS::MutableHandleValue rval);

typedef bool
(* JSLocaleToLowerCase)(JSContext* cx, JS::HandleString src, JS::MutableHandleValue rval);

typedef bool
(* JSLocaleCompare)(JSContext* cx, JS::HandleString src1, JS::HandleString src2,
                    JS::MutableHandleValue rval);

typedef bool
(* JSLocaleToUnicode)(JSContext* cx, const char* src, JS::MutableHandleValue rval);










typedef JSObject*
(* JSWrapObjectCallback)(JSContext* cx, JS::HandleObject existing, JS::HandleObject obj);






typedef JSObject*
(* JSPreWrapCallback)(JSContext* cx, JS::HandleObject scope, JS::HandleObject obj,
                      JS::HandleObject objectPassedToWrap);

struct JSWrapObjectCallbacks
{
    JSWrapObjectCallback wrap;
    JSPreWrapCallback preWrap;
};

typedef void
(* JSDestroyCompartmentCallback)(JSFreeOp* fop, JSCompartment* compartment);

typedef void
(* JSZoneCallback)(JS::Zone* zone);

typedef void
(* JSCompartmentNameCallback)(JSRuntime* rt, JSCompartment* compartment,
                              char* buf, size_t bufsize);



static MOZ_ALWAYS_INLINE jsval
JS_NumberValue(double d)
{
    int32_t i;
    d = JS::CanonicalizeNaN(d);
    if (mozilla::NumberIsInt32(d, &i))
        return INT_TO_JSVAL(i);
    return DOUBLE_TO_JSVAL(d);
}



JS_PUBLIC_API(bool)
JS_StringHasBeenInterned(JSContext* cx, JSString* str);

namespace JS {





















class MOZ_STACK_CLASS SourceBufferHolder final
{
  public:
    enum Ownership {
      NoOwnership,
      GiveOwnership
    };

    SourceBufferHolder(const char16_t* data, size_t dataLength, Ownership ownership)
      : data_(data),
        length_(dataLength),
        ownsChars_(ownership == GiveOwnership)
    {
        
        
        static const char16_t NullChar_ = 0;
        if (!get()) {
            data_ = &NullChar_;
            length_ = 0;
            ownsChars_ = false;
        }
    }

    ~SourceBufferHolder() {
        if (ownsChars_)
            js_free(const_cast<char16_t*>(data_));
    }

    
    const char16_t* get() const { return data_; }

    
    size_t length() const { return length_; }

    
    
    bool ownsChars() const { return ownsChars_; }

    
    
    
    
    
    
    
    
    
    
    
    
    char16_t* take() {
        MOZ_ASSERT(ownsChars_);
        ownsChars_ = false;
        return const_cast<char16_t*>(data_);
    }

  private:
    SourceBufferHolder(SourceBufferHolder&) = delete;
    SourceBufferHolder& operator=(SourceBufferHolder&) = delete;

    const char16_t* data_;
    size_t length_;
    bool ownsChars_;
};

} 









#define JSPROP_ENUMERATE        0x01    /* property is visible to for/in loop */
#define JSPROP_READONLY         0x02    /* not settable: assignment is no-op.
                                           This flag is only valid when neither
                                           JSPROP_GETTER nor JSPROP_SETTER is
                                           set. */
#define JSPROP_PERMANENT        0x04    /* property cannot be deleted */
#define JSPROP_PROPOP_ACCESSORS 0x08    /* Passed to JS_Define(UC)Property* and
                                           JS_DefineElement if getters/setters
                                           are JSGetterOp/JSSetterOp */
#define JSPROP_GETTER           0x10    /* property holds getter function */
#define JSPROP_SETTER           0x20    /* property holds setter function */
#define JSPROP_SHARED           0x40    /* don't allocate a value slot for this
                                           property; don't copy the property on
                                           set of the same-named property in an
                                           object that delegates to a prototype
                                           containing this property */
#define JSPROP_INTERNAL_USE_BIT 0x80    /* internal JS engine use only */
#define JSPROP_DEFINE_LATE     0x100    /* Don't define property when initially creating
                                           the constructor. Some objects like Function/Object
                                           have self-hosted functions that can only be defined
                                           after the initialization is already finished. */
#define JSFUN_STUB_GSOPS       0x200    /* use JS_PropertyStub getter/setter
                                           instead of defaulting to class gsops
                                           for property holding function */

#define JSFUN_CONSTRUCTOR      0x400    /* native that can be called as a ctor */

#define JSPROP_REDEFINE_NONCONFIGURABLE 0x800 /* If set, will allow redefining a
                                                 non-configurable property, but
                                                 only on a non-DOM global.  This
                                                 is a temporary hack that will
                                                 need to go away in bug
                                                 1105518 */

#define JSPROP_IGNORE_ENUMERATE 0x1000  /* ignore the value in JSPROP_ENUMERATE.
                                           This flag only valid when defining over
                                           an existing property. */
#define JSPROP_IGNORE_READONLY  0x2000  /* ignore the value in JSPROP_READONLY.
                                           This flag only valid when defining over
                                           an existing property. */
#define JSPROP_IGNORE_PERMANENT 0x4000  /* ignore the value in JSPROP_PERMANENT.
                                           This flag only valid when defining over
                                           an existing property. */
#define JSPROP_IGNORE_VALUE     0x8000  /* ignore the Value in the descriptor. Nothing was
                                           specified when passed to Object.defineProperty
                                           from script. */











#define JSFUN_GENERIC_NATIVE   0x800

#define JSFUN_FLAGS_MASK       0xe00    /* | of all the JSFUN_* flags */









extern JS_PUBLIC_API(bool)
JS_CallOnce(JSCallOnceType* once, JSInitCallback func);


extern JS_PUBLIC_API(int64_t)
JS_Now(void);


extern JS_PUBLIC_API(jsval)
JS_GetNaNValue(JSContext* cx);

extern JS_PUBLIC_API(jsval)
JS_GetNegativeInfinityValue(JSContext* cx);

extern JS_PUBLIC_API(jsval)
JS_GetPositiveInfinityValue(JSContext* cx);

extern JS_PUBLIC_API(jsval)
JS_GetEmptyStringValue(JSContext* cx);

extern JS_PUBLIC_API(JSString*)
JS_GetEmptyString(JSRuntime* rt);

extern JS_PUBLIC_API(bool)
JS_ValueToObject(JSContext* cx, JS::HandleValue v, JS::MutableHandleObject objp);

extern JS_PUBLIC_API(JSFunction*)
JS_ValueToFunction(JSContext* cx, JS::HandleValue v);

extern JS_PUBLIC_API(JSFunction*)
JS_ValueToConstructor(JSContext* cx, JS::HandleValue v);

extern JS_PUBLIC_API(JSString*)
JS_ValueToSource(JSContext* cx, JS::Handle<JS::Value> v);

extern JS_PUBLIC_API(bool)
JS_DoubleIsInt32(double d, int32_t *ip);

extern JS_PUBLIC_API(JSType)
JS_TypeOfValue(JSContext* cx, JS::Handle<JS::Value> v);

extern JS_PUBLIC_API(bool)
JS_StrictlyEqual(JSContext* cx, JS::Handle<JS::Value> v1, JS::Handle<JS::Value> v2, bool* equal);

extern JS_PUBLIC_API(bool)
JS_LooselyEqual(JSContext* cx, JS::Handle<JS::Value> v1, JS::Handle<JS::Value> v2, bool* equal);

extern JS_PUBLIC_API(bool)
JS_SameValue(JSContext* cx, JS::Handle<JS::Value> v1, JS::Handle<JS::Value> v2, bool* same);


extern JS_PUBLIC_API(bool)
JS_IsBuiltinEvalFunction(JSFunction* fun);


extern JS_PUBLIC_API(bool)
JS_IsBuiltinFunctionConstructor(JSFunction* fun);
























extern JS_PUBLIC_API(bool)
JS_Init(void);


















extern JS_PUBLIC_API(void)
JS_ShutDown(void);

extern JS_PUBLIC_API(JSRuntime*)
JS_NewRuntime(uint32_t maxbytes,
              uint32_t maxNurseryBytes = JS::DefaultNurseryBytes,
              JSRuntime* parentRuntime = nullptr);

extern JS_PUBLIC_API(void)
JS_DestroyRuntime(JSRuntime* rt);




typedef void* (*JS_ICUAllocFn)(const void*, size_t size);
typedef void* (*JS_ICUReallocFn)(const void*, void* p, size_t size);
typedef void (*JS_ICUFreeFn)(const void*, void* p);



extern JS_PUBLIC_API(bool)
JS_SetICUMemoryFunctions(JS_ICUAllocFn allocFn, JS_ICUReallocFn reallocFn, JS_ICUFreeFn freeFn);

JS_PUBLIC_API(void*)
JS_GetRuntimePrivate(JSRuntime* rt);

extern JS_PUBLIC_API(JSRuntime*)
JS_GetRuntime(JSContext* cx);

extern JS_PUBLIC_API(JSRuntime*)
JS_GetParentRuntime(JSContext* cx);

JS_PUBLIC_API(void)
JS_SetRuntimePrivate(JSRuntime* rt, void* data);

extern JS_PUBLIC_API(void)
JS_BeginRequest(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_EndRequest(JSContext* cx);

namespace js {

void
AssertHeapIsIdle(JSRuntime* rt);

void
AssertHeapIsIdle(JSContext* cx);

} 

class JSAutoRequest
{
  public:
    explicit JSAutoRequest(JSContext* cx
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
    JSContext* mContext;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

#if 0
  private:
    static void* operator new(size_t) CPP_THROW_NEW { return 0; }
    static void operator delete(void*, size_t) { }
#endif
};

extern JS_PUBLIC_API(void)
JS_SetContextCallback(JSRuntime* rt, JSContextCallback cxCallback, void* data);

extern JS_PUBLIC_API(JSContext*)
JS_NewContext(JSRuntime* rt, size_t stackChunkSize);

extern JS_PUBLIC_API(void)
JS_DestroyContext(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_DestroyContextNoGC(JSContext* cx);

extern JS_PUBLIC_API(void*)
JS_GetContextPrivate(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_SetContextPrivate(JSContext* cx, void* data);

extern JS_PUBLIC_API(void*)
JS_GetSecondContextPrivate(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_SetSecondContextPrivate(JSContext* cx, void* data);

extern JS_PUBLIC_API(JSRuntime*)
JS_GetRuntime(JSContext* cx);

extern JS_PUBLIC_API(JSContext*)
JS_ContextIterator(JSRuntime* rt, JSContext** iterp);

extern JS_PUBLIC_API(JSVersion)
JS_GetVersion(JSContext* cx);







JS_PUBLIC_API(void)
JS_SetVersionForCompartment(JSCompartment* compartment, JSVersion version);

extern JS_PUBLIC_API(const char*)
JS_VersionToString(JSVersion version);

extern JS_PUBLIC_API(JSVersion)
JS_StringToVersion(const char* string);

namespace JS {

class JS_PUBLIC_API(RuntimeOptions) {
  public:
    RuntimeOptions()
      : baseline_(false),
        ion_(false),
        asmJS_(false),
        nativeRegExp_(false),
        unboxedObjects_(false),
        werror_(false),
        strictMode_(false),
        extraWarnings_(false),
        varObjFix_(false)
    {
    }

    bool baseline() const { return baseline_; }
    RuntimeOptions& setBaseline(bool flag) {
        baseline_ = flag;
        return *this;
    }
    RuntimeOptions& toggleBaseline() {
        baseline_ = !baseline_;
        return *this;
    }

    bool ion() const { return ion_; }
    RuntimeOptions& setIon(bool flag) {
        ion_ = flag;
        return *this;
    }
    RuntimeOptions& toggleIon() {
        ion_ = !ion_;
        return *this;
    }

    bool asmJS() const { return asmJS_; }
    RuntimeOptions& setAsmJS(bool flag) {
        asmJS_ = flag;
        return *this;
    }
    RuntimeOptions& toggleAsmJS() {
        asmJS_ = !asmJS_;
        return *this;
    }

    bool nativeRegExp() const { return nativeRegExp_; }
    RuntimeOptions& setNativeRegExp(bool flag) {
        nativeRegExp_ = flag;
        return *this;
    }

    bool unboxedObjects() const { return unboxedObjects_; }
    RuntimeOptions& setUnboxedObjects(bool flag) {
        unboxedObjects_ = flag;
        return *this;
    }

    bool werror() const { return werror_; }
    RuntimeOptions& setWerror(bool flag) {
        werror_ = flag;
        return *this;
    }
    RuntimeOptions& toggleWerror() {
        werror_ = !werror_;
        return *this;
    }

    bool strictMode() const { return strictMode_; }
    RuntimeOptions& setStrictMode(bool flag) {
        strictMode_ = flag;
        return *this;
    }
    RuntimeOptions& toggleStrictMode() {
        strictMode_ = !strictMode_;
        return *this;
    }

    bool extraWarnings() const { return extraWarnings_; }
    RuntimeOptions& setExtraWarnings(bool flag) {
        extraWarnings_ = flag;
        return *this;
    }
    RuntimeOptions& toggleExtraWarnings() {
        extraWarnings_ = !extraWarnings_;
        return *this;
    }

    bool varObjFix() const { return varObjFix_; }
    RuntimeOptions& setVarObjFix(bool flag) {
        varObjFix_ = flag;
        return *this;
    }
    RuntimeOptions& toggleVarObjFix() {
        varObjFix_ = !varObjFix_;
        return *this;
    }

  private:
    bool baseline_ : 1;
    bool ion_ : 1;
    bool asmJS_ : 1;
    bool nativeRegExp_ : 1;
    bool unboxedObjects_ : 1;
    bool werror_ : 1;
    bool strictMode_ : 1;
    bool extraWarnings_ : 1;
    bool varObjFix_ : 1;
};

JS_PUBLIC_API(RuntimeOptions&)
RuntimeOptionsRef(JSRuntime* rt);

JS_PUBLIC_API(RuntimeOptions&)
RuntimeOptionsRef(JSContext* cx);

class JS_PUBLIC_API(ContextOptions) {
  public:
    ContextOptions()
      : privateIsNSISupports_(false),
        dontReportUncaught_(false),
        autoJSAPIOwnsErrorReporting_(false)
    {
    }

    bool privateIsNSISupports() const { return privateIsNSISupports_; }
    ContextOptions& setPrivateIsNSISupports(bool flag) {
        privateIsNSISupports_ = flag;
        return *this;
    }
    ContextOptions& togglePrivateIsNSISupports() {
        privateIsNSISupports_ = !privateIsNSISupports_;
        return *this;
    }

    bool dontReportUncaught() const { return dontReportUncaught_; }
    ContextOptions& setDontReportUncaught(bool flag) {
        dontReportUncaught_ = flag;
        return *this;
    }
    ContextOptions& toggleDontReportUncaught() {
        dontReportUncaught_ = !dontReportUncaught_;
        return *this;
    }

    bool autoJSAPIOwnsErrorReporting() const { return autoJSAPIOwnsErrorReporting_; }
    ContextOptions& setAutoJSAPIOwnsErrorReporting(bool flag) {
        autoJSAPIOwnsErrorReporting_ = flag;
        return *this;
    }
    ContextOptions& toggleAutoJSAPIOwnsErrorReporting() {
        autoJSAPIOwnsErrorReporting_ = !autoJSAPIOwnsErrorReporting_;
        return *this;
    }


  private:
    bool privateIsNSISupports_ : 1;
    bool dontReportUncaught_ : 1;
    
    
    
    
    
    
    
    bool autoJSAPIOwnsErrorReporting_ : 1;
};

JS_PUBLIC_API(ContextOptions&)
ContextOptionsRef(JSContext* cx);

class JS_PUBLIC_API(AutoSaveContextOptions) {
  public:
    explicit AutoSaveContextOptions(JSContext* cx)
      : cx_(cx),
        oldOptions_(ContextOptionsRef(cx_))
    {
    }

    ~AutoSaveContextOptions()
    {
        ContextOptionsRef(cx_) = oldOptions_;
    }

  private:
    JSContext* cx_;
    JS::ContextOptions oldOptions_;
};

} 

extern JS_PUBLIC_API(const char*)
JS_GetImplementationVersion(void);

extern JS_PUBLIC_API(void)
JS_SetDestroyCompartmentCallback(JSRuntime* rt, JSDestroyCompartmentCallback callback);

extern JS_PUBLIC_API(void)
JS_SetDestroyZoneCallback(JSRuntime* rt, JSZoneCallback callback);

extern JS_PUBLIC_API(void)
JS_SetSweepZoneCallback(JSRuntime* rt, JSZoneCallback callback);

extern JS_PUBLIC_API(void)
JS_SetCompartmentNameCallback(JSRuntime* rt, JSCompartmentNameCallback callback);

extern JS_PUBLIC_API(void)
JS_SetWrapObjectCallbacks(JSRuntime* rt, const JSWrapObjectCallbacks* callbacks);

extern JS_PUBLIC_API(void)
JS_SetCompartmentPrivate(JSCompartment* compartment, void* data);

extern JS_PUBLIC_API(void*)
JS_GetCompartmentPrivate(JSCompartment* compartment);

extern JS_PUBLIC_API(void)
JS_SetZoneUserData(JS::Zone* zone, void* data);

extern JS_PUBLIC_API(void*)
JS_GetZoneUserData(JS::Zone* zone);

extern JS_PUBLIC_API(bool)
JS_WrapObject(JSContext* cx, JS::MutableHandleObject objp);

extern JS_PUBLIC_API(bool)
JS_WrapValue(JSContext* cx, JS::MutableHandleValue vp);

extern JS_PUBLIC_API(JSObject*)
JS_TransplantObject(JSContext* cx, JS::HandleObject origobj, JS::HandleObject target);

extern JS_PUBLIC_API(bool)
JS_RefreshCrossCompartmentWrappers(JSContext* cx, JS::Handle<JSObject*> obj);




































class JS_PUBLIC_API(JSAutoCompartment)
{
    JSContext* cx_;
    JSCompartment* oldCompartment_;
  public:
    JSAutoCompartment(JSContext* cx, JSObject* target);
    JSAutoCompartment(JSContext* cx, JSScript* target);
    ~JSAutoCompartment();
};

class JS_PUBLIC_API(JSAutoNullableCompartment)
{
    JSContext* cx_;
    JSCompartment* oldCompartment_;
  public:
    explicit JSAutoNullableCompartment(JSContext* cx, JSObject* targetOrNull);
    ~JSAutoNullableCompartment();
};


extern JS_PUBLIC_API(JSCompartment*)
JS_EnterCompartment(JSContext* cx, JSObject* target);

extern JS_PUBLIC_API(void)
JS_LeaveCompartment(JSContext* cx, JSCompartment* oldCompartment);

typedef void (*JSIterateCompartmentCallback)(JSRuntime* rt, void* data, JSCompartment* compartment);






extern JS_PUBLIC_API(void)
JS_IterateCompartments(JSRuntime* rt, void* data,
                       JSIterateCompartmentCallback compartmentCallback);








extern JS_PUBLIC_API(bool)
JS_InitStandardClasses(JSContext* cx, JS::Handle<JSObject*> obj);














extern JS_PUBLIC_API(bool)
JS_ResolveStandardClass(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolved);

extern JS_PUBLIC_API(bool)
JS_EnumerateStandardClasses(JSContext* cx, JS::HandleObject obj);

extern JS_PUBLIC_API(bool)
JS_GetClassObject(JSContext* cx, JSProtoKey key, JS::MutableHandle<JSObject*> objp);

extern JS_PUBLIC_API(bool)
JS_GetClassPrototype(JSContext* cx, JSProtoKey key, JS::MutableHandle<JSObject*> objp);

namespace JS {






extern JS_PUBLIC_API(JSProtoKey)
IdentifyStandardInstance(JSObject* obj);

extern JS_PUBLIC_API(JSProtoKey)
IdentifyStandardPrototype(JSObject* obj);

extern JS_PUBLIC_API(JSProtoKey)
IdentifyStandardInstanceOrPrototype(JSObject* obj);

extern JS_PUBLIC_API(JSProtoKey)
IdentifyStandardConstructor(JSObject* obj);

extern JS_PUBLIC_API(void)
ProtoKeyToId(JSContext* cx, JSProtoKey key, JS::MutableHandleId idp);

} 

extern JS_PUBLIC_API(JSProtoKey)
JS_IdToProtoKey(JSContext* cx, JS::HandleId id);





extern JS_PUBLIC_API(JSObject*)
JS_GetFunctionPrototype(JSContext* cx, JS::HandleObject forObj);





extern JS_PUBLIC_API(JSObject*)
JS_GetObjectPrototype(JSContext* cx, JS::HandleObject forObj);





extern JS_PUBLIC_API(JSObject*)
JS_GetArrayPrototype(JSContext* cx, JS::HandleObject forObj);





extern JS_PUBLIC_API(JSObject*)
JS_GetErrorPrototype(JSContext* cx);

extern JS_PUBLIC_API(JSObject*)
JS_GetGlobalForObject(JSContext* cx, JSObject* obj);

extern JS_PUBLIC_API(bool)
JS_IsGlobalObject(JSObject* obj);





extern JS_PUBLIC_API(JSObject*)
JS_GetGlobalForCompartmentOrNull(JSContext* cx, JSCompartment* c);

namespace JS {

extern JS_PUBLIC_API(JSObject*)
CurrentGlobalOrNull(JSContext* cx);

}




extern JS_PUBLIC_API(JSObject*)
JS_InitReflect(JSContext* cx, JS::HandleObject global);





extern JS_PUBLIC_API(bool)
JS_DefineProfilingFunctions(JSContext* cx, JS::HandleObject obj);


extern JS_PUBLIC_API(bool)
JS_DefineDebuggerObject(JSContext* cx, JS::HandleObject obj);

#ifdef JS_HAS_CTYPES




extern JS_PUBLIC_API(bool)
JS_InitCTypesClass(JSContext* cx, JS::HandleObject global);






typedef char*
(* JSCTypesUnicodeToNativeFun)(JSContext* cx, const char16_t* source, size_t slen);






struct JSCTypesCallbacks {
    JSCTypesUnicodeToNativeFun unicodeToNative;
};

typedef struct JSCTypesCallbacks JSCTypesCallbacks;







extern JS_PUBLIC_API(void)
JS_SetCTypesCallbacks(JSObject* ctypesObj, const JSCTypesCallbacks* callbacks);
#endif

extern JS_PUBLIC_API(void*)
JS_malloc(JSContext* cx, size_t nbytes);

extern JS_PUBLIC_API(void*)
JS_realloc(JSContext* cx, void* p, size_t oldBytes, size_t newBytes);






extern JS_PUBLIC_API(void)
JS_free(JSContext* cx, void* p);





extern JS_PUBLIC_API(void)
JS_freeop(JSFreeOp* fop, void* p);

extern JS_PUBLIC_API(JSFreeOp*)
JS_GetDefaultFreeOp(JSRuntime* rt);

extern JS_PUBLIC_API(void)
JS_updateMallocCounter(JSContext* cx, size_t nbytes);

extern JS_PUBLIC_API(char*)
JS_strdup(JSContext* cx, const char* s);


extern JS_PUBLIC_API(char*)
JS_strdup(JSRuntime* rt, const char* s);








extern JS_PUBLIC_API(bool)
JS_AddExtraGCRootsTracer(JSRuntime* rt, JSTraceDataOp traceOp, void* data);


extern JS_PUBLIC_API(void)
JS_RemoveExtraGCRootsTracer(JSRuntime* rt, JSTraceDataOp traceOp, void* data);




extern JS_PUBLIC_API(void)
JS_GC(JSRuntime* rt);

extern JS_PUBLIC_API(void)
JS_MaybeGC(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_SetGCCallback(JSRuntime* rt, JSGCCallback cb, void* data);

extern JS_PUBLIC_API(bool)
JS_AddFinalizeCallback(JSRuntime* rt, JSFinalizeCallback cb, void* data);

extern JS_PUBLIC_API(void)
JS_RemoveFinalizeCallback(JSRuntime* rt, JSFinalizeCallback cb);


























extern JS_PUBLIC_API(bool)
JS_AddWeakPointerCallback(JSRuntime* rt, JSWeakPointerCallback cb, void* data);

extern JS_PUBLIC_API(void)
JS_RemoveWeakPointerCallback(JSRuntime* rt, JSWeakPointerCallback cb);

extern JS_PUBLIC_API(void)
JS_UpdateWeakPointerAfterGC(JS::Heap<JSObject*>* objp);

extern JS_PUBLIC_API(void)
JS_UpdateWeakPointerAfterGCUnbarriered(JSObject** objp);

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

    
    JSGC_ALLOCATION_THRESHOLD = 19,

    




    JSGC_DECOMMIT_THRESHOLD = 20,

    



    JSGC_MIN_EMPTY_CHUNK_COUNT = 21,

    
    JSGC_MAX_EMPTY_CHUNK_COUNT = 22,

    
    JSGC_COMPACTING_ENABLED = 23
} JSGCParamKey;

extern JS_PUBLIC_API(void)
JS_SetGCParameter(JSRuntime* rt, JSGCParamKey key, uint32_t value);

extern JS_PUBLIC_API(uint32_t)
JS_GetGCParameter(JSRuntime* rt, JSGCParamKey key);

extern JS_PUBLIC_API(void)
JS_SetGCParameterForThread(JSContext* cx, JSGCParamKey key, uint32_t value);

extern JS_PUBLIC_API(uint32_t)
JS_GetGCParameterForThread(JSContext* cx, JSGCParamKey key);

extern JS_PUBLIC_API(void)
JS_SetGCParametersBasedOnAvailableMemory(JSRuntime* rt, uint32_t availMem);





extern JS_PUBLIC_API(JSString*)
JS_NewExternalString(JSContext* cx, const char16_t* chars, size_t length,
                     const JSStringFinalizer* fin);





extern JS_PUBLIC_API(bool)
JS_IsExternalString(JSString* str);




extern JS_PUBLIC_API(const JSStringFinalizer*)
JS_GetExternalStringFinalizer(JSString* str);



















extern JS_PUBLIC_API(void)
JS_SetNativeStackQuota(JSRuntime* cx, size_t systemCodeStackSize,
                       size_t trustedScriptStackSize = 0,
                       size_t untrustedScriptStackSize = 0);



extern JS_PUBLIC_API(int)
JS_IdArrayLength(JSContext* cx, JSIdArray* ida);

extern JS_PUBLIC_API(jsid)
JS_IdArrayGet(JSContext* cx, JSIdArray* ida, unsigned index);

extern JS_PUBLIC_API(void)
JS_DestroyIdArray(JSContext* cx, JSIdArray* ida);

namespace JS {

class AutoIdArray : private AutoGCRooter
{
  public:
    AutoIdArray(JSContext* cx, JSIdArray* ida
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, IDARRAY), context(cx), idArray(ida)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoIdArray() {
        if (idArray)
            JS_DestroyIdArray(context, idArray);
    }
    bool operator!() const {
        return !idArray;
    }
    jsid operator[](size_t i) const {
        MOZ_ASSERT(idArray);
        return JS_IdArrayGet(context, idArray, unsigned(i));
    }
    size_t length() const {
        return JS_IdArrayLength(context, idArray);
    }

    friend void AutoGCRooter::trace(JSTracer* trc);

    JSIdArray* steal() {
        JSIdArray* copy = idArray;
        idArray = nullptr;
        return copy;
    }

  protected:
    inline void trace(JSTracer* trc);

  private:
    JSContext* context;
    JSIdArray* idArray;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    AutoIdArray(AutoIdArray& ida) = delete;
    void operator=(AutoIdArray& ida) = delete;
};

} 

extern JS_PUBLIC_API(bool)
JS_ValueToId(JSContext* cx, JS::HandleValue v, JS::MutableHandleId idp);

extern JS_PUBLIC_API(bool)
JS_StringToId(JSContext* cx, JS::HandleString s, JS::MutableHandleId idp);

extern JS_PUBLIC_API(bool)
JS_IdToValue(JSContext* cx, jsid id, JS::MutableHandle<JS::Value> vp);







extern JS_PUBLIC_API(bool)
JS_DefaultValue(JSContext* cx, JS::Handle<JSObject*> obj, JSType hint,
                JS::MutableHandle<JS::Value> vp);

extern JS_PUBLIC_API(bool)
JS_PropertyStub(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_StrictPropertyStub(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                      JS::MutableHandleValue vp, JS::ObjectOpResult& result);

template<typename T>
struct JSConstScalarSpec {
    const char* name;
    T val;
};

typedef JSConstScalarSpec<double> JSConstDoubleSpec;
typedef JSConstScalarSpec<int32_t> JSConstIntegerSpec;

struct JSJitInfo;






typedef struct JSNativeWrapper {
    JSNative        op;
    const JSJitInfo* info;
} JSNativeWrapper;





#define JSNATIVE_WRAPPER(native) { {native, nullptr} }






struct JSPropertySpec {
    struct SelfHostedWrapper {
        void*      unused;
        const char* funname;
    };

    const char*                 name;
    uint8_t                     flags;
    union {
        JSNativeWrapper     native;
        SelfHostedWrapper   selfHosted;
    } getter;
    union {
        JSNativeWrapper           native;
        SelfHostedWrapper         selfHosted;
    } setter;

    bool isSelfHosted() const {
#ifdef DEBUG
        
        if (flags & JSPROP_GETTER)
            checkAccessorsAreSelfHosted();
        else
            checkAccessorsAreNative();
#endif
        return (flags & JSPROP_GETTER);
    }

    static_assert(sizeof(SelfHostedWrapper) == sizeof(JSNativeWrapper),
                  "JSPropertySpec::getter/setter must be compact");
    static_assert(offsetof(SelfHostedWrapper, funname) == offsetof(JSNativeWrapper, info),
                  "JS_SELF_HOSTED* macros below require that "
                  "SelfHostedWrapper::funname overlay "
                  "JSNativeWrapper::info");
private:
    void checkAccessorsAreNative() const {
        MOZ_ASSERT(getter.native.op);
        
        
        
        
        MOZ_ASSERT_IF(setter.native.info, setter.native.op);
    }

    void checkAccessorsAreSelfHosted() const {
        MOZ_ASSERT(!getter.selfHosted.unused);
        MOZ_ASSERT(!setter.selfHosted.unused);
    }
};

namespace JS {
namespace detail {


inline int CheckIsNative(JSNative native);


template<size_t N>
inline int
CheckIsCharacterLiteral(const char (&arr)[N]);


inline int CheckIsGetterOp(JSGetterOp op);


inline int CheckIsSetterOp(JSSetterOp op);


} 
} 

#define JS_CAST_NATIVE_TO(v, To) \
  (static_cast<void>(sizeof(JS::detail::CheckIsNative(v))), \
   reinterpret_cast<To>(v))

#define JS_CAST_STRING_TO(s, To) \
  (static_cast<void>(sizeof(JS::detail::CheckIsCharacterLiteral(s))), \
   reinterpret_cast<To>(s))

#define JS_CHECK_ACCESSOR_FLAGS(flags) \
  (static_cast<mozilla::EnableIf<((flags) & ~(JSPROP_ENUMERATE | JSPROP_PERMANENT)) == 0>::Type>(0), \
   (flags))

#define JS_PROPERTYOP_GETTER(v) \
  (static_cast<void>(sizeof(JS::detail::CheckIsGetterOp(v))), \
   reinterpret_cast<JSNative>(v))

#define JS_PROPERTYOP_SETTER(v) \
  (static_cast<void>(sizeof(JS::detail::CheckIsSetterOp(v))), \
   reinterpret_cast<JSNative>(v))

#define JS_STUBGETTER JS_PROPERTYOP_GETTER(JS_PropertyStub)

#define JS_STUBSETTER JS_PROPERTYOP_SETTER(JS_StrictPropertyStub)






#define JS_PSG(name, getter, flags) \
    {name, \
     uint8_t(JS_CHECK_ACCESSOR_FLAGS(flags) | JSPROP_SHARED), \
     JSNATIVE_WRAPPER(getter), \
     JSNATIVE_WRAPPER(nullptr)}
#define JS_PSGS(name, getter, setter, flags) \
    {name, \
     uint8_t(JS_CHECK_ACCESSOR_FLAGS(flags) | JSPROP_SHARED), \
     JSNATIVE_WRAPPER(getter), \
     JSNATIVE_WRAPPER(setter)}
#define JS_SELF_HOSTED_GET(name, getterName, flags) \
    {name, \
     uint8_t(JS_CHECK_ACCESSOR_FLAGS(flags) | JSPROP_SHARED | JSPROP_GETTER), \
     { { nullptr, JS_CAST_STRING_TO(getterName, const JSJitInfo*) } }, \
     JSNATIVE_WRAPPER(nullptr) }
#define JS_SELF_HOSTED_GETSET(name, getterName, setterName, flags) \
    {name, \
     uint8_t(JS_CHECK_ACCESSOR_FLAGS(flags) | JSPROP_SHARED | JSPROP_GETTER | JSPROP_SETTER), \
     { nullptr, JS_CAST_STRING_TO(getterName, const JSJitInfo*) },  \
     { nullptr, JS_CAST_STRING_TO(setterName, const JSJitInfo*) } }
#define JS_PS_END { nullptr, 0, JSNATIVE_WRAPPER(nullptr), JSNATIVE_WRAPPER(nullptr) }






struct JSFunctionSpec {
    const char*     name;
    JSNativeWrapper call;
    uint16_t        nargs;
    uint16_t        flags;
    const char*     selfHostedName;
};





#define JS_FS_END JS_FS(nullptr,nullptr,0,0)












#define JS_FS(name,call,nargs,flags)                                          \
    JS_FNSPEC(name, call, nullptr, nargs, flags, nullptr)
#define JS_FN(name,call,nargs,flags)                                          \
    JS_FNSPEC(name, call, nullptr, nargs, (flags) | JSFUN_STUB_GSOPS, nullptr)
#define JS_SYM_FN(name,call,nargs,flags)                                      \
    JS_SYM_FNSPEC(symbol, call, nullptr, nargs, (flags) | JSFUN_STUB_GSOPS, nullptr)
#define JS_FNINFO(name,call,info,nargs,flags)                                 \
    JS_FNSPEC(name, call, info, nargs, flags, nullptr)
#define JS_SELF_HOSTED_FN(name,selfHostedName,nargs,flags)                    \
    JS_FNSPEC(name, nullptr, nullptr, nargs, flags, selfHostedName)
#define JS_SELF_HOSTED_SYM_FN(symbol, selfHostedName, nargs, flags)           \
    JS_SYM_FNSPEC(symbol, nullptr, nullptr, nargs, flags, selfHostedName)
#define JS_SYM_FNSPEC(symbol, call, info, nargs, flags, selfHostedName)       \
    JS_FNSPEC(reinterpret_cast<const char*>(                                 \
                  uint32_t(::JS::SymbolCode::symbol) + 1),                    \
              call, info, nargs, flags, selfHostedName)
#define JS_FNSPEC(name,call,info,nargs,flags,selfHostedName)                  \
    {name, {call, info}, nargs, flags, selfHostedName}

extern JS_PUBLIC_API(JSObject*)
JS_InitClass(JSContext* cx, JS::HandleObject obj, JS::HandleObject parent_proto,
             const JSClass* clasp, JSNative constructor, unsigned nargs,
             const JSPropertySpec* ps, const JSFunctionSpec* fs,
             const JSPropertySpec* static_ps, const JSFunctionSpec* static_fs);





extern JS_PUBLIC_API(bool)
JS_LinkConstructorAndPrototype(JSContext* cx, JS::Handle<JSObject*> ctor,
                               JS::Handle<JSObject*> proto);

extern JS_PUBLIC_API(const JSClass*)
JS_GetClass(JSObject* obj);

extern JS_PUBLIC_API(bool)
JS_InstanceOf(JSContext* cx, JS::Handle<JSObject*> obj, const JSClass* clasp, JS::CallArgs* args);

extern JS_PUBLIC_API(bool)
JS_HasInstance(JSContext* cx, JS::Handle<JSObject*> obj, JS::Handle<JS::Value> v, bool* bp);

extern JS_PUBLIC_API(void*)
JS_GetPrivate(JSObject* obj);

extern JS_PUBLIC_API(void)
JS_SetPrivate(JSObject* obj, void* data);

extern JS_PUBLIC_API(void*)
JS_GetInstancePrivate(JSContext* cx, JS::Handle<JSObject*> obj, const JSClass* clasp,
                      JS::CallArgs* args);

extern JS_PUBLIC_API(bool)
JS_GetPrototype(JSContext* cx, JS::HandleObject obj, JS::MutableHandleObject protop);

extern JS_PUBLIC_API(bool)
JS_SetPrototype(JSContext* cx, JS::HandleObject obj, JS::HandleObject proto);

extern JS_PUBLIC_API(JSObject*)
JS_GetConstructor(JSContext* cx, JS::Handle<JSObject*> proto);

namespace JS {

enum ZoneSpecifier {
    FreshZone = 0,
    SystemZone = 1
};

class JS_PUBLIC_API(CompartmentOptions)
{
  public:
    class Override {
      public:
        Override() : mode_(Default) {}

        bool get(bool defaultValue) const {
            if (mode_ == Default)
                return defaultValue;
            return mode_ == ForceTrue;
        }

        void set(bool overrideValue) {
            mode_ = overrideValue ? ForceTrue : ForceFalse;
        }

        void reset() {
            mode_ = Default;
        }

      private:
        enum Mode {
            Default,
            ForceTrue,
            ForceFalse
        };

        Mode mode_;
    };

    explicit CompartmentOptions()
      : version_(JSVERSION_UNKNOWN)
      , invisibleToDebugger_(false)
      , mergeable_(false)
      , discardSource_(false)
      , disableLazyParsing_(false)
      , cloneSingletons_(false)
      , traceGlobal_(nullptr)
      , singletonsAsTemplates_(true)
      , addonId_(nullptr)
      , preserveJitCode_(false)
    {
        zone_.spec = JS::FreshZone;
    }

    JSVersion version() const { return version_; }
    CompartmentOptions& setVersion(JSVersion aVersion) {
        MOZ_ASSERT(aVersion != JSVERSION_UNKNOWN);
        version_ = aVersion;
        return *this;
    }

    
    
    
    
    bool invisibleToDebugger() const { return invisibleToDebugger_; }
    CompartmentOptions& setInvisibleToDebugger(bool flag) {
        invisibleToDebugger_ = flag;
        return *this;
    }

    
    
    
    
    bool mergeable() const { return mergeable_; }
    CompartmentOptions& setMergeable(bool flag) {
        mergeable_ = flag;
        return *this;
    }

    
    
    bool discardSource() const { return discardSource_; }
    CompartmentOptions& setDiscardSource(bool flag) {
        discardSource_ = flag;
        return *this;
    }

    bool disableLazyParsing() const { return disableLazyParsing_; }
    CompartmentOptions& setDisableLazyParsing(bool flag) {
        disableLazyParsing_ = flag;
        return *this;
    }

    bool cloneSingletons() const { return cloneSingletons_; }
    CompartmentOptions& setCloneSingletons(bool flag) {
        cloneSingletons_ = flag;
        return *this;
    }

    bool extraWarnings(JSRuntime* rt) const;
    bool extraWarnings(JSContext* cx) const;
    Override& extraWarningsOverride() { return extraWarningsOverride_; }

    void* zonePointer() const {
        MOZ_ASSERT(uintptr_t(zone_.pointer) > uintptr_t(JS::SystemZone));
        return zone_.pointer;
    }
    ZoneSpecifier zoneSpecifier() const { return zone_.spec; }
    CompartmentOptions& setZone(ZoneSpecifier spec);
    CompartmentOptions& setSameZoneAs(JSObject* obj);

    void setSingletonsAsValues() {
        singletonsAsTemplates_ = false;
    }
    bool getSingletonsAsTemplates() const {
        return singletonsAsTemplates_;
    }

    
    
    JSAddonId* addonIdOrNull() const { return addonId_; }
    CompartmentOptions& setAddonId(JSAddonId* id) {
        addonId_ = id;
        return *this;
    }

    CompartmentOptions& setTrace(JSTraceOp op) {
        traceGlobal_ = op;
        return *this;
    }
    JSTraceOp getTrace() const {
        return traceGlobal_;
    }

    bool preserveJitCode() const { return preserveJitCode_; }
    CompartmentOptions& setPreserveJitCode(bool flag) {
        preserveJitCode_ = flag;
        return *this;
    }

  private:
    JSVersion version_;
    bool invisibleToDebugger_;
    bool mergeable_;
    bool discardSource_;
    bool disableLazyParsing_;
    bool cloneSingletons_;
    Override extraWarningsOverride_;
    union {
        ZoneSpecifier spec;
        void* pointer; 
    } zone_;
    JSTraceOp traceGlobal_;

    
    
    
    bool singletonsAsTemplates_;

    JSAddonId* addonId_;
    bool preserveJitCode_;
};

JS_PUBLIC_API(CompartmentOptions&)
CompartmentOptionsRef(JSCompartment* compartment);

JS_PUBLIC_API(CompartmentOptions&)
CompartmentOptionsRef(JSObject* obj);

JS_PUBLIC_API(CompartmentOptions&)
CompartmentOptionsRef(JSContext* cx);





















enum OnNewGlobalHookOption {
    FireOnNewGlobalHook,
    DontFireOnNewGlobalHook
};

} 

extern JS_PUBLIC_API(JSObject*)
JS_NewGlobalObject(JSContext* cx, const JSClass* clasp, JSPrincipals* principals,
                   JS::OnNewGlobalHookOption hookOption,
                   const JS::CompartmentOptions& options = JS::CompartmentOptions());









extern JS_PUBLIC_API(void)
JS_GlobalObjectTraceHook(JSTracer* trc, JSObject* global);

extern JS_PUBLIC_API(void)
JS_FireOnNewGlobalObject(JSContext* cx, JS::HandleObject global);

extern JS_PUBLIC_API(JSObject*)
JS_NewObject(JSContext* cx, const JSClass* clasp);


extern JS_PUBLIC_API(bool)
JS_IsExtensible(JSContext* cx, JS::HandleObject obj, bool* extensible);

extern JS_PUBLIC_API(bool)
JS_IsNative(JSObject* obj);

extern JS_PUBLIC_API(JSRuntime*)
JS_GetObjectRuntime(JSObject* obj);





extern JS_PUBLIC_API(JSObject*)
JS_NewObjectWithGivenProto(JSContext* cx, const JSClass* clasp, JS::Handle<JSObject*> proto);


extern JS_PUBLIC_API(JSObject*)
JS_NewPlainObject(JSContext* cx);






extern JS_PUBLIC_API(bool)
JS_DeepFreezeObject(JSContext* cx, JS::Handle<JSObject*> obj);




extern JS_PUBLIC_API(bool)
JS_FreezeObject(JSContext* cx, JS::Handle<JSObject*> obj);








extern JS_PUBLIC_API(bool)
JS_PreventExtensions(JSContext* cx, JS::HandleObject obj, JS::ObjectOpResult& result);

extern JS_PUBLIC_API(JSObject*)
JS_New(JSContext* cx, JS::HandleObject ctor, const JS::HandleValueArray& args);




struct JSPropertyDescriptor {
    JSObject* obj;
    unsigned attrs;
    JSGetterOp getter;
    JSSetterOp setter;
    JS::Value value;

    JSPropertyDescriptor()
      : obj(nullptr), attrs(0), getter(nullptr), setter(nullptr), value(JSVAL_VOID)
    {}

    void trace(JSTracer* trc);

    static js::ThingRootKind rootKind() { return js::THING_ROOT_PROPERTY_DESCRIPTOR; }
};

namespace JS {

template <typename Outer>
class PropertyDescriptorOperations
{
    const JSPropertyDescriptor * desc() const { return static_cast<const Outer*>(this)->extract(); }

    bool has(unsigned bit) const {
        MOZ_ASSERT(bit != 0);
        MOZ_ASSERT((bit & (bit - 1)) == 0);  
        return (desc()->attrs & bit) != 0;
    }

    bool hasAny(unsigned bits) const {
        return (desc()->attrs & bits) != 0;
    }

    bool hasAll(unsigned bits) const {
        return (desc()->attrs & bits) == bits;
    }

    
    enum { SHADOWABLE = JSPROP_INTERNAL_USE_BIT };

  public:
    
    
    bool isAccessorDescriptor() const { return hasAny(JSPROP_GETTER | JSPROP_SETTER); }
    bool isGenericDescriptor() const {
        return (desc()->attrs&
                (JSPROP_GETTER | JSPROP_SETTER | JSPROP_IGNORE_READONLY | JSPROP_IGNORE_VALUE)) ==
               (JSPROP_IGNORE_READONLY | JSPROP_IGNORE_VALUE);
    }
    bool isDataDescriptor() const { return !isAccessorDescriptor() && !isGenericDescriptor(); }

    bool hasConfigurable() const { return !has(JSPROP_IGNORE_PERMANENT); }
    bool configurable() const { MOZ_ASSERT(hasConfigurable()); return !has(JSPROP_PERMANENT); }

    bool hasEnumerable() const { return !has(JSPROP_IGNORE_ENUMERATE); }
    bool enumerable() const { MOZ_ASSERT(hasEnumerable()); return has(JSPROP_ENUMERATE); }

    bool hasValue() const { return !isAccessorDescriptor() && !has(JSPROP_IGNORE_VALUE); }
    JS::HandleValue value() const {
        return JS::HandleValue::fromMarkedLocation(&desc()->value);
    }

    bool hasWritable() const { return !isAccessorDescriptor() && !has(JSPROP_IGNORE_READONLY); }
    bool writable() const { MOZ_ASSERT(hasWritable()); return !has(JSPROP_READONLY); }

    bool hasGetterObject() const { return has(JSPROP_GETTER); }
    JS::HandleObject getterObject() const {
        MOZ_ASSERT(hasGetterObject());
        return JS::HandleObject::fromMarkedLocation(
                reinterpret_cast<JSObject* const*>(&desc()->getter));
    }
    bool hasSetterObject() const { return has(JSPROP_SETTER); }
    JS::HandleObject setterObject() const {
        MOZ_ASSERT(hasSetterObject());
        return JS::HandleObject::fromMarkedLocation(
                reinterpret_cast<JSObject* const*>(&desc()->setter));
    }

    bool hasGetterOrSetter() const { return desc()->getter || desc()->setter; }
    bool isShared() const { return has(JSPROP_SHARED); }

    JS::HandleObject object() const {
        return JS::HandleObject::fromMarkedLocation(&desc()->obj);
    }
    unsigned attributes() const { return desc()->attrs; }
    JSGetterOp getter() const { return desc()->getter; }
    JSSetterOp setter() const { return desc()->setter; }

    void assertValid() const {
#ifdef DEBUG
        MOZ_ASSERT((attributes() & ~(JSPROP_ENUMERATE | JSPROP_IGNORE_ENUMERATE |
                                     JSPROP_PERMANENT | JSPROP_IGNORE_PERMANENT |
                                     JSPROP_READONLY | JSPROP_IGNORE_READONLY |
                                     JSPROP_IGNORE_VALUE |
                                     JSPROP_GETTER |
                                     JSPROP_SETTER |
                                     JSPROP_SHARED |
                                     JSPROP_REDEFINE_NONCONFIGURABLE |
                                     SHADOWABLE)) == 0);
        MOZ_ASSERT(!hasAll(JSPROP_IGNORE_ENUMERATE | JSPROP_ENUMERATE));
        MOZ_ASSERT(!hasAll(JSPROP_IGNORE_PERMANENT | JSPROP_PERMANENT));
        if (isAccessorDescriptor()) {
            MOZ_ASSERT(has(JSPROP_SHARED));
            MOZ_ASSERT(!has(JSPROP_READONLY));
            MOZ_ASSERT(!has(JSPROP_IGNORE_READONLY));
            MOZ_ASSERT(!has(JSPROP_IGNORE_VALUE));
            MOZ_ASSERT(!has(SHADOWABLE));
            MOZ_ASSERT(value().isUndefined());
            MOZ_ASSERT_IF(!has(JSPROP_GETTER), !getter());
            MOZ_ASSERT_IF(!has(JSPROP_SETTER), !setter());
        } else {
            MOZ_ASSERT(!hasAll(JSPROP_IGNORE_READONLY | JSPROP_READONLY));
            MOZ_ASSERT_IF(has(JSPROP_IGNORE_VALUE), value().isUndefined());
        }
        MOZ_ASSERT(getter() != JS_PropertyStub);
        MOZ_ASSERT(setter() != JS_StrictPropertyStub);
#endif
    }

    void assertComplete() const {
#ifdef DEBUG
        assertValid();
        MOZ_ASSERT((attributes() & ~(JSPROP_ENUMERATE |
                                     JSPROP_PERMANENT |
                                     JSPROP_READONLY |
                                     JSPROP_GETTER |
                                     JSPROP_SETTER |
                                     JSPROP_SHARED |
                                     JSPROP_REDEFINE_NONCONFIGURABLE |
                                     SHADOWABLE)) == 0);
        MOZ_ASSERT_IF(isAccessorDescriptor(), has(JSPROP_GETTER) && has(JSPROP_SETTER));
#endif
    }

    void assertCompleteIfFound() const {
#ifdef DEBUG
        if (object())
            assertComplete();
#endif
    }
};

template <typename Outer>
class MutablePropertyDescriptorOperations : public PropertyDescriptorOperations<Outer>
{
    JSPropertyDescriptor * desc() { return static_cast<Outer*>(this)->extractMutable(); }

  public:
    void clear() {
        object().set(nullptr);
        setAttributes(0);
        setGetter(nullptr);
        setSetter(nullptr);
        value().setUndefined();
    }

    void initFields(HandleObject obj, HandleValue v, unsigned attrs,
                    JSGetterOp getterOp, JSSetterOp setterOp) {
        MOZ_ASSERT(getterOp != JS_PropertyStub);
        MOZ_ASSERT(setterOp != JS_StrictPropertyStub);

        object().set(obj);
        value().set(v);
        setAttributes(attrs);
        setGetter(getterOp);
        setSetter(setterOp);
    }

    void assign(JSPropertyDescriptor& other) {
        object().set(other.obj);
        setAttributes(other.attrs);
        setGetter(other.getter);
        setSetter(other.setter);
        value().set(other.value);
    }

    void setDataDescriptor(HandleValue v, unsigned attrs) {
        MOZ_ASSERT((attrs & ~(JSPROP_ENUMERATE |
                              JSPROP_PERMANENT |
                              JSPROP_READONLY |
                              JSPROP_IGNORE_ENUMERATE |
                              JSPROP_IGNORE_PERMANENT |
                              JSPROP_IGNORE_READONLY)) == 0);
        object().set(nullptr);
        setAttributes(attrs);
        setGetter(nullptr);
        setSetter(nullptr);
        value().set(v);
    }

    JS::MutableHandleObject object() {
        return JS::MutableHandleObject::fromMarkedLocation(&desc()->obj);
    }
    unsigned& attributesRef() { return desc()->attrs; }
    JSGetterOp& getter() { return desc()->getter; }
    JSSetterOp& setter() { return desc()->setter; }
    JS::MutableHandleValue value() {
        return JS::MutableHandleValue::fromMarkedLocation(&desc()->value);
    }
    void setValue(JS::HandleValue v) {
        MOZ_ASSERT(!(desc()->attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        attributesRef() &= ~JSPROP_IGNORE_VALUE;
        value().set(v);
    }

    void setConfigurable(bool configurable) {
        setAttributes((desc()->attrs & ~(JSPROP_IGNORE_PERMANENT | JSPROP_PERMANENT)) |
                      (configurable ? 0 : JSPROP_PERMANENT));
    }
    void setEnumerable(bool enumerable) {
        setAttributes((desc()->attrs & ~(JSPROP_IGNORE_ENUMERATE | JSPROP_ENUMERATE)) |
                      (enumerable ? JSPROP_ENUMERATE : 0));
    }
    void setWritable(bool writable) {
        MOZ_ASSERT(!(desc()->attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        setAttributes((desc()->attrs & ~(JSPROP_IGNORE_READONLY | JSPROP_READONLY)) |
                      (writable ? 0 : JSPROP_READONLY));
    }
    void setAttributes(unsigned attrs) { desc()->attrs = attrs; }

    void setGetter(JSGetterOp op) {
        MOZ_ASSERT(op != JS_PropertyStub);
        desc()->getter = op;
    }
    void setSetter(JSSetterOp op) {
        MOZ_ASSERT(op != JS_StrictPropertyStub);
        desc()->setter = op;
    }
    void setGetterObject(JSObject* obj) {
        desc()->getter = reinterpret_cast<JSGetterOp>(obj);
        desc()->attrs &= ~(JSPROP_IGNORE_VALUE | JSPROP_IGNORE_READONLY | JSPROP_READONLY);
        desc()->attrs |= JSPROP_GETTER | JSPROP_SHARED;
    }
    void setSetterObject(JSObject* obj) {
        desc()->setter = reinterpret_cast<JSSetterOp>(obj);
        desc()->attrs &= ~(JSPROP_IGNORE_VALUE | JSPROP_IGNORE_READONLY | JSPROP_READONLY);
        desc()->attrs |= JSPROP_SETTER | JSPROP_SHARED;
    }

    JS::MutableHandleObject getterObject() {
        MOZ_ASSERT(this->hasGetterObject());
        return JS::MutableHandleObject::fromMarkedLocation(
                reinterpret_cast<JSObject**>(&desc()->getter));
    }
    JS::MutableHandleObject setterObject() {
        MOZ_ASSERT(this->hasSetterObject());
        return JS::MutableHandleObject::fromMarkedLocation(
                reinterpret_cast<JSObject**>(&desc()->setter));
    }
};

} 

namespace js {

template <>
struct GCMethods<JSPropertyDescriptor> {
    static JSPropertyDescriptor initial() { return JSPropertyDescriptor(); }
};

template <>
class RootedBase<JSPropertyDescriptor>
  : public JS::MutablePropertyDescriptorOperations<JS::Rooted<JSPropertyDescriptor> >
{
    friend class JS::PropertyDescriptorOperations<JS::Rooted<JSPropertyDescriptor> >;
    friend class JS::MutablePropertyDescriptorOperations<JS::Rooted<JSPropertyDescriptor> >;
    const JSPropertyDescriptor* extract() const {
        return static_cast<const JS::Rooted<JSPropertyDescriptor>*>(this)->address();
    }
    JSPropertyDescriptor* extractMutable() {
        return static_cast<JS::Rooted<JSPropertyDescriptor>*>(this)->address();
    }
};

template <>
class HandleBase<JSPropertyDescriptor>
  : public JS::PropertyDescriptorOperations<JS::Handle<JSPropertyDescriptor> >
{
    friend class JS::PropertyDescriptorOperations<JS::Handle<JSPropertyDescriptor> >;
    const JSPropertyDescriptor* extract() const {
        return static_cast<const JS::Handle<JSPropertyDescriptor>*>(this)->address();
    }
};

template <>
class MutableHandleBase<JSPropertyDescriptor>
  : public JS::MutablePropertyDescriptorOperations<JS::MutableHandle<JSPropertyDescriptor> >
{
    friend class JS::PropertyDescriptorOperations<JS::MutableHandle<JSPropertyDescriptor> >;
    friend class JS::MutablePropertyDescriptorOperations<JS::MutableHandle<JSPropertyDescriptor> >;
    const JSPropertyDescriptor* extract() const {
        return static_cast<const JS::MutableHandle<JSPropertyDescriptor>*>(this)->address();
    }
    JSPropertyDescriptor* extractMutable() {
        return static_cast<JS::MutableHandle<JSPropertyDescriptor>*>(this)->address();
    }
};

} 

namespace JS {

extern JS_PUBLIC_API(bool)
ObjectToCompletePropertyDescriptor(JSContext* cx,
                                   JS::HandleObject obj,
                                   JS::HandleValue descriptor,
                                   JS::MutableHandle<JSPropertyDescriptor> desc);

} 




extern JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext* cx, JS::HandleObject obj, const char* name, JS::HandleValue value,
                  unsigned attrs,
                  JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext* cx, JS::HandleObject obj, const char* name, JS::HandleObject value,
                  unsigned attrs,
                  JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext* cx, JS::HandleObject obj, const char* name, JS::HandleString value,
                  unsigned attrs,
                  JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext* cx, JS::HandleObject obj, const char* name, int32_t value,
                  unsigned attrs,
                  JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext* cx, JS::HandleObject obj, const char* name, uint32_t value,
                  unsigned attrs,
                  JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext* cx, JS::HandleObject obj, const char* name, double value,
                  unsigned attrs,
                  JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue value,
                      unsigned attrs,
                      JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject value,
                      unsigned attrs,
                      JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleString value,
                      unsigned attrs,
                      JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, int32_t value,
                      unsigned attrs,
                      JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, uint32_t value,
                      unsigned attrs,
                      JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, double value,
                      unsigned attrs,
                      JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                      JS::Handle<JSPropertyDescriptor> desc,
                      JS::ObjectOpResult& result);

extern JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                      JS::Handle<JSPropertyDescriptor> desc);

extern JS_PUBLIC_API(JSObject*)
JS_DefineObject(JSContext* cx, JS::HandleObject obj, const char* name,
                const JSClass* clasp = nullptr, unsigned attrs = 0);

extern JS_PUBLIC_API(bool)
JS_DefineConstDoubles(JSContext* cx, JS::HandleObject obj, const JSConstDoubleSpec* cds);

extern JS_PUBLIC_API(bool)
JS_DefineConstIntegers(JSContext* cx, JS::HandleObject obj, const JSConstIntegerSpec* cis);

extern JS_PUBLIC_API(bool)
JS_DefineProperties(JSContext* cx, JS::HandleObject obj, const JSPropertySpec* ps);




extern JS_PUBLIC_API(bool)
JS_AlreadyHasOwnProperty(JSContext* cx, JS::HandleObject obj, const char* name,
                         bool* foundp);

extern JS_PUBLIC_API(bool)
JS_AlreadyHasOwnPropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                             bool* foundp);

extern JS_PUBLIC_API(bool)
JS_HasProperty(JSContext* cx, JS::HandleObject obj, const char* name, bool* foundp);

extern JS_PUBLIC_API(bool)
JS_HasPropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* foundp);


extern JS_PUBLIC_API(bool)
JS_GetOwnPropertyDescriptorById(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                                JS::MutableHandle<JSPropertyDescriptor> desc);

extern JS_PUBLIC_API(bool)
JS_GetOwnPropertyDescriptor(JSContext* cx, JS::HandleObject obj, const char* name,
                            JS::MutableHandle<JSPropertyDescriptor> desc);

extern JS_PUBLIC_API(bool)
JS_GetOwnUCPropertyDescriptor(JSContext* cx, JS::HandleObject obj, const char16_t* name,
                              JS::MutableHandle<JSPropertyDescriptor> desc);






extern JS_PUBLIC_API(bool)
JS_GetPropertyDescriptorById(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                             JS::MutableHandle<JSPropertyDescriptor> desc);

extern JS_PUBLIC_API(bool)
JS_GetPropertyDescriptor(JSContext* cx, JS::HandleObject obj, const char* name,
                         JS::MutableHandle<JSPropertyDescriptor> desc);

extern JS_PUBLIC_API(bool)
JS_GetProperty(JSContext* cx, JS::HandleObject obj, const char* name, JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_GetPropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_ForwardGetPropertyTo(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject onBehalfOf,
                        JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_SetProperty(JSContext* cx, JS::HandleObject obj, const char* name, JS::HandleValue v);

extern JS_PUBLIC_API(bool)
JS_SetPropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v);

extern JS_PUBLIC_API(bool)
JS_ForwardSetPropertyTo(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue v,
                        JS::HandleValue receiver, JS::ObjectOpResult& result);

extern JS_PUBLIC_API(bool)
JS_DeleteProperty(JSContext* cx, JS::HandleObject obj, const char* name);

extern JS_PUBLIC_API(bool)
JS_DeleteProperty(JSContext* cx, JS::HandleObject obj, const char* name,
                  JS::ObjectOpResult& result);

extern JS_PUBLIC_API(bool)
JS_DeletePropertyById(JSContext* cx, JS::HandleObject obj, jsid id);

extern JS_PUBLIC_API(bool)
JS_DeletePropertyById(JSContext* cx, JS::HandleObject obj, JS::HandleId id,
                      JS::ObjectOpResult& result);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    JS::HandleValue value, unsigned attrs,
                    JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    JS::HandleObject value, unsigned attrs,
                    JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    JS::HandleString value, unsigned attrs,
                    JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    int32_t value, unsigned attrs,
                    JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    uint32_t value, unsigned attrs,
                    JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    double value, unsigned attrs,
                    JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    JS::Handle<JSPropertyDescriptor> desc,
                    JS::ObjectOpResult& result);

extern JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    JS::Handle<JSPropertyDescriptor> desc);

extern JS_PUBLIC_API(bool)
JS_AlreadyHasOwnUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name,
                           size_t namelen, bool* foundp);

extern JS_PUBLIC_API(bool)
JS_HasUCProperty(JSContext* cx, JS::HandleObject obj,
                 const char16_t* name, size_t namelen,
                 bool* vp);

extern JS_PUBLIC_API(bool)
JS_GetUCProperty(JSContext* cx, JS::HandleObject obj,
                 const char16_t* name, size_t namelen,
                 JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_SetUCProperty(JSContext* cx, JS::HandleObject obj,
                 const char16_t* name, size_t namelen,
                 JS::HandleValue v);

extern JS_PUBLIC_API(bool)
JS_DeleteUCProperty(JSContext* cx, JS::HandleObject obj, const char16_t* name, size_t namelen,
                    JS::ObjectOpResult& result);

extern JS_PUBLIC_API(JSObject*)
JS_NewArrayObject(JSContext* cx, const JS::HandleValueArray& contents);

extern JS_PUBLIC_API(JSObject*)
JS_NewArrayObject(JSContext* cx, size_t length);

extern JS_PUBLIC_API(bool)
JS_IsArrayObject(JSContext* cx, JS::HandleValue value);

extern JS_PUBLIC_API(bool)
JS_IsArrayObject(JSContext* cx, JS::HandleObject obj);

extern JS_PUBLIC_API(bool)
JS_GetArrayLength(JSContext* cx, JS::Handle<JSObject*> obj, uint32_t* lengthp);

extern JS_PUBLIC_API(bool)
JS_SetArrayLength(JSContext* cx, JS::Handle<JSObject*> obj, uint32_t length);

extern JS_PUBLIC_API(bool)
JS_DefineElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::HandleValue value,
                 unsigned attrs,
                 JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::HandleObject value,
                 unsigned attrs,
                 JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::HandleString value,
                 unsigned attrs,
                 JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineElement(JSContext* cx, JS::HandleObject obj, uint32_t index, int32_t value,
                 unsigned attrs,
                 JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineElement(JSContext* cx, JS::HandleObject obj, uint32_t index, uint32_t value,
                 unsigned attrs,
                 JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_DefineElement(JSContext* cx, JS::HandleObject obj, uint32_t index, double value,
                 unsigned attrs,
                 JSNative getter = nullptr, JSNative setter = nullptr);

extern JS_PUBLIC_API(bool)
JS_AlreadyHasOwnElement(JSContext* cx, JS::HandleObject obj, uint32_t index, bool* foundp);

extern JS_PUBLIC_API(bool)
JS_HasElement(JSContext* cx, JS::HandleObject obj, uint32_t index, bool* foundp);

extern JS_PUBLIC_API(bool)
JS_GetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_ForwardGetElementTo(JSContext* cx, JS::HandleObject obj, uint32_t index,
                       JS::HandleObject onBehalfOf, JS::MutableHandleValue vp);

extern JS_PUBLIC_API(bool)
JS_SetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::HandleValue v);

extern JS_PUBLIC_API(bool)
JS_SetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::HandleObject v);

extern JS_PUBLIC_API(bool)
JS_SetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::HandleString v);

extern JS_PUBLIC_API(bool)
JS_SetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, int32_t v);

extern JS_PUBLIC_API(bool)
JS_SetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, uint32_t v);

extern JS_PUBLIC_API(bool)
JS_SetElement(JSContext* cx, JS::HandleObject obj, uint32_t index, double v);

extern JS_PUBLIC_API(bool)
JS_DeleteElement(JSContext* cx, JS::HandleObject obj, uint32_t index);

extern JS_PUBLIC_API(bool)
JS_DeleteElement(JSContext* cx, JS::HandleObject obj, uint32_t index, JS::ObjectOpResult& result);





JS_PUBLIC_API(void)
JS_SetAllNonReservedSlotsToUndefined(JSContext* cx, JSObject* objArg);






extern JS_PUBLIC_API(JSObject*)
JS_NewArrayBufferWithContents(JSContext* cx, size_t nbytes, void* contents);







extern JS_PUBLIC_API(void*)
JS_StealArrayBufferContents(JSContext* cx, JS::HandleObject obj);






extern JS_PUBLIC_API(JSObject*)
JS_NewMappedArrayBufferWithContents(JSContext* cx, size_t nbytes, void* contents);





extern JS_PUBLIC_API(void*)
JS_CreateMappedArrayBufferContents(int fd, size_t offset, size_t length);








extern JS_PUBLIC_API(void)
JS_ReleaseMappedArrayBufferContents(void* contents, size_t length);

extern JS_PUBLIC_API(JSIdArray*)
JS_Enumerate(JSContext* cx, JS::HandleObject obj);

extern JS_PUBLIC_API(jsval)
JS_GetReservedSlot(JSObject* obj, uint32_t index);

extern JS_PUBLIC_API(void)
JS_SetReservedSlot(JSObject* obj, uint32_t index, jsval v);






extern JS_PUBLIC_API(JSFunction*)
JS_NewFunction(JSContext* cx, JSNative call, unsigned nargs, unsigned flags,
               const char* name);





extern JS_PUBLIC_API(JSFunction*)
JS_NewFunctionById(JSContext* cx, JSNative call, unsigned nargs, unsigned flags,
                   JS::Handle<jsid> id);

namespace JS {

extern JS_PUBLIC_API(JSFunction*)
GetSelfHostedFunction(JSContext* cx, const char* selfHostedName, JS::Handle<jsid> id,
                      unsigned nargs);

} 

extern JS_PUBLIC_API(JSObject*)
JS_GetFunctionObject(JSFunction* fun);







extern JS_PUBLIC_API(JSString*)
JS_GetFunctionId(JSFunction* fun);








extern JS_PUBLIC_API(JSString*)
JS_GetFunctionDisplayId(JSFunction* fun);




extern JS_PUBLIC_API(uint16_t)
JS_GetFunctionArity(JSFunction* fun);





namespace JS {

extern JS_PUBLIC_API(bool)
IsCallable(JSObject* obj);

extern JS_PUBLIC_API(bool)
IsConstructor(JSObject* obj);

} 







extern JS_PUBLIC_API(bool)
JS_ObjectIsFunction(JSContext* cx, JSObject* obj);

extern JS_PUBLIC_API(bool)
JS_IsNativeFunction(JSObject* funobj, JSNative call);


extern JS_PUBLIC_API(bool)
JS_IsConstructor(JSFunction* fun);






extern JS_PUBLIC_API(JSObject*)
JS_BindCallable(JSContext* cx, JS::Handle<JSObject*> callable, JS::Handle<JSObject*> newThis);




enum PropertyDefinitionBehavior {
    DefineAllProperties,
    OnlyDefineLateProperties,
    DontDefineLateProperties
};

extern JS_PUBLIC_API(bool)
JS_DefineFunctions(JSContext* cx, JS::Handle<JSObject*> obj, const JSFunctionSpec* fs,
                   PropertyDefinitionBehavior behavior = DefineAllProperties);

extern JS_PUBLIC_API(JSFunction*)
JS_DefineFunction(JSContext* cx, JS::Handle<JSObject*> obj, const char* name, JSNative call,
                  unsigned nargs, unsigned attrs);

extern JS_PUBLIC_API(JSFunction*)
JS_DefineUCFunction(JSContext* cx, JS::Handle<JSObject*> obj,
                    const char16_t* name, size_t namelen, JSNative call,
                    unsigned nargs, unsigned attrs);

extern JS_PUBLIC_API(JSFunction*)
JS_DefineFunctionById(JSContext* cx, JS::Handle<JSObject*> obj, JS::Handle<jsid> id, JSNative call,
                      unsigned nargs, unsigned attrs);

namespace JS {





extern JS_PUBLIC_API(JSObject*)
CloneFunctionObject(JSContext* cx, HandleObject funobj);






extern JS_PUBLIC_API(JSObject*)
CloneFunctionObject(JSContext* cx, HandleObject funobj, AutoObjectVector& scopeChain);

} 








extern JS_PUBLIC_API(bool)
JS_BufferIsCompilableUnit(JSContext* cx, JS::Handle<JSObject*> obj, const char* utf8,
                          size_t length);




extern JS_PUBLIC_API(bool)
JS_CompileScript(JSContext* cx, const char* ascii, size_t length,
                 const JS::CompileOptions& options,
                 JS::MutableHandleScript script);




extern JS_PUBLIC_API(bool)
JS_CompileUCScript(JSContext* cx, const char16_t* chars, size_t length,
                   const JS::CompileOptions& options,
                   JS::MutableHandleScript script);

extern JS_PUBLIC_API(JSObject*)
JS_GetGlobalFromScript(JSScript* script);

extern JS_PUBLIC_API(const char*)
JS_GetScriptFilename(JSScript* script);

extern JS_PUBLIC_API(unsigned)
JS_GetScriptBaseLineNumber(JSContext* cx, JSScript* script);

extern JS_PUBLIC_API(JSScript*)
JS_GetFunctionScript(JSContext* cx, JS::HandleFunction fun);

namespace JS {













































class JS_FRIEND_API(ReadOnlyCompileOptions)
{
    friend class CompileOptions;

  protected:
    
    
    
    
    
    
    
    
    
    
    
    bool mutedErrors_;
    const char* filename_;
    const char* introducerFilename_;
    const char16_t* sourceMapURL_;

    
    
    
    
    ReadOnlyCompileOptions()
      : mutedErrors_(false),
        filename_(nullptr),
        introducerFilename_(nullptr),
        sourceMapURL_(nullptr),
        version(JSVERSION_UNKNOWN),
        versionSet(false),
        utf8(false),
        lineno(1),
        column(0),
        hasPollutedGlobalScope(false),
        isRunOnce(false),
        forEval(false),
        noScriptRval(false),
        selfHostingMode(false),
        canLazilyParse(true),
        strictOption(false),
        extraWarningsOption(false),
        werrorOption(false),
        asmJSOption(false),
        forceAsync(false),
        installedFile(false),
        sourceIsLazy(false),
        introductionType(nullptr),
        introductionLineno(0),
        introductionOffset(0),
        hasIntroductionInfo(false)
    { }

    
    
    void copyPODOptions(const ReadOnlyCompileOptions& rhs);

  public:
    
    
    bool mutedErrors() const { return mutedErrors_; }
    const char* filename() const { return filename_; }
    const char* introducerFilename() const { return introducerFilename_; }
    const char16_t* sourceMapURL() const { return sourceMapURL_; }
    virtual JSObject* element() const = 0;
    virtual JSString* elementAttributeName() const = 0;
    virtual JSScript* introductionScript() const = 0;

    
    JSVersion version;
    bool versionSet;
    bool utf8;
    unsigned lineno;
    unsigned column;
    bool hasPollutedGlobalScope;
    
    bool isRunOnce;
    bool forEval;
    bool noScriptRval;
    bool selfHostingMode;
    bool canLazilyParse;
    bool strictOption;
    bool extraWarningsOption;
    bool werrorOption;
    bool asmJSOption;
    bool forceAsync;
    bool installedFile;  
    bool sourceIsLazy;

    
    
    const char* introductionType;
    unsigned introductionLineno;
    uint32_t introductionOffset;
    bool hasIntroductionInfo;

  private:
    static JSObject * const nullObjectPtr;
    void operator=(const ReadOnlyCompileOptions&) = delete;
};














class JS_FRIEND_API(OwningCompileOptions) : public ReadOnlyCompileOptions
{
    JSRuntime* runtime;
    PersistentRootedObject elementRoot;
    PersistentRootedString elementAttributeNameRoot;
    PersistentRootedScript introductionScriptRoot;

  public:
    
    
    
    
    explicit OwningCompileOptions(JSContext* cx);
    ~OwningCompileOptions();

    JSObject* element() const override { return elementRoot; }
    JSString* elementAttributeName() const override { return elementAttributeNameRoot; }
    JSScript* introductionScript() const override { return introductionScriptRoot; }

    
    bool copy(JSContext* cx, const ReadOnlyCompileOptions& rhs);

    
    bool setFile(JSContext* cx, const char* f);
    bool setFileAndLine(JSContext* cx, const char* f, unsigned l);
    bool setSourceMapURL(JSContext* cx, const char16_t* s);
    bool setIntroducerFilename(JSContext* cx, const char* s);

    
    OwningCompileOptions& setLine(unsigned l)             { lineno = l; return *this; }
    OwningCompileOptions& setElement(JSObject* e) {
        elementRoot = e;
        return *this;
    }
    OwningCompileOptions& setElementAttributeName(JSString* p) {
        elementAttributeNameRoot = p;
        return *this;
    }
    OwningCompileOptions& setIntroductionScript(JSScript* s) {
        introductionScriptRoot = s;
        return *this;
    }
    OwningCompileOptions& setMutedErrors(bool mute) {
        mutedErrors_ = mute;
        return *this;
    }
    OwningCompileOptions& setVersion(JSVersion v) {
        version = v;
        versionSet = true;
        return *this;
    }
    OwningCompileOptions& setUTF8(bool u) { utf8 = u; return *this; }
    OwningCompileOptions& setColumn(unsigned c) { column = c; return *this; }
    OwningCompileOptions& setHasPollutedScope(bool p) { hasPollutedGlobalScope = p; return *this; }
    OwningCompileOptions& setIsRunOnce(bool once) { isRunOnce = once; return *this; }
    OwningCompileOptions& setForEval(bool eval) { forEval = eval; return *this; }
    OwningCompileOptions& setNoScriptRval(bool nsr) { noScriptRval = nsr; return *this; }
    OwningCompileOptions& setSelfHostingMode(bool shm) { selfHostingMode = shm; return *this; }
    OwningCompileOptions& setCanLazilyParse(bool clp) { canLazilyParse = clp; return *this; }
    OwningCompileOptions& setSourceIsLazy(bool l) { sourceIsLazy = l; return *this; }
    OwningCompileOptions& setIntroductionType(const char* t) { introductionType = t; return *this; }
    bool setIntroductionInfo(JSContext* cx, const char* introducerFn, const char* intro,
                             unsigned line, JSScript* script, uint32_t offset)
    {
        if (!setIntroducerFilename(cx, introducerFn))
            return false;
        introductionType = intro;
        introductionLineno = line;
        introductionScriptRoot = script;
        introductionOffset = offset;
        hasIntroductionInfo = true;
        return true;
    }

  private:
    void operator=(const CompileOptions& rhs) = delete;
};








class MOZ_STACK_CLASS JS_FRIEND_API(CompileOptions) : public ReadOnlyCompileOptions
{
    RootedObject elementRoot;
    RootedString elementAttributeNameRoot;
    RootedScript introductionScriptRoot;

  public:
    explicit CompileOptions(JSContext* cx, JSVersion version = JSVERSION_UNKNOWN);
    CompileOptions(js::ContextFriendFields* cx, const ReadOnlyCompileOptions& rhs)
      : ReadOnlyCompileOptions(), elementRoot(cx), elementAttributeNameRoot(cx),
        introductionScriptRoot(cx)
    {
        copyPODOptions(rhs);

        mutedErrors_ = rhs.mutedErrors_;
        filename_ = rhs.filename();
        sourceMapURL_ = rhs.sourceMapURL();
        elementRoot = rhs.element();
        elementAttributeNameRoot = rhs.elementAttributeName();
        introductionScriptRoot = rhs.introductionScript();
    }

    JSObject* element() const override { return elementRoot; }
    JSString* elementAttributeName() const override { return elementAttributeNameRoot; }
    JSScript* introductionScript() const override { return introductionScriptRoot; }

    CompileOptions& setFile(const char* f) { filename_ = f; return *this; }
    CompileOptions& setLine(unsigned l) { lineno = l; return *this; }
    CompileOptions& setFileAndLine(const char* f, unsigned l) {
        filename_ = f; lineno = l; return *this;
    }
    CompileOptions& setSourceMapURL(const char16_t* s) { sourceMapURL_ = s; return *this; }
    CompileOptions& setElement(JSObject* e)          { elementRoot = e; return *this; }
    CompileOptions& setElementAttributeName(JSString* p) {
        elementAttributeNameRoot = p;
        return *this;
    }
    CompileOptions& setIntroductionScript(JSScript* s) {
        introductionScriptRoot = s;
        return *this;
    }
    CompileOptions& setMutedErrors(bool mute) {
        mutedErrors_ = mute;
        return *this;
    }
    CompileOptions& setVersion(JSVersion v) {
        version = v;
        versionSet = true;
        return *this;
    }
    CompileOptions& setUTF8(bool u) { utf8 = u; return *this; }
    CompileOptions& setColumn(unsigned c) { column = c; return *this; }
    CompileOptions& setHasPollutedScope(bool p) { hasPollutedGlobalScope = p; return *this; }
    CompileOptions& setIsRunOnce(bool once) { isRunOnce = once; return *this; }
    CompileOptions& setForEval(bool eval) { forEval = eval; return *this; }
    CompileOptions& setNoScriptRval(bool nsr) { noScriptRval = nsr; return *this; }
    CompileOptions& setSelfHostingMode(bool shm) { selfHostingMode = shm; return *this; }
    CompileOptions& setCanLazilyParse(bool clp) { canLazilyParse = clp; return *this; }
    CompileOptions& setSourceIsLazy(bool l) { sourceIsLazy = l; return *this; }
    CompileOptions& setIntroductionType(const char* t) { introductionType = t; return *this; }
    CompileOptions& setIntroductionInfo(const char* introducerFn, const char* intro,
                                        unsigned line, JSScript* script, uint32_t offset)
    {
        introducerFilename_ = introducerFn;
        introductionType = intro;
        introductionLineno = line;
        introductionScriptRoot = script;
        introductionOffset = offset;
        hasIntroductionInfo = true;
        return *this;
    }
    CompileOptions& maybeMakeStrictMode(bool strict) {
        strictOption = strictOption || strict;
        return *this;
    }

  private:
    void operator=(const CompileOptions& rhs) = delete;
};




extern JS_PUBLIC_API(bool)
Compile(JSContext* cx, const ReadOnlyCompileOptions& options,
        SourceBufferHolder& srcBuf, JS::MutableHandleScript script);

extern JS_PUBLIC_API(bool)
Compile(JSContext* cx, const ReadOnlyCompileOptions& options,
        const char* bytes, size_t length, JS::MutableHandleScript script);

extern JS_PUBLIC_API(bool)
Compile(JSContext* cx, const ReadOnlyCompileOptions& options,
        const char16_t* chars, size_t length, JS::MutableHandleScript script);

extern JS_PUBLIC_API(bool)
Compile(JSContext* cx, const ReadOnlyCompileOptions& options, FILE* file,
        JS::MutableHandleScript script);

extern JS_PUBLIC_API(bool)
Compile(JSContext* cx, const ReadOnlyCompileOptions& options, const char* filename,
        JS::MutableHandleScript script);

extern JS_PUBLIC_API(bool)
CanCompileOffThread(JSContext* cx, const ReadOnlyCompileOptions& options, size_t length);

















extern JS_PUBLIC_API(bool)
CompileOffThread(JSContext* cx, const ReadOnlyCompileOptions& options,
                 const char16_t* chars, size_t length,
                 OffThreadCompileCallback callback, void* callbackData);

extern JS_PUBLIC_API(JSScript*)
FinishOffThreadScript(JSContext* maybecx, JSRuntime* rt, void* token);








extern JS_PUBLIC_API(bool)
CompileFunction(JSContext* cx, AutoObjectVector& scopeChain,
                const ReadOnlyCompileOptions& options,
                const char* name, unsigned nargs, const char* const* argnames,
                const char16_t* chars, size_t length, JS::MutableHandleFunction fun);




extern JS_PUBLIC_API(bool)
CompileFunction(JSContext* cx, AutoObjectVector& scopeChain,
                const ReadOnlyCompileOptions& options,
                const char* name, unsigned nargs, const char* const* argnames,
                SourceBufferHolder& srcBuf, JS::MutableHandleFunction fun);




extern JS_PUBLIC_API(bool)
CompileFunction(JSContext* cx, AutoObjectVector& scopeChain,
                const ReadOnlyCompileOptions& options,
                const char* name, unsigned nargs, const char* const* argnames,
                const char* bytes, size_t length, JS::MutableHandleFunction fun);

} 

extern JS_PUBLIC_API(JSString*)
JS_DecompileScript(JSContext* cx, JS::Handle<JSScript*> script, const char* name, unsigned indent);





#define JS_DONT_PRETTY_PRINT    ((unsigned)0x8000)

extern JS_PUBLIC_API(JSString*)
JS_DecompileFunction(JSContext* cx, JS::Handle<JSFunction*> fun, unsigned indent);

extern JS_PUBLIC_API(JSString*)
JS_DecompileFunctionBody(JSContext* cx, JS::Handle<JSFunction*> fun, unsigned indent);





































extern JS_PUBLIC_API(bool)
JS_ExecuteScript(JSContext* cx, JS::HandleScript script, JS::MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
JS_ExecuteScript(JSContext* cx, JS::HandleScript script);






extern JS_PUBLIC_API(bool)
JS_ExecuteScript(JSContext* cx, JS::AutoObjectVector& scopeChain,
                 JS::HandleScript script, JS::MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
JS_ExecuteScript(JSContext* cx, JS::AutoObjectVector& scopeChain, JS::HandleScript script);

namespace JS {





extern JS_PUBLIC_API(bool)
CloneAndExecuteScript(JSContext* cx, JS::Handle<JSScript*> script);

} 

namespace JS {




extern JS_PUBLIC_API(bool)
Evaluate(JSContext* cx, const ReadOnlyCompileOptions& options,
         SourceBufferHolder& srcBuf, JS::MutableHandleValue rval);






extern JS_PUBLIC_API(bool)
Evaluate(JSContext* cx, AutoObjectVector& scopeChain, const ReadOnlyCompileOptions& options,
         SourceBufferHolder& srcBuf, JS::MutableHandleValue rval);




extern JS_PUBLIC_API(bool)
Evaluate(JSContext* cx, const ReadOnlyCompileOptions& options,
         const char16_t* chars, size_t length, JS::MutableHandleValue rval);






extern JS_PUBLIC_API(bool)
Evaluate(JSContext* cx, AutoObjectVector& scopeChain, const ReadOnlyCompileOptions& options,
         const char16_t* chars, size_t length, JS::MutableHandleValue rval);




extern JS_PUBLIC_API(bool)
Evaluate(JSContext* cx, const ReadOnlyCompileOptions& options,
         const char* bytes, size_t length, JS::MutableHandleValue rval);




extern JS_PUBLIC_API(bool)
Evaluate(JSContext* cx, const ReadOnlyCompileOptions& options,
         const char* filename, JS::MutableHandleValue rval);

} 

extern JS_PUBLIC_API(bool)
JS_CallFunction(JSContext* cx, JS::HandleObject obj, JS::HandleFunction fun,
                const JS::HandleValueArray& args, JS::MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
JS_CallFunctionName(JSContext* cx, JS::HandleObject obj, const char* name,
                    const JS::HandleValueArray& args, JS::MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
JS_CallFunctionValue(JSContext* cx, JS::HandleObject obj, JS::HandleValue fval,
                     const JS::HandleValueArray& args, JS::MutableHandleValue rval);

namespace JS {

static inline bool
Call(JSContext* cx, JS::HandleObject thisObj, JS::HandleFunction fun,
     const JS::HandleValueArray& args, MutableHandleValue rval)
{
    return !!JS_CallFunction(cx, thisObj, fun, args, rval);
}

static inline bool
Call(JSContext* cx, JS::HandleObject thisObj, const char* name, const JS::HandleValueArray& args,
     MutableHandleValue rval)
{
    return !!JS_CallFunctionName(cx, thisObj, name, args, rval);
}

static inline bool
Call(JSContext* cx, JS::HandleObject thisObj, JS::HandleValue fun, const JS::HandleValueArray& args,
     MutableHandleValue rval)
{
    return !!JS_CallFunctionValue(cx, thisObj, fun, args, rval);
}

extern JS_PUBLIC_API(bool)
Call(JSContext* cx, JS::HandleValue thisv, JS::HandleValue fun, const JS::HandleValueArray& args,
     MutableHandleValue rval);

static inline bool
Call(JSContext* cx, JS::HandleValue thisv, JS::HandleObject funObj, const JS::HandleValueArray& args,
     MutableHandleValue rval)
{
    MOZ_ASSERT(funObj);
    JS::RootedValue fun(cx, JS::ObjectValue(*funObj));
    return Call(cx, thisv, fun, args, rval);
}

extern JS_PUBLIC_API(bool)
Construct(JSContext* cx, JS::HandleValue fun,
          const JS::HandleValueArray& args,
          MutableHandleValue rval);

} 














extern JS_PUBLIC_API(JSInterruptCallback)
JS_SetInterruptCallback(JSRuntime* rt, JSInterruptCallback callback);

extern JS_PUBLIC_API(JSInterruptCallback)
JS_GetInterruptCallback(JSRuntime* rt);

extern JS_PUBLIC_API(void)
JS_RequestInterruptCallback(JSRuntime* rt);

extern JS_PUBLIC_API(bool)
JS_IsRunning(JSContext* cx);












extern JS_PUBLIC_API(bool)
JS_SaveFrameChain(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_RestoreFrameChain(JSContext* cx);

namespace JS {















class MOZ_STACK_CLASS JS_PUBLIC_API(AutoSetAsyncStackForNewCalls)
{
    JSContext* cx;
    RootedObject oldAsyncStack;
    RootedString oldAsyncCause;

  public:
    
    
    
    
    AutoSetAsyncStackForNewCalls(JSContext* cx, HandleObject stack,
                                 HandleString asyncCause);
    ~AutoSetAsyncStackForNewCalls();
};

}












extern JS_PUBLIC_API(JSString*)
JS_NewStringCopyN(JSContext* cx, const char* s, size_t n);

extern JS_PUBLIC_API(JSString*)
JS_NewStringCopyZ(JSContext* cx, const char* s);

extern JS_PUBLIC_API(JSString*)
JS_InternJSString(JSContext* cx, JS::HandleString str);

extern JS_PUBLIC_API(JSString*)
JS_InternStringN(JSContext* cx, const char* s, size_t length);

extern JS_PUBLIC_API(JSString*)
JS_InternString(JSContext* cx, const char* s);

extern JS_PUBLIC_API(JSString*)
JS_NewUCString(JSContext* cx, char16_t* chars, size_t length);

extern JS_PUBLIC_API(JSString*)
JS_NewUCStringCopyN(JSContext* cx, const char16_t* s, size_t n);

extern JS_PUBLIC_API(JSString*)
JS_NewUCStringCopyZ(JSContext* cx, const char16_t* s);

extern JS_PUBLIC_API(JSString*)
JS_InternUCStringN(JSContext* cx, const char16_t* s, size_t length);

extern JS_PUBLIC_API(JSString*)
JS_InternUCString(JSContext* cx, const char16_t* s);

extern JS_PUBLIC_API(bool)
JS_CompareStrings(JSContext* cx, JSString* str1, JSString* str2, int32_t* result);

extern JS_PUBLIC_API(bool)
JS_StringEqualsAscii(JSContext* cx, JSString* str, const char* asciiBytes, bool* match);

extern JS_PUBLIC_API(size_t)
JS_PutEscapedString(JSContext* cx, char* buffer, size_t size, JSString* str, char quote);

extern JS_PUBLIC_API(bool)
JS_FileEscapedString(FILE* fp, JSString* str, char quote);









































extern JS_PUBLIC_API(size_t)
JS_GetStringLength(JSString* str);

extern JS_PUBLIC_API(bool)
JS_StringIsFlat(JSString* str);


extern JS_PUBLIC_API(bool)
JS_StringHasLatin1Chars(JSString* str);

extern JS_PUBLIC_API(const JS::Latin1Char*)
JS_GetLatin1StringCharsAndLength(JSContext* cx, const JS::AutoCheckCannotGC& nogc, JSString* str,
                                 size_t* length);

extern JS_PUBLIC_API(const char16_t*)
JS_GetTwoByteStringCharsAndLength(JSContext* cx, const JS::AutoCheckCannotGC& nogc, JSString* str,
                                  size_t* length);

extern JS_PUBLIC_API(bool)
JS_GetStringCharAt(JSContext* cx, JSString* str, size_t index, char16_t* res);

extern JS_PUBLIC_API(char16_t)
JS_GetFlatStringCharAt(JSFlatString* str, size_t index);

extern JS_PUBLIC_API(const char16_t*)
JS_GetTwoByteExternalStringChars(JSString* str);

extern JS_PUBLIC_API(bool)
JS_CopyStringChars(JSContext* cx, mozilla::Range<char16_t> dest, JSString* str);

extern JS_PUBLIC_API(const JS::Latin1Char*)
JS_GetLatin1InternedStringChars(const JS::AutoCheckCannotGC& nogc, JSString* str);

extern JS_PUBLIC_API(const char16_t*)
JS_GetTwoByteInternedStringChars(const JS::AutoCheckCannotGC& nogc, JSString* str);

extern JS_PUBLIC_API(JSFlatString*)
JS_FlattenString(JSContext* cx, JSString* str);

extern JS_PUBLIC_API(const JS::Latin1Char*)
JS_GetLatin1FlatStringChars(const JS::AutoCheckCannotGC& nogc, JSFlatString* str);

extern JS_PUBLIC_API(const char16_t*)
JS_GetTwoByteFlatStringChars(const JS::AutoCheckCannotGC& nogc, JSFlatString* str);

static MOZ_ALWAYS_INLINE JSFlatString*
JSID_TO_FLAT_STRING(jsid id)
{
    MOZ_ASSERT(JSID_IS_STRING(id));
    return (JSFlatString*)(JSID_BITS(id));
}

static MOZ_ALWAYS_INLINE JSFlatString*
JS_ASSERT_STRING_IS_FLAT(JSString* str)
{
    MOZ_ASSERT(JS_StringIsFlat(str));
    return (JSFlatString*)str;
}

static MOZ_ALWAYS_INLINE JSString*
JS_FORGET_STRING_FLATNESS(JSFlatString* fstr)
{
    return (JSString*)fstr;
}





extern JS_PUBLIC_API(bool)
JS_FlatStringEqualsAscii(JSFlatString* str, const char* asciiBytes);

extern JS_PUBLIC_API(size_t)
JS_PutEscapedFlatString(char* buffer, size_t size, JSFlatString* str, char quote);






extern JS_PUBLIC_API(JSString*)
JS_NewDependentString(JSContext* cx, JS::HandleString str, size_t start,
                      size_t length);





extern JS_PUBLIC_API(JSString*)
JS_ConcatStrings(JSContext* cx, JS::HandleString left, JS::HandleString right);














JS_PUBLIC_API(bool)
JS_DecodeBytes(JSContext* cx, const char* src, size_t srclen, char16_t* dst,
               size_t* dstlenp);





JS_PUBLIC_API(char*)
JS_EncodeString(JSContext* cx, JSString* str);




JS_PUBLIC_API(char*)
JS_EncodeStringToUTF8(JSContext* cx, JS::HandleString str);






JS_PUBLIC_API(size_t)
JS_GetStringEncodingLength(JSContext* cx, JSString* str);









JS_PUBLIC_API(size_t)
JS_EncodeStringToBuffer(JSContext* cx, JSString* str, char* buffer, size_t length);

class JSAutoByteString
{
  public:
    JSAutoByteString(JSContext* cx, JSString* str
                     MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mBytes(JS_EncodeString(cx, str))
    {
        MOZ_ASSERT(cx);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    explicit JSAutoByteString(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
      : mBytes(nullptr)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~JSAutoByteString() {
        js_free(mBytes);
    }

    
    void initBytes(char* bytes) {
        MOZ_ASSERT(!mBytes);
        mBytes = bytes;
    }

    char* encodeLatin1(JSContext* cx, JSString* str) {
        MOZ_ASSERT(!mBytes);
        MOZ_ASSERT(cx);
        mBytes = JS_EncodeString(cx, str);
        return mBytes;
    }

    char* encodeLatin1(js::ExclusiveContext* cx, JSString* str);

    char* encodeUtf8(JSContext* cx, JS::HandleString str) {
        MOZ_ASSERT(!mBytes);
        MOZ_ASSERT(cx);
        mBytes = JS_EncodeStringToUTF8(cx, str);
        return mBytes;
    }

    void clear() {
        js_free(mBytes);
        mBytes = nullptr;
    }

    char* ptr() const {
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
    char*       mBytes;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    JSAutoByteString(const JSAutoByteString& another);
    JSAutoByteString& operator=(const JSAutoByteString& another);
};

namespace JS {

extern JS_PUBLIC_API(JSAddonId*)
NewAddonId(JSContext* cx, JS::HandleString str);

extern JS_PUBLIC_API(JSString*)
StringOfAddonId(JSAddonId* id);

extern JS_PUBLIC_API(JSAddonId*)
AddonIdOfObject(JSObject* obj);

} 






namespace JS {








JS_PUBLIC_API(Symbol*)
NewSymbol(JSContext* cx, HandleString description);








JS_PUBLIC_API(Symbol*)
GetSymbolFor(JSContext* cx, HandleString key);







JS_PUBLIC_API(JSString*)
GetSymbolDescription(HandleSymbol symbol);


enum class SymbolCode : uint32_t {
    iterator,                       
    match,                          
    InSymbolRegistry = 0xfffffffe,  
    UniqueSymbol = 0xffffffff       
};


const size_t WellKnownSymbolLimit = 2;






JS_PUBLIC_API(SymbolCode)
GetSymbolCode(Handle<Symbol*> symbol);







JS_PUBLIC_API(Symbol*)
GetWellKnownSymbol(JSContext* cx, SymbolCode which);





inline bool
PropertySpecNameIsSymbol(const char* name)
{
    uintptr_t u = reinterpret_cast<uintptr_t>(name);
    return u != 0 && u - 1 < WellKnownSymbolLimit;
}

JS_PUBLIC_API(bool)
PropertySpecNameEqualsId(const char* name, HandleId id);









JS_PUBLIC_API(bool)
PropertySpecNameToPermanentId(JSContext* cx, const char* name, jsid* idp);

} 





typedef bool (* JSONWriteCallback)(const char16_t* buf, uint32_t len, void* data);




JS_PUBLIC_API(bool)
JS_Stringify(JSContext* cx, JS::MutableHandleValue value, JS::HandleObject replacer,
             JS::HandleValue space, JSONWriteCallback callback, void* data);




JS_PUBLIC_API(bool)
JS_ParseJSON(JSContext* cx, const char16_t* chars, uint32_t len, JS::MutableHandleValue vp);

JS_PUBLIC_API(bool)
JS_ParseJSON(JSContext* cx, JS::HandleString str, JS::MutableHandleValue vp);

JS_PUBLIC_API(bool)
JS_ParseJSONWithReviver(JSContext* cx, const char16_t* chars, uint32_t len, JS::HandleValue reviver,
                        JS::MutableHandleValue vp);

JS_PUBLIC_API(bool)
JS_ParseJSONWithReviver(JSContext* cx, JS::HandleString str, JS::HandleValue reviver,
                        JS::MutableHandleValue vp);










extern JS_PUBLIC_API(bool)
JS_SetDefaultLocale(JSRuntime* rt, const char* locale);




extern JS_PUBLIC_API(void)
JS_ResetDefaultLocale(JSRuntime* rt);




struct JSLocaleCallbacks {
    JSLocaleToUpperCase     localeToUpperCase;
    JSLocaleToLowerCase     localeToLowerCase;
    JSLocaleCompare         localeCompare; 
    JSLocaleToUnicode       localeToUnicode;
};





extern JS_PUBLIC_API(void)
JS_SetLocaleCallbacks(JSRuntime* rt, const JSLocaleCallbacks* callbacks);





extern JS_PUBLIC_API(const JSLocaleCallbacks*)
JS_GetLocaleCallbacks(JSRuntime* rt);












extern JS_PUBLIC_API(void)
JS_ReportError(JSContext* cx, const char* format, ...);




extern JS_PUBLIC_API(void)
JS_ReportErrorNumber(JSContext* cx, JSErrorCallback errorCallback,
                     void* userRef, const unsigned errorNumber, ...);

#ifdef va_start
extern JS_PUBLIC_API(void)
JS_ReportErrorNumberVA(JSContext* cx, JSErrorCallback errorCallback,
                       void* userRef, const unsigned errorNumber, va_list ap);
#endif




extern JS_PUBLIC_API(void)
JS_ReportErrorNumberUC(JSContext* cx, JSErrorCallback errorCallback,
                     void* userRef, const unsigned errorNumber, ...);

extern JS_PUBLIC_API(void)
JS_ReportErrorNumberUCArray(JSContext* cx, JSErrorCallback errorCallback,
                            void* userRef, const unsigned errorNumber,
                            const char16_t** args);







extern JS_PUBLIC_API(bool)
JS_ReportWarning(JSContext* cx, const char* format, ...);

extern JS_PUBLIC_API(bool)
JS_ReportErrorFlagsAndNumber(JSContext* cx, unsigned flags,
                             JSErrorCallback errorCallback, void* userRef,
                             const unsigned errorNumber, ...);

extern JS_PUBLIC_API(bool)
JS_ReportErrorFlagsAndNumberUC(JSContext* cx, unsigned flags,
                               JSErrorCallback errorCallback, void* userRef,
                               const unsigned errorNumber, ...);




extern JS_PUBLIC_API(void)
JS_ReportOutOfMemory(JSContext* cx);




extern JS_PUBLIC_API(void)
JS_ReportAllocationOverflow(JSContext* cx);

class JSErrorReport
{
  public:
    JSErrorReport()
      : filename(nullptr), lineno(0), column(0), isMuted(false), linebuf(nullptr),
        tokenptr(nullptr), uclinebuf(nullptr), uctokenptr(nullptr), flags(0), errorNumber(0),
        ucmessage(nullptr), messageArgs(nullptr), exnType(0)
    {}

    const char*     filename;      
    unsigned        lineno;         
    unsigned        column;         
    bool            isMuted;        
    const char*     linebuf;       
    const char*     tokenptr;      
    const char16_t* uclinebuf;     
    const char16_t* uctokenptr;    
    unsigned        flags;          
    unsigned        errorNumber;    
    const char16_t* ucmessage;     
    const char16_t** messageArgs;  
    int16_t         exnType;        
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
JS_GetErrorReporter(JSRuntime* rt);

extern JS_PUBLIC_API(JSErrorReporter)
JS_SetErrorReporter(JSRuntime* rt, JSErrorReporter er);

namespace JS {

extern JS_PUBLIC_API(bool)
CreateError(JSContext* cx, JSExnType type, HandleObject stack,
            HandleString fileName, uint32_t lineNumber, uint32_t columnNumber,
            JSErrorReport* report, HandleString message, MutableHandleValue rval);







extern JS_PUBLIC_API(JSObject*)
NewWeakMapObject(JSContext* cx);

extern JS_PUBLIC_API(bool)
IsWeakMapObject(JSObject* obj);

extern JS_PUBLIC_API(bool)
GetWeakMapEntry(JSContext* cx, JS::HandleObject mapObj, JS::HandleObject key,
                JS::MutableHandleValue val);

extern JS_PUBLIC_API(bool)
SetWeakMapEntry(JSContext* cx, JS::HandleObject mapObj, JS::HandleObject key,
                JS::HandleValue val);




extern JS_PUBLIC_API(JSObject*)
NewMapObject(JSContext* cx);

extern JS_PUBLIC_API(uint32_t)
MapSize(JSContext* cx, HandleObject obj);

extern JS_PUBLIC_API(bool)
MapGet(JSContext* cx, HandleObject obj,
       HandleValue key, MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
MapHas(JSContext* cx, HandleObject obj, HandleValue key, bool* rval);

extern JS_PUBLIC_API(bool)
MapSet(JSContext* cx, HandleObject obj, HandleValue key, HandleValue val);

extern JS_PUBLIC_API(bool)
MapClear(JSContext* cx, HandleObject obj);

extern JS_PUBLIC_API(bool)
MapKeys(JSContext* cx, HandleObject obj, MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
MapValues(JSContext* cx, HandleObject obj, MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
MapEntries(JSContext* cx, HandleObject obj, MutableHandleValue rval);

} 





extern JS_PUBLIC_API(JSObject*)
JS_NewDateObject(JSContext* cx, int year, int mon, int mday, int hour, int min, int sec);

extern JS_PUBLIC_API(JSObject*)
JS_NewDateObjectMsec(JSContext* cx, double msec);




extern JS_PUBLIC_API(bool)
JS_ObjectIsDate(JSContext* cx, JS::HandleObject obj);





extern JS_PUBLIC_API(void)
JS_ClearDateCaches(JSContext* cx);






#define JSREG_FOLD      0x01u   /* fold uppercase to lowercase */
#define JSREG_GLOB      0x02u   /* global exec, creates array of matches */
#define JSREG_MULTILINE 0x04u   /* treat ^ and $ as begin and end of line */
#define JSREG_STICKY    0x08u   /* only match starting at lastIndex */

extern JS_PUBLIC_API(JSObject*)
JS_NewRegExpObject(JSContext* cx, JS::HandleObject obj, const char* bytes, size_t length,
                   unsigned flags);

extern JS_PUBLIC_API(JSObject*)
JS_NewUCRegExpObject(JSContext* cx, JS::HandleObject obj, const char16_t* chars, size_t length,
                     unsigned flags);

extern JS_PUBLIC_API(bool)
JS_SetRegExpInput(JSContext* cx, JS::HandleObject obj, JS::HandleString input,
                  bool multiline);

extern JS_PUBLIC_API(bool)
JS_ClearRegExpStatics(JSContext* cx, JS::HandleObject obj);

extern JS_PUBLIC_API(bool)
JS_ExecuteRegExp(JSContext* cx, JS::HandleObject obj, JS::HandleObject reobj,
                 char16_t* chars, size_t length, size_t* indexp, bool test,
                 JS::MutableHandleValue rval);



extern JS_PUBLIC_API(JSObject*)
JS_NewRegExpObjectNoStatics(JSContext* cx, char* bytes, size_t length, unsigned flags);

extern JS_PUBLIC_API(JSObject*)
JS_NewUCRegExpObjectNoStatics(JSContext* cx, char16_t* chars, size_t length, unsigned flags);

extern JS_PUBLIC_API(bool)
JS_ExecuteRegExpNoStatics(JSContext* cx, JS::HandleObject reobj, char16_t* chars, size_t length,
                          size_t* indexp, bool test, JS::MutableHandleValue rval);

extern JS_PUBLIC_API(bool)
JS_ObjectIsRegExp(JSContext* cx, JS::HandleObject obj);

extern JS_PUBLIC_API(unsigned)
JS_GetRegExpFlags(JSContext* cx, JS::HandleObject obj);

extern JS_PUBLIC_API(JSString*)
JS_GetRegExpSource(JSContext* cx, JS::HandleObject obj);



extern JS_PUBLIC_API(bool)
JS_IsExceptionPending(JSContext* cx);

extern JS_PUBLIC_API(bool)
JS_GetPendingException(JSContext* cx, JS::MutableHandleValue vp);

extern JS_PUBLIC_API(void)
JS_SetPendingException(JSContext* cx, JS::HandleValue v);

extern JS_PUBLIC_API(void)
JS_ClearPendingException(JSContext* cx);

extern JS_PUBLIC_API(bool)
JS_ReportPendingException(JSContext* cx);

namespace JS {













class JS_PUBLIC_API(AutoSaveExceptionState)
{
  private:
    JSContext* context;
    bool wasPropagatingForcedReturn;
    bool wasOverRecursed;
    bool wasThrowing;
    RootedValue exceptionValue;

  public:
    



    explicit AutoSaveExceptionState(JSContext* cx);

    



    ~AutoSaveExceptionState();

    



    void drop() {
        wasPropagatingForcedReturn = false;
        wasOverRecursed = false;
        wasThrowing = false;
        exceptionValue.setUndefined();
    }

    




    void restore();
};

} 


extern JS_PUBLIC_API(JSExceptionState*)
JS_SaveExceptionState(JSContext* cx);

extern JS_PUBLIC_API(void)
JS_RestoreExceptionState(JSContext* cx, JSExceptionState* state);

extern JS_PUBLIC_API(void)
JS_DropExceptionState(JSContext* cx, JSExceptionState* state);








extern JS_PUBLIC_API(JSErrorReport*)
JS_ErrorFromException(JSContext* cx, JS::HandleObject obj);




extern JS_PUBLIC_API(bool)
JS_ThrowStopIteration(JSContext* cx);

extern JS_PUBLIC_API(bool)
JS_IsStopIteration(jsval v);

extern JS_PUBLIC_API(intptr_t)
JS_GetCurrentThread();











extern JS_PUBLIC_API(void)
JS_AbortIfWrongThread(JSRuntime* rt);








extern JS_PUBLIC_API(JSObject*)
JS_NewObjectForConstructor(JSContext* cx, const JSClass* clasp, const JS::CallArgs& args);



#ifdef JS_GC_ZEAL
#define JS_DEFAULT_ZEAL_FREQ 100

extern JS_PUBLIC_API(void)
JS_GetGCZeal(JSContext* cx, uint8_t* zeal, uint32_t* frequency, uint32_t* nextScheduled);

extern JS_PUBLIC_API(void)
JS_SetGCZeal(JSContext* cx, uint8_t zeal, uint32_t frequency);

extern JS_PUBLIC_API(void)
JS_ScheduleGC(JSContext* cx, uint32_t count);
#endif

extern JS_PUBLIC_API(void)
JS_SetParallelParsingEnabled(JSRuntime* rt, bool enabled);

extern JS_PUBLIC_API(void)
JS_SetOffthreadIonCompilationEnabled(JSRuntime* rt, bool enabled);

#define JIT_COMPILER_OPTIONS(Register)                                     \
    Register(BASELINE_WARMUP_TRIGGER, "baseline.warmup.trigger")           \
    Register(ION_WARMUP_TRIGGER, "ion.warmup.trigger")                     \
    Register(ION_GVN_ENABLE, "ion.gvn.enable")                             \
    Register(ION_ENABLE, "ion.enable")                                     \
    Register(BASELINE_ENABLE, "baseline.enable")                           \
    Register(OFFTHREAD_COMPILATION_ENABLE, "offthread-compilation.enable") \
    Register(SIGNALS_ENABLE, "signals.enable")

typedef enum JSJitCompilerOption {
#define JIT_COMPILER_DECLARE(key, str) \
    JSJITCOMPILER_ ## key,

    JIT_COMPILER_OPTIONS(JIT_COMPILER_DECLARE)
#undef JIT_COMPILER_DECLARE

    JSJITCOMPILER_NOT_AN_OPTION
} JSJitCompilerOption;

extern JS_PUBLIC_API(void)
JS_SetGlobalJitCompilerOption(JSRuntime* rt, JSJitCompilerOption opt, uint32_t value);
extern JS_PUBLIC_API(int)
JS_GetGlobalJitCompilerOption(JSRuntime* rt, JSJitCompilerOption opt);




extern JS_PUBLIC_API(bool)
JS_IndexToId(JSContext* cx, uint32_t index, JS::MutableHandleId);






extern JS_PUBLIC_API(bool)
JS_CharsToId(JSContext* cx, JS::TwoByteChars chars, JS::MutableHandleId);




extern JS_PUBLIC_API(bool)
JS_IsIdentifier(JSContext* cx, JS::HandleString str, bool* isIdentifier);






extern JS_PUBLIC_API(bool)
JS_IsIdentifier(const char16_t* chars, size_t length);

namespace JS {





class MOZ_STACK_CLASS JS_PUBLIC_API(AutoFilename)
{
    void* scriptSource_;

    AutoFilename(const AutoFilename&) = delete;
    void operator=(const AutoFilename&) = delete;

  public:
    AutoFilename() : scriptSource_(nullptr) {}
    ~AutoFilename() { reset(nullptr); }

    const char* get() const;

    void reset(void* newScriptSource);
};








extern JS_PUBLIC_API(bool)
DescribeScriptedCaller(JSContext* cx, AutoFilename* filename = nullptr,
                       unsigned* lineno = nullptr);

extern JS_PUBLIC_API(JSObject*)
GetScriptedCallerGlobal(JSContext* cx);













extern JS_PUBLIC_API(void)
HideScriptedCaller(JSContext* cx);

extern JS_PUBLIC_API(void)
UnhideScriptedCaller(JSContext* cx);

class AutoHideScriptedCaller
{
  public:
    explicit AutoHideScriptedCaller(JSContext* cx
                                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        HideScriptedCaller(mContext);
    }
    ~AutoHideScriptedCaller() {
        UnhideScriptedCaller(mContext);
    }

  protected:
    JSContext* mContext;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 





extern JS_PUBLIC_API(void*)
JS_EncodeScript(JSContext* cx, JS::HandleScript script, uint32_t* lengthp);

extern JS_PUBLIC_API(void*)
JS_EncodeInterpretedFunction(JSContext* cx, JS::HandleObject funobj, uint32_t* lengthp);

extern JS_PUBLIC_API(JSScript*)
JS_DecodeScript(JSContext* cx, const void* data, uint32_t length);

extern JS_PUBLIC_API(JSObject*)
JS_DecodeInterpretedFunction(JSContext* cx, const void* data, uint32_t length);

namespace JS {










typedef bool
(* OpenAsmJSCacheEntryForReadOp)(HandleObject global, const char16_t* begin, const char16_t* limit,
                                 size_t* size, const uint8_t** memory, intptr_t* handle);
typedef void
(* CloseAsmJSCacheEntryForReadOp)(size_t size, const uint8_t* memory, intptr_t handle);


enum AsmJSCacheResult
{
    AsmJSCache_MIN,
    AsmJSCache_Success = AsmJSCache_MIN,
    AsmJSCache_ModuleTooSmall,
    AsmJSCache_SynchronousScript,
    AsmJSCache_QuotaExceeded,
    AsmJSCache_StorageInitFailure,
    AsmJSCache_Disabled_Internal,
    AsmJSCache_Disabled_ShellFlags,
    AsmJSCache_Disabled_JitInspector,
    AsmJSCache_InternalError,
    AsmJSCache_LIMIT
};
















typedef AsmJSCacheResult
(* OpenAsmJSCacheEntryForWriteOp)(HandleObject global, bool installed,
                                  const char16_t* begin, const char16_t* end,
                                  size_t size, uint8_t** memory, intptr_t* handle);
typedef void
(* CloseAsmJSCacheEntryForWriteOp)(size_t size, uint8_t* memory, intptr_t handle);

typedef js::Vector<char, 0, js::SystemAllocPolicy> BuildIdCharVector;







typedef bool
(* BuildIdOp)(BuildIdCharVector* buildId);

struct AsmJSCacheOps
{
    OpenAsmJSCacheEntryForReadOp openEntryForRead;
    CloseAsmJSCacheEntryForReadOp closeEntryForRead;
    OpenAsmJSCacheEntryForWriteOp openEntryForWrite;
    CloseAsmJSCacheEntryForWriteOp closeEntryForWrite;
    BuildIdOp buildId;
};

extern JS_PUBLIC_API(void)
SetAsmJSCacheOps(JSRuntime* rt, const AsmJSCacheOps* callbacks);


















class MOZ_STACK_CLASS JS_PUBLIC_API(ForOfIterator) {
  protected:
    JSContext* cx_;
    













    JS::RootedObject iterator;
    uint32_t index;

    static const uint32_t NOT_ARRAY = UINT32_MAX;

    ForOfIterator(const ForOfIterator&) = delete;
    ForOfIterator& operator=(const ForOfIterator&) = delete;

  public:
    explicit ForOfIterator(JSContext* cx) : cx_(cx), iterator(cx_), index(NOT_ARRAY) { }

    enum NonIterableBehavior {
        ThrowOnNonIterable,
        AllowNonIterable
    };

    





    bool init(JS::HandleValue iterable,
              NonIterableBehavior nonIterableBehavior = ThrowOnNonIterable);

    




    bool initWithIterator(JS::HandleValue aIterator);

    



    bool next(JS::MutableHandleValue val, bool* done);

    



    bool valueIsIterable() const {
        return iterator;
    }

  private:
    inline bool nextFromOptimizedArray(MutableHandleValue val, bool* done);
    bool materializeArrayIterator();
};









typedef void
(* LargeAllocationFailureCallback)(void* data);

extern JS_PUBLIC_API(void)
SetLargeAllocationFailureCallback(JSRuntime* rt, LargeAllocationFailureCallback afc, void* data);












typedef void
(* OutOfMemoryCallback)(JSContext* cx, void* data);

extern JS_PUBLIC_API(void)
SetOutOfMemoryCallback(JSRuntime* rt, OutOfMemoryCallback cb, void* data);








extern JS_PUBLIC_API(bool)
CaptureCurrentStack(JSContext* cx, MutableHandleObject stackp, unsigned maxFrameCount = 0);

























enum class SavedFrameResult {
    Ok,
    AccessDenied
};





extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameSource(JSContext* cx, HandleObject savedFrame, MutableHandleString sourcep);




extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameLine(JSContext* cx, HandleObject savedFrame, uint32_t* linep);




extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameColumn(JSContext* cx, HandleObject savedFrame, uint32_t* columnp);






extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameFunctionDisplayName(JSContext* cx, HandleObject savedFrame, MutableHandleString namep);




extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameAsyncCause(JSContext* cx, HandleObject savedFrame, MutableHandleString asyncCausep);






extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameAsyncParent(JSContext* cx, HandleObject savedFrame, MutableHandleObject asyncParentp);






extern JS_PUBLIC_API(SavedFrameResult)
GetSavedFrameParent(JSContext* cx, HandleObject savedFrame, MutableHandleObject parentp);










extern JS_PUBLIC_API(bool)
BuildStackString(JSContext* cx, HandleObject stack, MutableHandleString stringp);

} 




namespace js {

struct AutoStopwatch;



struct PerformanceData {
    
    
    
    
    
    
    
    uint64_t durations[10];

    
    
    uint64_t totalUserTime;
    uint64_t totalSystemTime;
    uint64_t totalCPOWTime;

    
    
    
    uint64_t ticks;

    PerformanceData()
      : totalUserTime(0)
      , totalSystemTime(0)
      , totalCPOWTime(0)
      , ticks(0)
    {
        mozilla::PodArrayZero(durations);
    }
    PerformanceData(const PerformanceData& from)
      : totalUserTime(from.totalUserTime)
      , totalSystemTime(from.totalSystemTime)
      , totalCPOWTime(from.totalCPOWTime)
      , ticks(from.ticks)
    {
        mozilla::PodArrayCopy(durations, from.durations);
    }
    PerformanceData& operator=(const PerformanceData& from)
    {
        mozilla::PodArrayCopy(durations, from.durations);
        totalUserTime = from.totalUserTime;
        totalSystemTime = from.totalSystemTime;
        totalCPOWTime = from.totalCPOWTime;
        ticks = from.ticks;
        return *this;
    }
};











struct PerformanceGroup {

    
    PerformanceData data;

    
    
    
    bool hasStopwatch(uint64_t iteration) const {
        return stopwatch_ != nullptr && iteration_ == iteration;
    }

    
    
    void acquireStopwatch(uint64_t iteration, const AutoStopwatch *stopwatch) {
        iteration_ = iteration;
        stopwatch_ = stopwatch;
    }

    
    
    void releaseStopwatch(uint64_t iteration, const AutoStopwatch *stopwatch) {
        if (iteration_ != iteration)
            return;

        MOZ_ASSERT(stopwatch == stopwatch_ || stopwatch_ == nullptr);
        stopwatch_ = nullptr;
    }

    PerformanceGroup()
      : stopwatch_(nullptr)
      , iteration_(0)
      , refCount_(0)
    { }
    ~PerformanceGroup()
    {
        MOZ_ASSERT(refCount_ == 0);
    }
  private:
    PerformanceGroup& operator=(const PerformanceGroup&) = delete;
    PerformanceGroup(const PerformanceGroup&) = delete;

    
    
    const AutoStopwatch *stopwatch_;

    
    
    uint64_t iteration_;

    
    uint64_t incRefCount() {
        MOZ_ASSERT(refCount_ + 1 > 0);
        return ++refCount_;
    }
    uint64_t decRefCount() {
        MOZ_ASSERT(refCount_ > 0);
        return --refCount_;
    }
    friend struct PerformanceGroupHolder;

  private:
    
    uint64_t refCount_;
};





struct PerformanceGroupHolder {
    
    
    
    js::PerformanceGroup *getGroup();

    
    
    
    
    inline bool isLinked() const {
        return group_ != nullptr;
    }

    
    
    
    void unlink();

    PerformanceGroupHolder(JSRuntime *runtime, JSCompartment *compartment)
      : runtime_(runtime)
      , compartment_(compartment)
      , group_(nullptr)
    {   }
    ~PerformanceGroupHolder();
private:
    
    
    
    void* getHashKey();

    JSRuntime *runtime_;
    JSCompartment *compartment_;

    
    
    
    js::PerformanceGroup *group_;
};






extern JS_PUBLIC_API(void)
ResetStopwatches(JSRuntime*);







extern JS_PUBLIC_API(bool)
SetStopwatchActive(JSRuntime*, bool);
extern JS_PUBLIC_API(bool)
IsStopwatchActive(JSRuntime*);




extern JS_PUBLIC_API(PerformanceData*)
GetPerformanceData(JSRuntime*);





struct PerformanceStats {
    



    JSAddonId *addonId;

    



    char name[1024];

    




    bool isSystem;

    


    js::PerformanceData performance;

    PerformanceStats()
      : addonId(nullptr)
      , isSystem(false)
    {
        name[0] = '\0';
    }
};

typedef js::Vector<PerformanceStats, 0, js::SystemAllocPolicy> PerformanceStatsVector;

    






extern JS_PUBLIC_API(bool)
GetPerformanceStats(JSRuntime *rt, js::PerformanceStatsVector &stats, js::PerformanceStats &global);

} 


#endif
