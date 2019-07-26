








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

    


    float nextF() {
        
        return (float)(this->nextU() * 2.32830644e-10);
    }

    


    float nextRangeF(float min, float max) {
        return min + this->nextF() * (max - min);
    }

    



    uint32_t nextBits(unsigned bitCount) {
        SkASSERT(bitCount > 0 && bitCount <= 32);
        return this->nextU() >> (32 - bitCount);
    }

    


    uint32_t nextRangeU(uint32_t min, uint32_t max) {
        SkASSERT(min <= max);
        uint32_t range = max - min + 1;
        if (0 == range) {
            return this->nextU();
        } else {
            return min + this->nextU() % range;
        }
    }

    


    uint32_t nextULessThan(uint32_t count) {
        SkASSERT(count > 0);
        return this->nextRangeU(0, count - 1);
    }

    


    SkFixed nextUFixed1() { return this->nextU() >> 16; }

    


    SkFixed nextSFixed1() { return this->nextS() >> 15; }

    


    SkScalar nextUScalar1() { return SkFixedToScalar(this->nextUFixed1()); }

    


    SkScalar nextRangeScalar(SkScalar min, SkScalar max) {
        return SkScalarMul(this->nextUScalar1(), (max - min)) + min;
    }

    


    SkScalar nextSScalar1() { return SkFixedToScalar(this->nextSFixed1()); }

    

    bool nextBool() { return this->nextU() >= 0x80000000; }

    

    bool nextBiasedBool(SkScalar fractionTrue) {
        SkASSERT(fractionTrue >= 0 && fractionTrue <= SK_Scalar1);
        return this->nextUScalar1() <= fractionTrue;
    }

    

    void next64(Sk64* a) {
        SkASSERT(a);
        a->set(this->nextS(), this->nextU());
    }

    



    int32_t getSeed() const { return fSeed; }

    



    void setSeed(int32_t seed) { fSeed = (uint32_t)seed; }

private:
    
    enum {
        kMul = 1664525,
        kAdd = 1013904223
    };
    uint32_t fSeed;
};









class SkMWCRandom {
public:
    SkMWCRandom() { init(0); }
    SkMWCRandom(uint32_t seed) { init(seed); }
    SkMWCRandom(const SkMWCRandom& rand) : fK(rand.fK), fJ(rand.fJ) {}

    SkMWCRandom& operator=(const SkMWCRandom& rand) {
        fK = rand.fK;
        fJ = rand.fJ;

        return *this;
    }

    

    uint32_t nextU() {
        fK = kKMul*(fK & 0xffff) + (fK >> 16);
        fJ = kJMul*(fJ & 0xffff) + (fJ >> 16);
        return (((fK << 16) | (fK >> 16)) + fJ);
    }

    

    int32_t nextS() { return (int32_t)this->nextU(); }

    

    U16CPU nextU16() { return this->nextU() >> 16; }

    

    S16CPU nextS16() { return this->nextS() >> 16; }

    


    float nextF() {
        unsigned int floatint = 0x3f800000 | (this->nextU() >> 9);
        float f = SkBits2Float(floatint) - 1.0f;
        return f;
    }

    


    float nextRangeF(float min, float max) {
        return min + this->nextF() * (max - min);
    }

    



    uint32_t nextBits(unsigned bitCount) {
        SkASSERT(bitCount > 0 && bitCount <= 32);
        return this->nextU() >> (32 - bitCount);
    }

    


    uint32_t nextRangeU(uint32_t min, uint32_t max) {
        SkASSERT(min <= max);
        uint32_t range = max - min + 1;
        if (0 == range) {
            return this->nextU();
        } else {
            return min + this->nextU() % range;
        }
    }

    


    uint32_t nextULessThan(uint32_t count) {
        SkASSERT(count > 0);
        return this->nextRangeU(0, count - 1);
    }

    


    SkFixed nextUFixed1() { return this->nextU() >> 16; }

    


    SkFixed nextSFixed1() { return this->nextS() >> 15; }

    


    SkScalar nextUScalar1() { return SkFixedToScalar(this->nextUFixed1()); }

    


    SkScalar nextRangeScalar(SkScalar min, SkScalar max) {
        return SkScalarMul(this->nextUScalar1(), (max - min)) + min;
    }

    


    SkScalar nextSScalar1() { return SkFixedToScalar(this->nextSFixed1()); }

    

    bool nextBool() { return this->nextU() >= 0x80000000; }

    

    bool nextBiasedBool(SkScalar fractionTrue) {
        SkASSERT(fractionTrue >= 0 && fractionTrue <= SK_Scalar1);
        return this->nextUScalar1() <= fractionTrue;
    }

    

    void next64(Sk64* a) {
        SkASSERT(a);
        a->set(this->nextS(), this->nextU());
    }

    

    void setSeed(uint32_t seed) { init(seed); }

private:
    
    
    
    void init(uint32_t seed) {
        fK = NextLCG(seed);
        if (0 == fK) {
            fK = NextLCG(fK);
        }
        fJ = NextLCG(fK);
        if (0 == fJ) {
            fJ = NextLCG(fJ);
        }
        SkASSERT(0 != fK && 0 != fJ);
    }
    static uint32_t NextLCG(uint32_t seed) { return kMul*seed + kAdd; }

    
    
    enum {
        kMul = 1664525,
        kAdd = 1013904223
    };
    
    enum {
        kKMul = 30345,
        kJMul = 18000,
    };

    uint32_t fK;
    uint32_t fJ;
};

#endif
