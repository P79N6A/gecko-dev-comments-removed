









#ifndef GrTDArray_DEFINED
#define GrTDArray_DEFINED

#include "GrTypes.h"
#include "GrRefCnt.h"

static int GrInitialArrayAllocationCount() {
    return 4;
}

static int GrNextArrayAllocationCount(int count) {
    return count + ((count + 1) >> 1);
}

template <typename T> class GrTDArray {
public:
    GrTDArray() : fArray(NULL), fAllocated(0), fCount(0) {}
    GrTDArray(const GrTDArray& src) {
        fCount = fAllocated = src.fCount;
        fArray = (T*)GrMalloc(fAllocated * sizeof(T));
        memcpy(fArray, src.fArray, fCount * sizeof(T));
    }
    ~GrTDArray() {
        if (fArray) {
            GrFree(fArray);
        }
    }

    bool isEmpty() const { return 0 == fCount; }
    int count() const { return fCount; }

    const T& at(int index) const {
        GrAssert((unsigned)index < (unsigned)fCount);
        return fArray[index];
    }
    T& at(int index) {
        GrAssert((unsigned)index < (unsigned)fCount);
        return fArray[index];
    }

    const T& operator[](int index) const { return this->at(index); }
    T& operator[](int index) { return this->at(index); }

    GrTDArray& operator=(const GrTDArray& src) {
        if (fAllocated < src.fCount) {
            fAllocated = src.fCount;
            GrFree(fArray);
            fArray = (T*)GrMalloc(fAllocated * sizeof(T));
        }
        fCount = src.fCount;
        memcpy(fArray, src.fArray, fCount * sizeof(T));
        return *this;
    }

    void reset() {
        if (fArray) {
            GrFree(fArray);
            fArray = NULL;
        }
        fAllocated = fCount = 0;
    }

    T* begin() const { return fArray; }
    T* end() const { return fArray + fCount; }
    T* back() const { GrAssert(fCount); return fArray + (fCount - 1); } 

    T* prepend() {
        this->growAt(0);
        return fArray;
    }

    T* append() {
        this->growAt(fCount);
        return fArray + fCount - 1;
    }

    


    T* insert(int index) {
        GrAssert((unsigned)index <= (unsigned)fCount);
        this->growAt(index);
        return fArray + index;
    }

    void remove(int index) {
        GrAssert((unsigned)index < (unsigned)fCount);
        fCount -= 1;
        if (index < fCount) {
            int remaining = fCount - index;
            memmove(fArray + index, fArray + index + 1, remaining * sizeof(T));
        }
    }

    void removeShuffle(int index) {
        GrAssert((unsigned)index < (unsigned)fCount);
        fCount -= 1;
        if (index < fCount) {
            memmove(fArray + index, fArray + fCount, sizeof(T));
        }
    }

    

    



    void freeAll() {
        T* stop = this->end();
        for (T* curr = this->begin(); curr < stop; curr++) {
            GrFree(*curr);
        }
        this->reset();
    }

    



    void deleteAll() {
        T* stop = this->end();
        for (T* curr = this->begin(); curr < stop; curr++) {
            delete *curr;
        }
        this->reset();
    }

    



    void unrefAll() {
        T* stop = this->end();
        for (T* curr = this->begin(); curr < stop; curr++) {
            GrSafeUnref(*curr);
        }
        this->reset();
    }

    void visit(void visitor(T&)) const {
        T* stop = this->end();
        for (T* curr = this->begin(); curr < stop; curr++) {
            if (*curr) {
                visitor(*curr);
            }
        }
    }

    int find(const T& elem) const {
        int count = this->count();
        T* curr = this->begin();
        for (int i = 0; i < count; i++) {
            if (elem == curr[i]) {
                return i;
            }
        }
        return -1;
    }

    friend bool operator==(const GrTDArray<T>& a, const GrTDArray<T>& b) {
        return a.count() == b.count() &&
               (0 == a.count() ||
                0 == memcmp(a.begin(), b.begin(), a.count() * sizeof(T)));
    }
    friend bool operator!=(const GrTDArray<T>& a, const GrTDArray<T>& b) {
        return !(a == b);
    }

private:
    T*  fArray;
    int fAllocated, fCount;

    
    
    void growAt(int index) {
        GrAssert(fCount <= fAllocated);
        if (0 == fAllocated) {
            fAllocated = GrInitialArrayAllocationCount();
            fArray = (T*)GrMalloc(fAllocated * sizeof(T));
        } else if (fCount == fAllocated) {
            fAllocated = GrNextArrayAllocationCount(fAllocated);
            T* newArray = (T*)GrMalloc(fAllocated * sizeof(T));
            memcpy(newArray, fArray, index * sizeof(T));
            memcpy(newArray + index + 1, fArray + index,
                   (fCount - index) * sizeof(T));
            GrFree(fArray);
            fArray = newArray;
        } else {
            
            if (index < fCount) {
                memmove(fArray + index + 1, fArray + index,
                        (fCount - index) * sizeof(T));
            }
        }
        GrAssert(fCount < fAllocated);
        fCount += 1;
    }
};

extern void* GrTDArray_growAt(void*, int* allocated, int& count, int index,
                              size_t);


#endif

