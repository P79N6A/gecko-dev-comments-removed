






































#include "nanojit.h"

namespace nanojit
{
    BitSet::BitSet(Allocator& allocator, int nbits)
        : allocator(allocator)
        , cap((nbits+63)>>6)
        , bits((int64_t*)allocator.alloc(cap * sizeof(int64_t)))
    {
        reset();
    }

    void BitSet::reset()
    {
        for (int i=0, n=cap; i < n; i++)
            bits[i] = 0;
    }

    bool BitSet::setFrom(BitSet& other)
    {
        int c = other.cap;
        if (c > cap)
            grow(c);
        int64_t *bits = this->bits;
        int64_t *otherbits = other.bits;
        int64_t newbits = 0;
        for (int i=0; i < c; i++) {
            int64_t b = bits[i];
            int64_t b2 = otherbits[i];
            newbits |= b2 & ~b; 
            bits[i] = b|b2;
        }
        return newbits != 0;
    }

    
    void BitSet::grow(int w)
    {
        int cap2 = cap;
        do {
            cap2 <<= 1;
        } while (w > cap2);
        int64_t *bits2 = (int64_t*) allocator.alloc(cap2 * sizeof(int64_t));
        int j=0;
        for (; j < cap; j++)
            bits2[j] = bits[j];
        for (; j < cap2; j++)
            bits2[j] = 0;
        cap = cap2;
        bits = bits2;
    }
}
