





#ifndef js_RootingAPI_h
#define js_RootingAPI_h

#include "mozilla/GuardObjects.h"
#include "mozilla/TypeTraits.h"

#include "js/Utility.h"
#include "js/TemplateLib.h"

#include "jspubtd.h"


















































































namespace js {

class Module;
class ScriptSourceObject;

template <typename T>
struct GCMethods {};

template <typename T>
class RootedBase {};

template <typename T>
class HandleBase {};

template <typename T>
class MutableHandleBase {};

template <typename T>
class HeapBase {};














struct NullPtr
{
    static void * const constNullValue;
};

namespace gc {
struct Cell;
} 

} 

namespace JS {

template <typename T> class Rooted;

template <typename T> class Handle;
template <typename T> class MutableHandle;


JS_FRIEND_API(bool) isGCEnabled();

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
extern void
CheckStackRoots(JSContext *cx);
#endif










struct JS_PUBLIC_API(NullPtr)
{
    static void * const constNullValue;
};


















template <typename T>
class Heap : public js::HeapBase<T>
{
  public:
    Heap() {
        MOZ_STATIC_ASSERT(sizeof(T) == sizeof(Heap<T>),
                          "Heap<T> must be binary compatible with T.");
        init(js::GCMethods<T>::initial());
    }
    explicit Heap(T p) { init(p); }
    explicit Heap(const Heap<T> &p) { init(p.ptr); }

    ~Heap() {
        if (js::GCMethods<T>::needsPostBarrier(ptr))
            relocate();
    }

    bool operator!=(const T &other) const { return ptr != other; }
    bool operator==(const T &other) const { return ptr == other; }

    operator T() const { return ptr; }
    T operator->() const { return ptr; }
    const T *address() const { return &ptr; }
    const T &get() const { return ptr; }

    T *unsafeGet() { return &ptr; }

    Heap<T> &operator=(T p) {
        set(p);
        return *this;
    }

    void set(T newPtr) {
        JS_ASSERT(!js::GCMethods<T>::poisoned(newPtr));
        if (js::GCMethods<T>::needsPostBarrier(newPtr)) {
            ptr = newPtr;
            post();
        } else if (js::GCMethods<T>::needsPostBarrier(ptr)) {
            relocate();  
            ptr = newPtr;
        } else {
            ptr = newPtr;
        }
    }

  private:
    void init(T newPtr) {
        JS_ASSERT(!js::GCMethods<T>::poisoned(newPtr));
        ptr = newPtr;
        if (js::GCMethods<T>::needsPostBarrier(ptr))
            post();
    }

    void post() {
#ifdef JSGC_GENERATIONAL
        JS_ASSERT(js::GCMethods<T>::needsPostBarrier(ptr));
        js::GCMethods<T>::postBarrier(&ptr);
#endif
    }

    void relocate() {
#ifdef JSGC_GENERATIONAL
        js::GCMethods<T>::relocate(&ptr);
#endif
    }

    T ptr;
};









template <typename T>
class MOZ_NONHEAP_CLASS Handle : public js::HandleBase<T>
{
    friend class MutableHandle<T>;

  public:
    
    template <typename S>
    Handle(Handle<S> handle,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0)
    {
        MOZ_STATIC_ASSERT(sizeof(Handle<T>) == sizeof(T *),
                          "Handle must be binary compatible with T*.");
        ptr = reinterpret_cast<const T *>(handle.address());
    }

    
    Handle(js::NullPtr) {
        MOZ_STATIC_ASSERT(mozilla::IsPointer<T>::value,
                          "js::NullPtr overload not valid for non-pointer types");
        ptr = reinterpret_cast<const T *>(&js::NullPtr::constNullValue);
    }

    
    Handle(JS::NullPtr) {
        MOZ_STATIC_ASSERT(mozilla::IsPointer<T>::value,
                          "JS::NullPtr overload not valid for non-pointer types");
        ptr = reinterpret_cast<const T *>(&JS::NullPtr::constNullValue);
    }

    Handle(MutableHandle<T> handle) {
        ptr = handle.address();
    }

    Handle(const Heap<T> &heapPtr) {
        ptr = heapPtr.address();
    }

    






    static Handle fromMarkedLocation(const T *p) {
        Handle h;
        h.ptr = p;
        return h;
    }

    



