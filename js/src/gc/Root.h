






































#ifndef jsgc_root_h__
#define jsgc_root_h__

#include "jsapi.h"
#include "jsprvtd.h"

namespace js {
















































template <> struct RootMethods<const jsid>
{
    static jsid initial() { return JSID_VOID; }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(jsid id) { return IsPoisonedId(id); }
};

template <> struct RootMethods<jsid>
{
    static jsid initial() { return JSID_VOID; }
    static ThingRootKind kind() { return THING_ROOT_ID; }
    static bool poisoned(jsid id) { return IsPoisonedId(id); }
};

template <> struct RootMethods<const Value>
{
    static Value initial() { return UndefinedValue(); }
    static ThingRootKind kind() { return THING_ROOT_VALUE; }
    static bool poisoned(const Value &v) { return IsPoisonedValue(v); }
};

template <> struct RootMethods<Value>
{
    static Value initial() { return UndefinedValue(); }
    static ThingRootKind kind() { return THING_ROOT_VALUE; }
    static bool poisoned(const Value &v) { return IsPoisonedValue(v); }
};

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
    Root(JSContext *cx, const T *ptr
         JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
#ifdef JSGC_ROOT_ANALYSIS
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

typedef Root<JSObject*>          RootObject;
typedef Root<JSFunction*>        RootFunction;
typedef Root<Shape*>             RootShape;
typedef Root<BaseShape*>         RootBaseShape;
typedef Root<types::TypeObject*> RootTypeObject;
typedef Root<JSString*>          RootString;
typedef Root<JSAtom*>            RootAtom;
typedef Root<jsid>               RootId;
typedef Root<Value>              RootValue;


class CheckRoot
{
#if defined(DEBUG) && defined(JSGC_ROOT_ANALYSIS)

    CheckRoot **stack, *prev;
    const uint8_t *ptr;

  public:
    template <typename T>
    CheckRoot(JSContext *cx, const T *ptr
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        this->stack = &cx->checkGCRooters;
        this->prev = *stack;
        *stack = this;
        this->ptr = static_cast<const uint8_t*>(ptr);
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~CheckRoot()
    {
        JS_ASSERT(*stack == this);
        *stack = prev;
    }

    CheckRoot *previous() { return prev; }

    bool contains(const uint8_t *v, size_t len) {
        return ptr >= v && ptr < v + len;
    }

#else 

  public:
    template <typename T>
    CheckRoot(JSContext *cx, const T *ptr
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

    operator T () { return ptr; }
    T operator ->() { return ptr; }
    T * address() { return &ptr; }
    const T * address() const { return &ptr; }
    T raw() { return ptr; }

    T & operator =(T value)
    {
        JS_ASSERT(!RootMethods<T>::poisoned(value));
        ptr = value;
        return ptr;
    }

  private:
    T ptr;
    Root<T> root;
};

template <typename T> template <typename S>
inline
Handle<T>::Handle(const RootedVar<S> &root)
{
    ptr = reinterpret_cast<const T *>(root.address());
}

typedef RootedVar<JSObject*>          RootedVarObject;
typedef RootedVar<JSFunction*>        RootedVarFunction;
typedef RootedVar<Shape*>             RootedVarShape;
typedef RootedVar<BaseShape*>         RootedVarBaseShape;
typedef RootedVar<types::TypeObject*> RootedVarTypeObject;
typedef RootedVar<JSString*>          RootedVarString;
typedef RootedVar<JSAtom*>            RootedVarAtom;
typedef RootedVar<jsid>               RootedVarId;
typedef RootedVar<Value>              RootedVarValue;

}  
#endif  
