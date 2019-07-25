








#ifndef Sk64_DEFINED
#define Sk64_DEFINED

#include "SkFixed.h"





struct SK_API Sk64 {
    int32_t  fHi;   
    uint32_t fLo;   

    

    SkBool is32() const { return fHi == ((int32_t)fLo >> 31); }

    

    SkBool is64() const { return fHi != ((int32_t)fLo >> 31); }

    


    SkBool isFixed() const;

    

    int32_t get32() const { SkASSERT(this->is32()); return (int32_t)fLo; }

    

    SkFixed getFixed() const {
        SkASSERT(this->isFixed());

        uint32_t sum = fLo + (1 << 15);
        int32_t  hi = fHi;
        if (sum < fLo) {
            hi += 1;
        }
        return (hi << 16) | (sum >> 16);
    }

    


    SkFract getFract() const;

    
    int32_t getSqrt() const;

    


    int getClzAbs() const;

    
    SkBool  isZero() const { return (fHi | fLo) == 0; }

    
    SkBool  nonZero() const { return fHi | fLo; }

    
    SkBool  isNeg() const { return (uint32_t)fHi >> 31; }

    
    SkBool  isPos() const { return ~(fHi >> 31) & (fHi | fLo); }

    
    int     getSign() const { return (fHi >> 31) | Sk32ToBool(fHi | fLo); }

    
    void    negate();

    

    void    abs();

    


    int     shiftToMake32() const;

    
    void    setZero() { fHi = fLo = 0; }

    
    void    set(int32_t hi, uint32_t lo) { fHi = hi; fLo = lo; }

    
    void    set(int32_t a) { fHi = a >> 31; fLo = a; }

    
    void    setMul(int32_t a, int32_t b);

    



    int32_t getShiftRight(unsigned bitCount) const;

    


    void    shiftLeft(unsigned bits);

    



    void    shiftRight(unsigned bits);

    




    void    roundRight(unsigned bits);

    
    void add(int32_t lo) {
        int32_t  hi = lo >> 31; 
        uint32_t sum = fLo + (uint32_t)lo;

        fHi = fHi + hi + (sum < fLo);
        fLo = sum;
    }
    
    
    void add(int32_t hi, uint32_t lo) {
        uint32_t sum = fLo + lo;

        fHi = fHi + hi + (sum < fLo);
        fLo = sum;
    }
    
    
    void    add(const Sk64& other) { this->add(other.fHi, other.fLo); }
    
    

    void    sub(const Sk64& num);
    
    

    void    rsub(const Sk64& num);
    
    

    void    mul(int32_t);

    enum DivOptions {
        kTrunc_DivOption,   
        kRound_DivOption    
    };
    
    


    void    div(int32_t, DivOptions);

    
    SkFixed addGetFixed(const Sk64& other) const {
        return this->addGetFixed(other.fHi, other.fLo);
    }

    
    SkFixed addGetFixed(int32_t hi, uint32_t lo) const {
#ifdef SK_DEBUG
        Sk64    tmp(*this);
        tmp.add(hi, lo);
#endif

        uint32_t sum = fLo + lo;
        hi += fHi + (sum < fLo);
        lo = sum;

        sum = lo + (1 << 15);
        if (sum < lo)
            hi += 1;

        hi = (hi << 16) | (sum >> 16);
        SkASSERT(hi == tmp.getFixed());
        return hi;
    }

    


    SkFixed getFixedDiv(const Sk64& denom) const;

    friend bool operator==(const Sk64& a, const Sk64& b) {
        return a.fHi == b.fHi && a.fLo == b.fLo;
    }

    friend bool operator!=(const Sk64& a, const Sk64& b) {
        return a.fHi != b.fHi || a.fLo != b.fLo;
    }
    
    friend bool operator<(const Sk64& a, const Sk64& b) {
        return a.fHi < b.fHi || (a.fHi == b.fHi && a.fLo < b.fLo);
    }
    
    friend bool operator<=(const Sk64& a, const Sk64& b) {
        return a.fHi < b.fHi || (a.fHi == b.fHi && a.fLo <= b.fLo);
    }
    
    friend bool operator>(const Sk64& a, const Sk64& b) {
        return a.fHi > b.fHi || (a.fHi == b.fHi && a.fLo > b.fLo);
    }
    
    friend bool operator>=(const Sk64& a, const Sk64& b) {
        return a.fHi > b.fHi || (a.fHi == b.fHi && a.fLo >= b.fLo);
    }

#ifdef SkLONGLONG
    SkLONGLONG getLongLong() const;
#endif
};

#endif

