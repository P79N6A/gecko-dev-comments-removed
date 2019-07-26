





#ifndef js_RootingAPI_h
#define js_RootingAPI_h

#include "mozilla/Attributes.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/LinkedList.h"
#include "mozilla/NullPtr.h"
#include "mozilla/TypeTraits.h"

#include "jspubtd.h"

#include "js/GCAPI.h"
#include "js/HeapAPI.h"
#include "js/TypeDecls.h"
#include "js/Utility.h"


















































































namespace js {

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
template<typename T>
struct PersistentRootedMarker;
} 

} 

namespace JS {

template <typename T> class Rooted;
template <typename T> class PersistentRooted;


JS_FRIEND_API(bool) isGCEnabled();










struct JS_PUBLIC_API(NullPtr)
{
    static void * const constNullValue;
};




















template <typename T>
class Heap : public js::HeapBase<T>
{
  public:
    Heap() {
        static_assert(sizeof(T) == sizeof(Heap<T>),
                      "Heap<T> must be binary compatible with T.");
        init(js::GCMethods<T>::initial());
    }
    explicit Heap(T p) { init(p); }

    





    explicit Heap(const Heap<T> &p) { init(p.ptr); }

    ~Heap() {
        if (js::GCMethods<T>::needsPostBarrier(ptr))
            relocate();
    }

    bool operator==(const Heap<T> &other) { return ptr == other.ptr; }
    bool operator!=(const Heap<T> &other) { return ptr != other.ptr; }

    bool operator==(const T &other) const { return ptr == other; }
    bool operator!=(const T &other) const { return ptr != other; }

    operator T() const { return ptr; }
    T operator->() const { return ptr; }
    const T *address() const { return &ptr; }
    const T &get() const { return ptr; }

    T *unsafeGet() { return &ptr; }

    Heap<T> &operator=(T p) {
        set(p);
        return *this;
    }

    Heap<T> &operator=(const Heap<T>& other) {
        set(other.get());
        return *this;
    }

    void set(T newPtr) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(newPtr));
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

    



    void setToCrashOnTouch() {
        ptr = reinterpret_cast<T>(crashOnTouchPointer);
    }

    bool isSetToCrashOnTouch() {
        return ptr == crashOnTouchPointer;
    }

  private:
    void init(T newPtr) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(newPtr));
        ptr = newPtr;
        if (js::GCMethods<T>::needsPostBarrier(ptr))
            post();
    }

    void post() {
#ifdef JSGC_GENERATIONAL
        MOZ_ASSERT(js::GCMethods<T>::needsPostBarrier(ptr));
        js::GCMethods<T>::postBarrier(&ptr);
#endif
    }

    void relocate() {
#ifdef JSGC_GENERATIONAL
        js::GCMethods<T>::relocate(&ptr);
#endif
    }

    enum {
        crashOnTouchPointer = 1
    };

    T ptr;
};

#ifdef JS_DEBUG




extern JS_FRIEND_API(void)
AssertGCThingMustBeTenured(JSObject* obj);
#else
inline void
AssertGCThingMustBeTenured(JSObject *obj) {}
#endif






























template <typename T>
class TenuredHeap : public js::HeapBase<T>
{
  public:
    TenuredHeap() : bits(0) {
        static_assert(sizeof(T) == sizeof(TenuredHeap<T>),
                      "TenuredHeap<T> must be binary compatible with T.");
    }
    explicit TenuredHeap(T p) : bits(0) { setPtr(p); }
    explicit TenuredHeap(const TenuredHeap<T> &p) : bits(0) { setPtr(p.getPtr()); }

    bool operator==(const TenuredHeap<T> &other) { return bits == other.bits; }
    bool operator!=(const TenuredHeap<T> &other) { return bits != other.bits; }

    void setPtr(T newPtr) {
        MOZ_ASSERT((reinterpret_cast<uintptr_t>(newPtr) & flagsMask) == 0);
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(newPtr));
        if (newPtr)
            AssertGCThingMustBeTenured(newPtr);
        bits = (bits & flagsMask) | reinterpret_cast<uintptr_t>(newPtr);
    }

    void setFlags(uintptr_t flagsToSet) {
        MOZ_ASSERT((flagsToSet & ~flagsMask) == 0);
        bits |= flagsToSet;
    }

    void unsetFlags(uintptr_t flagsToUnset) {
        MOZ_ASSERT((flagsToUnset & ~flagsMask) == 0);
        bits &= ~flagsToUnset;
    }

    bool hasFlag(uintptr_t flag) const {
        MOZ_ASSERT((flag & ~flagsMask) == 0);
        return (bits & flag) != 0;
    }

    T getPtr() const { return reinterpret_cast<T>(bits & ~flagsMask); }
    uintptr_t getFlags() const { return bits & flagsMask; }

    operator T() const { return getPtr(); }
    T operator->() const { return getPtr(); }

    TenuredHeap<T> &operator=(T p) {
        setPtr(p);
        return *this;
    }

    TenuredHeap<T> &operator=(const TenuredHeap<T>& other) {
        bits = other.bits;
        return *this;
    }

  private:
    enum {
        maskBits = 3,
        flagsMask = (1 << maskBits) - 1,
    };

    uintptr_t bits;
};









