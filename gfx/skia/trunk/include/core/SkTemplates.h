








#ifndef SkTemplates_DEFINED
#define SkTemplates_DEFINED

#include "SkTypes.h"
#include <limits.h>
#include <new>











template<typename T> inline void sk_ignore_unused_variable(const T&) { }





template <typename T> struct SkTIsConst {
    static T* t;
    static uint16_t test(const volatile void*);
    static uint32_t test(volatile void *);
    static const bool value = (sizeof(uint16_t) == sizeof(test(t)));
};



template <typename T, bool CONST> struct SkTConstType {
    typedef T type;
};
template <typename T> struct SkTConstType<T, true> {
    typedef const T type;
};





template <typename D, typename S> static D* SkTAfter(S* ptr, size_t count = 1) {
    return reinterpret_cast<D*>(ptr + count);
}




template <typename D, typename S> static D* SkTAddOffset(S* ptr, size_t byteOffset) {
    
    
    return reinterpret_cast<D*>(
        reinterpret_cast<typename SkTConstType<char, SkTIsConst<D>::value>::type*>(ptr) + byteOffset
    );
}









template <typename T, void (*P)(T*)> class SkAutoTCallVProc : SkNoncopyable {
public:
    SkAutoTCallVProc(T* obj): fObj(obj) {}
    ~SkAutoTCallVProc() { if (fObj) P(fObj); }
    T* detach() { T* obj = fObj; fObj = NULL; return obj; }
private:
    T* fObj;
};









template <typename T, int (*P)(T*)> class SkAutoTCallIProc : SkNoncopyable {
public:
    SkAutoTCallIProc(T* obj): fObj(obj) {}
    ~SkAutoTCallIProc() { if (fObj) P(fObj); }
    T* detach() { T* obj = fObj; fObj = NULL; return obj; }
private:
    T* fObj;
};











template <typename T> class SkAutoTDelete : SkNoncopyable {
public:
    SkAutoTDelete(T* obj = NULL) : fObj(obj) {}
    ~SkAutoTDelete() { SkDELETE(fObj); }

    T* get() const { return fObj; }
    T& operator*() const { SkASSERT(fObj); return *fObj; }
    T* operator->() const { SkASSERT(fObj); return fObj; }

    void reset(T* obj) {
        if (fObj != obj) {
            SkDELETE(fObj);
            fObj = obj;
        }
    }

    


    void free() {
        SkDELETE(fObj);
        fObj = NULL;
    }

    




    T* detach() {
        T* obj = fObj;
        fObj = NULL;
        return obj;
    }

    void swap(SkAutoTDelete* that) {
        SkTSwap(fObj, that->fObj);
    }

private:
    T*  fObj;
};


template <typename T> class SkAutoTDestroy : SkNoncopyable {
public:
    SkAutoTDestroy(T* obj = NULL) : fObj(obj) {}
    ~SkAutoTDestroy() {
        if (NULL != fObj) {
            fObj->~T();
        }
    }

    T* get() const { return fObj; }
    T& operator*() const { SkASSERT(fObj); return *fObj; }
    T* operator->() const { SkASSERT(fObj); return fObj; }

private:
    T*  fObj;
};

template <typename T> class SkAutoTDeleteArray : SkNoncopyable {
public:
    SkAutoTDeleteArray(T array[]) : fArray(array) {}
    ~SkAutoTDeleteArray() { SkDELETE_ARRAY(fArray); }

    T*      get() const { return fArray; }
    void    free() { SkDELETE_ARRAY(fArray); fArray = NULL; }
    T*      detach() { T* array = fArray; fArray = NULL; return array; }

    void reset(T array[]) {
        if (fArray != array) {
            SkDELETE_ARRAY(fArray);
            fArray = array;
        }
    }

private:
    T*  fArray;
};



template <typename T> class SkAutoTArray : SkNoncopyable {
public:
    SkAutoTArray() {
        fArray = NULL;
        SkDEBUGCODE(fCount = 0;)
    }
    

    explicit SkAutoTArray(int count) {
        SkASSERT(count >= 0);
        fArray = NULL;
        if (count) {
            fArray = SkNEW_ARRAY(T, count);
        }
        SkDEBUGCODE(fCount = count;)
    }

    

    void reset(int count) {
        SkDELETE_ARRAY(fArray);
        SkASSERT(count >= 0);
        fArray = NULL;
        if (count) {
            fArray = SkNEW_ARRAY(T, count);
        }
        SkDEBUGCODE(fCount = count;)
    }

    ~SkAutoTArray() {
        SkDELETE_ARRAY(fArray);
    }

    

    T* get() const { return fArray; }

    

    T&  operator[](int index) const {
        SkASSERT((unsigned)index < (unsigned)fCount);
        return fArray[index];
    }

private:
    T*  fArray;
    SkDEBUGCODE(int fCount;)
};



