






#ifndef jsgc_root_h__
#define jsgc_root_h__

#include "mozilla/GuardObjects.h"
#include "mozilla/TypeTraits.h"

#include "js/Utility.h"
#include "js/TemplateLib.h"

#include "jspubtd.h"




















































































































namespace js {

template <typename T> class Rooted;
template <typename T> class Unrooted;

template <typename T>
struct RootMethods {};

template <typename T>
class RootedBase {};

template <typename T>
class HandleBase {};

template <typename T>
class MutableHandleBase {};

} 

namespace JS {

class AutoAssertNoGC;

template <typename T> class Handle;
template <typename T> class MutableHandle;

JS_FRIEND_API(void) EnterAssertNoGCScope();
JS_FRIEND_API(void) LeaveAssertNoGCScope();


JS_FRIEND_API(bool) InNoGCScope();
JS_FRIEND_API(bool) isGCEnabled();








struct NullPtr
{
    static void * const constNullValue;
};









template <typename T>
class Handle : public js::HandleBase<T>
{
    friend class MutableHandle<T>;

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

    operator T() const { return get(); }
    T operator->() const { return get(); }

    bool operator!=(const T &other) { return *ptr != other; }

  private:
    Handle() {}

    const T *ptr;

    template <typename S>
    void operator=(S v) MOZ_DELETE;
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
    inline MutableHandle(js::Rooted<T> *root);

    void set(T v) {
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

    operator T() const { return get(); }
    T operator->() const { return get(); }

  private:
    MutableHandle() {}

    T *ptr;

    template <typename S>
    void operator=(S v) MOZ_DELETE;
};

typedef MutableHandle<JSObject*>   MutableHandleObject;
typedef MutableHandle<JSFunction*> MutableHandleFunction;
typedef MutableHandle<JSScript*>   MutableHandleScript;
typedef MutableHandle<JSString*>   MutableHandleString;
typedef MutableHandle<jsid>        MutableHandleId;
typedef MutableHandle<Value>       MutableHandleValue;

} 

namespace js {





typedef JSObject *                  RawObject;
typedef JSString *                  RawString;
typedef jsid                        RawId;
typedef JS::Value                   RawValue;







template <typename T>
class InternalHandle {};

template <typename T>
class InternalHandle<T*>
{
    void * const *holder;
    size_t offset;

  public:
    



    template<typename H>
    InternalHandle(const JS::Handle<H> &handle, T *field)
      : holder((void**)handle.address()), offset(uintptr_t(field) - uintptr_t(handle.get()))
    {}

    


    template<typename R>
    InternalHandle(const Rooted<R> &root, T *field)
      : holder((void**)root.address()), offset(uintptr_t(field) - uintptr_t(root.get()))
    {}

    T *get() const { return reinterpret_cast<T*>(uintptr_t(*holder) + offset); }

    const T &operator*() const { return *get(); }
    T *operator->() const { return get(); }

    static InternalHandle<T*> fromMarkedLocation(T *fieldPtr) {
        return InternalHandle(fieldPtr);
    }

  private:
    








    InternalHandle(T *field)
      : holder(reinterpret_cast<void * const *>(&JS::NullPtr::constNullValue)),
        offset(uintptr_t(field))
    {}
};

#ifdef DEBUG




template <typename T>
class Unrooted
{
  public:
    Unrooted() : ptr_(UninitializedTag()) {}

    