template <typename T>
class MOZ_NONHEAP_CLASS Handle : public js::HandleBase<T>
{
    friend class JS::MutableHandle<T>;

  public:
    
    template <typename S>
    Handle(Handle<S> handle,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0)
    {
        static_assert(sizeof(Handle<T>) == sizeof(T *),
                      "Handle must be binary compatible with T*.");
        ptr = reinterpret_cast<const T *>(handle.address());
    }

    
    MOZ_IMPLICIT Handle(js::NullPtr) {
        static_assert(mozilla::IsPointer<T>::value,
                      "js::NullPtr overload not valid for non-pointer types");
        ptr = reinterpret_cast<const T *>(&js::NullPtr::constNullValue);
    }

    
    MOZ_IMPLICIT Handle(JS::NullPtr) {
        static_assert(mozilla::IsPointer<T>::value,
                      "JS::NullPtr overload not valid for non-pointer types");
        ptr = reinterpret_cast<const T *>(&JS::NullPtr::constNullValue);
    }

    MOZ_IMPLICIT Handle(MutableHandle<T> handle) {
        ptr = handle.address();
    }

    














    static MOZ_CONSTEXPR Handle fromMarkedLocation(const T *p) {
        return Handle(p, DeliberatelyChoosingThisOverload,
                      ImUsingThisOnlyInFromFromMarkedLocation);
    }

    



    template <typename S>
    inline
    Handle(const Rooted<S> &root,
           typename mozilla::EnableIf<mozilla::IsConvertible<S, T>::value, int>::Type dummy = 0);

    template <typename S>
    inline
    Handle(const PersistentRooted<S> &root,
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

    enum Disambiguator { DeliberatelyChoosingThisOverload = 42 };
    enum CallerIdentity { ImUsingThisOnlyInFromFromMarkedLocation = 17 };
    MOZ_CONSTEXPR Handle(const T *p, Disambiguator, CallerIdentity) : ptr(p) {}

    const T *ptr;

    template <typename S> void operator=(S) MOZ_DELETE;
    void operator=(Handle) MOZ_DELETE;
};









template <typename T>
class MOZ_STACK_CLASS MutableHandle : public js::MutableHandleBase<T>
{
  public:
    inline MOZ_IMPLICIT MutableHandle(Rooted<T> *root);
    inline MOZ_IMPLICIT MutableHandle(PersistentRooted<T> *root);

  private:
    
    
    template<typename N>
    MutableHandle(N,
                  typename mozilla::EnableIf<mozilla::IsNullPointer<N>::value ||
                                             mozilla::IsSame<N, int>::value ||
                                             mozilla::IsSame<N, long>::value,
                                             int>::Type dummy = 0)
    MOZ_DELETE;

  public:
    void set(T v) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(v));
        *ptr = v;
    }

    






    static MutableHandle fromMarkedLocation(T *p) {
        MutableHandle h;
        h.ptr = p;
        return h;
    }

    T *address() const { return ptr; }
    const T& get() const { return *ptr; }

    



    operator const T&() const { return get(); }
    T operator->() const { return get(); }

  private:
    MutableHandle() {}

    T *ptr;

    template <typename S> void operator=(S v) MOZ_DELETE;
    void operator=(MutableHandle other) MOZ_DELETE;
};

#ifdef JSGC_GENERATIONAL
JS_FRIEND_API(void) HeapCellPostBarrier(js::gc::Cell **cellp);
JS_FRIEND_API(void) HeapCellRelocate(js::gc::Cell **cellp);
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

    InternalHandle(const InternalHandle<T*>& other)
      : holder(other.holder), offset(other.offset) {}

    T *get() const { return reinterpret_cast<T*>(uintptr_t(*holder) + offset); }

    const T &operator*() const { return *get(); }
    T *operator->() const { return get(); }

    static InternalHandle<T*> fromMarkedLocation(T *fieldPtr) {
        return InternalHandle(fieldPtr);
    }

  private:
    








    explicit InternalHandle(T *field)
      : holder(reinterpret_cast<void * const *>(&js::NullPtr::constNullValue)),
        offset(uintptr_t(field))
    {}

    void operator=(InternalHandle<T*> other) MOZ_DELETE;
};






