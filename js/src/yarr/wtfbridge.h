






































#ifndef jswtfbridge_h__
#define jswtfbridge_h__






#include "jsstr.h"
#include "jsprvtd.h"
#include "jstl.h"
#include "assembler/wtf/Platform.h"
#include "assembler/jit/ExecutableAllocator.h"

namespace JSC { namespace Yarr {





typedef jschar UChar;
typedef JSLinearString UString;

class Unicode {
  public:
    static UChar toUpper(UChar c) { return JS_TOUPPER(c); }
    static UChar toLower(UChar c) { return JS_TOLOWER(c); }
};






template<typename T>
class RefCounted {
};

template<typename T>
class RefPtr {
    T *ptr;
  public:
    RefPtr(T *p) { ptr = p; }
    operator bool() const { return ptr != NULL; }
    const T *operator ->() const { return ptr; }
    T *get() { return ptr; }
};

template<typename T>
class PassRefPtr {
    T *ptr;
  public:
    PassRefPtr(T *p) { ptr = p; }
    operator T*() { return ptr; }
};

template<typename T>
class PassOwnPtr {
    T *ptr;
  public:
    PassOwnPtr(T *p) { ptr = p; }

    T *get() { return ptr; }
};

template<typename T>
class OwnPtr {
    T *ptr;
  public:
    OwnPtr() : ptr(NULL) { }
    OwnPtr(PassOwnPtr<T> p) : ptr(p.get()) { }

    ~OwnPtr() {
        if (ptr)
            js::Foreground::delete_(ptr);
    }

    OwnPtr<T> &operator=(PassOwnPtr<T> p) {
        ptr = p.get();
        return *this;
    }

    T *operator ->() { return ptr; }

    T *get() { return ptr; }

    T *release() {
        T *result = ptr;
        ptr = NULL;
        return result;
    }
};

template<typename T>
PassRefPtr<T> adoptRef(T *p) { return PassRefPtr<T>(p); }

template<typename T>
PassOwnPtr<T> adoptPtr(T *p) { return PassOwnPtr<T>(p); }

#define WTF_MAKE_FAST_ALLOCATED





template<typename T, size_t N = 0>
class Vector {
  public:
    js::Vector<T, N, js::SystemAllocPolicy> impl;
  public:
    Vector() {}

    Vector(const Vector &v) {
        
        (void) append(v);
    }

    size_t size() const {
        return impl.length();
    }

    T &operator[](size_t i) {
        return impl[i];
    }

    const T &operator[](size_t i) const {
        return impl[i];
    }

    T &at(size_t i) {
        return impl[i];
    }

    const T *begin() const {
        return impl.begin();
    }

    T &last() {
        return impl.back();
    }

    bool isEmpty() const {
        return impl.empty();
    }

    template <typename U>
    void append(const U &u) {
        
        (void) impl.append(static_cast<T>(u));
    }

    template <size_t M>
    void append(const Vector<T,M> &v) {
        
        (void) impl.append(v.impl);
    }

    void insert(size_t i, const T& t) {
        
        (void) impl.insert(&impl[i], t);
    }

    void remove(size_t i) {
        impl.erase(&impl[i]);
    }

    void clear() {
        return impl.clear();
    }

    void shrink(size_t newLength) {
        
        JS_ASSERT(newLength <= impl.length());
        (void) impl.resize(newLength);
    }

    void deleteAllValues() {
        for (T *p = impl.begin(); p != impl.end(); ++p)
            js::Foreground::delete_(*p);
    }
};

template<typename T>
class Vector<OwnPtr<T> > {
  public:
    js::Vector<T *, 0, js::SystemAllocPolicy> impl;
  public:
    Vector() {}

    size_t size() const {
        return impl.length();
    }

    void append(T *t) {
        
        (void) impl.append(t);
    }

    PassOwnPtr<T> operator[](size_t i) {
        return PassOwnPtr<T>(impl[i]);
    }

    void clear() {
        for (T **p = impl.begin(); p != impl.end(); ++p)
            js::Foreground::delete_(*p);
        return impl.clear();
    }
};

template <typename T, size_t N>
inline void
deleteAllValues(Vector<T, N> &v) {
    v.deleteAllValues();
}




class JSGlobalData {
  public:
    ExecutableAllocator *regexAllocator;

    JSGlobalData(ExecutableAllocator *regexAllocator)
     : regexAllocator(regexAllocator) { }
};




const size_t notFound = size_t(-1);

 



#define UNUSED_PARAM(e)

} 






namespace std {





#if WTF_COMPILER_MSVC
# undef min
# undef max
#endif

template<typename T>
inline T
min(T t1, T t2)
{
    return JS_MIN(t1, t2);
}

template<typename T>
inline T
max(T t1, T t2)
{
    return JS_MAX(t1, t2);
}

template<typename T>
inline void
swap(T &t1, T &t2)
{
    T tmp = t1;
    t1 = t2;
    t2 = tmp;
}
} 

} 

#endif