    template <typename S>
    inline
    Handle(const Rooted<S> &root,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    
    template <typename S>
    inline
    Handle(MutableHandle<S> &root,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    const T *address() const { return ptr; }
    const T& get() const { return *ptr; }

    



    operator const T&() const { return get(); }
    T operator->() const { return get(); }

    bool operator!=(const T &other) const { return *ptr != other; }
    bool operator==(const T &other) const { return *ptr == other; }

  private:
    Handle() {}

    const T *ptr;

    template <typename S>
    void operator=(S v) MOZ_DELETE;
};

typedef Handle<JSObject*>                   HandleObject;
typedef Handle<js::Module*>                 HandleModule;
typedef Handle<js::ScriptSourceObject *>    HandleScriptSource;
typedef Handle<JSFunction*>                 HandleFunction;
typedef Handle<JSScript*>                   HandleScript;
typedef Handle<JSString*>                   HandleString;
typedef Handle<jsid>                        HandleId;
typedef Handle<Value>                       HandleValue;









template <typename T>
class MOZ_STACK_CLASS MutableHandle : public js::MutableHandleBase<T>
{
  public:
    inline MutableHandle(Rooted<T> *root);

    void set(T v) {
        JS_ASSERT(!js::GCMethods<T>::poisoned(v));
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

#ifdef JSGC_GENERATIONAL
JS_PUBLIC_API(void) HeapCellPostBarrier(js::gc::Cell **cellp);
JS_PUBLIC_API(void) HeapCellRelocate(js::gc::Cell **cellp);
#endif

} 

namespace js {







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
    InternalHandle(const JS::Rooted<R> &root, T *field)
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
      : holder(reinterpret_cast<void * const *>(&js::NullPtr::constNullValue)),
        offset(uintptr_t(field))
    {}
};






template <typename T>
struct RootKind<T *>
{
    static ThingRootKind rootKind() { return T::rootKind(); }
};

template <typename T>
struct GCMethods<T *>
{
    static T *initial() { return NULL; }
    static ThingRootKind kind() { return RootKind<T *>::rootKind(); }
    static bool poisoned(T *v) { return JS::IsPoisonedPtr(v); }
    static bool needsPostBarrier(T *v) { return v; }
#ifdef JSGC_GENERATIONAL
    static void postBarrier(T **vp) {
        JS::HeapCellPostBarrier(reinterpret_cast<js::gc::Cell **>(vp));
    }
    static void relocate(T **vp) {
        JS::HeapCellRelocate(reinterpret_cast<js::gc::Cell **>(vp));
    }
#endif
};

#if defined(DEBUG) && defined(JS_THREADSAFE)

extern JS_PUBLIC_API(bool)
IsInRequest(JSContext *cx);
#endif

} 

namespace JS {









template <typename T>
class MOZ_STACK_CLASS Rooted : public js::RootedBase<T>
{
    void init(JSContext *cxArg) {
        MOZ_ASSERT(cxArg);
#ifdef JS_THREADSAFE
        MOZ_ASSERT(js::IsInRequest(cxArg));
#endif
#ifdef JSGC_TRACK_EXACT_ROOTS
        js::ContextFriendFields *cx = js::ContextFriendFields::get(cxArg);
        commonInit(cx->thingGCRooters);
#endif
    }

    void init(js::PerThreadDataFriendFields *pt) {
        MOZ_ASSERT(pt);
#ifdef JSGC_TRACK_EXACT_ROOTS
        commonInit(pt->thingGCRooters);
#endif
    }

  public:
    Rooted(JSContext *cx
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(js::GCMethods<T>::initial())
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

    Rooted(js::PerThreadData *pt
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(js::GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(js::PerThreadDataFriendFields::get(pt));
    }

    Rooted(js::PerThreadData *pt, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(js::PerThreadDataFriendFields::get(pt));
    }

    Rooted(JSRuntime *rt
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(js::GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(js::PerThreadDataFriendFields::getMainThread(rt));
    }

    Rooted(JSRuntime *rt, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(js::PerThreadDataFriendFields::getMainThread(rt));
    }

    ~Rooted() {
#ifdef JSGC_TRACK_EXACT_ROOTS
        JS_ASSERT(*stack == reinterpret_cast<Rooted<void*>*>(this));
        *stack = prev;
#endif
    }

#ifdef JSGC_TRACK_EXACT_ROOTS
    Rooted<T> *previous() { return prev; }
#endif

    



    operator const T&() const { return ptr; }
    T operator->() const { return ptr; }
    T *address() { return &ptr; }
    const T *address() const { return &ptr; }
    T &get() { return ptr; }
    const T &get() const { return ptr; }

    T &operator=(T value) {
        JS_ASSERT(!js::GCMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    T &operator=(const Rooted &value) {
        ptr = value;
        return ptr;
    }

    void set(T value) {
        JS_ASSERT(!js::GCMethods<T>::poisoned(value));
        ptr = value;
    }

    bool operator!=(const T &other) const { return ptr != other; }
    bool operator==(const T &other) const { return ptr == other; }

  private:
    void commonInit(Rooted<void*> **thingGCRooters) {
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
        scanned = false;
#endif
#ifdef JSGC_TRACK_EXACT_ROOTS
        js::ThingRootKind kind = js::GCMethods<T>::kind();
        this->stack = &thingGCRooters[kind];
        this->prev = *stack;
        *stack = reinterpret_cast<Rooted<void*>*>(this);

        JS_ASSERT(!js::GCMethods<T>::poisoned(ptr));
#endif
    }

#ifdef JSGC_TRACK_EXACT_ROOTS
    Rooted<void*> **stack, *prev;
#endif

#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    
    friend void JS::CheckStackRoots(JSContext*);
#endif

#ifdef JSGC_ROOT_ANALYSIS
    bool scanned;
#endif

    



    T ptr;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    Rooted(const Rooted &) MOZ_DELETE;
};

#if !(defined(JSGC_ROOT_ANALYSIS) || defined(JSGC_USE_EXACT_ROOTING))

template <>
class Rooted<JSStableString *>;
#endif

typedef Rooted<JSObject*>                   RootedObject;
typedef Rooted<js::Module*>                 RootedModule;
typedef Rooted<js::ScriptSourceObject *>    RootedScriptSource;
typedef Rooted<JSFunction*>                 RootedFunction;
typedef Rooted<JSScript*>                   RootedScript;
typedef Rooted<JSString*>                   RootedString;
typedef Rooted<jsid>                        RootedId;
typedef Rooted<JS::Value>                   RootedValue;

} 

namespace js {







class SkipRoot
{
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)

    SkipRoot **stack, *prev;
    const uint8_t *start;
    const uint8_t *end;

    template <typename T>
    void init(SkipRoot **head, const T *ptr, size_t count) {
        this->stack = head;
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
        init(&ContextFriendFields::get(cx)->skipGCRooters, ptr, count);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    template <typename T>
    SkipRoot(js::PerThreadData *ptd, const T *ptr, size_t count = 1
             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        PerThreadDataFriendFields *ptff = PerThreadDataFriendFields::get(ptd);
        init(&ptff->skipGCRooters, ptr, count);
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

    template <typename T>
    SkipRoot(PerThreadData *ptd, const T *ptr, size_t count = 1
             MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

#endif 

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


template <typename T>
class FakeRooted : public RootedBase<T>
{
  public:
    FakeRooted(JSContext *cx
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    FakeRooted(JSContext *cx, T initial
                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    operator T() const { return ptr; }
    T operator->() const { return ptr; }
    T *address() { return &ptr; }
    const T *address() const { return &ptr; }
    T &get() { return ptr; }
    const T &get() const { return ptr; }

    T &operator=(T value) {
        JS_ASSERT(!GCMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    bool operator!=(const T &other) const { return ptr != other; }
    bool operator==(const T &other) const { return ptr == other; }

  private:
    T ptr;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    FakeRooted(const FakeRooted &) MOZ_DELETE;
};


template <typename T>
class FakeMutableHandle : public js::MutableHandleBase<T>
{
  public:
    FakeMutableHandle(T *t) {
        ptr = t;
    }

    FakeMutableHandle(FakeRooted<T> *root) {
        ptr = root->address();
    }

    void set(T v) {
        JS_ASSERT(!js::GCMethods<T>::poisoned(v));
        *ptr = v;
    }

    T *address() const { return ptr; }
    T get() const { return *ptr; }

    operator T() const { return get(); }
    T operator->() const { return get(); }

  private:
    FakeMutableHandle() {}

    T *ptr;

    template <typename S>
    void operator=(S v) MOZ_DELETE;
};










enum AllowGC {
    NoGC = 0,
    CanGC = 1
};
template <typename T, AllowGC allowGC>
class MaybeRooted
{
};

template <typename T> class MaybeRooted<T, CanGC>
{
  public:
    typedef JS::Handle<T> HandleType;
    typedef JS::Rooted<T> RootType;
    typedef JS::MutableHandle<T> MutableHandleType;

    static inline JS::Handle<T> toHandle(HandleType v) {
        return v;
    }

    static inline JS::MutableHandle<T> toMutableHandle(MutableHandleType v) {
        return v;
    }
};

template <typename T> class MaybeRooted<T, NoGC>
{
  public:
    typedef T HandleType;
    typedef FakeRooted<T> RootType;
    typedef FakeMutableHandle<T> MutableHandleType;

    static inline JS::Handle<T> toHandle(HandleType v) {
        MOZ_ASSUME_NOT_REACHED("Bad conversion");
        return JS::Handle<T>::fromMarkedLocation(NULL);
    }

    static inline JS::MutableHandle<T> toMutableHandle(MutableHandleType v) {
        MOZ_ASSUME_NOT_REACHED("Bad conversion");
        return JS::MutableHandle<T>::fromMarkedLocation(NULL);
    }
};

} 

namespace JS {

template <typename T> template <typename S>
inline
Handle<T>::Handle(const Rooted<S> &root,
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
MutableHandle<T>::MutableHandle(Rooted<T> *root)
{
    MOZ_STATIC_ASSERT(sizeof(MutableHandle<T>) == sizeof(T *),
                      "MutableHandle must be binary compatible with T*.");
    ptr = root->address();
}

} 

namespace js {





inline void MaybeCheckStackRoots(JSContext *cx)
{
#if defined(DEBUG) && defined(JS_GC_ZEAL) && defined(JSGC_ROOT_ANALYSIS) && !defined(JS_THREADSAFE)
    JS::CheckStackRoots(cx);
#endif
}


class CompilerRootNode
{
  protected:
    CompilerRootNode(js::gc::Cell *ptr) : next(NULL), ptr_(ptr) {}

  public:
    void **address() { return (void **)&ptr_; }

  public:
    CompilerRootNode *next;

  protected:
    js::gc::Cell *ptr_;
};

}  

#endif  
