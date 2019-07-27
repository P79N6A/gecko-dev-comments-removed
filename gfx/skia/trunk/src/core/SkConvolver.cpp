



#include "SkConvolver.h"
#include "SkSize.h"
#include "SkTypes.h"

namespace {

    
    
    inline unsigned char ClampTo8(int a) {
        if (static_cast<unsigned>(a) < 256) {
            return a;  
        }
        if (a < 0) {
            return 0;
        }
        return 255;
    }

    
    
    
    class CircularRowBuffer {
    public:
        
        
        
        
        
        
        CircularRowBuffer(int destRowPixelWidth, int maxYFilterSize,
                          int firstInputRow)
            : fRowByteWidth(destRowPixelWidth * 4),
              fNumRows(maxYFilterSize),
              fNextRow(0),
              fNextRowCoordinate(firstInputRow) {
            fBuffer.reset(fRowByteWidth * maxYFilterSize);
            fRowAddresses.reset(fNumRows);
        }

        
        
        unsigned char* advanceRow() {
            unsigned char* row = &fBuffer[fNextRow * fRowByteWidth];
            fNextRowCoordinate++;

            
            fNextRow++;
            if (fNextRow == fNumRows) {
                fNextRow = 0;
            }
            return row;
        }

        
        
        
        
        
        
        unsigned char* const* GetRowAddresses(int* firstRowIndex) {
            
            
            
            
            
            
            
            
            
            
            *firstRowIndex = fNextRowCoordinate - fNumRows;

            int curRow = fNextRow;
            for (int i = 0; i < fNumRows; i++) {
                fRowAddresses[i] = &fBuffer[curRow * fRowByteWidth];

                
                curRow++;
                if (curRow == fNumRows) {
                    curRow = 0;
                }
            }
            return &fRowAddresses[0];
        }

    private:
        
        SkTArray<unsigned char> fBuffer;

        
        int fRowByteWidth;

        
        int fNumRows;

        
        
        int fNextRow;

        
        
        int fNextRowCoordinate;

        
        SkTArray<unsigned char*> fRowAddresses;
    };



template<bool hasAlpha>
    void ConvolveHorizontally(const unsigned char* srcData,
                              const SkConvolutionFilter1D& filter,
                              unsigned char* outRow) {
        
        int numValues = filter.numValues();
        for (int outX = 0; outX < numValues; outX++) {
            
            int filterOffset, filterLength;
            const SkConvolutionFilter1D::ConvolutionFixed* filterValues =
                filter.FilterForValue(outX, &filterOffset, &filterLength);

            
            
            const unsigned char* rowToFilter = &srcData[filterOffset * 4];

            
            int accum[4] = {0};
            for (int filterX = 0; filterX < filterLength; filterX++) {
                SkConvolutionFilter1D::ConvolutionFixed curFilter = filterValues[filterX];
                accum[0] += curFilter * rowToFilter[filterX * 4 + 0];
                accum[1] += curFilter * rowToFilter[filterX * 4 + 1];
                accum[2] += curFilter * rowToFilter[filterX * 4 + 2];
                if (hasAlpha) {
                    accum[3] += curFilter * rowToFilter[filterX * 4 + 3];
                }
            }

            
            
            accum[0] >>= SkConvolutionFilter1D::kShiftBits;
            accum[1] >>= SkConvolutionFilter1D::kShiftBits;
            accum[2] >>= SkConvolutionFilter1D::kShiftBits;
            if (hasAlpha) {
                accum[3] >>= SkConvolutionFilter1D::kShiftBits;
            }

            
            outRow[outX * 4 + 0] = ClampTo8(accum[0]);
            outRow[outX * 4 + 1] = ClampTo8(accum[1]);
            outRow[outX * 4 + 2] = ClampTo8(accum[2]);
            if (hasAlpha) {
                outRow[outX * 4 + 3] = ClampTo8(accum[3]);
            }
        }
    }

    
    
    #if defined(__i386) && SK_HAS_ATTRIBUTE(optimize) && defined(SK_RELEASE)
        #define SK_MAYBE_DISABLE_VECTORIZATION __attribute__((optimize("O2"), noinline))
    #else
        #define SK_MAYBE_DISABLE_VECTORIZATION
    #endif

    SK_MAYBE_DISABLE_VECTORIZATION
    static void ConvolveHorizontallyAlpha(const unsigned char* srcData,
                                          const SkConvolutionFilter1D& filter,
                                          unsigned char* outRow) {
        return ConvolveHorizontally<true>(srcData, filter, outRow);
    }

    SK_MAYBE_DISABLE_VECTORIZATION
    static void ConvolveHorizontallyNoAlpha(const unsigned char* srcData,
                                            const SkConvolutionFilter1D& filter,
                                            unsigned char* outRow) {
        return ConvolveHorizontally<false>(srcData, filter, outRow);
    }

