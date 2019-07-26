






#ifndef SkTSet_DEFINED
#define SkTSet_DEFINED

#include "SkTSort.h"
#include "SkTDArray.h"
#include "SkTypes.h"












template <typename T> class SkTSet {
public:
    SkTSet() {
        fSetArray = SkNEW(SkTDArray<T>);
        fOrderedArray = SkNEW(SkTDArray<T>);
    }

    ~SkTSet() {
        SkASSERT(fSetArray);
        SkDELETE(fSetArray);
        SkASSERT(fOrderedArray);
        SkDELETE(fOrderedArray);
    }

    SkTSet(const SkTSet<T>& src) {
        this->fSetArray = SkNEW_ARGS(SkTDArray<T>, (*src.fSetArray));
        this->fOrderedArray = SkNEW_ARGS(SkTDArray<T>, (*src.fOrderedArray));
#ifdef SK_DEBUG
        validate();
#endif
    }

    SkTSet<T>& operator=(const SkTSet<T>& src) {
        *this->fSetArray = *src.fSetArray;
        *this->fOrderedArray = *src.fOrderedArray;
#ifdef SK_DEBUG
        validate();
#endif
        return *this;
    }

    








    int mergeInto(const SkTSet<T>& src) {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);

        
        for (int i = 0; i < src.count(); ++i) {
            if (!contains((*src.fOrderedArray)[i])) {
                fOrderedArray->push((*src.fOrderedArray)[i]);
            }
        }

        
        int duplicates = 0;

        SkTDArray<T>* fArrayNew = new SkTDArray<T>();
        fArrayNew->setReserve(fOrderedArray->count());
        int i = 0;
        int j = 0;

        while (i < fSetArray->count() && j < src.count()) {
            if ((*fSetArray)[i] < (*src.fSetArray)[j]) {
                fArrayNew->push((*fSetArray)[i]);
                i++;
            } else if ((*fSetArray)[i] > (*src.fSetArray)[j]) {
                fArrayNew->push((*src.fSetArray)[j]);
                j++;
            } else {
                duplicates++;
                j++; 
            }
        }

        while (i < fSetArray->count()) {
            fArrayNew->push((*fSetArray)[i]);
            i++;
        }

        while (j < src.count()) {
            fArrayNew->push((*src.fSetArray)[j]);
            j++;
        }
        SkDELETE(fSetArray);
        fSetArray = fArrayNew;
        fArrayNew = NULL;

#ifdef SK_DEBUG
        validate();
#endif
        return duplicates;
    }

    


    bool add(const T& elem) {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);

        int pos = 0;
        int i = find(elem, &pos);
        if (i >= 0) {
            return false;
        }
        *fSetArray->insert(pos) = elem;
        fOrderedArray->push(elem);
#ifdef SK_DEBUG
        validate();
#endif
        return true;
    }

    

    bool isEmpty() const {
        SkASSERT(fOrderedArray);
        SkASSERT(fSetArray);
        SkASSERT(fSetArray->isEmpty() == fOrderedArray->isEmpty());
        return fOrderedArray->isEmpty();
    }

    

    int count() const {
        SkASSERT(fOrderedArray);
        SkASSERT(fSetArray);
        SkASSERT(fSetArray->count() == fOrderedArray->count());
        return fOrderedArray->count();
    }

    

    size_t bytes() const {
        SkASSERT(fOrderedArray);
        return fOrderedArray->bytes();
    }

    


    const T*  begin() const {
        SkASSERT(fOrderedArray);
        return fOrderedArray->begin();
    }

    

    const T*  end() const {
        SkASSERT(fOrderedArray);
        return fOrderedArray->end();
    }

    const T&  operator[](int index) const {
        SkASSERT(fOrderedArray);
        return (*fOrderedArray)[index];
    }

    

    void reset() {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);
        fSetArray->reset();
        fOrderedArray->reset();
    }

    

    void rewind() {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);
        fSetArray->rewind();
        fOrderedArray->rewind();
    }

    

    void setReserve(int reserve) {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);
        fSetArray->setReserve(reserve);
        fOrderedArray->setReserve(reserve);
    }

    

    bool contains(const T& elem) const {
        SkASSERT(fSetArray);
        return (this->find(elem) >= 0);
    }

    

    void copy(T* dst) const {
        SkASSERT(fOrderedArray);
        fOrderedArray->copyRange(dst, 0, fOrderedArray->count());
    }

    

    const SkTDArray<T>& toArray() {
        SkASSERT(fOrderedArray);
        return *fOrderedArray;
    }

    

    void unrefAll() {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);
        fOrderedArray->unrefAll();
        
        
        fSetArray->reset();
    }

    

    void safeUnrefAll() {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);
        fOrderedArray->safeUnrefAll();
        
        
        fSetArray->reset();
    }

#ifdef SK_DEBUG
    void validate() const {
        SkASSERT(fSetArray);
        SkASSERT(fOrderedArray);
        fSetArray->validate();
        fOrderedArray->validate();
        SkASSERT(isSorted() && !hasDuplicates() && arraysConsistent());
    }

    bool hasDuplicates() const {
        for (int i = 0; i < fSetArray->count() - 1; ++i) {
            if ((*fSetArray)[i] == (*fSetArray)[i + 1]) {
                return true;
            }
        }
        return false;
    }

    bool isSorted() const {
        for (int i = 0; i < fSetArray->count() - 1; ++i) {
            
            if (!((*fSetArray)[i] < (*fSetArray)[i + 1])) {
                return false;
            }
        }
        return true;
    }

    

    bool arraysConsistent() const {
        if (fSetArray->count() != fOrderedArray->count()) {
            return false;
        }
        if (fOrderedArray->count() == 0) {
            return true;
        }

        
        
        SkAutoMalloc sortedArray(fOrderedArray->bytes());
        T* sortedBase = reinterpret_cast<T*>(sortedArray.get());
        int count = fOrderedArray->count();
        fOrderedArray->copyRange(sortedBase, 0, count);

        SkTQSort<T>(sortedBase, sortedBase + count - 1);

        for (int i = 0; i < count; ++i) {
            if (sortedBase[i] != (*fSetArray)[i]) {
                return false;
            }
        }

        return true;
    }
#endif

private:
    SkTDArray<T>* fSetArray;        
                                    
    SkTDArray<T>* fOrderedArray;    
                                    

    





    int find(const T& elem, int* posToInsertSorted = NULL) const {
        SkASSERT(fSetArray);

        if (fSetArray->count() == 0) {
            if (posToInsertSorted) {
                *posToInsertSorted = 0;
            }
            return -1;
        }
        int iMin = 0;
        int iMax = fSetArray->count();

        while (iMin < iMax - 1) {
            int iMid = (iMin + iMax) / 2;
            if (elem < (*fSetArray)[iMid]) {
                iMax = iMid;
            } else {
                iMin = iMid;
            }
        }
        if (elem == (*fSetArray)[iMin]) {
            return iMin;
        }
        if (posToInsertSorted) {
            if (elem < (*fSetArray)[iMin]) {
                *posToInsertSorted = iMin;
            } else {
                *posToInsertSorted = iMin + 1;
            }
        }

        return -1;
    }
};

#endif
