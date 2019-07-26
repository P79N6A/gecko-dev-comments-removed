






#ifndef SkConstexprMath_DEFINED
#define SkConstexprMath_DEFINED

#include "SkTypes.h"
#include <limits.h>

template <uintmax_t N, uintmax_t B>
struct SK_LOG {
    
    static const uintmax_t value = 1 + SK_LOG<N/B, B>::value;
};
template <uintmax_t B>
struct SK_LOG<1, B> {
    static const uintmax_t value = 0;
};
template <uintmax_t B>
struct SK_LOG<0, B> {
    static const uintmax_t value = 0;
};

template<uintmax_t N>
struct SK_2N1 {
    
    static const uintmax_t value = (SK_2N1<N-1>::value << 1) + 1;
};
template<>
struct SK_2N1<1> {
    static const uintmax_t value = 1;
};




#define SK_BASE_N_DIGITS_IN(n, t) (\
    SK_LOG<SK_2N1<(sizeof(t) * CHAR_BIT)>::value, n>::value\
)



#define SK_DIGITS_IN(t) SK_BASE_N_DIGITS_IN(10, (t))


#define SK_MAX(a,b) (((a) > (b)) ? (a) : (b))

#endif