    #undef SK_MAYBE_DISABLE_VECTORIZATION








template<bool hasAlpha>
    void ConvolveVertically(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                            int filterLength,
                            unsigned char* const* sourceDataRows,
                            int pixelWidth,
                            unsigned char* outRow) {
        
        
        for (int outX = 0; outX < pixelWidth; outX++) {
            
            
            int byteOffset = outX * 4;

            
            int accum[4] = {0};
            for (int filterY = 0; filterY < filterLength; filterY++) {
                SkConvolutionFilter1D::ConvolutionFixed curFilter = filterValues[filterY];
                accum[0] += curFilter * sourceDataRows[filterY][byteOffset + 0];
                accum[1] += curFilter * sourceDataRows[filterY][byteOffset + 1];
                accum[2] += curFilter * sourceDataRows[filterY][byteOffset + 2];
                if (hasAlpha) {
                    accum[3] += curFilter * sourceDataRows[filterY][byteOffset + 3];
                }
            }

            
            
            accum[0] >>= SkConvolutionFilter1D::kShiftBits;
            accum[1] >>= SkConvolutionFilter1D::kShiftBits;
            accum[2] >>= SkConvolutionFilter1D::kShiftBits;
            if (hasAlpha) {
                accum[3] >>= SkConvolutionFilter1D::kShiftBits;
            }

            
            outRow[byteOffset + 0] = ClampTo8(accum[0]);
            outRow[byteOffset + 1] = ClampTo8(accum[1]);
            outRow[byteOffset + 2] = ClampTo8(accum[2]);
            if (hasAlpha) {
                unsigned char alpha = ClampTo8(accum[3]);

                
                
                
                
                
                
                
                int maxColorChannel = SkTMax(outRow[byteOffset + 0],
                                               SkTMax(outRow[byteOffset + 1],
                                                      outRow[byteOffset + 2]));
                if (alpha < maxColorChannel) {
                    outRow[byteOffset + 3] = maxColorChannel;
                } else {
                    outRow[byteOffset + 3] = alpha;
                }
            } else {
                
                outRow[byteOffset + 3] = 0xff;
            }
        }
    }

    void ConvolveVertically(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                            int filterLength,
                            unsigned char* const* sourceDataRows,
                            int pixelWidth,
                            unsigned char* outRow,
                            bool sourceHasAlpha) {
        if (sourceHasAlpha) {
            ConvolveVertically<true>(filterValues, filterLength,
                                     sourceDataRows, pixelWidth,
                                     outRow);
        } else {
            ConvolveVertically<false>(filterValues, filterLength,
                                      sourceDataRows, pixelWidth,
                                      outRow);
        }
    }

}  



SkConvolutionFilter1D::SkConvolutionFilter1D()
: fMaxFilter(0) {
}

SkConvolutionFilter1D::~SkConvolutionFilter1D() {
}

void SkConvolutionFilter1D::AddFilter(int filterOffset,
                                      const float* filterValues,
                                      int filterLength) {
    SkASSERT(filterLength > 0);

    SkTArray<ConvolutionFixed> fixedValues;
    fixedValues.reset(filterLength);

    for (int i = 0; i < filterLength; ++i) {
        fixedValues.push_back(FloatToFixed(filterValues[i]));
    }

    AddFilter(filterOffset, &fixedValues[0], filterLength);
}

void SkConvolutionFilter1D::AddFilter(int filterOffset,
                                      const ConvolutionFixed* filterValues,
                                      int filterLength) {
    
    
    
    
    int filterSize = filterLength;
    int firstNonZero = 0;
    while (firstNonZero < filterLength && filterValues[firstNonZero] == 0) {
        firstNonZero++;
    }

    if (firstNonZero < filterLength) {
        
        int lastNonZero = filterLength - 1;
        while (lastNonZero >= 0 && filterValues[lastNonZero] == 0) {
            lastNonZero--;
        }

        filterOffset += firstNonZero;
        filterLength = lastNonZero + 1 - firstNonZero;
        SkASSERT(filterLength > 0);

        for (int i = firstNonZero; i <= lastNonZero; i++) {
            fFilterValues.push_back(filterValues[i]);
        }
    } else {
        
        filterLength = 0;
    }

    FilterInstance instance;

    
    instance.fDataLocation = (static_cast<int>(fFilterValues.count()) -
                                               filterLength);
    instance.fOffset = filterOffset;
    instance.fTrimmedLength = filterLength;
    instance.fLength = filterSize;
    fFilters.push_back(instance);

    fMaxFilter = SkTMax(fMaxFilter, filterLength);
}

