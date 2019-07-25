








































#ifndef jsion_bitset_h__
#define jsion_bitset_h__

#include "IonAllocPolicy.h"

namespace js {
namespace ion {




class BitSet : private TempObject
{
  public:
    static size_t RawLengthForBits(size_t bits) {
        return 1 + bits / (8 * sizeof(uint32));
    }

  private:
    BitSet(unsigned int max) :
        max_(max),
        bits_(NULL) {};

    unsigned int max_;
    uint32 *bits_;

    static inline uint32 bitForValue(unsigned int value) {
        return 1l << (uint32)(value % (8 * sizeof(uint32)));
    }

    static inline unsigned int wordForValue(unsigned int value) {
        return value / (8 * sizeof(uint32));
    }

    inline unsigned int numWords() const {
        return RawLengthForBits(max_);
    }

    bool init();

  public:
    class Iterator;

    static BitSet *New(unsigned int max);

    unsigned int getMax() const {
        return max_;
    }

    
    bool contains(unsigned int value) const {
        JS_ASSERT(bits_);
        JS_ASSERT(value <= max_);

        return !!(bits_[wordForValue(value)] & bitForValue(value));
    }

    
    bool empty() const;

    
    void insert(unsigned int value) {
        JS_ASSERT(bits_);
        JS_ASSERT(value <= max_);

        bits_[wordForValue(value)] |= bitForValue(value);
    }

    
    void insertAll(const BitSet *other);

    
    void remove(unsigned int value) {
        JS_ASSERT(bits_);
        JS_ASSERT(value <= max_);

        bits_[wordForValue(value)] &= ~bitForValue(value);
    }

    
    void removeAll(const BitSet *other);

    
    void intersect(const BitSet *other);

    
    
    bool fixedPointIntersect(const BitSet *other);

    
    void complement();

    
    void clear();

    uint32 *raw() const {
        return bits_;
    }
    size_t rawLength() const {
        return numWords();
    }
};

class BitSet::Iterator
{
  private:
    BitSet &set_;
    unsigned index_;
    unsigned word_;
    uint32 value_;

  public:
    Iterator(BitSet &set) :
      set_(set),
      index_(0),
      word_(0),
      value_(set.bits_[0])
    {
        if (!set_.contains(index_))
            (*this)++;
    }

    inline bool more() const {
        return word_ < set_.numWords();
    }
    inline operator bool() const {
        return more();
    }

    inline Iterator& operator++(int dummy) {
        JS_ASSERT(more());
        JS_ASSERT(index_ <= set_.max_);

        index_++;
        value_ >>= 1;

        
        while (value_ == 0) {
            word_++;
            if (!more())
                return *this;

            index_ = word_ * sizeof(value_) * 8;
            value_ = set_.bits_[word_];
        }

        
        JS_ASSERT(value_ != 0);

        int numZeros = js_bitscan_ctz32(value_);
        index_ += numZeros;
        value_ >>= numZeros;

        JS_ASSERT_IF(index_ <= set_.max_, set_.contains(index_));
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
