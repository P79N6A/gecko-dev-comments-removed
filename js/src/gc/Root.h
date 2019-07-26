






#ifndef jsgc_root_h__
#define jsgc_root_h__

#ifdef __cplusplus

#include "mozilla/TypeTraits.h"
#include "mozilla/GuardObjects.h"

#include "js/TemplateLib.h"

#include "jspubtd.h"












































namespace js {

template <typename T> class Rooted;

template <typename T>
struct RootMethods {};

template <typename T>
class HandleBase {};

template <typename T>
class MutableHandleBase {};

} 

namespace JS {

class AutoAssertNoGC;

template <typename T> class MutableHandle;

JS_FRIEND_API(void) EnterAssertNoGCScope();
JS_FRIEND_API(void) LeaveAssertNoGCScope();
JS_FRIEND_API(bool) InNoGCScope();








struct NullPtr
{
    static void * const constNullValue;
};

template <typename T>
class MutableHandle;









template <typename T>
class Handle : public js::HandleBase<T>
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
    Handle(js::Rooted<S> &root,
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
class MutableHandle : public js::MutableHandleBase<T>
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
    MutableHandle(js::Rooted<S> *root,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    void set(T v)
    {
        JS_ASSERT(!js::RootMethods<T>::poisoned(v));
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

typedef MutableHandle<JSObject*>   MutableHandleObject;
typedef MutableHandle<JSFunction*> MutableHandleFunction;
typedef MutableHandle<JSScript*>   MutableHandleScript;
typedef MutableHandle<JSString*>   MutableHandleString;
typedef MutableHandle<jsid>        MutableHandleId;
typedef MutableHandle<Value>       MutableHandleValue;





typedef JSObject *                  RawObject;
typedef JSFunction *                RawFunction;
typedef JSScript *                  RawScript;
typedef JSString *                  RawString;
typedef jsid                        RawId;
typedef Value                       RawValue;

} 

namespace js {







template <typename T>
class InternalHandle { };

template <typename T>
class InternalHandle<T*>
{
    void * const *holder;
    size_t offset;

  public:
    



    template<typename H>
    InternalHandle(const JS::Handle<H> &handle, T *field)
      : holder((void**)handle.address()), offset(uintptr_t(field) - uintptr_t(handle.get()))
    {
    }

    


    template<typename R>
    InternalHandle(const Rooted<R> &root, T *field)
      : holder((void**)root.address()), offset(uintptr_t(field) - uintptr_t(root.get()))
    {
    }

    T *get() const { return reinterpret_cast<T*>(uintptr_t(*holder) + offset); }

    const T& operator *() const { return *get(); }
    T* operator ->() const { return get(); }

    static InternalHandle<T*> fromMarkedLocation(T *fieldPtr) {
        return InternalHandle(fieldPtr);
    }

  private:
    








    InternalHandle(T *field)
      : holder(reinterpret_cast<void * const *>(&NullPtr::constNullValue)),
        offset(uintptr_t(field))
    {
    }
};

#ifdef DEBUG
template <typename T>
class IntermediateNoGC
{
    T t_;

  public:
    IntermediateNoGC(const T &t) : t_(t) {
        EnterAssertNoGCScope();
    }
    IntermediateNoGC(const IntermediateNoGC &) {
        EnterAssertNoGCScope();
    }
    ~IntermediateNoGC() {
        LeaveAssertNoGCScope();
    }

    const T &operator->() { return t_; }
    operator const T &() { return t_; }
};
#endif















































template <typename T>
class Return
{
    friend class Rooted<T>;

    const T ptr_;

  public:
    template <typename S>
    Return(const S &ptr,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0)
      : ptr_(ptr)
    {}

    Return(NullPtr) : ptr_(NULL) {}

    









    const T &get(AutoAssertNoGC &) const {
        return ptr_;
    }

    





















#ifdef DEBUG
    IntermediateNoGC<T> operator->() const {
        return IntermediateNoGC<T>(ptr_);
    }
#else
    const T &operator->() const {
        return ptr_;
    }
#endif

    





#ifdef DEBUG
    IntermediateNoGC<T> unsafeGet() const {
        return IntermediateNoGC<T>(ptr_);
    }
#else
    const T &unsafeGet() const {
        return ptr_;
    }
#endif

    









    bool operator==(const T &other) { return ptr_ == other; }
    bool operator!=(const T &other) { return ptr_ != other; }
    bool operator==(const Return<T> &other) { return ptr_ == other.ptr_; }
    bool operator==(const JS::Handle<T> &other) { return ptr_ == other.get(); }
    inline bool operator==(const Rooted<T> &other);
};






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
    Rooted(JSRuntime *rt
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(RootMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }

    Rooted(JSRuntime *rt, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }

    Rooted(JSContext *cx
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(RootMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    Rooted(JSContext *cx, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    template <typename S>
    Rooted(JSContext *cx, const Return<S> &initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial.ptr_)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

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

    template <typename S>
    T & operator =(const Return<S> &value)
    {
        ptr = value.ptr_;
        return ptr;
    }

  private:
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    Rooted<T> **stack, *prev;
#endif
    T ptr;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    Rooted(const Rooted &) MOZ_DELETE;
};

#if !(defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING))

template <>
class Rooted<JSStableString *>;
#endif

template <typename T>
bool
Return<T>::operator==(const Rooted<T> &other)
{
    return ptr_ == other.get();
}

typedef Rooted<JSObject*>    RootedObject;
typedef Rooted<JSFunction*>  RootedFunction;
typedef Rooted<JSScript*>    RootedScript;
typedef Rooted<JSString*>    RootedString;
typedef Rooted<jsid>         RootedId;
typedef Rooted<Value>        RootedValue;







class SkipRoot
{
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)

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

} 

namespace JS {

template<typename T> template <typename S>
inline
Handle<T>::Handle(js::Rooted<S> &root,
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
MutableHandle<T>::MutableHandle(js::Rooted<S> *root,
                                typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
{
    ptr = root->address();
}






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




#ifdef DEBUG
JS_ALWAYS_INLINE void
AssertCanGC()
{
    JS_ASSERT(!InNoGCScope());
}
#else
# define AssertCanGC()
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
extern void
CheckStackRoots(JSContext *cx);
#endif

JS_FRIEND_API(bool) NeedRelaxedRootChecks();

} 

namespace js {





inline void MaybeCheckStackRoots(JSContext *cx, bool relax = true)
{
    AssertCanGC();
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    if (relax && NeedRelaxedRootChecks())
        return;
    CheckStackRoots(cx);
#endif
}

namespace gc {
struct Cell;
} 


class CompilerRootNode
{
  protected:
    CompilerRootNode(js::gc::Cell *ptr)
      : next(NULL), ptr(ptr)
    { }

  public:
    void **address() { return (void **)&ptr; }

  public:
    CompilerRootNode *next;

  protected:
    js::gc::Cell *ptr;
};

}  

#endif  

#endif  
