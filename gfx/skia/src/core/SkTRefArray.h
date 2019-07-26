







#ifndef SkTRefArray_DEFINED
#define SkTRefArray_DEFINED

#include "SkRefCnt.h"
#include <new>





template <typename T> class SkTRefArray : public SkRefCnt {
    




    static SkTRefArray<T>* Alloc(int count) {
        
        size_t size = sizeof(SkTRefArray<T>) + count * sizeof(T);
        SkTRefArray<T>* obj = (SkTRefArray<T>*)sk_malloc_throw(size);

        SkNEW_PLACEMENT(obj, SkTRefArray<T>);
        obj->fCount = count;
        return obj;
    }

public:
    




    static SkTRefArray<T>* Create(int count) {
        SkTRefArray<T>* obj = Alloc(count);
        T* array = const_cast<T*>(obj->begin());
        for (int i = 0; i < count; ++i) {
            SkNEW_PLACEMENT(&array[i], T);
        }
        return obj;
    }

    




    static SkTRefArray<T>* Create(const T src[], int count) {
        SkTRefArray<T>* obj = Alloc(count);
        T* array = const_cast<T*>(obj->begin());
        for (int i = 0; i < count; ++i) {
            SkNEW_PLACEMENT_ARGS(&array[i], T, (src[i]));
        }
        return obj;
    }

    int count() const { return fCount; }
    const T* begin() const { return (const T*)(this + 1); }
    const T* end() const { return this->begin() + fCount; }
    const T& at(int index) const {
        SkASSERT((unsigned)index < (unsigned)fCount);
        return this->begin()[index];
    }
    const T& operator[](int index) const { return this->at(index); }

    
    

    T* writableBegin() {
        SkASSERT(1 == this->getRefCnt());
        return (T*)(this + 1);
    }
    T* writableEnd() {
        return this->writableBegin() + fCount;
    }
    T& writableAt(int index) {
        SkASSERT((unsigned)index < (unsigned)fCount);
        return this->writableBegin()[index];
    }

protected:
    virtual void internal_dispose() const SK_OVERRIDE {
        T* array = const_cast<T*>(this->begin());
        int n = fCount;

        for (int i = 0; i < n; ++i) {
            array->~T();
            array += 1;
        }

        this->internal_dispose_restore_refcnt_to_1();
        this->~SkTRefArray<T>();
        sk_free((void*)this);
    }

private:
    int fCount;

    
    virtual ~SkTRefArray() {}

    typedef SkRefCnt INHERITED;
};

#endif
