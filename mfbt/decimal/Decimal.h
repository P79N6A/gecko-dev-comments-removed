



































#ifndef Decimal_h
#define Decimal_h

#include "mozilla/Assertions.h"
#include <stdint.h>
#include "mozilla/Types.h"

#include <string>

#ifndef ASSERT
#define DEFINED_ASSERT_FOR_DECIMAL_H 1
#define ASSERT MOZ_ASSERT
#endif




#define WTF_MAKE_FAST_ALLOCATED \
  void ignore_this_dummy_method() = delete

namespace WebCore {

namespace DecimalPrivate {
class SpecialValueHandler;
} 






class Decimal {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum Sign {
        Positive,
        Negative,
    };

    
    class EncodedData {
        
        friend class Decimal;
        friend class DecimalPrivate::SpecialValueHandler;
    public:
        EncodedData(Sign, int exponent, uint64_t coefficient);

        bool operator==(const EncodedData&) const;
        bool operator!=(const EncodedData& another) const { return !operator==(another); }

        uint64_t coefficient() const { return m_coefficient; }
        int countDigits() const;
        int exponent() const { return m_exponent; }
        bool isFinite() const { return !isSpecial(); }
        bool isInfinity() const { return m_formatClass == ClassInfinity; }
        bool isNaN() const { return m_formatClass == ClassNaN; }
        bool isSpecial() const { return m_formatClass == ClassInfinity || m_formatClass == ClassNaN; }
        bool isZero() const { return m_formatClass == ClassZero; }
        Sign sign() const { return m_sign; }
        void setSign(Sign sign) { m_sign = sign; }

    private:
        enum FormatClass {
            ClassInfinity,
            ClassNormal,
            ClassNaN,
            ClassZero,
        };

        EncodedData(Sign, FormatClass);
        FormatClass formatClass() const { return m_formatClass; }

        uint64_t m_coefficient;
        int16_t m_exponent;
        FormatClass m_formatClass;
        Sign m_sign;
    };

    MFBT_API explicit Decimal(int32_t = 0);
    MFBT_API Decimal(Sign, int exponent, uint64_t coefficient);
    MFBT_API Decimal(const Decimal&);

    MFBT_API Decimal& operator=(const Decimal&);
    MFBT_API Decimal& operator+=(const Decimal&);
    MFBT_API Decimal& operator-=(const Decimal&);
    MFBT_API Decimal& operator*=(const Decimal&);
    MFBT_API Decimal& operator/=(const Decimal&);

    MFBT_API Decimal operator-() const;

    MFBT_API bool operator==(const Decimal&) const;
    MFBT_API bool operator!=(const Decimal&) const;
    MFBT_API bool operator<(const Decimal&) const;
    MFBT_API bool operator<=(const Decimal&) const;
    MFBT_API bool operator>(const Decimal&) const;
    MFBT_API bool operator>=(const Decimal&) const;

    MFBT_API Decimal operator+(const Decimal&) const;
    MFBT_API Decimal operator-(const Decimal&) const;
    MFBT_API Decimal operator*(const Decimal&) const;
    MFBT_API Decimal operator/(const Decimal&) const;

    int exponent() const
    {
        ASSERT(isFinite());
        return m_data.exponent();
    }

    bool isFinite() const { return m_data.isFinite(); }
    bool isInfinity() const { return m_data.isInfinity(); }
    bool isNaN() const { return m_data.isNaN(); }
    bool isNegative() const { return sign() == Negative; }
    bool isPositive() const { return sign() == Positive; }
    bool isSpecial() const { return m_data.isSpecial(); }
    bool isZero() const { return m_data.isZero(); }

    MFBT_API Decimal abs() const;
    MFBT_API Decimal ceiling() const;
    MFBT_API Decimal floor() const;
    MFBT_API Decimal remainder(const Decimal&) const;
    MFBT_API Decimal round() const;

    MFBT_API double toDouble() const;
    
    MFBT_API std::string toString() const;
    MFBT_API bool toString(char* strBuf, size_t bufLength) const;

    static MFBT_API Decimal fromDouble(double);
    
    
    
    
    
    
    
    static MFBT_API Decimal fromString(const std::string& aValue);
    static MFBT_API Decimal infinity(Sign);
    static MFBT_API Decimal nan();
    static MFBT_API Decimal zero(Sign);

    
    MFBT_API explicit Decimal(const EncodedData&);
    const EncodedData& value() const { return m_data; }

private:
    struct AlignedOperands {
        uint64_t lhsCoefficient;
        uint64_t rhsCoefficient;
        int exponent;
    };

    MFBT_API explicit Decimal(double);
    MFBT_API Decimal compareTo(const Decimal&) const;

    static MFBT_API AlignedOperands alignOperands(const Decimal& lhs, const Decimal& rhs);
    static inline Sign invertSign(Sign sign) { return sign == Negative ? Positive : Negative; }

    Sign sign() const { return m_data.sign(); }

    EncodedData m_data;
};

} 

namespace mozilla {
  typedef WebCore::Decimal Decimal;
} 

#undef WTF_MAKE_FAST_ALLOCATED

#ifdef DEFINED_ASSERT_FOR_DECIMAL_H
#undef DEFINED_ASSERT_FOR_DECIMAL_H
#undef ASSERT
#endif

#endif

