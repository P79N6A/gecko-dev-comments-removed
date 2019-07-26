
















#ifndef SkTLogic_DEFINED
#define SkTLogic_DEFINED




template <typename T, T v> struct SkTIntegralConstant {
    static const T value = v;
    typedef T value_type;
    typedef SkTIntegralConstant<T, v> type;
};


template <bool b> struct SkTBool : SkTIntegralConstant<bool, b> { };


typedef SkTBool<true> SkTrue;


typedef SkTBool<false> SkFalse;




template <bool condition, typename T, typename F> struct SkTIf_c {
    typedef F type;
};
template <typename T, typename F> struct SkTIf_c<true, T, F> {
    typedef T type;
};


template <typename Condition, typename T, typename F> struct SkTIf {
    typedef typename SkTIf_c<static_cast<bool>(Condition::value), T, F>::type type;
};


template <typename a, typename b, typename Both, typename A, typename B, typename Neither>
struct SkTMux {
    typedef typename SkTIf<a, typename SkTIf<b, Both, A>::type,
                              typename SkTIf<b, B, Neither>::type>::type type;
};

#endif
