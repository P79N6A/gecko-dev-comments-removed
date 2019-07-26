
























#ifndef MatchResult_h
#define MatchResult_h

#include "wtfbridge.h"

typedef uint64_t EncodedMatchResult;

struct MatchResult {
    MatchResult(size_t start, size_t end)
        : start(start)
        , end(end)
    {
    }

    explicit MatchResult(EncodedMatchResult encoded)
    {
        union u {
            uint64_t encoded;
            struct s {
                size_t start;
                size_t end;
            } split;
        } value;
        value.encoded = encoded;
        start = value.split.start;
        end = value.split.end;
    }

    static MatchResult failed()
    {
        return MatchResult(WTF::notFound, 0);
    }

    operator bool()
    {
        return start != WTF::notFound;
    }

    bool empty()
    {
        return start == end;
    }

    size_t start;
    size_t end;
};

#endif
