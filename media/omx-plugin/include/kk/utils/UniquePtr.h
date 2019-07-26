























#ifndef UNIQUE_PTR_H_included
#define UNIQUE_PTR_H_included

#include <cstdlib> 


template <typename T>
struct DefaultDelete {
    enum { type_must_be_complete = sizeof(T) };
    DefaultDelete() {}
    void operator()(T* p) const {
        delete p;
    }
};


template <typename T>
struct DefaultDelete<T[]> {
    enum { type_must_be_complete = sizeof(T) };
    void operator()(T* p) const {
        delete[] p;
    }
};









template <typename T, typename D = DefaultDelete<T> >
class UniquePtr {
public:
    
    explicit UniquePtr(T* ptr = NULL) : mPtr(ptr) {
    }

    ~UniquePtr() {
        reset();
    }

    
    T& operator*() const { return *mPtr; }
    T* operator->() const { return mPtr; }
    T* get() const { return mPtr; }

    
    
    T* release() __attribute__((warn_unused_result)) {
        T* result = mPtr;
        mPtr = NULL;
        return result;
    }

    
    
    
    void reset(T* ptr = NULL) {
        if (ptr != mPtr) {
            D()(mPtr);
            mPtr = ptr;
        }
    }

private:
    
    T* mPtr;

    
    template <typename T2> bool operator==(const UniquePtr<T2>& p) const;
    template <typename T2> bool operator!=(const UniquePtr<T2>& p) const;

    
    UniquePtr(const UniquePtr&);
    void operator=(const UniquePtr&);
};



template <typename T, typename D>
class UniquePtr<T[], D> {
public:
    explicit UniquePtr(T* ptr = NULL) : mPtr(ptr) {
    }

    ~UniquePtr() {
        reset();
    }

    T& operator[](size_t i) const {
        return mPtr[i];
    }
    T* get() const { return mPtr; }

    T* release() __attribute__((warn_unused_result)) {
        T* result = mPtr;
        mPtr = NULL;
        return result;
    }

    void reset(T* ptr = NULL) {
        if (ptr != mPtr) {
            D()(mPtr);
            mPtr = ptr;
        }
    }

private:
    T* mPtr;

    
    UniquePtr(const UniquePtr&);
    void operator=(const UniquePtr&);
};

#if UNIQUE_PTR_TESTS




#include <stdio.h>

static void assert(bool b) {
    if (!b) {
        fprintf(stderr, "FAIL\n");
        abort();
    }
    fprintf(stderr, "OK\n");
}
static int cCount = 0;
struct C {
    C() { ++cCount; }
    ~C() { --cCount; }
};
static bool freed = false;
struct Freer {
    void operator()(int* p) {
        assert(*p == 123);
        free(p);
        freed = true;
    }
};

int main(int argc, char* argv[]) {
    
    
    

    
    {
        UniquePtr<C> c(new C);
        assert(cCount == 1);
    }
    assert(cCount == 0);
    
    C* rawC;
    {
        UniquePtr<C> c(new C);
        assert(cCount == 1);
        rawC = c.release();
    }
    assert(cCount == 1);
    delete rawC;
    
    {
        UniquePtr<C> c(new C);
        assert(cCount == 1);
        c.reset(new C);
        assert(cCount == 1);
    }
    assert(cCount == 0);

    
    
    

    
    {
        UniquePtr<C[]> cs(new C[4]);
        assert(cCount == 4);
    }
    assert(cCount == 0);
    
    {
        UniquePtr<C[]> c(new C[4]);
        assert(cCount == 4);
        rawC = c.release();
    }
    assert(cCount == 4);
    delete[] rawC;
    
    {
        UniquePtr<C[]> c(new C[4]);
        assert(cCount == 4);
        c.reset(new C[2]);
        assert(cCount == 2);
    }
    assert(cCount == 0);

    
    
    
    assert(!freed);
    {
        UniquePtr<int, Freer> i(reinterpret_cast<int*>(malloc(sizeof(int))));
        *i = 123;
    }
    assert(freed);
    return 0;
}
#endif

#endif