const SkConvolutionFilter1D::ConvolutionFixed* SkConvolutionFilter1D::GetSingleFilter(
                                        int* specifiedFilterlength,
                                        int* filterOffset,
                                        int* filterLength) const {
    const FilterInstance& filter = fFilters[0];
    *filterOffset = filter.fOffset;
    *filterLength = filter.fTrimmedLength;
    *specifiedFilterlength = filter.fLength;
    if (filter.fTrimmedLength == 0) {
        return NULL;
    }

    return &fFilterValues[filter.fDataLocation];
}

void BGRAConvolve2D(const unsigned char* sourceData,
                    int sourceByteRowStride,
                    bool sourceHasAlpha,
                    const SkConvolutionFilter1D& filterX,
                    const SkConvolutionFilter1D& filterY,
                    int outputByteRowStride,
                    unsigned char* output,
                    const SkConvolutionProcs& convolveProcs,
                    bool useSimdIfPossible) {

    int maxYFilterSize = filterY.maxFilter();

    
    
    
    
    
    int filterOffset, filterLength;
    const SkConvolutionFilter1D::ConvolutionFixed* filterValues =
        filterY.FilterForValue(0, &filterOffset, &filterLength);
    int nextXRow = filterOffset;

    
    
    
    
    
    
    
    
    
    
    int rowBufferWidth = (filterX.numValues() + 15) & ~0xF;
    int rowBufferHeight = maxYFilterSize +
                          (convolveProcs.fConvolve4RowsHorizontally ? 4 : 0);
    CircularRowBuffer rowBuffer(rowBufferWidth,
                                rowBufferHeight,
                                filterOffset);

    
    
    SkASSERT(outputByteRowStride >= filterX.numValues() * 4);
    int numOutputRows = filterY.numValues();

    
    
    int lastFilterOffset, lastFilterLength;

    
    
    
    
    
    
    
    filterX.FilterForValue(filterX.numValues() - 1, &lastFilterOffset,
                           &lastFilterLength);
    int avoidSimdRows = 1 + convolveProcs.fExtraHorizontalReads /
        (lastFilterOffset + lastFilterLength);

    filterY.FilterForValue(numOutputRows - 1, &lastFilterOffset,
                           &lastFilterLength);

    for (int outY = 0; outY < numOutputRows; outY++) {
        filterValues = filterY.FilterForValue(outY,
                                              &filterOffset, &filterLength);

        
        while (nextXRow < filterOffset + filterLength) {
            if (convolveProcs.fConvolve4RowsHorizontally &&
                nextXRow + 3 < lastFilterOffset + lastFilterLength -
                avoidSimdRows) {
                const unsigned char* src[4];
                unsigned char* outRow[4];
                for (int i = 0; i < 4; ++i) {
                    src[i] = &sourceData[(uint64_t)(nextXRow + i) * sourceByteRowStride];
                    outRow[i] = rowBuffer.advanceRow();
                }
                convolveProcs.fConvolve4RowsHorizontally(src, filterX, outRow);
                nextXRow += 4;
            } else {
                
                if (convolveProcs.fConvolveHorizontally &&
                    nextXRow < lastFilterOffset + lastFilterLength -
                    avoidSimdRows) {
                    convolveProcs.fConvolveHorizontally(
                        &sourceData[(uint64_t)nextXRow * sourceByteRowStride],
                        filterX, rowBuffer.advanceRow(), sourceHasAlpha);
                } else {
                    if (sourceHasAlpha) {
                        ConvolveHorizontallyAlpha(
                            &sourceData[(uint64_t)nextXRow * sourceByteRowStride],
                            filterX, rowBuffer.advanceRow());
                    } else {
                        ConvolveHorizontallyNoAlpha(
                            &sourceData[(uint64_t)nextXRow * sourceByteRowStride],
                            filterX, rowBuffer.advanceRow());
                    }
                }
                nextXRow++;
            }
        }

        
        unsigned char* curOutputRow = &output[(uint64_t)outY * outputByteRowStride];

        
        int firstRowInCircularBuffer;
        unsigned char* const* rowsToConvolve =
            rowBuffer.GetRowAddresses(&firstRowInCircularBuffer);

        
        
        unsigned char* const* firstRowForFilter =
            &rowsToConvolve[filterOffset - firstRowInCircularBuffer];

        if (convolveProcs.fConvolveVertically) {
            convolveProcs.fConvolveVertically(filterValues, filterLength,
                                               firstRowForFilter,
                                               filterX.numValues(), curOutputRow,
                                               sourceHasAlpha);
        } else {
            ConvolveVertically(filterValues, filterLength,
                               firstRowForFilter,
                               filterX.numValues(), curOutputRow,
                               sourceHasAlpha);
        }
    }
}