    template <typename S>
    inline Unrooted(const Rooted<S> &root,
                    typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    template <typename S>
    Unrooted(const JS::Handle<S> &root,
             typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0)
      : ptr_(root.get())
    {
        JS_ASSERT(ptr_ != UninitializedTag());
        JS::EnterAssertNoGCScope();
    }

    






    template <typename S>
    Unrooted(const Unrooted<S> &other)
        
      : ptr_(static_cast<T>(static_cast<S>(other)))
    {
        if (ptr_ != UninitializedTag())
            JS::EnterAssertNoGCScope();
    }

    Unrooted(const Unrooted &other) : ptr_(other.ptr_) {
        if (ptr_ != UninitializedTag())
            JS::EnterAssertNoGCScope();
    }

    Unrooted(const T &p) : ptr_(p) {
        JS_ASSERT(ptr_ != UninitializedTag());
        JS::EnterAssertNoGCScope();
    }

    Unrooted(const JS::NullPtr &) : ptr_(NULL) {
        JS::EnterAssertNoGCScope();
    }

    ~Unrooted() {
        if (ptr_ != UninitializedTag())
            JS::LeaveAssertNoGCScope();
    }

    void drop() {
        if (ptr_ != UninitializedTag())
            JS::LeaveAssertNoGCScope();
        ptr_ = UninitializedTag();
    }

    
    Unrooted &operator=(T other) {
        JS_ASSERT(other != UninitializedTag());
        if (ptr_ == UninitializedTag())
            JS::EnterAssertNoGCScope();
        ptr_ = other;
        return *this;
    }
    Unrooted &operator=(Unrooted other) {
        JS_ASSERT(other.ptr_ != UninitializedTag());
        if (ptr_ == UninitializedTag())
            JS::EnterAssertNoGCScope();
        ptr_ = other.ptr_;
        return *this;
    }

    operator T() const { return (ptr_ == UninitializedTag()) ? NULL : ptr_; }
    T *operator&() { return &ptr_; }
    const T operator->() const { JS_ASSERT(ptr_ != UninitializedTag()); return ptr_; }
    bool operator==(const T &other) { return ptr_ == other; }
    bool operator!=(const T &other) { return ptr_ != other; }

  private:
    










    static inline T UninitializedTag() { return reinterpret_cast<T>(2); };

    T ptr_;
};





# define ForwardDeclare(type)                        \
    class type;                                      \
    typedef Unrooted<type*> Unrooted##type;          \
    typedef type * Raw##type

# define ForwardDeclareJS(type)                      \
    class JS##type;                                  \
    namespace js {                                   \
        typedef js::Unrooted<JS##type*> Unrooted##type; \
        typedef JS##type * Raw##type;                \
    }                                                \
    class JS##type

template <typename T>
T DropUnrooted(Unrooted<T> &unrooted)
{
    T rv = unrooted;
    unrooted.drop();
    return rv;
}

template <typename T>
T DropUnrooted(T &unrooted)
{
    T rv = unrooted;
    JS::PoisonPtr(&unrooted);
    return rv;
}

template <>
inline RawId DropUnrooted(RawId &id) { return id; }

#else 


# define ForwardDeclare(type)        \
    class type;                      \
    typedef type * Unrooted##type;   \
    typedef type * Raw##type

# define ForwardDeclareJS(type)                                               \
    class JS##type;                                                           \
    namespace js {                                                            \
        typedef JS##type * Unrooted##type;                                    \
        typedef JS##type * Raw##type;                                         \
    }                                                                         \
    class JS##type

template <typename T>
class Unrooted
{
  private:
    Unrooted() MOZ_DELETE;
    Unrooted(const Unrooted &) MOZ_DELETE;
    ~Unrooted() MOZ_DELETE;
};

template <typename T>
T DropUnrooted(T &unrooted) { return unrooted; }

#endif 






template <typename T>
struct RootKind<T *>
{
    static ThingRootKind rootKind() { return T::rootKind(); }
};

template <typename T>
struct RootMethods<T *>
{
    static T *initial() { return NULL; }
    static ThingRootKind kind() { return RootKind<T *>::rootKind(); }
    static bool poisoned(T *v) { return IsPoisonedPtr(v); }
};









template <typename T>
class Rooted : public RootedBase<T>
{
    void init(JSContext *cxArg) {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        ContextFriendFields *cx = ContextFriendFields::get(cxArg);
        commonInit(cx->thingGCRooters);
#endif
    }

    void init(JSRuntime *rtArg) {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        PerThreadDataFriendFields *pt = PerThreadDataFriendFields::getMainThread(rtArg);
        commonInit(pt->thingGCRooters);
#endif
    }

    void init(js::PerThreadData *ptArg) {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        PerThreadDataFriendFields *pt = PerThreadDataFriendFields::get(ptArg);
        commonInit(pt->thingGCRooters);
#endif
    }

  public:
    Rooted(JSRuntime *rt
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(RootMethods<T>::initial())
#if defined(JSGC_ROOT_ANALYSIS)
      , scanned(false)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }

    Rooted(JSRuntime *rt, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
#if defined(JSGC_ROOT_ANALYSIS)
      , scanned(false)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }

