








#ifndef SkPtrSet_DEFINED
#define SkPtrSet_DEFINED

#include "SkRefCnt.h"
#include "SkTDArray.h"







class SkPtrSet : public SkRefCnt {
public:
    



    uint32_t find(void*) const;

    





    uint32_t add(void*);
    
    


    int count() const { return fList.count(); }

    






    void copyToArray(void* array[]) const;

    



    void reset();

protected:
    virtual void incPtr(void* ptr) {}
    virtual void decPtr(void* ptr) {}

private:
    struct Pair {
        void*       fPtr;   
        uint32_t    fIndex; 
    };

    
    
    
    
    SkTDArray<Pair>  fList;
    
    static int Cmp(const Pair& a, const Pair& b);
    
    typedef SkRefCnt INHERITED;
};





template <typename T> class SkTPtrSet : public SkPtrSet {
public:
    uint32_t find(T ptr) {
        return this->INHERITED::find((void*)ptr);
    }
    uint32_t add(T ptr) {
        return this->INHERITED::add((void*)ptr);
    }
    
    void copyToArray(T* array) const {
        this->INHERITED::copyToArray((void**)array);
    }

private:
    typedef SkPtrSet INHERITED;
};

#endif
