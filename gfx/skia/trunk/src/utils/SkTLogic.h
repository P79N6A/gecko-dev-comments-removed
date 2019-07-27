
















#ifndef SkTLogic_DEFINED
#define SkTLogic_DEFINED




template <typename T, T v> struct SkTIntegralConstant {
    static const T value = v;
    typedef T value_type;
    typedef SkTIntegralConstant<T, v> type;
};


template <bool b> struct SkTBool : SkTIntegralConstant<bool, b> { };


template <typename T>
class SkTIsEmpty {
    struct Derived : public T { char unused; };
public:
    static const bool value = sizeof(Derived) == sizeof(char);
};


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


template <bool condition, class T = void> struct SkTEnableIf_c { };
template <class T> struct SkTEnableIf_c<true, T> {
    typedef T type;
};


template <class Condition, class T = void> struct SkTEnableIf
    : public SkTEnableIf_c<static_cast<bool>(Condition::value), T> { };






#define SK_WHEN(cond_prefix, T) typename SkTEnableIf_c<cond_prefix::value, T>::type


#define SK_CREATE_MEMBER_DETECTOR(member)                                           \
template <typename T>                                                               \
class HasMember_##member {                                                          \
    struct Fallback { int member; };                                                \
    struct Derived : T, Fallback {};                                                \
    template <typename U, U> struct Check;                                          \
    template <typename U> static uint8_t func(Check<int Fallback::*, &U::member>*); \
    template <typename U> static uint16_t func(...);                                \
public:                                                                             \
    typedef HasMember_##member type;                                                \
    static const bool value = sizeof(func<Derived>(NULL)) == sizeof(uint16_t);      \
}


#define SK_CREATE_TYPE_DETECTOR(type)                                   \
template <typename T>                                                   \
class HasType_##type {                                                  \
    template <typename U> static uint8_t func(typename U::type*);       \
    template <typename U> static uint16_t func(...);                    \
public:                                                                 \
    static const bool value = sizeof(func<T>(NULL)) == sizeof(uint8_t); \
}

#endif