    Rooted(JSContext *cx
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(RootMethods<T>::initial())
#if defined(JSGC_ROOT_ANALYSIS)
      , scanned(false)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    Rooted(JSContext *cx, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
#if defined(JSGC_ROOT_ANALYSIS)
      , scanned(false)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    Rooted(js::PerThreadData *pt
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(RootMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(pt);
    }

    Rooted(js::PerThreadData *pt, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(pt);
    }

    template <typename S>
    Rooted(JSContext *cx, const Unrooted<S> &initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(static_cast<S>(initial))
#if defined(JSGC_ROOT_ANALYSIS)
      , scanned(false)
#endif
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    ~Rooted() {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        JS_ASSERT(*stack == this);
        *stack = prev;
#endif
    }

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    Rooted<T> *previous() { return prev; }
#endif

    operator T() const { return ptr; }
    T operator->() const { return ptr; }
    T *address() { return &ptr; }
    const T *address() const { return &ptr; }
    T &get() { return ptr; }
    const T &get() const { return ptr; }

    T &operator=(T value) {
        JS_ASSERT(!RootMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    T &operator=(const Rooted &value) {
        ptr = value;
        return ptr;
    }

  private:
    void commonInit(Rooted<void*> **thingGCRooters) {
#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
        ThingRootKind kind = RootMethods<T>::kind();
        this->stack = reinterpret_cast<Rooted<T>**>(&thingGCRooters[kind]);
        this->prev = *stack;
        *stack = this;

        JS_ASSERT(!RootMethods<T>::poisoned(ptr));
#endif
    }

#if defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING)
    Rooted<T> **stack, *prev;
#endif
    T ptr;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    Rooted(const Rooted &) MOZ_DELETE;

#if defined(JSGC_ROOT_ANALYSIS)
  public:
    
    bool scanned;
#endif
};

#if !(defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING))

template <>
class Rooted<JSStableString *>;
#endif

#ifdef DEBUG
template <typename T> template <typename S>
inline
Unrooted<T>::Unrooted(const Rooted<S> &root,
                      typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
  : ptr_(root.get())
{
    JS_ASSERT(ptr_ != UninitializedTag());
    JS::EnterAssertNoGCScope();
}
#endif 

typedef Rooted<JSObject*>    RootedObject;
typedef Rooted<JSFunction*>  RootedFunction;
typedef Rooted<JSScript*>    RootedScript;
typedef Rooted<JSString*>    RootedString;
typedef Rooted<jsid>         RootedId;
typedef Rooted<JS::Value>    RootedValue;







class SkipRoot
{
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)

    SkipRoot **stack, *prev;
    const uint8_t *start;
    const uint8_t *end;

    template <typename T>
    void init(ContextFriendFields *cx, const T *ptr, size_t count) {
        this->stack = &cx->skipGCRooters;
        this->prev = *stack;
        *stack = this;
        this->start = (const uint8_t *) ptr;
        this->end = this->start + (sizeof(T) * count);
    }

  public:
    template <typename T>
    SkipRoot(JSContext *cx, const T *ptr, size_t count = 1
             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        init(ContextFriendFields::get(cx), ptr, count);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~SkipRoot() {
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
             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

#endif 

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

namespace JS {

template <typename T> template <typename S>
inline
Handle<T>::Handle(js::Rooted<S> &root,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
{
    ptr = reinterpret_cast<const T *>(root.address());
}

template <typename T> template <typename S>
inline
Handle<T>::Handle(MutableHandle<S> &root,
                  typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy)
{
    ptr = reinterpret_cast<const T *>(root.address());
}

template <typename T>
inline
MutableHandle<T>::MutableHandle(js::Rooted<T> *root)
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




JS_ALWAYS_INLINE void
AssertCanGC()
{
    JS_ASSERT_IF(isGCEnabled(), !InNoGCScope());
}

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
extern void
CheckStackRoots(JSContext *cx);
#endif

JS_FRIEND_API(bool) NeedRelaxedRootChecks();

} 

namespace js {





inline void MaybeCheckStackRoots(JSContext *cx, bool relax = true)
{
    JS::AssertCanGC();
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
    CompilerRootNode(js::gc::Cell *ptr) : next(NULL), ptr(ptr) {}

  public:
    void **address() { return (void **)&ptr; }

  public:
    CompilerRootNode *next;

  protected:
    js::gc::Cell *ptr;
};

}  

ForwardDeclareJS(Script);
ForwardDeclareJS(Function);

#endif  
