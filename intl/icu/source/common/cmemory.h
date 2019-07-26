






















#ifndef CMEMORY_H
#define CMEMORY_H

#include "unicode/utypes.h"

#include <stddef.h>
#include <string.h>
#include "unicode/localpointer.h"

#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
#include <stdio.h>
#endif

#if U_DEBUG








U_CAPI void uprv_checkValidMemory(const void *p, size_t n);

#define uprv_memcpy(dst, src, size) ( \
    uprv_checkValidMemory(src, 1), \
    U_STANDARD_CPP_NAMESPACE memcpy(dst, src, size))
#define uprv_memmove(dst, src, size) ( \
    uprv_checkValidMemory(src, 1), \
    U_STANDARD_CPP_NAMESPACE memmove(dst, src, size))

#else

#define uprv_memcpy(dst, src, size) U_STANDARD_CPP_NAMESPACE memcpy(dst, src, size)
#define uprv_memmove(dst, src, size) U_STANDARD_CPP_NAMESPACE memmove(dst, src, size)

#endif  

#define uprv_memset(buffer, mark, size) U_STANDARD_CPP_NAMESPACE memset(buffer, mark, size)
#define uprv_memcmp(buffer1, buffer2, size) U_STANDARD_CPP_NAMESPACE memcmp(buffer1, buffer2,size)

U_CAPI void * U_EXPORT2
uprv_malloc(size_t s) U_MALLOC_ATTR U_ALLOC_SIZE_ATTR(1);

U_CAPI void * U_EXPORT2
uprv_realloc(void *mem, size_t size) U_ALLOC_SIZE_ATTR(2);

U_CAPI void U_EXPORT2
uprv_free(void *mem);

U_CAPI void * U_EXPORT2
uprv_calloc(size_t num, size_t size) U_MALLOC_ATTR U_ALLOC_SIZE_ATTR2(1,2);





typedef union {
    long    t1;
    double  t2;
    void   *t3;
} UAlignedMemory;










#define U_POINTER_MASK_LSB(ptr, mask) (((ptrdiff_t)(char *)(ptr)) & (mask))





#define U_ALIGNMENT_OFFSET(ptr) U_POINTER_MASK_LSB(ptr, sizeof(UAlignedMemory) - 1)





#define U_ALIGNMENT_OFFSET_UP(ptr) (sizeof(UAlignedMemory) - U_ALIGNMENT_OFFSET(ptr))





U_CFUNC UBool 
cmemory_inUse(void);






U_CFUNC UBool 
cmemory_cleanup(void);








typedef void U_CALLCONV UObjectDeleter(void* obj);





U_CAPI void U_EXPORT2
uprv_deleteUObject(void *obj);

#ifdef __cplusplus

U_NAMESPACE_BEGIN








template<typename T>
class LocalMemory : public LocalPointerBase<T> {
public:
    



    explicit LocalMemory(T *p=NULL) : LocalPointerBase<T>(p) {}
    


    ~LocalMemory() {
        uprv_free(LocalPointerBase<T>::ptr);
    }
    




    void adoptInstead(T *p) {
        uprv_free(LocalPointerBase<T>::ptr);
        LocalPointerBase<T>::ptr=p;
    }
    







    inline T *allocateInsteadAndReset(int32_t newCapacity=1);
    










    inline T *allocateInsteadAndCopy(int32_t newCapacity=1, int32_t length=0);
    





    T &operator[](ptrdiff_t i) const { return LocalPointerBase<T>::ptr[i]; }
};

template<typename T>
inline T *LocalMemory<T>::allocateInsteadAndReset(int32_t newCapacity) {
    if(newCapacity>0) {
        T *p=(T *)uprv_malloc(newCapacity*sizeof(T));
        if(p!=NULL) {
            uprv_memset(p, 0, newCapacity*sizeof(T));
            uprv_free(LocalPointerBase<T>::ptr);
            LocalPointerBase<T>::ptr=p;
        }
        return p;
    } else {
        return NULL;
    }
}


