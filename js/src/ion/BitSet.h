








































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
    class Iterator;

    static BitSet *New(unsigned int max);

    unsigned int getMax() const {
        return max_;
    }

    
    bool contains(unsigned int value) const;

    
    bool empty() const;

    
    void insert(unsigned int value);

    
    void insertAll(const BitSet *other);

    
    void remove(unsigned int value);

    
    void removeAll(const BitSet *other);

    
    void intersect(const BitSet *other);

    
    
    bool fixedPointIntersect(const BitSet *other);

    
    void complement();

    
    void clear();

    
    Iterator begin();

    
    Iterator end();

};

class BitSet::Iterator
{
  private:
    BitSet &set_;
    unsigned index_;

  public:
    Iterator(BitSet &set, unsigned int index) :
      set_(set),
      index_(index)
    {
        if (index_ <= set_.max_ && !set_.contains(index_))
            (*this)++;
    }

    bool operator!=(const Iterator &other) const {
        return index_ != other.index_;
    }

    
    Iterator& operator++(int dummy) {
        JS_ASSERT(index_ <= set_.max_);
        do {
            index_++;
        } while (index_ <= set_.max_ && !set_.contains(index_));
        return *this;
    }

    unsigned int operator *() {
        JS_ASSERT(index_ <= set_.max_);
        return index_;
    }
};

}
}

#endif
