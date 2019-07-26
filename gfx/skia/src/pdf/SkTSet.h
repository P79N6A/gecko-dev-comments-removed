






#ifndef SkTSet_DEFINED
#define SkTSet_DEFINED

#include "SkTDArray.h"
#include "SkTypes.h"











template <typename T> class SK_API SkTSet {
public:
    SkTSet() {
        fArray = SkNEW(SkTDArray<T>);
    }

    ~SkTSet() {
        SkASSERT(fArray);
        SkDELETE(fArray);
    }

    SkTSet(const SkTSet<T>& src) {
        this->fArray = SkNEW_ARGS(SkTDArray<T>, (*src.fArray));
#ifdef SK_DEBUG
        validate();
#endif
    }

    SkTSet<T>& operator=(const SkTSet<T>& src) {
        *this->fArray = *src.fArray;
#ifdef SK_DEBUG
        validate();
#endif
        return *this;
    }

    


    int mergeInto(const SkTSet<T>& src) {
        SkASSERT(fArray);
        int duplicates = 0;

        SkTDArray<T>* fArrayNew = new SkTDArray<T>();
        fArrayNew->setReserve(count() + src.count());
        int i = 0;
        int j = 0;

        while (i < count() && j < src.count()) {
            if ((*fArray)[i] < (*src.fArray)[j]) {
                fArrayNew->push((*fArray)[i]);
                i++;
            } else if ((*fArray)[i] > (*src.fArray)[j]) {
                fArrayNew->push((*src.fArray)[j]);
                j++;
            } else {
                duplicates++;
                j++; 
            }
        }

        while (i < count()) {
            fArrayNew->push((*fArray)[i]);
            i++;
        }

        while (j < src.count()) {
            fArrayNew->push((*src.fArray)[j]);
            j++;
        }
        SkDELETE(fArray);
        fArray = fArrayNew;
        fArrayNew = NULL;

#ifdef SK_DEBUG
        validate();
#endif
        return duplicates;
    }

    


    bool add(const T& elem) {
        SkASSERT(fArray);

        int pos = 0;
        int i = find(elem, &pos);
        if (i >= 0) {
            return false;
        }
        *fArray->insert(pos) = elem;
#ifdef SK_DEBUG
        validate();
#endif
        return true;
    }

    

    bool isEmpty() const {
        SkASSERT(fArray);
        return fArray->isEmpty();
    }

    

    int count() const {
        SkASSERT(fArray);
        return fArray->count();
    }

    

    size_t bytes() const {
        SkASSERT(fArray);
        return fArray->bytes();
    }

    


    const T*  begin() const {
        SkASSERT(fArray);
        return fArray->begin();
    }

    

    const T*  end() const {
        SkASSERT(fArray);
        return fArray->end();
    }

    const T&  operator[](int index) const {
        SkASSERT(fArray);
        return (*fArray)[index];
    }

    

    void reset() {
        SkASSERT(fArray);
        fArray->reset();
    }

    

    void rewind() {
        SkASSERT(fArray);
        fArray->rewind();
    }

    

    void setReserve(size_t reserve) {
        SkASSERT(fArray);
        fArray->setReserve(reserve);
    }

    





    int find(const T& elem, int* posToInsertSorted = NULL) const {
        SkASSERT(fArray);

        if (fArray->count() == 0) {
            if (posToInsertSorted) {
                *posToInsertSorted = 0;
            }
            return -1;
        }
        int iMin = 0;
        int iMax = fArray->count();

        while (iMin < iMax - 1) {
            int iMid = (iMin + iMax) / 2;
            if (elem < (*fArray)[iMid]) {
                iMax = iMid;
            } else {
                iMin = iMid;
            }
        }
        if (elem == (*fArray)[iMin]) {
            return iMin;
        }
        if (posToInsertSorted) {
            if (elem < (*fArray)[iMin]) {
                *posToInsertSorted = iMin;
            } else {
                *posToInsertSorted = iMin + 1;
            }
        }

        return -1;
    }

    

    bool contains(const T& elem) const {
        SkASSERT(fArray);
        return (this->find(elem) >= 0);
    }

    

    void copy(T* dst) const {
        SkASSERT(fArray);
        fArray->copyRange(0, fArray->count(), dst);
    }

    

    const SkTDArray<T>& toArray() {
        SkASSERT(fArray);
        return *fArray;
    }

    

    void unrefAll() {
        SkASSERT(fArray);
        fArray->unrefAll();
    }

    

     void safeUnrefAll() {
        SkASSERT(fArray);
        fArray->safeUnrefAll();
    }

#ifdef SK_DEBUG
    void validate() const {
        SkASSERT(fArray);
        fArray->validate();
        SkASSERT(isSorted() && !hasDuplicates());
    }

    bool hasDuplicates() const {
        for (int i = 0; i < fArray->count() - 1; ++i) {
            if ((*fArray)[i] == (*fArray)[i + 1]) {
                return true;
            }
        }
        return false;
    }

    bool isSorted() const {
        for (int i = 0; i < fArray->count() - 1; ++i) {
            
            if (!((*fArray)[i] < (*fArray)[i + 1])) {
                return false;
            }
        }
        return true;
    }
#endif

private:
    SkTDArray<T>* fArray;
};

#endif
