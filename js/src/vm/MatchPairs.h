





#ifndef vm_MatchPairs_h
#define vm_MatchPairs_h

#include "jsalloc.h"

#include "ds/LifoAlloc.h"
#include "js/Vector.h"









namespace js {

struct MatchPair
{
    int32_t start;
    int32_t limit;

    MatchPair()
      : start(-1), limit(-1)
    { }

    MatchPair(int32_t start, int32_t limit)
      : start(start), limit(limit)
    { }

    size_t length()      const { MOZ_ASSERT(!isUndefined()); return limit - start; }
    bool isEmpty()       const { return length() == 0; }
    bool isUndefined()   const { return start < 0; }

    void displace(size_t amount) {
        start += (start < 0) ? 0 : amount;
        limit += (limit < 0) ? 0 : amount;
    }

    inline bool check() const {
        MOZ_ASSERT(limit >= start);
        MOZ_ASSERT_IF(start < 0, start == -1);
        MOZ_ASSERT_IF(limit < 0, limit == -1);
        return true;
    }
};


class MatchPairs
{
  protected:
    
    uint32_t pairCount_;

    
    MatchPair* pairs_;

  protected:
    
    MatchPairs()
      : pairCount_(0), pairs_(nullptr)
    { }

  protected:
    
    friend class RegExpShared;
    friend class RegExpStatics;

    
    virtual bool allocOrExpandArray(size_t pairCount) = 0;

    bool initArrayFrom(MatchPairs& copyFrom);
    void forgetArray() { pairs_ = nullptr; }

    void displace(size_t disp);
    void checkAgainst(size_t inputLength) {
#ifdef DEBUG
        for (size_t i = 0; i < pairCount_; i++) {
            const MatchPair& p = (*this)[i];
            MOZ_ASSERT(p.check());
            if (p.isUndefined())
                continue;
            MOZ_ASSERT(size_t(p.limit) <= inputLength);
        }
#endif
    }

  public:
    
    bool   empty() const           { return pairCount_ == 0; }
    size_t pairCount() const       { MOZ_ASSERT(pairCount_ > 0); return pairCount_; }
    size_t parenCount() const      { return pairCount_ - 1; }

    static size_t offsetOfPairs() { return offsetof(MatchPairs, pairs_); }
    static size_t offsetOfPairCount() { return offsetof(MatchPairs, pairCount_); }

    int32_t* pairsRaw() { return reinterpret_cast<int32_t*>(pairs_); }

  public:
    size_t length() const { return pairCount_; }

    const MatchPair& operator[](size_t i) const {
        MOZ_ASSERT(i < pairCount_);
        return pairs_[i];
    }
    MatchPair& operator[](size_t i) {
        MOZ_ASSERT(i < pairCount_);
        return pairs_[i];
    }
};


class ScopedMatchPairs : public MatchPairs
{
    LifoAllocScope lifoScope_;

  public:
    
    explicit ScopedMatchPairs(LifoAlloc* lifoAlloc)
      : lifoScope_(lifoAlloc)
    { }

  protected:
    bool allocOrExpandArray(size_t pairCount);
};





class VectorMatchPairs : public MatchPairs
{
    Vector<MatchPair, 10, SystemAllocPolicy> vec_;

  public:
    VectorMatchPairs() {
        vec_.clear();
    }

  protected:
    friend class RegExpStatics;
    bool allocOrExpandArray(size_t pairCount);
};

} 

#endif 
