
























#ifndef MatchResult_h
#define MatchResult_h

#include "wtfbridge.h"

typedef uint64_t EncodedMatchResult;

struct MatchResult {
    MatchResult(int start, int end)
        : start(start)
        , end(end)
    {
    }

#if !WTF_CPU_X86_64 || WTF_PLATFORM_WIN
    explicit MatchResult(EncodedMatchResult encoded)
    {
        union u {
            uint64_t encoded;
            struct s {
                int start;
                int end;
            } split;
        } value;
        value.encoded = encoded;
        start = value.split.start;
        end = value.split.end;
    }
#endif

    static MatchResult failed()
    {
        return MatchResult(int(WTF::notFound), 0);
    }

    operator bool()
    {
        return start != int(WTF::notFound);
    }

    bool empty()
    {
        return start == end;
    }

    
    int start;
    int end;
};

#endif
