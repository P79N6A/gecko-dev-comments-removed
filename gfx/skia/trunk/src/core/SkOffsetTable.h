






#ifndef SkOffsetTable_DEFINED
#define SkOffsetTable_DEFINED

#include "SkRefCnt.h"
#include "SkTDArray.h"









class SkOffsetTable : public SkRefCnt {
public:
    SkOffsetTable() {}
    ~SkOffsetTable() {
        fOffsetArrays.deleteAll();
    }

    
    
    void add(int id, size_t offset) {
        if (id >= fOffsetArrays.count()) {
            int oldCount = fOffsetArrays.count();
            fOffsetArrays.setCount(id+1);
            for (int i = oldCount; i <= id; ++i) {
                fOffsetArrays[i] = NULL;
            }
        }

        if (NULL == fOffsetArrays[id]) {
            fOffsetArrays[id] = SkNEW(OffsetArray);
        }
        fOffsetArrays[id]->add(offset);
    }

    int numIDs() const {
        return fOffsetArrays.count();
    }

    
    
    bool overlap(int id, size_t min, size_t max) {
        SkASSERT(id < fOffsetArrays.count());

        if (NULL == fOffsetArrays[id]) {
            return false;
        }

        
        SkASSERT(fOffsetArrays[id]->count() > 0);
        if (max < fOffsetArrays[id]->min() || min > fOffsetArrays[id]->max()) {
            return false;
        }

        return true;
    }

    bool includes(int id, size_t offset) {
        SkASSERT(id < fOffsetArrays.count());

        OffsetArray* array = fOffsetArrays[id];

        for (int i = 0; i < array->fOffsets.count(); ++i) {
            if (array->fOffsets[i] == offset) {
                return true;
            } else if (array->fOffsets[i] > offset) {
                return false;
            }
        }

        
        
        SkASSERT(0);
        return false;
    }

protected:
    class OffsetArray {
    public:
        void add(size_t offset) {
            SkASSERT(fOffsets.count() == 0 || offset > this->max());
            *fOffsets.append() = offset;
        }
        size_t min() const {
            SkASSERT(fOffsets.count() > 0);
            return fOffsets[0];
        }
        size_t max() const {
            SkASSERT(fOffsets.count() > 0);
            return fOffsets[fOffsets.count()-1];
        }
        int count() const {
            return fOffsets.count();
        }

        SkTDArray<size_t> fOffsets;
    };

    SkTDArray<OffsetArray*> fOffsetArrays;

private:
    typedef SkRefCnt INHERITED;
};

#endif
