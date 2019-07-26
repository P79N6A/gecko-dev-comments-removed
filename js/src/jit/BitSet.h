





#ifndef jit_BitSet_h
#define jit_BitSet_h

#include "mozilla/MathAlgorithms.h"

#include "jit/IonAllocPolicy.h"

namespace js {
namespace jit {





class BitSet : private TempObject
{
  public:
    static const size_t BitsPerWord = 8 * sizeof(uint32_t);

    static size_t RawLengthForBits(size_t bits) {
        return (bits + BitsPerWord - 1) / BitsPerWord;
    }

  private:
    BitSet(unsigned int numBits) :
        numBits_(numBits),
        bits_(nullptr) {}

    unsigned int numBits_;
    uint32_t *bits_;

    static inline uint32_t bitForValue(unsigned int value) {
        return 1l << uint32_t(value % BitsPerWord);
    }

    static inline unsigned int wordForValue(unsigned int value) {
        return value / BitsPerWord;
    }

    inline unsigned int numWords() const {
        return RawLengthForBits(numBits_);
    }

    bool init(TempAllocator &alloc);

  public:
    class Iterator;

    static BitSet *New(TempAllocator &alloc, unsigned int numBits);

    unsigned int getNumBits() const {
        return numBits_;
    }

    
    bool contains(unsigned int value) const {
        JS_ASSERT(bits_);
        JS_ASSERT(value < numBits_);

        return !!(bits_[wordForValue(value)] & bitForValue(value));
    }

    
    bool empty() const;

    
    void insert(unsigned int value) {
        JS_ASSERT(bits_);
        JS_ASSERT(value < numBits_);

        bits_[wordForValue(value)] |= bitForValue(value);
    }

    
    void insertAll(const BitSet *other);

    
    void remove(unsigned int value) {
        JS_ASSERT(bits_);
        JS_ASSERT(value < numBits_);

        bits_[wordForValue(value)] &= ~bitForValue(value);
    }

    
    void removeAll(const BitSet *other);

    
    void intersect(const BitSet *other);

    
    
    bool fixedPointIntersect(const BitSet *other);

    
    void complement();

    
    void clear();

    uint32_t *raw() const {
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
    uint32_t value_;

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
        JS_ASSERT(index_ < set_.numBits_);

        index_++;
        value_ >>= 1;

        
        while (value_ == 0) {
            word_++;
            if (!more())
                return *this;

            index_ = word_ * sizeof(value_) * 8;
            value_ = set_.bits_[word_];
        }

        
        
        int numZeros = mozilla::CountTrailingZeroes32(value_);
        index_ += numZeros;
        value_ >>= numZeros;

        JS_ASSERT_IF(index_ < set_.numBits_, set_.contains(index_));
        return *this;
    }

    unsigned int operator *() {
        JS_ASSERT(index_ < set_.numBits_);
        return index_;
    }
};

}
}

#endif 
