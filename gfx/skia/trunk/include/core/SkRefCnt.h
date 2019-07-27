








#ifndef SkRefCnt_DEFINED
#define SkRefCnt_DEFINED

#include "SkDynamicAnnotations.h"
#include "SkThread.h"
#include "SkInstCnt.h"
#include "SkTemplates.h"











class SK_API SkRefCntBase : SkNoncopyable {
public:
    SK_DECLARE_INST_COUNT_ROOT(SkRefCntBase)

    

    SkRefCntBase() : fRefCnt(1) {}

    

    virtual ~SkRefCntBase() {
#ifdef SK_DEBUG
        SkASSERTF(fRefCnt == 1, "fRefCnt was %d", fRefCnt);
        fRefCnt = 0;    
#endif
    }

    
    int32_t getRefCnt() const { return fRefCnt; }

    


    bool unique() const {
        
        
        bool const unique = (1 == SK_ANNOTATE_UNPROTECTED_READ(fRefCnt));
        if (unique) {
            
            
            
        }
        return unique;
    }

    

    void ref() const {
        SkASSERT(fRefCnt > 0);
        sk_atomic_inc(&fRefCnt);  
    }

    



    void unref() const {
        SkASSERT(fRefCnt > 0);
        
        if (sk_atomic_dec(&fRefCnt) == 1) {
            
            
            sk_membar_acquire__after_atomic_dec();
            internal_dispose();
        }
    }

#ifdef SK_DEBUG
    void validate() const {
        SkASSERT(fRefCnt > 0);
    }
#endif

protected:
    





    void internal_dispose_restore_refcnt_to_1() const {
#ifdef SK_DEBUG
        SkASSERT(0 == fRefCnt);
        fRefCnt = 1;
#endif
    }

private:
    


    virtual void internal_dispose() const {
        this->internal_dispose_restore_refcnt_to_1();
        SkDELETE(this);
    }

    
    
    friend class SkWeakRefCnt;

    mutable int32_t fRefCnt;

    typedef SkNoncopyable INHERITED;
};

#ifdef SK_REF_CNT_MIXIN_INCLUDE


#include SK_REF_CNT_MIXIN_INCLUDE
#else
class SK_API SkRefCnt : public SkRefCntBase { };
#endif







#define SkRefCnt_SafeAssign(dst, src)   \
    do {                                \
        if (src) src->ref();            \
        if (dst) dst->unref();          \
        dst = src;                      \
    } while (0)




template <typename T> static inline T* SkRef(T* obj) {
    SkASSERT(obj);
    obj->ref();
    return obj;
}



template <typename T> static inline T* SkSafeRef(T* obj) {
    if (obj) {
        obj->ref();
    }
    return obj;
}



template <typename T> static inline void SkSafeUnref(T* obj) {
    if (obj) {
        obj->unref();
    }
}

template<typename T> static inline void SkSafeSetNull(T*& obj) {
    if (NULL != obj) {
        obj->unref();
        obj = NULL;
    }
}






template <typename T> class SkAutoTUnref : SkNoncopyable {
public:
    explicit SkAutoTUnref(T* obj = NULL) : fObj(obj) {}
    ~SkAutoTUnref() { SkSafeUnref(fObj); }

    T* get() const { return fObj; }

    T* reset(T* obj) {
        SkSafeUnref(fObj);
        fObj = obj;
        return obj;
    }

    void swap(SkAutoTUnref* other) {
        T* tmp = fObj;
        fObj = other->fObj;
        other->fObj = tmp;
    }

    





    T* detach() {
        T* obj = fObj;
        fObj = NULL;
        return obj;
    }

    



    template<typename B> class BlockRef : public B {
    private:
        BlockRef();
        ~BlockRef();
        void ref() const;
        void unref() const;
    };

    
    typedef typename SkTConstType<BlockRef<T>, SkTIsConst<T>::value>::type BlockRefType;

    





    BlockRefType *operator->() const {
        return static_cast<BlockRefType*>(fObj);
    }
    operator T*() { return fObj; }

private:
    T*  fObj;
};


class SkAutoUnref : public SkAutoTUnref<SkRefCnt> {
public:
    SkAutoUnref(SkRefCnt* obj) : SkAutoTUnref<SkRefCnt>(obj) {}
};
#define SkAutoUnref(...) SK_REQUIRE_LOCAL_VAR(SkAutoUnref)

#endif
