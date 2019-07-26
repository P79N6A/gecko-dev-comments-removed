






#ifndef SkFloatUtils_DEFINED
#define SkFloatUtils_DEFINED

#include "SkTypes.h"
#include <limits.h>
#include <float.h>

template <size_t size>
class SkTypeWithSize {
public:
    
    typedef void UInt;
};

template <>
class SkTypeWithSize<32> {
public:
    typedef uint32_t UInt;
};

template <>
class SkTypeWithSize<64> {
public:
    typedef uint64_t UInt;
};

template <typename RawType>
struct SkNumericLimits {
    static const int digits = 0;
};

template <>
struct SkNumericLimits<double> {
    static const int digits = DBL_MANT_DIG;
};

template <>
struct SkNumericLimits<float> {
    static const int digits = FLT_MANT_DIG;
};






template <typename RawType, unsigned int ULPs>
class SkFloatingPoint {
public:
    
    typedef typename SkTypeWithSize<sizeof(RawType) * CHAR_BIT>::UInt Bits;

    
    static const size_t kBitCount = CHAR_BIT * sizeof(RawType);

    
    static const size_t kFractionBitCount = SkNumericLimits<RawType>::digits - 1;

    
    static const size_t kExponentBitCount = kBitCount - 1 - kFractionBitCount;

    
    static const Bits kSignBitMask = static_cast<Bits>(1) << (kBitCount - 1);

    
    static const Bits kFractionBitMask =
        ~static_cast<Bits>(0) >> (kExponentBitCount + 1);

    
    static const Bits kExponentBitMask = ~(kSignBitMask | kFractionBitMask);

    
    static const size_t kMaxUlps = ULPs;

    







    explicit SkFloatingPoint(const RawType& x) { fU.value = x; }

    
    Bits exponent_bits() const { return kExponentBitMask & fU.bits; }

    
    Bits fraction_bits() const { return kFractionBitMask & fU.bits; }

    
    bool is_nan() const {
        
        
        
        return (exponent_bits() == kExponentBitMask) && (fraction_bits() != 0);
    }

    






    bool AlmostEquals(const SkFloatingPoint& rhs) const {
        
        if (is_nan() || rhs.is_nan()) return false;

        const Bits dist = DistanceBetweenSignAndMagnitudeNumbers(fU.bits,
                                                                 rhs.fU.bits);
        
        return dist <= kMaxUlps;
    }

private:
    
    union FloatingPointUnion {
        
        RawType value;
        
        Bits bits;
    };

    
















    static Bits SignAndMagnitudeToBiased(const Bits &sam) {
        if (kSignBitMask & sam) {
            
            return ~sam + 1;
        } else {
            
            return kSignBitMask | sam;
        }
    }

    



    static Bits DistanceBetweenSignAndMagnitudeNumbers(const Bits &sam1,
                                                       const Bits &sam2) {
        const Bits biased1 = SignAndMagnitudeToBiased(sam1);
        const Bits biased2 = SignAndMagnitudeToBiased(sam2);
        return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
    }

    FloatingPointUnion fU;
};

#endif
