








































#ifndef jsion_bitset_h__
#define jsion_bitset_h__

#include "IonAllocPolicy.h"

namespace js {
namespace ion {




class BitSet : private TempObject
{
private:
    BitSet(unsigned int max) :
        max_(max),
        bits_(NULL) {};

    unsigned int max_;

    unsigned long *bits_;

    static inline unsigned long bitForValue(unsigned int value) {
        return 1l << (unsigned long)(value % (8 * sizeof(unsigned long)));
    }

    static inline unsigned int wordForValue(unsigned int value) {
        return value / (8 * sizeof(unsigned long));
    }

    inline unsigned int numWords() const {
        return 1 + max_ / (8 * sizeof(*bits_));
    }

    bool init();

public:
    static BitSet *New(unsigned int max);

    unsigned int getMax() const {
        return max_;
    }

    
    bool contains(unsigned int value) const;

    
    void insert(unsigned int value);

    
    void insertAll(const BitSet *other);

    
    void remove(unsigned int value);

    
    void removeAll(const BitSet *other);

    
    void intersect(const BitSet *other);

    
    
    bool fixedPointIntersect(const BitSet *other);

    
    void complement();

};

}
}

#endif
