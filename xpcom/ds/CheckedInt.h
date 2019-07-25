






































#ifndef mozilla_CheckedInt_h
#define mozilla_CheckedInt_h

#include "prtypes.h"

#include <climits>

namespace mozilla {

namespace CheckedInt_internal {








struct unsupported_type {};

template<typename T> struct integer_type_manually_recorded_info
{
    enum { is_supported = 0 };
    typedef unsupported_type twice_bigger_type;
};


#define CHECKEDINT_REGISTER_SUPPORTED_TYPE(T,_twice_bigger_type)  \
template<> struct integer_type_manually_recorded_info<T>       \
{                                                              \
    enum { is_supported = 1 };                                 \
    typedef _twice_bigger_type twice_bigger_type;              \
    static void TYPE_NOT_SUPPORTED_BY_CheckedInt() {}             \
};

CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRInt8,   PRInt16)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRUint8,  PRUint16)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRInt16,  PRInt32)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRUint16, PRUint32)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRInt32,  PRInt64)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRUint32, PRUint64)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRInt64,  unsupported_type)
CHECKEDINT_REGISTER_SUPPORTED_TYPE(PRUint64, unsupported_type)









template<typename T> struct is_unsupported_type { enum { answer = 0 }; };
template<> struct is_unsupported_type<unsupported_type> { enum { answer = 1 }; };

template<typename T> struct integer_traits
{
    typedef typename integer_type_manually_recorded_info<T>::twice_bigger_type twice_bigger_type;

    enum {
        is_supported = integer_type_manually_recorded_info<T>::is_supported,
        twice_bigger_type_is_supported
            = is_unsupported_type<
                  typename integer_type_manually_recorded_info<T>::twice_bigger_type
              >::answer ? 0 : 1,
        size = sizeof(T),
        position_of_sign_bit = CHAR_BIT * size - 1,
        is_signed = (T(-1) > T(0)) ? 0 : 1
    };

    static T min()
    {
        
        return is_signed ? T(T(1) << position_of_sign_bit) : T(0);
    }

    static T max()
    {
        return ~min();
    }
};







template<typename T> inline T has_sign_bit(T x)
{
    return x >> integer_traits<T>::position_of_sign_bit;
}

template<typename T> inline T binary_complement(T x)
{
    return ~x;
}

template<typename T, typename U,
         bool is_T_signed = integer_traits<T>::is_signed,
         bool is_U_signed = integer_traits<U>::is_signed>
struct is_in_range_impl {};

template<typename T, typename U>
struct is_in_range_impl<T, U, true, true>
{
    static T run(U x)
    {
        return (x <= integer_traits<T>::max()) &
               (x >= integer_traits<T>::min());
    }
};

template<typename T, typename U>
struct is_in_range_impl<T, U, false, false>
{
    static T run(U x)
    {
        return x <= integer_traits<T>::max();
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
            return x <= U(integer_traits<T>::max());
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
            return x >= 0 && x <= U(integer_traits<T>::max());
    }
};

template<typename T, typename U> inline T is_in_range(U x)
{
    return is_in_range_impl<T, U>::run(x);
}

template<typename T> inline T is_add_valid(T x, T y, T result)
{
    return integer_traits<T>::is_signed ?
                        
                        
                        
                        has_sign_bit(binary_complement(T((result^x) & (result^y))))
                    :
                        binary_complement(x) >= y;
}

template<typename T> inline T is_sub_valid(T x, T y, T result)
{
    return integer_traits<T>::is_signed ?
                        
                        has_sign_bit(binary_complement(T((result^x) & (x^y))))
                    :
                        x >= y;
}

template<typename T,
         bool is_signed =  integer_traits<T>::is_signed,
         bool twice_bigger_type_is_supported = integer_traits<T>::twice_bigger_type_is_supported>
struct is_mul_valid_impl {};

template<typename T>
struct is_mul_valid_impl<T, true, true>
{
    static T run(T x, T y)
    {
        typedef typename integer_traits<T>::twice_bigger_type twice_bigger_type;
        twice_bigger_type product = twice_bigger_type(x) * twice_bigger_type(y);
        return is_in_range<T>(product);
    }
};

