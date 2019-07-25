







































#ifndef mozilla_RefPtr_h_
#define mozilla_RefPtr_h_

#include "mozilla/Util.h"





namespace mozilla {

template<typename T> class RefCounted;
template<typename T> class RefPtr;
template<typename T> class TemporaryRef;
template<typename T> class OutParamRef;
template<typename T> OutParamRef<T> byRef(RefPtr<T>&);






















template<typename T>
class RefCounted
{
    friend class RefPtr<T>;

public:
    RefCounted() : refCnt(0) { }
    ~RefCounted() { MOZ_ASSERT(refCnt == -0xdead); }

    
    void AddRef() {
        MOZ_ASSERT(refCnt >= 0);
        ++refCnt;
    }

    void Release() {
        MOZ_ASSERT(refCnt > 0);
        if (0 == --refCnt) {
#ifdef DEBUG
            refCnt = -0xdead;
#endif
            delete static_cast<T*>(this);
        }
    }

    
    void ref() { AddRef(); }
    void deref() { Release(); }
    int refCount() const { return refCnt; }
    bool hasOneRef() const {
        MOZ_ASSERT(refCnt > 0);
        return refCnt == 1;
    }

private:
    int refCnt;
};











template<typename T>
class RefPtr
{
    
    friend class TemporaryRef<T>;
    friend class OutParamRef<T>;

public:
    RefPtr() : ptr(0) { }
    RefPtr(const RefPtr& o) : ptr(ref(o.ptr)) {}
    RefPtr(const TemporaryRef<T>& o) : ptr(o.drop()) {}
    RefPtr(T* t) : ptr(ref(t)) {}

    template<typename U>
    RefPtr(const RefPtr<U>& o) : ptr(ref(o.get())) {}

    ~RefPtr() { unref(ptr); }

    RefPtr& operator=(const RefPtr& o) {
        assign(ref(o.ptr));
        return *this;
    }
    RefPtr& operator=(const TemporaryRef<T>& o) {
        assign(o.drop());
        return *this;
    }
    RefPtr& operator=(T* t) {
        assign(ref(t));
        return *this;
    }

    template<typename U>
    RefPtr& operator=(const RefPtr<U>& o) {
        assign(ref(o.get()));
        return *this;
    }

    TemporaryRef<T> forget() {
        T* tmp = ptr;
        ptr = 0;
        return TemporaryRef<T>(tmp);
    }

    T* get() const { return ptr; }
    operator T*() const { return ptr; }
    T* operator->() const { return ptr; }
    T& operator*() const { return *ptr; }

private:
    void assign(T* t) {
        unref(ptr);
        ptr = t;
    }

    T* ptr;

    static MOZ_ALWAYS_INLINE T* ref(T* t) {
        if (t) {
            t->AddRef();
        }
        return t;
    }

    static MOZ_ALWAYS_INLINE void unref(T* t) {
        if (t) {
            t->Release();
        }
    }
};







template<typename T>
class TemporaryRef
{
    
    friend class RefPtr<T>;

public:
    TemporaryRef(const TemporaryRef& o) : ptr(o.drop()) {}

    template<typename U>
    TemporaryRef(const TemporaryRef<U>& o) : ptr(o.drop()) {}

    ~TemporaryRef() { RefPtr<T>::unref(ptr); }

    T* drop() const {
        T* tmp = ptr;
        ptr = 0;
        return tmp;
    }

private:
    TemporaryRef(T* t) : ptr(t) {}

    mutable T* ptr;

    TemporaryRef();
    TemporaryRef& operator=(const TemporaryRef&);
};












template<typename T>
class OutParamRef
{
    friend OutParamRef byRef<T>(RefPtr<T>&);

public:
    ~OutParamRef() { refPtr = tmp; }

    operator T**() { return &tmp; }

private:
    OutParamRef(RefPtr<T>& p) : refPtr(p), tmp(p.get()) {}

    RefPtr<T>& refPtr;
    T* tmp;

    OutParamRef();
    OutParamRef& operator=(const OutParamRef&);
};

template<typename T>
OutParamRef<T>
byRef(RefPtr<T>& ptr)
{
    return OutParamRef<T>(ptr);
}

} 

#endif 


#if 0





using namespace mozilla;

struct Foo : public RefCounted<Foo>
{
    Foo() : dead(false) { }
    ~Foo() {
        MOZ_ASSERT(!dead);
        dead = true;
        numDestroyed++;
    }

    bool dead;
    static int numDestroyed;
};
int Foo::numDestroyed;

struct Bar : public Foo { };

TemporaryRef<Foo>
NewFoo()
{
    RefPtr<Foo> f = new Foo();
    return f.forget();
}

TemporaryRef<Foo>
NewBar()
{
    RefPtr<Bar> b = new Bar();
    return b.forget();
}

void
GetNewFoo(Foo** f)
{
    *f = new Bar();
}

void
GetPassedFoo(Foo** f)
{}

void
GetNewFoo(RefPtr<Foo>* f)
{
    *f = new Bar();
}

void
GetPassedFoo(RefPtr<Foo>* f)
{}

int
main(int argc, char** argv)
{
    


    MOZ_ASSERT(0 == Foo::numDestroyed);
    {
        RefPtr<Foo> f = new Foo();
        MOZ_ASSERT(f->refCount() == 1);
    }
    MOZ_ASSERT(1 == Foo::numDestroyed);

    {
        RefPtr<Foo> f1 = NewFoo();
        RefPtr<Foo> f2(NewFoo());
        MOZ_ASSERT(1 == Foo::numDestroyed);
    }
    MOZ_ASSERT(3 == Foo::numDestroyed);

    {
        RefPtr<Foo> b = NewBar();
        MOZ_ASSERT(3 == Foo::numDestroyed);
    }
    MOZ_ASSERT(4 == Foo::numDestroyed);

    {
        RefPtr<Foo> f1;
        {
            f1 = new Foo();
            RefPtr<Foo> f2(f1);
            RefPtr<Foo> f3 = f2;
            MOZ_ASSERT(4 == Foo::numDestroyed);
        }
        MOZ_ASSERT(4 == Foo::numDestroyed);
    }
    MOZ_ASSERT(5 == Foo::numDestroyed);

    {
        RefPtr<Foo> f = new Foo();
        f.forget();
        MOZ_ASSERT(6 == Foo::numDestroyed);
    }

    {
        RefPtr<Foo> f = new Foo();
        GetNewFoo(byRef(f));
        MOZ_ASSERT(7 == Foo::numDestroyed);
    }
    MOZ_ASSERT(8 == Foo::numDestroyed);

    {
        RefPtr<Foo> f = new Foo();
        GetPassedFoo(byRef(f));
        MOZ_ASSERT(8 == Foo::numDestroyed);
    }
    MOZ_ASSERT(9 == Foo::numDestroyed);

    {
        RefPtr<Foo> f = new Foo();
        GetNewFoo(&f);
        MOZ_ASSERT(10 == Foo::numDestroyed);
    }
    MOZ_ASSERT(11 == Foo::numDestroyed);

    {
        RefPtr<Foo> f = new Foo();
        GetPassedFoo(&f);
        MOZ_ASSERT(11 == Foo::numDestroyed);
    }
    MOZ_ASSERT(12 == Foo::numDestroyed);

    {
        RefPtr<Foo> f1 = new Bar();
    }
    MOZ_ASSERT(13 == Foo::numDestroyed);

    return 0;
}

#endif
