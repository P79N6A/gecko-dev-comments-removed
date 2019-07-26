





























#ifndef Decimal_h
#define Decimal_h

#include <stdint.h>
#include <wtf/Assertions.h>
#include <wtf/text/WTFString.h>

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

    Decimal(int32_t = 0);
    Decimal(Sign, int exponent, uint64_t coefficient);
    Decimal(const Decimal&);

    Decimal& operator=(const Decimal&);
    Decimal& operator+=(const Decimal&);
    Decimal& operator-=(const Decimal&);
    Decimal& operator*=(const Decimal&);
    Decimal& operator/=(const Decimal&);

    Decimal operator-() const;

    bool operator==(const Decimal&) const;
    bool operator!=(const Decimal&) const;
    bool operator<(const Decimal&) const;
    bool operator<=(const Decimal&) const;
    bool operator>(const Decimal&) const;
    bool operator>=(const Decimal&) const;

    Decimal operator+(const Decimal&) const;
    Decimal operator-(const Decimal&) const;
    Decimal operator*(const Decimal&) const;
    Decimal operator/(const Decimal&) const;

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

    Decimal abs() const;
    Decimal ceiling() const;
    Decimal floor() const;
    Decimal remainder(const Decimal&) const;
    Decimal round() const;

    double toDouble() const;
    
    String toString() const;

    static Decimal fromDouble(double);
    
    
    
    
    
    
    
    static Decimal fromString(const String&);
    static Decimal infinity(Sign);
    static Decimal nan();
    static Decimal zero(Sign);

    
    explicit Decimal(const EncodedData&);
    const EncodedData& value() const { return m_data; }

private:
    struct AlignedOperands {
        uint64_t lhsCoefficient;
        uint64_t rhsCoefficient;
        int exponent;
    };

    Decimal(double);
    Decimal compareTo(const Decimal&) const;

    static AlignedOperands alignOperands(const Decimal& lhs, const Decimal& rhs);
    static inline Sign invertSign(Sign sign) { return sign == Negative ? Positive : Negative; }

    Sign sign() const { return m_data.sign(); }

    EncodedData m_data;
};

} 

#endif

