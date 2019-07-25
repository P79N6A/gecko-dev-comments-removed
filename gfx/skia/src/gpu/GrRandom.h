









#ifndef GrRandom_DEFINED
#define GrRandom_DEFINED

class GrRandom {
public:
    GrRandom() : fSeed(0) {}
    GrRandom(uint32_t seed) : fSeed(seed) {}

    uint32_t seed() const { return fSeed; }

    uint32_t nextU() {
        fSeed = fSeed * kMUL + kADD;
        return fSeed;
    }

    int32_t nextS() { return (int32_t)this->nextU(); }

    


    float nextF() {
        
        return (float)(this->nextU() * 2.32830644e-10);
    }

    


    float nextF(float min, float max) {
        return min + this->nextF() * (max - min);
    }

private:
    


    enum {
        kMUL = 1664525,
        kADD = 1013904223
    };
    uint32_t    fSeed;
};

#endif