template <typename T>
struct RootKind
{
    static ThingRootKind rootKind() { return T::rootKind(); }
};

template <typename T>
struct RootKind<T *>
{
    static ThingRootKind rootKind() { return T::rootKind(); }
};

template <typename T>
struct GCMethods<T *>
{
    static T *initial() { return nullptr; }
    static bool poisoned(T *v) { return JS::IsPoisonedPtr(v); }
    static bool needsPostBarrier(T *v) { return false; }
#ifdef JSGC_GENERATIONAL
    static void postBarrier(T **vp) {}
    static void relocate(T **vp) {}
#endif
};

template <>
struct GCMethods<JSObject *>
{
    static JSObject *initial() { return nullptr; }
    static bool poisoned(JSObject *v) { return JS::IsPoisonedPtr(v); }
    static bool needsPostBarrier(JSObject *v) {
        return v != nullptr && gc::IsInsideNursery(reinterpret_cast<gc::Cell *>(v));
    }
#ifdef JSGC_GENERATIONAL
    static void postBarrier(JSObject **vp) {
        JS::HeapCellPostBarrier(reinterpret_cast<js::gc::Cell **>(vp));
    }
    static void relocate(JSObject **vp) {
        JS::HeapCellRelocate(reinterpret_cast<js::gc::Cell **>(vp));
    }
#endif
};

#ifdef JS_DEBUG

extern JS_PUBLIC_API(bool)
IsInRequest(JSContext *cx);
#endif

} 

namespace JS {









template <typename T>
class MOZ_STACK_CLASS Rooted : public js::RootedBase<T>
{
    
    template <typename CX>
    void init(CX *cx) {
#ifdef JSGC_TRACK_EXACT_ROOTS
        js::ThingRootKind kind = js::RootKind<T>::rootKind();
        this->stack = &cx->thingGCRooters[kind];
        this->prev = *stack;
        *stack = reinterpret_cast<Rooted<void*>*>(this);

        MOZ_ASSERT(!js::GCMethods<T>::poisoned(ptr));
#endif
    }

