








#ifndef SkRandom_DEFINED
#define SkRandom_DEFINED

#include "Sk64.h"
#include "SkScalar.h"







class SkRandom {
public:
    SkRandom() : fSeed(0) {}
    SkRandom(uint32_t seed) : fSeed(seed) {}

    

    uint32_t nextU() { uint32_t r = fSeed * kMul + kAdd; fSeed = r; return r; }

    

    int32_t nextS() { return (int32_t)this->nextU(); }

    

    U16CPU nextU16() { return this->nextU() >> 16; }

    

    S16CPU nextS16() { return this->nextS() >> 16; }

    



    uint32_t nextBits(unsigned bitCount) {
        SkASSERT(bitCount > 0 && bitCount <= 32);
        return this->nextU() >> (32 - bitCount);
    }

    


    uint32_t nextRangeU(uint32_t min, uint32_t max) {
        SkASSERT(min <= max);
        return min + this->nextU() % (max - min + 1);
    }

    


    SkFixed nextUFixed1() { return this->nextU() >> 16; }

    


    SkFixed nextSFixed1() { return this->nextS() >> 15; }

    


    SkScalar nextUScalar1() { return SkFixedToScalar(this->nextUFixed1()); }

    


    SkScalar nextSScalar1() { return SkFixedToScalar(this->nextSFixed1()); }

    

    void next64(Sk64* a) {
        SkASSERT(a);
        a->set(this->nextS(), this->nextU());
    }

    



    void setSeed(int32_t seed) { fSeed = (uint32_t)seed; }

private:
    
    enum {
        kMul = 1664525,
        kAdd = 1013904223
    };
    uint32_t fSeed;
};

#endif

