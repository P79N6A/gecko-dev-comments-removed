






#ifndef SkTInternalSList_DEFINED
#define SkTInternalSList_DEFINED

#include "SkTInternalLList.h" 














#define SK_DECLARE_INTERNAL_SLIST_ADAPTER(ClassName, field)     \
    ClassName* getSListNext() {                                 \
        return this->field;                                     \
    }                                                           \
    void setSListNext(ClassName* next) {                        \
        this->field = next;                                     \
    }                                                           \
    friend class SkTInternalSList<ClassName>








#define SK_DECLARE_INTERNAL_SLIST_INTERFACE(ClassName)          \
    SK_DECLARE_INTERNAL_SLIST_ADAPTER(ClassName, fSListNext);   \
    SkPtrWrapper<ClassName> fSListNext









template<typename T> class SkTInternalSList {
public:
    SkTInternalSList() : fHead(NULL), fCount(0) {}

    



    void push(T* entry) {
        SkASSERT(entry->getSListNext() == NULL);
        entry->setSListNext(fHead);
        fHead = entry;
        ++fCount;
    }

    




    void pushAll(SkTInternalSList<T>* other) {
        if (this->isEmpty()) {
            this->swap(other);
            return;
        }
        while (!other->isEmpty()) {
            this->push(other->pop());
        }
    }

    




    T* pop() {
        if (NULL == fHead) {
            return NULL;
        }
        T* result = fHead;
        fHead = result->getSListNext();
        result->setSListNext(NULL);
        --fCount;
        return result;
    }

    T* head() const {
        return fHead;
    }

    


    bool isEmpty() const {
        return NULL == fHead;
    }

    



    void swap(SkTInternalSList<T>* other) {
        SkTSwap(fHead, other->fHead);
        SkTSwap(fCount, other->fCount);
    }

    


    int getCount() const {
        return fCount;
    }
private:
    T* fHead;
    int fCount;
};


#endif
