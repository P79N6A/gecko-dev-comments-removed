








#ifndef SkRefCnt_DEFINED
#define SkRefCnt_DEFINED

#include "SkThread.h"











class SK_API SkRefCnt : SkNoncopyable {
public:
    

    SkRefCnt() : fRefCnt(1) {}

    

    virtual ~SkRefCnt() {
#ifdef SK_DEBUG
        SkASSERT(fRefCnt == 1);
        fRefCnt = 0;    
#endif
    }

    

    int32_t getRefCnt() const { return fRefCnt; }

    

    void ref() const {
        SkASSERT(fRefCnt > 0);
        sk_atomic_inc(&fRefCnt);
    }

    




    void unref() const {
        SkASSERT(fRefCnt > 0);
        if (sk_atomic_dec(&fRefCnt) == 1) {
            fRefCnt = 1;    
            SkDELETE(this);
        }
    }

    void validate() const {
        SkASSERT(fRefCnt > 0);
    }

private:
    mutable int32_t fRefCnt;
};







#define SkRefCnt_SafeAssign(dst, src)   \
    do {                                \
        if (src) src->ref();            \
        if (dst) dst->unref();          \
        dst = src;                      \
    } while (0)




template <typename T> static inline void SkSafeRef(T* obj) {
    if (obj) {
        obj->ref();
    }
}



template <typename T> static inline void SkSafeUnref(T* obj) {
    if (obj) {
        obj->unref();
    }
}






template <typename T> class SkAutoTUnref : SkNoncopyable {
public:
    explicit SkAutoTUnref(T* obj = NULL) : fObj(obj) {}
    ~SkAutoTUnref() { SkSafeUnref(fObj); }

    T* get() const { return fObj; }

    void reset(T* obj) {
        SkSafeUnref(fObj);
        fObj = obj;
    }

    





    T* detach() {
        T* obj = fObj;
        fObj = NULL;
        return obj;
    }

private:
    T*  fObj;
};

class SkAutoUnref : public SkAutoTUnref<SkRefCnt> {
public:
    SkAutoUnref(SkRefCnt* obj) : SkAutoTUnref<SkRefCnt>(obj) {}
};

class SkAutoRef : SkNoncopyable {
public:
    SkAutoRef(SkRefCnt* obj) : fObj(obj) { SkSafeRef(obj); }
    ~SkAutoRef() { SkSafeUnref(fObj); }
private:
    SkRefCnt* fObj;
};




template <typename T> class SkRefPtr {
public:
    SkRefPtr() : fObj(NULL) {}
    SkRefPtr(T* obj) : fObj(obj) { SkSafeRef(fObj); }
    SkRefPtr(const SkRefPtr& o) : fObj(o.fObj) { SkSafeRef(fObj); }
    ~SkRefPtr() { SkSafeUnref(fObj); }

    SkRefPtr& operator=(const SkRefPtr& rp) {
        SkRefCnt_SafeAssign(fObj, rp.fObj);
        return *this;
    }
    SkRefPtr& operator=(T* obj) {
        SkRefCnt_SafeAssign(fObj, obj);
        return *this;
    }

    T* get() const { return fObj; }
    T& operator*() const { return *fObj; }
    T* operator->() const { return fObj; }

    typedef T* SkRefPtr::*unspecified_bool_type;
    operator unspecified_bool_type() const {
        return fObj ? &SkRefPtr::fObj : NULL;
    }

private:
    T* fObj;
};

#endif