template<typename T>
struct is_mul_valid_impl<T, false, true>
{
    static T run(T x, T y)
    {
        typedef typename integer_traits<T>::twice_bigger_type twice_bigger_type;
        twice_bigger_type product = twice_bigger_type(x) * twice_bigger_type(y);
        return is_in_range<T>(product);
    }
};

template<typename T>
struct is_mul_valid_impl<T, true, false>
{
    static T run(T x, T y)
    {
        const T max_value = integer_traits<T>::max();
        const T min_value = integer_traits<T>::min();

        if (x == 0 || y == 0) return true;

        if (x > 0) {
            if (y > 0)
                return x <= max_value / y;
            else
                return y >= min_value / x;
        } else {
            if (y > 0)
                return x >= min_value / y;
            else
                return y >= max_value / x;
        }
    }
};

template<typename T>
struct is_mul_valid_impl<T, false, false>
{
    static T run(T x, T y)
    {
        const T max_value = integer_traits<T>::max();
        if (x == 0 || y == 0) return true;
        return x <= max_value / y;
    }
};

template<typename T> inline T is_mul_valid(T x, T y, T )
{
    return is_mul_valid_impl<T>::run(x, y);
}

template<typename T> inline T is_div_valid(T x, T y)
{
    return integer_traits<T>::is_signed ?
                        
                        y != 0 && (x != integer_traits<T>::min() || y != T(-1))
                    :
                        y != 0;
}

} 























































template<typename T>
class CheckedInt
{
protected:
    T mValue;
    T mIsValid; 
                

    template<typename U>
    CheckedInt(const U& value, PRBool isValid) : mValue(value), mIsValid(isValid)
    {
        CheckedInt_internal::integer_type_manually_recorded_info<T>
            ::TYPE_NOT_SUPPORTED_BY_CheckedInt();
    }

public:
    







    template<typename U>
    CheckedInt(const U& value)
        : mValue(value),
          mIsValid(CheckedInt_internal::is_in_range<T>(value))
    {
        CheckedInt_internal::integer_type_manually_recorded_info<T>
            ::TYPE_NOT_SUPPORTED_BY_CheckedInt();
    }

    
    CheckedInt() : mIsValid(1)
    {
        CheckedInt_internal::integer_type_manually_recorded_info<T>
            ::TYPE_NOT_SUPPORTED_BY_CheckedInt();
    }

    
    T value() const { return mValue; }

    


    PRBool valid() const { return mIsValid; }

    
    template<typename U> friend CheckedInt<U> operator +(const CheckedInt<U>& lhs, const CheckedInt<U>& rhs);
    
    template<typename U> CheckedInt& operator +=(const U &rhs);
    
    template<typename U> friend CheckedInt<U> operator -(const CheckedInt<U>& lhs, const CheckedInt<U> &rhs);
    
    template<typename U> CheckedInt& operator -=(const U &rhs);
    
    template<typename U> friend CheckedInt<U> operator *(const CheckedInt<U>& lhs, const CheckedInt<U> &rhs);
    
    template<typename U> CheckedInt& operator *=(const U &rhs);
    
    template<typename U> friend CheckedInt<U> operator /(const CheckedInt<U>& lhs, const CheckedInt<U> &rhs);
    