template<typename T>
inline T *LocalMemory<T>::allocateInsteadAndCopy(int32_t newCapacity, int32_t length) {
    if(newCapacity>0) {
        T *p=(T *)uprv_malloc(newCapacity*sizeof(T));
        if(p!=NULL) {
            if(length>0) {
                if(length>newCapacity) {
                    length=newCapacity;
                }
                uprv_memcpy(p, LocalPointerBase<T>::ptr, length*sizeof(T));
            }
            uprv_free(LocalPointerBase<T>::ptr);
            LocalPointerBase<T>::ptr=p;
        }
        return p;
    } else {
        return NULL;
    }
}












template<typename T, int32_t stackCapacity>
class MaybeStackArray {
public:
    


    MaybeStackArray() : ptr(stackArray), capacity(stackCapacity), needToRelease(FALSE) {}
    


    ~MaybeStackArray() { releaseArray(); }
    



    int32_t getCapacity() const { return capacity; }
    



    T *getAlias() const { return ptr; }
    



    T *getArrayLimit() const { return getAlias()+capacity; }
    
    
    





    const T &operator[](ptrdiff_t i) const { return ptr[i]; }
    





    T &operator[](ptrdiff_t i) { return ptr[i]; }
    





    void aliasInstead(T *otherArray, int32_t otherCapacity) {
        if(otherArray!=NULL && otherCapacity>0) {
            releaseArray();
            ptr=otherArray;
            capacity=otherCapacity;
            needToRelease=FALSE;
        }
    }
    









    inline T *resize(int32_t newCapacity, int32_t length=0);
    









    inline T *orphanOrClone(int32_t length, int32_t &resultCapacity);
private:
    T *ptr;
    int32_t capacity;
    UBool needToRelease;
    T stackArray[stackCapacity];
    void releaseArray() {
        if(needToRelease) {
            uprv_free(ptr);
        }
    }
    
    bool operator==(const MaybeStackArray & ) {return FALSE;}
    bool operator!=(const MaybeStackArray & ) {return TRUE;}
    
    MaybeStackArray(const MaybeStackArray & ) {}
    void operator=(const MaybeStackArray & ) {}

    
    
    
    
    
    
    
    
    
    
#if U_HAVE_PLACEMENT_NEW
    
#endif
};

template<typename T, int32_t stackCapacity>
inline T *MaybeStackArray<T, stackCapacity>::resize(int32_t newCapacity, int32_t length) {
    if(newCapacity>0) {
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
      ::fprintf(::stderr,"MaybeStacArray (resize) alloc %d * %lu\n", newCapacity,sizeof(T));
#endif
        T *p=(T *)uprv_malloc(newCapacity*sizeof(T));
        if(p!=NULL) {
            if(length>0) {
                if(length>capacity) {
                    length=capacity;
                }
                if(length>newCapacity) {
                    length=newCapacity;
                }
                uprv_memcpy(p, ptr, length*sizeof(T));
            }
            releaseArray();
            ptr=p;
            capacity=newCapacity;
            needToRelease=TRUE;
        }
        return p;
    } else {
        return NULL;
    }
}

template<typename T, int32_t stackCapacity>
inline T *MaybeStackArray<T, stackCapacity>::orphanOrClone(int32_t length, int32_t &resultCapacity) {
    T *p;
    if(needToRelease) {
        p=ptr;
    } else if(length<=0) {
        return NULL;
    } else {
        if(length>capacity) {
            length=capacity;
        }
        p=(T *)uprv_malloc(length*sizeof(T));
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
      ::fprintf(::stderr,"MaybeStacArray (orphan) alloc %d * %lu\n", length,sizeof(T));
#endif
        if(p==NULL) {
            return NULL;
        }
        uprv_memcpy(p, ptr, length*sizeof(T));
    }
    resultCapacity=length;
    ptr=stackArray;
    capacity=stackCapacity;
    needToRelease=FALSE;
    return p;
}











