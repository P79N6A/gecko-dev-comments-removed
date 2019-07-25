






































#ifndef mozilla_CheckedInt_h
#define mozilla_CheckedInt_h






#define CHECKEDINT_ENABLE_PR_INTEGER_TYPES


#define CHECKEDINT_ENABLE_LONG_LONG



#define CHECKEDINT_ENABLE_MOZ_ASSERTS





#ifdef CHECKEDINT_ENABLE_MOZ_ASSERTS
    #include <mozilla/Assertions.h>
#else
    #ifndef MOZ_STATIC_ASSERT
        #define MOZ_STATIC_ASSERT(x)
    #endif
#endif

#ifdef CHECKEDINT_ENABLE_PR_INTEGER_TYPES
    #include "prtypes.h"
#endif

#include <climits>

namespace mozilla {

namespace CheckedInt_internal {

















struct unsupported_type {};

template<typename integer_type> struct is_supported_pass_3 {
    enum { value = 0 };
};
template<typename integer_type> struct is_supported_pass_2 {
    enum { value = is_supported_pass_3<integer_type>::value };
};
template<typename integer_type> struct is_supported {
    enum { value = is_supported_pass_2<integer_type>::value };
};

template<> struct is_supported<int8_t>   { enum { value = 1 }; };
template<> struct is_supported<uint8_t>  { enum { value = 1 }; };
template<> struct is_supported<int16_t>  { enum { value = 1 }; };
template<> struct is_supported<uint16_t> { enum { value = 1 }; };
template<> struct is_supported<int32_t>  { enum { value = 1 }; };
template<> struct is_supported<uint32_t> { enum { value = 1 }; };
template<> struct is_supported<int64_t>  { enum { value = 1 }; };
template<> struct is_supported<uint64_t> { enum { value = 1 }; };

template<> struct is_supported_pass_2<char>   { enum { value = 1 }; };
template<> struct is_supported_pass_2<unsigned char>  { enum { value = 1 }; };
template<> struct is_supported_pass_2<short>  { enum { value = 1 }; };
template<> struct is_supported_pass_2<unsigned short> { enum { value = 1 }; };
template<> struct is_supported_pass_2<int>  { enum { value = 1 }; };
template<> struct is_supported_pass_2<unsigned int> { enum { value = 1 }; };
template<> struct is_supported_pass_2<long>  { enum { value = 1 }; };
template<> struct is_supported_pass_2<unsigned long> { enum { value = 1 }; };
#ifdef CHECKEDINT_ENABLE_LONG_LONG
    template<> struct is_supported_pass_2<long long>  { enum { value = 1 }; };
    template<> struct is_supported_pass_2<unsigned long long> { enum { value = 1 }; };
#endif

#ifdef CHECKEDINT_ENABLE_PR_INTEGER_TYPES
    template<> struct is_supported_pass_3<PRInt8>   { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRUint8>  { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRInt16>  { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRUint16> { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRInt32>  { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRUint32> { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRInt64>  { enum { value = 1 }; };
    template<> struct is_supported_pass_3<PRUint64> { enum { value = 1 }; };
#endif







template<int size, bool signedness> struct stdint_type_for_size_and_signedness {};
template<> struct stdint_type_for_size_and_signedness<1, true>   { typedef int8_t   type; };
template<> struct stdint_type_for_size_and_signedness<1, false>  { typedef uint8_t  type; };
template<> struct stdint_type_for_size_and_signedness<2, true>   { typedef int16_t  type; };
template<> struct stdint_type_for_size_and_signedness<2, false>  { typedef uint16_t type; };
template<> struct stdint_type_for_size_and_signedness<4, true>   { typedef int32_t  type; };
template<> struct stdint_type_for_size_and_signedness<4, false>  { typedef uint32_t type; };
template<> struct stdint_type_for_size_and_signedness<8, true>   { typedef int64_t  type; };
template<> struct stdint_type_for_size_and_signedness<8, false>  { typedef uint64_t type; };

template<typename integer_type> struct unsigned_type {
    typedef typename stdint_type_for_size_and_signedness<sizeof(integer_type), false>::type type;
};

template<typename integer_type> struct is_signed {
    enum { value = integer_type(-1) <= integer_type(0) };
};

template<typename integer_type, int size=sizeof(integer_type)>
struct twice_bigger_type {
    typedef typename stdint_type_for_size_and_signedness<
                       sizeof(integer_type) * 2,
                       is_signed<integer_type>::value
                     >::type type;
};

template<typename integer_type>
struct twice_bigger_type<integer_type, 8> {
    typedef unsupported_type type;
};

template<typename integer_type> struct position_of_sign_bit
{
    enum {
        value = CHAR_BIT * sizeof(integer_type) - 1
    };
};

template<typename integer_type> struct min_value
{
    static integer_type value()
    {
        
        
        
        
        
        return is_signed<integer_type>::value
                 ? integer_type(typename unsigned_type<integer_type>::type(1) << position_of_sign_bit<integer_type>::value)
                 : integer_type(0);
    }
};

template<typename integer_type> struct max_value
{
    static integer_type value()
    {
        return ~min_value<integer_type>::value();
    }
};







template<typename T> inline T has_sign_bit(T x)
{
    
    
    
    
    return T(typename unsigned_type<T>::type(x) >> position_of_sign_bit<T>::value);
}

template<typename T> inline T binary_complement(T x)
{
    return ~x;
}

template<typename T, typename U,
         bool is_T_signed = is_signed<T>::value,
         bool is_U_signed = is_signed<U>::value>
struct is_in_range_impl {};

template<typename T, typename U>
struct is_in_range_impl<T, U, true, true>
{
    static T run(U x)
    {
        return (x <= max_value<T>::value()) &&
               (x >= min_value<T>::value());
    }
};

template<typename T, typename U>
struct is_in_range_impl<T, U, false, false>
{
    static T run(U x)
    {
        return x <= max_value<T>::value();
    }
};

template<typename T, typename U>
struct is_in_range_impl<T, U, true, false>
{
    static T run(U x)
    {
        if (sizeof(T) > sizeof(U))
            return 1;
        else
            return x <= U(max_value<T>::value());
    }
};

template<typename T, typename U>
struct is_in_range_impl<T, U, false, true>
{
    static T run(U x)
    {
        if (sizeof(T) >= sizeof(U))
            return x >= 0;
        else
            return (x >= 0) && (x <= U(max_value<T>::value()));
    }
};

template<typename T, typename U> inline T is_in_range(U x)
{
    return is_in_range_impl<T, U>::run(x);
}

template<typename T> inline T is_add_valid(T x, T y, T result)
{
    return is_signed<T>::value ?
                 
                 
                 
                 has_sign_bit(binary_complement(T((result^x) & (result^y))))
             :
                 binary_complement(x) >= y;
}

template<typename T> inline T is_sub_valid(T x, T y, T result)
{
    return is_signed<T>::value ?
                 
                 has_sign_bit(binary_complement(T((result^x) & (x^y))))
             :
                 x >= y;
}

template<typename T,
         bool is_signed =  is_signed<T>::value,
         bool twice_bigger_type_is_supported = is_supported<typename twice_bigger_type<T>::type>::value>
struct is_mul_valid_impl {};

template<typename T, bool is_signed>
struct is_mul_valid_impl<T, is_signed, true>
{
    static T run(T x, T y)
    {
        typedef typename twice_bigger_type<T>::type twice_bigger_type;
        twice_bigger_type product = twice_bigger_type(x) * twice_bigger_type(y);
        return is_in_range<T>(product);
    }
};

template<typename T>
struct is_mul_valid_impl<T, true, false>
{
    static T run(T x, T y)
    {
        const T max = max_value<T>::value();
        const T min = min_value<T>::value();

        if (x == 0 || y == 0) return true;

        if (x > 0) {
            if (y > 0)
                return x <= max / y;
            else
                return y >= min / x;
        } else {
            if (y > 0)
                return x >= min / y;
            else
                return y >= max / x;
        }
    }
};

template<typename T>
struct is_mul_valid_impl<T, false, false>
{
    static T run(T x, T y)
    {
        const T max = max_value<T>::value();
        if (x == 0 || y == 0) return true;
        return x <= max / y;
    }
};

template<typename T> inline T is_mul_valid(T x, T y, T )
{
    return is_mul_valid_impl<T>::run(x, y);
}

template<typename T> inline T is_div_valid(T x, T y)
{
    return is_signed<T>::value ?
                 
                 (y != 0) && (x != min_value<T>::value() || y != T(-1))
             :
                 y != 0;
}


template<typename T, bool is_signed = is_signed<T>::value>
struct opposite_if_signed_impl
{
    static T run(T x) { return -x; }
};
template<typename T>
struct opposite_if_signed_impl<T, false>
{
    static T run(T x) { return x; }
};
template<typename T>
inline T opposite_if_signed(T x) { return opposite_if_signed_impl<T>::run(x); }



} 























































template<typename T>
class CheckedInt
{
protected:
    T mValue;
    T mIsValid; 
                

    template<typename U>
    CheckedInt(U value, T isValid) : mValue(value), mIsValid(isValid)
    {
        MOZ_STATIC_ASSERT(CheckedInt_internal::is_supported<T>::value, "This type is not supported by CheckedInt");
    }

public:
    







    template<typename U>
    CheckedInt(U value)
        : mValue(T(value)),
          mIsValid(CheckedInt_internal::is_in_range<T>(value))
    {
        MOZ_STATIC_ASSERT(CheckedInt_internal::is_supported<T>::value, "This type is not supported by CheckedInt");
    }

    
    CheckedInt() : mValue(0), mIsValid(1)
    {
        MOZ_STATIC_ASSERT(CheckedInt_internal::is_supported<T>::value, "This type is not supported by CheckedInt");
    }

    
    T value() const { return mValue; }

    


    bool valid() const
    {
        return bool(mIsValid);
    }

    
    template<typename U> friend CheckedInt<U> operator +(const CheckedInt<U>& lhs, const CheckedInt<U>& rhs);
    
    template<typename U> CheckedInt& operator +=(U rhs);
    
    template<typename U> friend CheckedInt<U> operator -(const CheckedInt<U>& lhs, const CheckedInt<U> &rhs);
    
    template<typename U> CheckedInt& operator -=(U rhs);
    
    template<typename U> friend CheckedInt<U> operator *(const CheckedInt<U>& lhs, const CheckedInt<U> &rhs);
    
    template<typename U> CheckedInt& operator *=(U rhs);
    
    template<typename U> friend CheckedInt<U> operator /(const CheckedInt<U>& lhs, const CheckedInt<U> &rhs);
    
    template<typename U> CheckedInt& operator /=(U rhs);

    
    CheckedInt operator -() const
    {
        
        
        T result = CheckedInt_internal::opposite_if_signed(value());
        
        return CheckedInt(result,
                          mIsValid & CheckedInt_internal::is_sub_valid(T(0), value(), result));
    }

    
    bool operator ==(const CheckedInt& other) const
    {
        return bool(mIsValid & other.mIsValid & (value() == other.mValue));
    }

    
    CheckedInt& operator++()
    {
        *this = *this + 1;
        return *this;
    }

    
    CheckedInt operator++(int)
    {
        CheckedInt tmp = *this;
        *this = *this + 1;
        return tmp;
    }

    
    CheckedInt& operator--()
    {
        *this = *this - 1;
        return *this;
    }

    
    CheckedInt operator--(int)
    {
        CheckedInt tmp = *this;
        *this = *this - 1;
        return tmp;
    }

private:
    


    template<typename U>
    bool operator !=(U other) const { return !(*this == other); }
};

#define CHECKEDINT_BASIC_BINARY_OPERATOR(NAME, OP)               \
template<typename T>                                          \
inline CheckedInt<T> operator OP(const CheckedInt<T> &lhs, const CheckedInt<T> &rhs) \
{                                                                     \
    T x = lhs.mValue;                                                \
    T y = rhs.mValue;                                                \
    T result = x OP y;                                                \
    T is_op_valid                                                     \
        = CheckedInt_internal::is_##NAME##_valid(x, y, result);       \
    /* give the compiler a good chance to perform RVO */              \
    return CheckedInt<T>(result,                                      \
                         lhs.mIsValid & rhs.mIsValid & is_op_valid);  \
}

CHECKEDINT_BASIC_BINARY_OPERATOR(add, +)
CHECKEDINT_BASIC_BINARY_OPERATOR(sub, -)
CHECKEDINT_BASIC_BINARY_OPERATOR(mul, *)



template<typename T>
inline CheckedInt<T> operator /(const CheckedInt<T> &lhs, const CheckedInt<T> &rhs)
{
    T x = lhs.mValue;
    T y = rhs.mValue;
    T is_op_valid = CheckedInt_internal::is_div_valid(x, y);
    T result = is_op_valid ? (x / y) : 0;
    
    return CheckedInt<T>(result,
                         lhs.mIsValid & rhs.mIsValid & is_op_valid);
}





template<typename T, typename U>
struct cast_to_CheckedInt_impl
{
    typedef CheckedInt<T> return_type;
    static CheckedInt<T> run(U u) { return u; }
};

template<typename T>
struct cast_to_CheckedInt_impl<T, CheckedInt<T> >
{
    typedef const CheckedInt<T>& return_type;
    static const CheckedInt<T>& run(const CheckedInt<T>& u) { return u; }
};

template<typename T, typename U>
inline typename cast_to_CheckedInt_impl<T, U>::return_type
cast_to_CheckedInt(U u)
{
    return cast_to_CheckedInt_impl<T, U>::run(u);
}

#define CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(OP, COMPOUND_OP) \
template<typename T>                                          \
template<typename U>                                          \
CheckedInt<T>& CheckedInt<T>::operator COMPOUND_OP(U rhs)    \
{                                                             \
    *this = *this OP cast_to_CheckedInt<T>(rhs);                 \
    return *this;                                             \
}                                                             \
template<typename T, typename U>                              \
inline CheckedInt<T> operator OP(const CheckedInt<T> &lhs, U rhs) \
{                                                             \
    return lhs OP cast_to_CheckedInt<T>(rhs);                    \
}                                                             \
template<typename T, typename U>                              \
inline CheckedInt<T> operator OP(U lhs, const CheckedInt<T> &rhs) \
{                                                             \
    return cast_to_CheckedInt<T>(lhs) OP rhs;                    \
}

CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(+, +=)
CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(*, *=)
CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(-, -=)
CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(/, /=)

template<typename T, typename U>
inline bool operator ==(const CheckedInt<T> &lhs, U rhs)
{
    return lhs == cast_to_CheckedInt<T>(rhs);
}

template<typename T, typename U>
inline bool operator ==(U  lhs, const CheckedInt<T> &rhs)
{
    return cast_to_CheckedInt<T>(lhs) == rhs;
}


typedef CheckedInt<int8_t>   CheckedInt8;
typedef CheckedInt<uint8_t>  CheckedUint8;
typedef CheckedInt<int16_t>  CheckedInt16;
typedef CheckedInt<uint16_t> CheckedUint16;
typedef CheckedInt<int32_t>  CheckedInt32;
typedef CheckedInt<uint32_t> CheckedUint32;
typedef CheckedInt<int64_t>  CheckedInt64;
typedef CheckedInt<uint64_t> CheckedUint64;

} 

#endif 
