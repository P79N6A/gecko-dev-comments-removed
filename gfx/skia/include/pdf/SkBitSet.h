








#ifndef SkBitSet_DEFINED
#define SkBitSet_DEFINED

#include "SkTypes.h"
#include "SkTDArray.h"

class SkBitSet {
public:
    

    explicit SkBitSet(int numberOfBits);
    explicit SkBitSet(const SkBitSet& source);

    const SkBitSet& operator=(const SkBitSet& rhs);
    bool operator==(const SkBitSet& rhs);
    bool operator!=(const SkBitSet& rhs);

    

    void clearAll();

    

    void setBit(int index, bool value);

    

    bool isBitSet(int index) const;

    


    bool orBits(const SkBitSet& source);

    

    template<typename T>
    void exportTo(SkTDArray<T>* array) const {
        SkASSERT(array);
        uint32_t* data = reinterpret_cast<uint32_t*>(fBitData.get());
        for (unsigned int i = 0; i < fDwordCount; ++i) {
            uint32_t value = data[i];
            if (value) {  
                unsigned int index = i * 32;
                for (unsigned int j = 0; j < 32; ++j) {
                    if (0x1 & (value >> j)) {
                        array->push(index + j);
                    }
                }
            }
        }
    }

private:
    SkAutoFree fBitData;
    
    size_t fDwordCount;
    size_t fBitCount;

    uint32_t* internalGet(int index) const {
        SkASSERT((size_t)index < fBitCount);
        size_t internalIndex = index / 32;
        SkASSERT(internalIndex < fDwordCount);
        return reinterpret_cast<uint32_t*>(fBitData.get()) + internalIndex;
    }
};


#endif
