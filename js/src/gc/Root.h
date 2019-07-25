






































#ifndef jsgc_root_h__
#define jsgc_root_h__

#include "jspubtd.h"

#include "js/Utility.h"

#ifdef __cplusplus

namespace JS {
















































template <typename T> class Root;
template <typename T> class RootedVar;

template <typename T>
struct RootMethods { };





template <typename T>
class Handle
{
  public:
    
    template <typename S> Handle(Handle<S> handle) {
        testAssign<S>();
        ptr = reinterpret_cast<const T *>(handle.address());
    }

    
    template <typename S> inline Handle(const Root<S> &root);
    template <typename S> inline Handle(const RootedVar<S> &root);

    const T *address() { return ptr; }

    operator T () { return value(); }
    T operator ->() { return value(); }

  private:
    const T *ptr;
    T value() { return *ptr; }

    template <typename S>
    void testAssign() {
#ifdef DEBUG
        T a = RootMethods<T>::initial();
        S b = RootMethods<S>::initial();
        a = b;
        (void)a;
#endif
    }
};

typedef Handle<JSObject*>    HandleObject;
typedef Handle<JSFunction*>  HandleFunction;
typedef Handle<JSString*>    HandleString;
typedef Handle<jsid>         HandleId;
typedef Handle<Value>        HandleValue;

template <typename T>
struct RootMethods<T *>
{
    static T *initial() { return NULL; }
    static ThingRootKind kind() { return T::rootKind(); }
    static bool poisoned(T *v) { return IsPoisonedPtr(v); }
};











template <typename T>
class Root
{
  public:
    Root(JSContext *cx_, const T *ptr
         JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
#ifdef JSGC_ROOT_ANALYSIS
        ContextFriendFields *cx = ContextFriendFields::get(cx_);

        ThingRootKind kind = RootMethods<T>::kind();
        this->stack = reinterpret_cast<Root<T>**>(&cx->thingGCRooters[kind]);
        this->prev = *stack;
        *stack = this;
#endif

        JS_ASSERT(!RootMethods<T>::poisoned(*ptr));

        this->ptr = ptr;

        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~Root()
    {
#ifdef JSGC_ROOT_ANALYSIS
        JS_ASSERT(*stack == this);
        *stack = prev;
#endif
    }

#ifdef JSGC_ROOT_ANALYSIS
    Root<T> *previous() { return prev; }
#endif

    const T *address() const { return ptr; }

  private:

#ifdef JSGC_ROOT_ANALYSIS
    Root<T> **stack, *prev;
#endif
    const T *ptr;

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<typename T> template <typename S>
inline
Handle<T>::Handle(const Root<S> &root)
{
    testAssign<S>();
    ptr = reinterpret_cast<const T *>(root.address());
}

typedef Root<JSObject*>    RootObject;
typedef Root<JSFunction*>  RootFunction;
typedef Root<JSString*>    RootString;
typedef Root<jsid>         RootId;
typedef Root<Value>        RootValue;







class SkipRoot
{
#if defined(DEBUG) && defined(JSGC_ROOT_ANALYSIS)

    SkipRoot **stack, *prev;
    const uint8_t *start;
    const uint8_t *end;

  public:
    template <typename T>
    SkipRoot(JSContext *cx_, const T *ptr
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        ContextFriendFields *cx = ContextFriendFields::get(cx_);

        this->stack = &cx->skipGCRooters;
        this->prev = *stack;
        *stack = this;
        this->start = (const uint8_t *) ptr;
        this->end = this->start + sizeof(T);
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
    SkipRoot(JSContext *cx, const T *ptr
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

#endif 

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};


template <typename T>
class RootedVar
{
  public:
    RootedVar(JSContext *cx)
      : ptr(RootMethods<T>::initial()), root(cx, &ptr)
    {}

    RootedVar(JSContext *cx, T initial)
      : ptr(initial), root(cx, &ptr)
    {}

    operator T () const { return ptr; }
    T operator ->() const { return ptr; }
    T * address() { return &ptr; }
    const T * address() const { return &ptr; }
    T & reference() { return ptr; }
    T raw() { return ptr; }

    






    operator Handle<T> () const { return Handle<T>(*this); }

    T & operator =(T value)
    {
        JS_ASSERT(!RootMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

    T & operator =(const RootedVar &value)
    {
        ptr = value;
        return ptr;
    }

  private:
    T ptr;
    Root<T> root;

    RootedVar() MOZ_DELETE;
    RootedVar(const RootedVar &) MOZ_DELETE;
};

template <typename T> template <typename S>
inline
Handle<T>::Handle(const RootedVar<S> &root)
{
    testAssign<S>();
    ptr = reinterpret_cast<const T *>(root.address());
}

typedef RootedVar<JSObject*>    RootedVarObject;
typedef RootedVar<JSFunction*>  RootedVarFunction;
typedef RootedVar<JSString*>    RootedVarString;
typedef RootedVar<jsid>         RootedVarId;
typedef RootedVar<Value>        RootedVarValue;

}  

#endif  

#endif  
