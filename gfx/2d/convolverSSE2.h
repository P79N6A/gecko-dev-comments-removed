



























#ifndef SKIA_EXT_CONVOLVER_SSE_H_
#define SKIA_EXT_CONVOLVER_SSE_H_

#include "convolver.h"

#include <algorithm>

#include "skia/SkTypes.h"

namespace skia {



void ConvolveHorizontally_SSE2(const unsigned char* src_data,
                               int begin, int end,
                               const ConvolutionFilter1D& filter,
                               unsigned char* out_row);





void ConvolveHorizontally4_SSE2(const unsigned char* src_data[4],
                                int begin, int end,
                                const ConvolutionFilter1D& filter,
                                unsigned char* out_row[4]);







void ConvolveVertically_SSE2(const ConvolutionFilter1D::Fixed* filter_values,
                             int filter_length,
                             unsigned char* const* source_data_rows,
                             int begin, int end,
                             unsigned char* out_row, bool has_alpha);

}  

#endif  