    template<typename U> CheckedInt& operator /=(const U &rhs);

    
    CheckedInt operator -() const
    {
        T result = -value();
        
        return CheckedInt(result,
                       mIsValid & CheckedInt_internal::is_sub_valid(T(0), value(), result));
    }

    
    PRBool operator ==(const CheckedInt& other) const
    {
        return PRBool(mIsValid & other.mIsValid & T(value() == other.value()));
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
    PRBool operator !=(const U& other) const { return !(*this == other); }
};

#define CHECKEDINT_BASIC_BINARY_OPERATOR(NAME, OP)               \
template<typename T>                                          \
inline CheckedInt<T> operator OP(const CheckedInt<T> &lhs, const CheckedInt<T> &rhs) \
{                                                             \
    T x = lhs.value();                                        \
    T y = rhs.value();                                        \
    T result = x OP y;                                        \
    T is_op_valid                                             \
        = CheckedInt_internal::is_##NAME##_valid(x, y, result);  \
    /* give the compiler a good chance to perform RVO */      \
    return CheckedInt<T>(result,                                 \
                      lhs.mIsValid &                          \
                      rhs.mIsValid &                          \
                      is_op_valid);                           \
}

CHECKEDINT_BASIC_BINARY_OPERATOR(add, +)
CHECKEDINT_BASIC_BINARY_OPERATOR(sub, -)
CHECKEDINT_BASIC_BINARY_OPERATOR(mul, *)



template<typename T>
inline CheckedInt<T> operator /(const CheckedInt<T> &lhs, const CheckedInt<T> &rhs)
{
    T x = lhs.value();
    T y = rhs.value();
    T is_op_valid = CheckedInt_internal::is_div_valid(x, y);
    T result = is_op_valid ? (x / y) : 0;
    
    return CheckedInt<T>(result,
                      lhs.mIsValid &
                      rhs.mIsValid &
                      is_op_valid);
}





template<typename T, typename U>
struct cast_to_CheckedInt_impl
{
    typedef CheckedInt<T> return_type;
    static CheckedInt<T> run(const U& u) { return u; }
};

template<typename T>
struct cast_to_CheckedInt_impl<T, CheckedInt<T> >
{
    typedef const CheckedInt<T>& return_type;
    static const CheckedInt<T>& run(const CheckedInt<T>& u) { return u; }
};

template<typename T, typename U>
inline typename cast_to_CheckedInt_impl<T, U>::return_type
cast_to_CheckedInt(const U& u)
{
    return cast_to_CheckedInt_impl<T, U>::run(u);
}

#define CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(OP, COMPOUND_OP) \
template<typename T>                                          \
template<typename U>                                          \
CheckedInt<T>& CheckedInt<T>::operator COMPOUND_OP(const U &rhs)    \
{                                                             \
    *this = *this OP cast_to_CheckedInt<T>(rhs);                 \
    return *this;                                             \
}                                                             \
template<typename T, typename U>                              \
inline CheckedInt<T> operator OP(const CheckedInt<T> &lhs, const U &rhs) \
{                                                             \
    return lhs OP cast_to_CheckedInt<T>(rhs);                    \
}                                                             \
template<typename T, typename U>                              \
inline CheckedInt<T> operator OP(const U & lhs, const CheckedInt<T> &rhs) \
{                                                             \
    return cast_to_CheckedInt<T>(lhs) OP rhs;                    \
}

CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(+, +=)
CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(*, *=)
CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(-, -=)
CHECKEDINT_CONVENIENCE_BINARY_OPERATORS(/, /=)

template<typename T, typename U>
inline PRBool operator ==(const CheckedInt<T> &lhs, const U &rhs)
{
    return lhs == cast_to_CheckedInt<T>(rhs);
}

template<typename T, typename U>
inline PRBool operator ==(const U & lhs, const CheckedInt<T> &rhs)
{
    return cast_to_CheckedInt<T>(lhs) == rhs;
}



#define CHECKEDINT_MAKE_TYPEDEF(Type) \
typedef CheckedInt<PR##Type> Checked##Type;

CHECKEDINT_MAKE_TYPEDEF(Int8)
CHECKEDINT_MAKE_TYPEDEF(Uint8)
CHECKEDINT_MAKE_TYPEDEF(Int16)
CHECKEDINT_MAKE_TYPEDEF(Uint16)
CHECKEDINT_MAKE_TYPEDEF(Int32)
CHECKEDINT_MAKE_TYPEDEF(Uint32)
CHECKEDINT_MAKE_TYPEDEF(Int64)
CHECKEDINT_MAKE_TYPEDEF(Uint64)

} 

#endif 