template <int N, typename T> class SkAutoSTArray : SkNoncopyable {
public:
    
    SkAutoSTArray() {
        fArray = NULL;
        fCount = 0;
    }

    

    SkAutoSTArray(int count) {
        fArray = NULL;
        fCount = 0;
        this->reset(count);
    }

    ~SkAutoSTArray() {
        this->reset(0);
    }

    
    void reset(int count) {
        T* start = fArray;
        T* iter = start + fCount;
        while (iter > start) {
            (--iter)->~T();
        }

        if (fCount != count) {
            if (fCount > N) {
                
                SkASSERT((T*) fStorage != fArray);
                sk_free(fArray);
            }

            if (count > N) {
                fArray = (T*) sk_malloc_throw(count * sizeof(T));
            } else if (count > 0) {
                fArray = (T*) fStorage;
            } else {
                fArray = NULL;
            }

            fCount = count;
        }

        iter = fArray;
        T* stop = fArray + count;
        while (iter < stop) {
            SkNEW_PLACEMENT(iter++, T);
        }
    }

    

    int count() const { return fCount; }

    

    T* get() const { return fArray; }

    

    T&  operator[](int index) const {
        SkASSERT(index < fCount);
        return fArray[index];
    }

private:
    int     fCount;
    T*      fArray;
    
    char    fStorage[N * sizeof(T)];
};




template <typename T> class SkAutoTMalloc : SkNoncopyable {
public:
    
    explicit SkAutoTMalloc(T* ptr = NULL) {
        fPtr = ptr;
    }

    
    explicit SkAutoTMalloc(size_t count) {
        fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW);
    }

    ~SkAutoTMalloc() {
        sk_free(fPtr);
    }

    
    void realloc(size_t count) {
        fPtr = reinterpret_cast<T*>(sk_realloc_throw(fPtr, count * sizeof(T)));
    }

    
    void reset(size_t count) {
        sk_free(fPtr);
        fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW);
    }

    T* get() const { return fPtr; }

    operator T*() {
        return fPtr;
    }

    operator const T*() const {
        return fPtr;
    }

    T& operator[](int index) {
        return fPtr[index];
    }

    const T& operator[](int index) const {
        return fPtr[index];
    }

    




    T* detach() {
        T* ptr = fPtr;
        fPtr = NULL;
        return ptr;
    }

private:
    T* fPtr;
};

template <size_t N, typename T> class SkAutoSTMalloc : SkNoncopyable {
public:
    SkAutoSTMalloc() {
        fPtr = NULL;
    }

    SkAutoSTMalloc(size_t count) {
        if (count > N) {
            fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW | SK_MALLOC_TEMP);
        } else if (count) {
            fPtr = fTStorage;
        } else {
            fPtr = NULL;
        }
    }

    ~SkAutoSTMalloc() {
        if (fPtr != fTStorage) {
            sk_free(fPtr);
        }
    }

    
    T* reset(size_t count) {
        if (fPtr != fTStorage) {
            sk_free(fPtr);
        }
        if (count > N) {
            fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW | SK_MALLOC_TEMP);
        } else if (count) {
            fPtr = fTStorage;
        } else {
            fPtr = NULL;
        }
        return fPtr;
    }

    T* get() const { return fPtr; }

    operator T*() {
        return fPtr;
    }

    operator const T*() const {
        return fPtr;
    }

    T& operator[](int index) {
        return fPtr[index];
    }

    const T& operator[](int index) const {
        return fPtr[index];
    }

private:
    T*          fPtr;
    union {
        uint32_t    fStorage32[(N*sizeof(T) + 3) >> 2];
        T           fTStorage[1];   
    };
};





template <size_t N> class SkAlignedSStorage : SkNoncopyable {
public:
    void* get() { return fData; }
private:
    union {
        void*   fPtr;
        double  fDouble;
        char    fData[N];
    };
};







template <int N, typename T> class SkAlignedSTStorage : SkNoncopyable {
public:
    



    void* get() { return fStorage.get(); }
private:
    SkAlignedSStorage<sizeof(T)*N> fStorage;
};

#endif
