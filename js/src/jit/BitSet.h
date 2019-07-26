





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
    explicit BitSet(unsigned int numBits) :
        bits_(nullptr),
        numBits_(numBits) {}

    uint32_t *bits_;
    const unsigned int numBits_;

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

    void skipEmpty() {
        
        unsigned numWords = set_.numWords();
        const uint32_t *bits = set_.bits_;
        while (value_ == 0) {
            word_++;
            if (word_ == numWords)
                return;

            JS_STATIC_ASSERT(sizeof(value_) * 8 == BitSet::BitsPerWord);
            index_ = word_ * sizeof(value_) * 8;
            value_ = bits[word_];
        }

        
        
        int numZeros = mozilla::CountTrailingZeroes32(value_);
        index_ += numZeros;
        value_ >>= numZeros;

        JS_ASSERT_IF(index_ < set_.numBits_, set_.contains(index_));
    }

  public:
    explicit Iterator(BitSet &set) :
      set_(set),
      index_(0),
      word_(0),
      value_(set.bits_[0])
    {
        skipEmpty();
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

        skipEmpty();
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
