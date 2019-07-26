






#ifndef jsgc_root_h__
#define jsgc_root_h__

#include "jspubtd.h"

#include "js/Utility.h"

#ifdef __cplusplus

namespace js {
namespace gc {
struct Cell;
} 
} 

namespace JS {












































template <typename T> class Rooted;

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

    






    static Handle fromMarkedLocation(const T *p) {
        Handle h;
        h.ptr = p;
        return h;
    }

    



    template <typename S> inline Handle(const Rooted<S> &root);

    const T *address() const { return ptr; }
    T value() const { return *ptr; }

    operator T () const { return value(); }
    T operator ->() const { return value(); }

  private:
    Handle() {}

    const T *ptr;

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
typedef Handle<JSScript*>    HandleScript;
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
class Rooted
{
    void init(JSContext *cx_, T initial)
    {
#ifdef JSGC_ROOT_ANALYSIS
        ContextFriendFields *cx = ContextFriendFields::get(cx_);

        ThingRootKind kind = RootMethods<T>::kind();
        this->stack = reinterpret_cast<Rooted<T>**>(&cx->thingGCRooters[kind]);
        this->prev = *stack;
        *stack = this;

        JS_ASSERT(!RootMethods<T>::poisoned(initial));
#endif

        ptr = initial;
    }

  public:
    Rooted(JSContext *cx) { init(cx, RootMethods<T>::initial()); }
    Rooted(JSContext *cx, T initial) { init(cx, initial); }

    






    operator Handle<T> () const { return Handle<T>(*this); }

    ~Rooted()
    {
#ifdef JSGC_ROOT_ANALYSIS
        JS_ASSERT(*stack == this);
        *stack = prev;
#endif
    }

#ifdef JSGC_ROOT_ANALYSIS
    Rooted<T> *previous() { return prev; }
#endif

    operator T () const { return ptr; }
    T operator ->() const { return ptr; }
    T * address() { return &ptr; }
    const T * address() const { return &ptr; }
    T & reference() { return ptr; }
    T raw() const { return ptr; }

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

#ifdef JSGC_ROOT_ANALYSIS
    Rooted<T> **stack, *prev;
#endif
    T ptr;

    Rooted() MOZ_DELETE;
    Rooted(const Rooted &) MOZ_DELETE;
};

template<typename T> template <typename S>
inline
Handle<T>::Handle(const Rooted<S> &root)
{
    testAssign<S>();
    ptr = reinterpret_cast<const T *>(root.address());
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
    SkipRoot(JSContext *cx, const T *ptr
             JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        init(ContextFriendFields::get(cx), ptr, 1);
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    template <typename T>
    SkipRoot(JSContext *cx, const T *ptr, size_t count
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
    SkipRoot(JSContext *cx, const T *ptr
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    template <typename T>
    SkipRoot(JSContext *cx, const T *ptr, size_t count
              JS_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

#endif 

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};





#if defined(JSGC_ROOT_ANALYSIS) && defined(DEBUG) && !defined(JS_THREADSAFE)
void CheckStackRoots(JSContext *cx);
inline void MaybeCheckStackRoots(JSContext *cx) { CheckStackRoots(cx); }
#else
inline void MaybeCheckStackRoots(JSContext *cx) {}
#endif


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
