








#ifndef SkTemplates_DEFINED
#define SkTemplates_DEFINED

#include "SkTypes.h"















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
    SkAutoTDelete(T* obj, bool deleteWhenDone = true) : fObj(obj) {
        fDeleteWhenDone = deleteWhenDone;
    }
    ~SkAutoTDelete() { if (fDeleteWhenDone) delete fObj; }

    T*      get() const { return fObj; }
    void    free() { delete fObj; fObj = NULL; }
    T*      detach() { T* obj = fObj; fObj = NULL; return obj; }

private:
    T*  fObj;
    bool fDeleteWhenDone;
};

template <typename T> class SkAutoTDeleteArray : SkNoncopyable {
public:
    SkAutoTDeleteArray(T array[]) : fArray(array) {}
    ~SkAutoTDeleteArray() { delete[] fArray; }

    T*      get() const { return fArray; }
    void    free() { delete[] fArray; fArray = NULL; }
    T*      detach() { T* array = fArray; fArray = NULL; return array; }

private:
    T*  fArray;
};



template <typename T> class SkAutoTArray : SkNoncopyable {
public:
    

    SkAutoTArray(size_t count) {
        fArray = NULL;
        if (count) {
            fArray = new T[count];
        }
        SkDEBUGCODE(fCount = count;)
    }

    ~SkAutoTArray() {
        delete[] fArray;
    }

    

    T* get() const { return fArray; }
    
    

    T&  operator[](int index) const {
        SkASSERT((unsigned)index < fCount);
        return fArray[index];
    }

private:
    T*  fArray;
    SkDEBUGCODE(size_t fCount;)
};



template <size_t N, typename T> class SkAutoSTArray : SkNoncopyable {
public:
    

    SkAutoSTArray(size_t count) {
        if (count > N) {
            fArray = new T[count];
        } else if (count) {
            fArray = new (fStorage) T[count];
        } else {
            fArray = NULL;
        }
        fCount = count;
    }
    
    ~SkAutoSTArray() {
        if (fCount > N) {
            delete[] fArray;
        } else {
            T* start = fArray;
            T* iter = start + fCount;
            while (iter > start) {
                (--iter)->~T();
            }
        }
    }
    
    

    size_t count() const { return fCount; }
    
    

    T* get() const { return fArray; }
    
    

    T&  operator[](int index) const {
        SkASSERT((unsigned)index < fCount);
        return fArray[index];
    }
    
private:
    size_t  fCount;
    T*      fArray;
    
    char    fStorage[N * sizeof(T)];
};




template <typename T> class SkAutoTMalloc : SkNoncopyable {
public:
    SkAutoTMalloc(size_t count) {
        fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW | SK_MALLOC_TEMP);
    }

    ~SkAutoTMalloc() {
        sk_free(fPtr);
    }

    
    void reset (size_t count) {
        sk_free(fPtr);
        fPtr = fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW | SK_MALLOC_TEMP);
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
    T*  fPtr;
};

template <size_t N, typename T> class SK_API SkAutoSTMalloc : SkNoncopyable {
public:
    SkAutoSTMalloc(size_t count) {
        if (count <= N) {
            fPtr = fTStorage;
        } else {
            fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW | SK_MALLOC_TEMP);
        }
    }

    ~SkAutoSTMalloc() {
        if (fPtr != fTStorage) {
            sk_free(fPtr);
        }
    }

    
    void reset(size_t count) {
        if (fPtr != fTStorage) {
            sk_free(fPtr);
        }
        if (count <= N) {
            fPtr = fTStorage;
        } else {
            fPtr = (T*)sk_malloc_flags(count * sizeof(T), SK_MALLOC_THROW | SK_MALLOC_TEMP);
        }
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

