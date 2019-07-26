



#ifndef SK_CONVOLVER_H
#define SK_CONVOLVER_H

#include "SkSize.h"
#include "SkTypes.h"
#include "SkTArray.h"


#if defined(__APPLE__)
#undef FloatToConvolutionFixed
#undef ConvolutionFixedToFloat
#undef FloatToFixed
#undef FixedToFloat
#endif









class SkConvolutionFilter1D {
public:
    typedef short ConvolutionFixed;

    
    enum { kShiftBits = 14 };

    SK_API SkConvolutionFilter1D();
    SK_API ~SkConvolutionFilter1D();

    
    static ConvolutionFixed FloatToFixed(float f) {
        return static_cast<ConvolutionFixed>(f * (1 << kShiftBits));
    }
    static unsigned char FixedToChar(ConvolutionFixed x) {
        return static_cast<unsigned char>(x >> kShiftBits);
    }
    static float FixedToFloat(ConvolutionFixed x) {
        
        
        
        SK_COMPILE_ASSERT(sizeof(ConvolutionFixed) == 2, ConvolutionFixed_type_should_fit_in_float_mantissa);
        float raw = static_cast<float>(x);
        return ldexpf(raw, -kShiftBits);
    }

    
    int maxFilter() const { return fMaxFilter; }

    
    
    int numValues() const { return static_cast<int>(fFilters.count()); }

    
    
    
    
    
    
    
    
    
    
    
    
    SK_API void AddFilter(int filterOffset,
                          const float* filterValues,
                          int filterLength);

    
    void AddFilter(int filterOffset,
                   const ConvolutionFixed* filterValues,
                   int filterLength);

    
    
    
    
    
    inline const ConvolutionFixed* FilterForValue(int valueOffset,
                                       int* filterOffset,
                                       int* filterLength) const {
        const FilterInstance& filter = fFilters[valueOffset];
        *filterOffset = filter.fOffset;
        *filterLength = filter.fTrimmedLength;
        if (filter.fTrimmedLength == 0) {
            return NULL;
        }
        return &fFilterValues[filter.fDataLocation];
    }

  
  
  
  
  
  
  
  
    SK_API const ConvolutionFixed* GetSingleFilter(int* specifiedFilterLength,
        int* filterOffset,
        int* filterLength) const;

    
    

    void addFilterValue( ConvolutionFixed val ) {
        fFilterValues.push_back( val );
    }
private:
    struct FilterInstance {
        
        int fDataLocation;

        
        int fOffset;

        
        int fTrimmedLength;

        
        
        
        int fLength;
    };

    
    SkTArray<FilterInstance> fFilters;

    
    
    
    SkTArray<ConvolutionFixed> fFilterValues;

    
    int fMaxFilter;
};

typedef void (*SkConvolveVertically_pointer)(
    const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
    int filterLength,
    unsigned char* const* sourceDataRows,
    int pixelWidth,
    unsigned char* outRow,
    bool hasAlpha);
typedef void (*SkConvolve4RowsHorizontally_pointer)(
    const unsigned char* srcData[4],
    const SkConvolutionFilter1D& filter,
    unsigned char* outRow[4]);
typedef void (*SkConvolveHorizontally_pointer)(
    const unsigned char* srcData,
    const SkConvolutionFilter1D& filter,
    unsigned char* outRow,
    bool hasAlpha);
typedef void (*SkConvolveFilterPadding_pointer)(
    SkConvolutionFilter1D* filter);

struct SkConvolutionProcs {
  
  
    int fExtraHorizontalReads;
    SkConvolveVertically_pointer fConvolveVertically;
    SkConvolve4RowsHorizontally_pointer fConvolve4RowsHorizontally;
    SkConvolveHorizontally_pointer fConvolveHorizontally;
    SkConvolveFilterPadding_pointer fApplySIMDPadding;
};





















SK_API void BGRAConvolve2D(const unsigned char* sourceData,
    int sourceByteRowStride,
    bool sourceHasAlpha,
    const SkConvolutionFilter1D& xfilter,
    const SkConvolutionFilter1D& yfilter,
    int outputByteRowStride,
    unsigned char* output,
    const SkConvolutionProcs&,
    bool useSimdIfPossible);

#endif  
