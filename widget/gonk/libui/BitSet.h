















#ifndef UTILS_BITSET_H
#define UTILS_BITSET_H

#include <stdint.h>





namespace android {


struct BitSet32 {
    uint32_t value;

    inline BitSet32() : value(0) { }
    explicit inline BitSet32(uint32_t value) : value(value) { }

    
    static inline uint32_t valueForBit(uint32_t n) { return 0x80000000 >> n; }

    
    inline void clear() { value = 0; }

    
    inline uint32_t count() const { return __builtin_popcount(value); }

    
    inline bool isEmpty() const { return ! value; }

    
    inline bool isFull() const { return value == 0xffffffff; }

    
    inline bool hasBit(uint32_t n) const { return value & valueForBit(n); }

    
    inline void markBit(uint32_t n) { value |= valueForBit(n); }

    
    inline void clearBit(uint32_t n) { value &= ~ valueForBit(n); }

    
    
    inline uint32_t firstMarkedBit() const { return __builtin_clz(value); }

    
    
    inline uint32_t firstUnmarkedBit() const { return __builtin_clz(~ value); }

    
    
    inline uint32_t lastMarkedBit() const { return 31 - __builtin_ctz(value); }

    
    
    inline uint32_t clearFirstMarkedBit() {
        uint32_t n = firstMarkedBit();
        clearBit(n);
        return n;
    }

    
    
    inline uint32_t markFirstUnmarkedBit() {
        uint32_t n = firstUnmarkedBit();
        markBit(n);
        return n;
    }

    
    
    inline uint32_t clearLastMarkedBit() {
        uint32_t n = lastMarkedBit();
        clearBit(n);
        return n;
    }

    
    
    inline uint32_t getIndexOfBit(uint32_t n) const {
        return __builtin_popcount(value & ~(0xffffffffUL >> n));
    }

    inline bool operator== (const BitSet32& other) const { return value == other.value; }
    inline bool operator!= (const BitSet32& other) const { return value != other.value; }
};

} 

#endif 
