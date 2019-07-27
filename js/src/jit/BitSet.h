





#ifndef jit_BitSet_h
#define jit_BitSet_h

#include "mozilla/MathAlgorithms.h"

#include "jit/JitAllocPolicy.h"

namespace js {
namespace jit {





class BitSet
{
  public:
    static const size_t BitsPerWord = 8 * sizeof(uint32_t);

    static size_t RawLengthForBits(size_t bits) {
        return (bits + BitsPerWord - 1) / BitsPerWord;
    }

  private:
    uint32_t* bits_;
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

    BitSet(const BitSet&) = delete;
    void operator=(const BitSet&) = delete;

  public:
    class Iterator;

    explicit BitSet(unsigned int numBits) :
        bits_(nullptr),
        numBits_(numBits) {}

    bool init(TempAllocator& alloc);

    unsigned int getNumBits() const {
        return numBits_;
    }

    
    bool contains(unsigned int value) const {
        MOZ_ASSERT(bits_);
        MOZ_ASSERT(value < numBits_);

        return !!(bits_[wordForValue(value)] & bitForValue(value));
    }

    
    bool empty() const;

    
    void insert(unsigned int value) {
        MOZ_ASSERT(bits_);
        MOZ_ASSERT(value < numBits_);

        bits_[wordForValue(value)] |= bitForValue(value);
    }

    
    void insertAll(const BitSet& other);

    
    void remove(unsigned int value) {
        MOZ_ASSERT(bits_);
        MOZ_ASSERT(value < numBits_);

        bits_[wordForValue(value)] &= ~bitForValue(value);
    }

    
    void removeAll(const BitSet& other);

    
    void intersect(const BitSet& other);

    
    
    bool fixedPointIntersect(const BitSet& other);

    
    void complement();

    
    void clear();

    uint32_t* raw() const {
        return bits_;
    }
    size_t rawLength() const {
        return numWords();
    }
};

class BitSet::Iterator
{
  private:
    BitSet& set_;
    unsigned index_;
    unsigned word_;
    uint32_t value_;

    void skipEmpty() {
        
        unsigned numWords = set_.numWords();
        const uint32_t* bits = set_.bits_;
        while (value_ == 0) {
            word_++;
            if (word_ == numWords)
                return;

            index_ = word_ * BitSet::BitsPerWord;
            value_ = bits[word_];
        }

        
        
        int numZeros = mozilla::CountTrailingZeroes32(value_);
        index_ += numZeros;
        value_ >>= numZeros;

        MOZ_ASSERT_IF(index_ < set_.numBits_, set_.contains(index_));
    }

  public:
    explicit Iterator(BitSet& set) :
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
    explicit operator bool() const {
        return more();
    }

    inline void operator++() {
        MOZ_ASSERT(more());
        MOZ_ASSERT(index_ < set_.numBits_);

        index_++;
        value_ >>= 1;

        skipEmpty();
    }

    unsigned int operator*() {
        MOZ_ASSERT(index_ < set_.numBits_);
        return index_;
    }
};

}
}

#endif 
