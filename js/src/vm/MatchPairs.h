







































#ifndef MatchPairs_h__
#define MatchPairs_h__









namespace js {

struct MatchPair
{
    int start;
    int limit;

    MatchPair(int start, int limit) : start(start), limit(limit) {}

    size_t length() const {
        JS_ASSERT(!isUndefined());
        return limit - start;
    }

    bool isUndefined() const {
        return start == -1;
    }

    void check() const {
        JS_ASSERT(limit >= start);
        JS_ASSERT_IF(!isUndefined(), start >= 0);
    }
};

class MatchPairs
{
    size_t  pairCount_;
    int     buffer_[1];

    explicit MatchPairs(size_t pairCount) : pairCount_(pairCount) {
        initPairValues();
    }

    void initPairValues() {
        for (int *it = buffer_; it < buffer_ + 2 * pairCount_; ++it)
            *it = -1;
    }

    static size_t calculateSize(size_t backingPairCount) {
        return sizeof(MatchPairs) - sizeof(int) + sizeof(int) * backingPairCount;
    }

    int *buffer() { return buffer_; }

    friend class detail::RegExpPrivate;

  public:
    



    static MatchPairs *create(LifoAlloc &alloc, size_t pairCount, size_t backingPairCount);

    size_t pairCount() const { return pairCount_; }

    MatchPair pair(size_t i) {
        JS_ASSERT(i < pairCount());
        return MatchPair(buffer_[2 * i], buffer_[2 * i + 1]);
    }

    void displace(size_t amount) {
        if (!amount)
            return;

        for (int *it = buffer_; it < buffer_ + 2 * pairCount_; ++it)
            *it = (*it < 0) ? -1 : *it + amount;
    }

    inline void checkAgainst(size_t length);
};

} 

#endif
