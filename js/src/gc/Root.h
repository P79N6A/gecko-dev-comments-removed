






#ifndef jsgc_root_h__
#define jsgc_root_h__

#ifdef __cplusplus

#include "mozilla/TypeTraits.h"
#include "mozilla/GuardObjects.h"

#include "js/TemplateLib.h"

#include "jspubtd.h"

namespace JS {












































template <typename T> class MutableHandle;
template <typename T> class Rooted;

template <typename T>
struct RootMethods { };








struct NullPtr
{
    static void * const constNullValue;
};

template <typename T>
class MutableHandle;

template <typename T>
class HandleBase {};









template <typename T>
class Handle : public HandleBase<T>
{
  public:
    
    template <typename S>
    Handle(Handle<S> handle,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0)
    {
        ptr = reinterpret_cast<const T *>(handle.address());
    }

    
    Handle(NullPtr) {
        typedef typename js::tl::StaticAssert<js::tl::IsPointerType<T>::result>::result _;
        ptr = reinterpret_cast<const T *>(&NullPtr::constNullValue);
    }

    friend class MutableHandle<T>;
    Handle(MutableHandle<T> handle) {
        ptr = handle.address();
    }

    






    static Handle fromMarkedLocation(const T *p) {
        Handle h;
        h.ptr = p;
        return h;
    }

    