  public:
    explicit Rooted(JSContext *cx
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(js::GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_DEBUG
        MOZ_ASSERT(js::IsInRequest(cx));
#endif
        init(js::ContextFriendFields::get(cx));
    }

    Rooted(JSContext *cx, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_DEBUG
        MOZ_ASSERT(js::IsInRequest(cx));
#endif
        init(js::ContextFriendFields::get(cx));
    }

    explicit Rooted(js::ContextFriendFields *cx
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(js::GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    Rooted(js::ContextFriendFields *cx, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx);
    }

    explicit Rooted(js::PerThreadDataFriendFields *pt
                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(js::GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(pt);
    }

    Rooted(js::PerThreadDataFriendFields *pt, T initial
           MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(initial)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(pt);
    }

    explicit Rooted(JSRuntime *rt
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

    
    
    
#ifdef JSGC_TRACK_EXACT_ROOTS
    ~Rooted() {
        MOZ_ASSERT(*stack == reinterpret_cast<Rooted<void*>*>(this));
        *stack = prev;
    }
#endif

#ifdef JSGC_TRACK_EXACT_ROOTS
    Rooted<T> *previous() { return reinterpret_cast<Rooted<T>*>(prev); }
#endif

    



    operator const T&() const { return ptr; }
    T operator->() const { return ptr; }
    T *address() { return &ptr; }
    const T *address() const { return &ptr; }
    T &get() { return ptr; }
    const T &get() const { return ptr; }

    T &operator=(T value) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    T &operator=(const Rooted &value) {
        ptr = value;
        return ptr;
    }

    void set(T value) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(value));
        ptr = value;
    }

    bool operator!=(const T &other) const { return ptr != other; }
    bool operator==(const T &other) const { return ptr == other; }

  private:
#ifdef JSGC_TRACK_EXACT_ROOTS
    




    Rooted<void *> **stack, *prev;
#endif

    



    T ptr;

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

    Rooted(const Rooted &) MOZ_DELETE;
};

} 

namespace js {











template <>
class RootedBase<JSObject*>
{
  public:
    template <class U>
    JS::Handle<U*> as() const;
};


template <typename T>
class FakeRooted : public RootedBase<T>
{
  public:
    template <typename CX>
    FakeRooted(CX *cx
               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : ptr(GCMethods<T>::initial())
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    template <typename CX>
    FakeRooted(CX *cx, T initial
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

    FakeRooted<T> &operator=(T value) {
        MOZ_ASSERT(!GCMethods<T>::poisoned(value));
        ptr = value;
        return *this;
    }

    FakeRooted<T> &operator=(const FakeRooted<T> &other) {
        MOZ_ASSERT(!GCMethods<T>::poisoned(other.ptr));
        ptr = other.ptr;
        return *this;
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
    MOZ_IMPLICIT FakeMutableHandle(T *t) {
        ptr = t;
    }

    MOZ_IMPLICIT FakeMutableHandle(FakeRooted<T> *root) {
        ptr = root->address();
    }

    void set(T v) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(v));
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

    void operator=(const FakeMutableHandle<T>& other) MOZ_DELETE;
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
        MOZ_ASSUME_UNREACHABLE("Bad conversion");
    }

    static inline JS::MutableHandle<T> toMutableHandle(MutableHandleType v) {
        MOZ_ASSUME_UNREACHABLE("Bad conversion");
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
Handle<T>::Handle(const PersistentRooted<S> &root,
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
    static_assert(sizeof(MutableHandle<T>) == sizeof(T *),
                  "MutableHandle must be binary compatible with T*.");
    ptr = root->address();
}

template <typename T>
inline
MutableHandle<T>::MutableHandle(PersistentRooted<T> *root)
{
    static_assert(sizeof(MutableHandle<T>) == sizeof(T *),
                  "MutableHandle must be binary compatible with T*.");
    ptr = root->address();
}

































template<typename T>
class PersistentRooted : private mozilla::LinkedListElement<PersistentRooted<T> > {
    friend class mozilla::LinkedList<PersistentRooted>;
    friend class mozilla::LinkedListElement<PersistentRooted>;

    friend class js::gc::PersistentRootedMarker<T>;

    void registerWithRuntime(JSRuntime *rt) {
        JS::shadow::Runtime *srt = JS::shadow::Runtime::asShadowRuntime(rt);
        srt->getPersistentRootedList<T>().insertBack(this);
    }

  public:
    explicit PersistentRooted(JSContext *cx) : ptr(js::GCMethods<T>::initial())
    {
        registerWithRuntime(js::GetRuntime(cx));
    }

    PersistentRooted(JSContext *cx, T initial) : ptr(initial)
    {
        registerWithRuntime(js::GetRuntime(cx));
    }

    explicit PersistentRooted(JSRuntime *rt) : ptr(js::GCMethods<T>::initial())
    {
        registerWithRuntime(rt);
    }

    PersistentRooted(JSRuntime *rt, T initial) : ptr(initial)
    {
        registerWithRuntime(rt);
    }

    PersistentRooted(PersistentRooted &rhs)
      : mozilla::LinkedListElement<PersistentRooted<T> >(),
        ptr(rhs.ptr)
    {
        




        rhs.setNext(this);
    }

    



    operator const T&() const { return ptr; }
    T operator->() const { return ptr; }
    T *address() { return &ptr; }
    const T *address() const { return &ptr; }
    T &get() { return ptr; }
    const T &get() const { return ptr; }

    T &operator=(T value) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    T &operator=(const PersistentRooted &value) {
        ptr = value;
        return ptr;
    }

    void set(T value) {
        MOZ_ASSERT(!js::GCMethods<T>::poisoned(value));
        ptr = value;
    }

    bool operator!=(const T &other) const { return ptr != other; }
    bool operator==(const T &other) const { return ptr == other; }

  private:
    T ptr;
};

class JS_PUBLIC_API(ObjectPtr)
{
    Heap<JSObject *> value;

  public:
    ObjectPtr() : value(nullptr) {}

    explicit ObjectPtr(JSObject *obj) : value(obj) {}

    
    ~ObjectPtr() { MOZ_ASSERT(!value); }

    void finalize(JSRuntime *rt) {
        if (IsIncrementalBarrierNeeded(rt))
            IncrementalObjectBarrier(value);
        value = nullptr;
    }

    void init(JSObject *obj) { value = obj; }

    JSObject *get() const { return value; }

    void writeBarrierPre(JSRuntime *rt) {
        IncrementalObjectBarrier(value);
    }

    bool isAboutToBeFinalized();

    ObjectPtr &operator=(JSObject *obj) {
        IncrementalObjectBarrier(value);
        value = obj;
        return *this;
    }

    void trace(JSTracer *trc, const char *name);

    JSObject &operator*() const { return *value; }
    JSObject *operator->() const { return value; }
    operator JSObject *() const { return value; }
};

} 

namespace js {


class CompilerRootNode
{
  protected:
    explicit CompilerRootNode(js::gc::Cell *ptr) : next(nullptr), ptr_(ptr) {}

  public:
    void **address() { return (void **)&ptr_; }

  public:
    CompilerRootNode *next;

  protected:
    js::gc::Cell *ptr_;
};

}  

#endif  
