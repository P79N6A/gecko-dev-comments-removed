 




















#ifndef TypeTraits_h
#define TypeTraits_h

#include "assembler/wtf/Platform.h"

#if (defined(__GLIBCXX__) && (__GLIBCXX__ >= 20070724) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || (defined(_MSC_VER) && (_MSC_VER >= 1600))
#include <type_traits>
#if defined(__GLIBCXX__) && (__GLIBCXX__ >= 20070724) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <tr1/memory>
#endif
#endif

namespace WTF {

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    template <bool Predicate, class If, class Then> struct Conditional  { typedef If Type; };
    template <class If, class Then> struct Conditional<false, If, Then> { typedef Then Type; };

    template<typename T> struct IsInteger           { static const bool value = false; };
    template<> struct IsInteger<bool>               { static const bool value = true; };
    template<> struct IsInteger<char>               { static const bool value = true; };
    template<> struct IsInteger<signed char>        { static const bool value = true; };
    template<> struct IsInteger<unsigned char>      { static const bool value = true; };
    template<> struct IsInteger<short>              { static const bool value = true; };
    template<> struct IsInteger<unsigned short>     { static const bool value = true; };
    template<> struct IsInteger<int>                { static const bool value = true; };
    template<> struct IsInteger<unsigned int>       { static const bool value = true; };
    template<> struct IsInteger<long>               { static const bool value = true; };
    template<> struct IsInteger<unsigned long>      { static const bool value = true; };
    template<> struct IsInteger<long long>          { static const bool value = true; };
    template<> struct IsInteger<unsigned long long> { static const bool value = true; };
#if WTF_COMPILER_MSVC || defined(_NATIVE_WCHAR_T_DEFINED)
    template<> struct IsInteger<wchar_t>            { static const bool value = true; };
#endif

    template<typename T> struct IsFloatingPoint     { static const bool value = false; };
    template<> struct IsFloatingPoint<float>        { static const bool value = true; };
    template<> struct IsFloatingPoint<double>       { static const bool value = true; };
    template<> struct IsFloatingPoint<long double>  { static const bool value = true; };

    template<typename T> struct IsArithmetic     { static const bool value = IsInteger<T>::value || IsFloatingPoint<T>::value; };

    
    
    template <typename T> struct IsPod           { static const bool value = IsArithmetic<T>::value; };
    template <typename P> struct IsPod<P*>       { static const bool value = true; };

    template<typename T> class IsConvertibleToInteger {
        
        
        template<bool performCheck, typename U> class IsConvertibleToDouble;
        template<typename U> class IsConvertibleToDouble<false, U> {
        public:
            static const bool value = false;
        };

        template<typename U> class IsConvertibleToDouble<true, U> {
            typedef char YesType;
            struct NoType {
                char padding[8];
            };

            static YesType floatCheck(long double);
            static NoType floatCheck(...);
            static T& t;
        public:
            static const bool value = sizeof(floatCheck(t)) == sizeof(YesType);
        };

    public:
        static const bool value = IsInteger<T>::value || IsConvertibleToDouble<!IsInteger<T>::value, T>::value;
    };


    template <class T> struct IsArray {
        static const bool value = false;
    };

    template <class T> struct IsArray<T[]> {
        static const bool value = true;
    };

    template <class T, size_t N> struct IsArray<T[N]> {
        static const bool value = true;
    };


    template <typename T, typename U> struct IsSameType {
        static const bool value = false;
    };

    template <typename T> struct IsSameType<T, T> {
        static const bool value = true;
    };

    template <typename T, typename U> class IsSubclass {
        typedef char YesType;
        struct NoType {
            char padding[8];
        };

        static YesType subclassCheck(U*);
        static NoType subclassCheck(...);
        static T* t;
    public:
        static const bool value = sizeof(subclassCheck(t)) == sizeof(YesType);
    };

    template <typename T, template<class V> class U> class IsSubclassOfTemplate {
        typedef char YesType;
        struct NoType {
            char padding[8];
        };

        template<typename W> static YesType subclassCheck(U<W>*);
        static NoType subclassCheck(...);
        static T* t;
    public:
        static const bool value = sizeof(subclassCheck(t)) == sizeof(YesType);
    };

    template <typename T, template <class V> class OuterTemplate> struct RemoveTemplate {
        typedef T Type;
    };

    template <typename T, template <class V> class OuterTemplate> struct RemoveTemplate<OuterTemplate<T>, OuterTemplate> {
        typedef T Type;
    };

    template <typename T> struct RemoveConst {
        typedef T Type;
    };

    template <typename T> struct RemoveConst<const T> {
        typedef T Type;
    };

    template <typename T> struct RemoveVolatile {
        typedef T Type;
    };

    template <typename T> struct RemoveVolatile<volatile T> {
        typedef T Type;
    };

    template <typename T> struct RemoveConstVolatile {
        typedef typename RemoveVolatile<typename RemoveConst<T>::Type>::Type Type;
    };

    template <typename T> struct RemovePointer {
        typedef T Type;
    };

    template <typename T> struct RemovePointer<T*> {
        typedef T Type;
    };

    template <typename T> struct RemoveReference {
        typedef T Type;
    };

    template <typename T> struct RemoveReference<T&> {
        typedef T Type;
    };

    template <typename T> struct RemoveExtent {
        typedef T Type;
    };

    template <typename T> struct RemoveExtent<T[]> {
        typedef T Type;
    };

    template <typename T, size_t N> struct RemoveExtent<T[N]> {
        typedef T Type;
    };

    template <class T> struct DecayArray {
        typedef typename RemoveReference<T>::Type U;
    public:
        typedef typename Conditional<
            IsArray<U>::value,
            typename RemoveExtent<U>::Type*,
            typename RemoveConstVolatile<U>::Type
        >::Type Type;
    };

#if WTF_COMPILER_CLANG || GCC_VERSION_AT_LEAST(4, 6, 0) || (defined(_MSC_VER) && (_MSC_VER >= 1400) && (_MSC_VER < 1600) && !defined(__INTEL_COMPILER))
    
    
    
    template <typename T> struct HasTrivialConstructor {
        static const bool value = __has_trivial_constructor(T) || IsPod<RemoveConstVolatile<T> >::value;
    };
    template <typename T> struct HasTrivialDestructor {
        static const bool value = __has_trivial_destructor(T) || IsPod<RemoveConstVolatile<T> >::value;
    };
#elif (defined(__GLIBCXX__) && (__GLIBCXX__ >= 20070724) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || (defined(_MSC_VER) && (_MSC_VER >= 1600))
    
    
    template<typename T> struct HasTrivialConstructor : public std::tr1::has_trivial_constructor<T> { };
    template<typename T> struct HasTrivialDestructor : public std::tr1::has_trivial_destructor<T> { };
#else
    
    
    
    
    template <typename T> struct HasTrivialConstructor {
        static const bool value = IsPod<RemoveConstVolatile<T> >::value;
    };
    template <typename T> struct HasTrivialDestructor {
        static const bool value = IsPod<RemoveConstVolatile<T> >::value;
    };
#endif

} 

#endif 