    template <typename S>
    inline
    Handle(Rooted<S> &root,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    
    template <typename S>
    inline
    Handle(MutableHandle<S> &root,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    const T *address() const { return ptr; }
    T get() const { return *ptr; }

    operator T () const { return get(); }
    T operator ->() const { return get(); }

  private:
    Handle() {}

    const T *ptr;

    template <typename S>
    void operator =(S v) MOZ_DELETE;
};

typedef Handle<JSObject*>    HandleObject;
typedef Handle<JSFunction*>  HandleFunction;
typedef Handle<JSScript*>    HandleScript;
typedef Handle<JSString*>    HandleString;
typedef Handle<jsid>         HandleId;
typedef Handle<Value>        HandleValue;

template <typename T>
class MutableHandleBase {};









template <typename T>
class MutableHandle : public MutableHandleBase<T>
{
  public:
    template <typename S>
    MutableHandle(MutableHandle<S> handle,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0)
    {
        this->ptr = reinterpret_cast<const T *>(handle.address());
    }

    template <typename S>
    inline
    MutableHandle(Rooted<S> *root,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    void set(T v)
    {
        JS_ASSERT(!RootMethods<T>::poisoned(v));
        *ptr = v;
    }

    






    static MutableHandle fromMarkedLocation(T *p) {
        MutableHandle h;
        h.ptr = p;
        return h;
    }

    T *address() const { return ptr; }
    T get() const { return *ptr; }

    operator T () const { return get(); }
    T operator ->() const { return get(); }

  private:
    MutableHandle() {}

    T *ptr;

    template <typename S>
    void operator =(S v) MOZ_DELETE;
};

typedef MutableHandle<JSObject*>    MutableHandleObject;
typedef MutableHandle<Value>        MutableHandleValue;
typedef MutableHandle<jsid>         MutableHandleId;





typedef JSObject *                  RawObject;
typedef JSString *                  RawString;
typedef Value                       RawValue;

extern mozilla::ThreadLocal<JSRuntime *> TlsRuntime;






template <typename T>
struct RootKind<T *> { static ThingRootKind rootKind() { return T::rootKind(); } };

template <typename T>
struct RootMethods<T *>
{
    static T *initial() { return NULL; }
    static ThingRootKind kind() { return RootKind<T *>::rootKind(); }
    static bool poisoned(T *v) { return IsPoisonedPtr(v); }
};

template <typename T>
class RootedBase {};









template <typename T>
class Rooted : public RootedBase<T>
{
    void init(JSContext *cxArg)
    {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        ContextFriendFields *cx = ContextFriendFields::get(cxArg);

        ThingRootKind kind = RootMethods<T>::kind();
        this->stack = reinterpret_cast<Rooted<T>**>(&cx->thingGCRooters[kind]);
        this->prev = *stack;
        *stack = this;

        JS_ASSERT(!RootMethods<T>::poisoned(ptr));
#endif
    }

    void init(JSRuntime *rtArg)
    {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        RuntimeFriendFields *rt = const_cast<RuntimeFriendFields *>(RuntimeFriendFields::get(rtArg));

        ThingRootKind kind = RootMethods<T>::kind();
        this->stack = reinterpret_cast<Rooted<T>**>(&rt->thingGCRooters[kind]);
        this->prev = *stack;
        *stack = this;

        JS_ASSERT(!RootMethods<T>::poisoned(ptr));
#endif
    }

  public:
    Rooted() : ptr(RootMethods<T>::initial()) { init(JS::TlsRuntime); }
    Rooted(const T &initial) : ptr(initial) { init(JS::TlsRuntime); }

    Rooted(JSRuntime *rt) : ptr(RootMethods<T>::initial()) { init(rt); }
    Rooted(JSRuntime *rt, T initial) : ptr(initial) { init(rt); }

    Rooted(JSContext *cx) : ptr(RootMethods<T>::initial()) { init(cx); }
    Rooted(JSContext *cx, T initial) : ptr(initial) { init(cx); }

    ~Rooted()
    {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        JS_ASSERT(*stack == this);
        *stack = prev;
#endif
    }

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    Rooted<T> *previous() { return prev; }
#endif

    operator T () const { return ptr; }
    T operator ->() const { return ptr; }
    T * address() { return &ptr; }
    const T * address() const { return &ptr; }
    T & get() { return ptr; }
    const T & get() const { return ptr; }

    T & operator =(T value)
    {
        JS_ASSERT(!RootMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    T & operator =(const Rooted &value)
    {
        ptr = value;
        return ptr;
    }

  private:
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    Rooted<T> **stack, *prev;
#endif
    T ptr;

    Rooted(const Rooted &) MOZ_DELETE;
};

template<typename T> template <typename S>
inline
Handle<T>::Handle(Rooted<S> &root,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
{
    ptr = reinterpret_cast<const T *>(root.address());
}

template<typename T> template <typename S>
inline
Handle<T>::Handle(MutableHandle<S> &root,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
{
    ptr = reinterpret_cast<const T *>(root.address());
}

template<typename T> template <typename S>
inline
MutableHandle<T>::MutableHandle(Rooted<S> *root,
                                typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
{
    ptr = root->address();
}

typedef Rooted<JSObject*>    RootedObject;
typedef Rooted<JSFunction*>  RootedFunction;
typedef Rooted<JSScript*>    RootedScript;
typedef Rooted<JSString*>    RootedString;
typedef Rooted<jsid>         RootedId;
typedef Rooted<Value>        RootedValue;







class SkipRoot
{
#if defined(DEBUG) && defined(JSGC_ROOT_ANALYSIS)

    SkipRoot **stack, *prev;
    const uint8_t *start;
    const uint8_t *end;

    template <typename T>
    void init(ContextFriendFields *cx, const T *ptr, size_t count)
    {
        this->stack = &cx->skipGCRooters;
        this->prev = *stack;
        *stack = this;
        this->start = (const uint8_t *) ptr;
        this->end = this->start + (sizeof(T) * count);
    }

  public:
    template <typename T>
    SkipRoot(JSContext *cx, const T *ptr, size_t count = 1
             JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        init(ContextFriendFields::get(cx), ptr, count);
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~SkipRoot()
    {
        JS_ASSERT(*stack == this);
        *stack = prev;
    }

    SkipRoot *previous() { return prev; }

    bool contains(const uint8_t *v, size_t len) {
        return v >= start && v + len <= end;
    }

#else 

  public:
    template <typename T>
    SkipRoot(JSContext *cx, const T *ptr, size_t count = 1
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

#endif 

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

JS_FRIEND_API(void) EnterAssertNoGCScope();
JS_FRIEND_API(void) LeaveAssertNoGCScope();
JS_FRIEND_API(bool) InNoGCScope();






class AutoAssertNoGC
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

public:
    AutoAssertNoGC(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef DEBUG
        EnterAssertNoGCScope();
#endif
    }

    ~AutoAssertNoGC() {
#ifdef DEBUG
        LeaveAssertNoGCScope();
#endif
    }
};

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
extern void
CheckStackRoots(JSContext *cx);
#endif

JS_FRIEND_API(bool) NeedRelaxedRootChecks();





inline void MaybeCheckStackRoots(JSContext *cx, bool relax = true)
{
#ifdef DEBUG
    JS_ASSERT(!InNoGCScope());
# if defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    if (relax && NeedRelaxedRootChecks())
        return;
    CheckStackRoots(cx);
# endif
#endif
}

}  

#endif  

#endif  