template<typename H, typename T, int32_t stackCapacity>
class MaybeStackHeaderAndArray {
public:
    


    MaybeStackHeaderAndArray() : ptr(&stackHeader), capacity(stackCapacity), needToRelease(FALSE) {}
    


    ~MaybeStackHeaderAndArray() { releaseMemory(); }
    



    int32_t getCapacity() const { return capacity; }
    



    H *getAlias() const { return ptr; }
    



    T *getArrayStart() const { return reinterpret_cast<T *>(getAlias()+1); }
    



    T *getArrayLimit() const { return getArrayStart()+capacity; }
    




    operator H *() const { return ptr; }
    





    T &operator[](ptrdiff_t i) { return getArrayStart()[i]; }
    





    void aliasInstead(H *otherMemory, int32_t otherCapacity) {
        if(otherMemory!=NULL && otherCapacity>0) {
            releaseMemory();
            ptr=otherMemory;
            capacity=otherCapacity;
            needToRelease=FALSE;
        }
    }
    










    inline H *resize(int32_t newCapacity, int32_t length=0);
    









    inline H *orphanOrClone(int32_t length, int32_t &resultCapacity);
private:
    H *ptr;
    int32_t capacity;
    UBool needToRelease;
    
    H stackHeader;
    T stackArray[stackCapacity];
    void releaseMemory() {
        if(needToRelease) {
            uprv_free(ptr);
        }
    }
    
    bool operator==(const MaybeStackHeaderAndArray & ) {return FALSE;}
    bool operator!=(const MaybeStackHeaderAndArray & ) {return TRUE;}
    
    MaybeStackHeaderAndArray(const MaybeStackHeaderAndArray & ) {}
    void operator=(const MaybeStackHeaderAndArray & ) {}

    
    
    
    
    
#if U_HAVE_PLACEMENT_NEW
    
#endif
};

template<typename H, typename T, int32_t stackCapacity>
inline H *MaybeStackHeaderAndArray<H, T, stackCapacity>::resize(int32_t newCapacity,
                                                                int32_t length) {
    if(newCapacity>=0) {
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
      ::fprintf(::stderr,"MaybeStackHeaderAndArray alloc %d + %d * %ul\n", sizeof(H),newCapacity,sizeof(T));
#endif
        H *p=(H *)uprv_malloc(sizeof(H)+newCapacity*sizeof(T));
        if(p!=NULL) {
            if(length<0) {
                length=0;
            } else if(length>0) {
                if(length>capacity) {
                    length=capacity;
                }
                if(length>newCapacity) {
                    length=newCapacity;
                }
            }
            uprv_memcpy(p, ptr, sizeof(H)+length*sizeof(T));
            releaseMemory();
            ptr=p;
            capacity=newCapacity;
            needToRelease=TRUE;
        }
        return p;
    } else {
        return NULL;
    }
}

template<typename H, typename T, int32_t stackCapacity>
inline H *MaybeStackHeaderAndArray<H, T, stackCapacity>::orphanOrClone(int32_t length,
                                                                       int32_t &resultCapacity) {
    H *p;
    if(needToRelease) {
        p=ptr;
    } else {
        if(length<0) {
            length=0;
        } else if(length>capacity) {
            length=capacity;
        }
#if U_DEBUG && defined(UPRV_MALLOC_COUNT)
      ::fprintf(::stderr,"MaybeStackHeaderAndArray (orphan) alloc %ul + %d * %lu\n", sizeof(H),length,sizeof(T));
#endif
        p=(H *)uprv_malloc(sizeof(H)+length*sizeof(T));
        if(p==NULL) {
            return NULL;
        }
        uprv_memcpy(p, ptr, sizeof(H)+length*sizeof(T));
    }
    resultCapacity=length;
    ptr=&stackHeader;
    capacity=stackCapacity;
    needToRelease=FALSE;
    return p;
}

U_NAMESPACE_END

#endif  
#endif  
