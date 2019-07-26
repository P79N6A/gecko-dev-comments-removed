#include "SkBitmapScaler.h"
#include "SkBitmapFilter.h"
#include "SkRect.h"
#include "SkTArray.h"
#include "SkErrorInternals.h"
#include "SkConvolver.h"





class SkResizeFilter {
public:
    SkResizeFilter(SkBitmapScaler::ResizeMethod method,
                   int srcFullWidth, int srcFullHeight,
                   int destWidth, int destHeight,
                   const SkIRect& destSubset,
                   const SkConvolutionProcs& convolveProcs);
    ~SkResizeFilter() {
        SkDELETE( fBitmapFilter );
    }

    
    const SkConvolutionFilter1D& xFilter() { return fXFilter; }
    const SkConvolutionFilter1D& yFilter() { return fYFilter; }

private:

    SkBitmapFilter* fBitmapFilter;

    
    
    
    
    
    
    
    
    
    

    void computeFilters(int srcSize,
                        int destSubsetLo, int destSubsetSize,
                        float scale,
                        SkConvolutionFilter1D* output,
                        const SkConvolutionProcs& convolveProcs);

    SkConvolutionFilter1D fXFilter;
    SkConvolutionFilter1D fYFilter;
};

SkResizeFilter::SkResizeFilter(SkBitmapScaler::ResizeMethod method,
                               int srcFullWidth, int srcFullHeight,
                               int destWidth, int destHeight,
                               const SkIRect& destSubset,
                               const SkConvolutionProcs& convolveProcs) {

    
    SkASSERT((SkBitmapScaler::RESIZE_FIRST_ALGORITHM_METHOD <= method) &&
             (method <= SkBitmapScaler::RESIZE_LAST_ALGORITHM_METHOD));

    switch(method) {
        case SkBitmapScaler::RESIZE_BOX:
            fBitmapFilter = SkNEW(SkBoxFilter);
            break;
        case SkBitmapScaler::RESIZE_TRIANGLE:
            fBitmapFilter = SkNEW(SkTriangleFilter);
            break;
        case SkBitmapScaler::RESIZE_MITCHELL:
            fBitmapFilter = SkNEW_ARGS(SkMitchellFilter, (1.f/3.f, 1.f/3.f));
            break;
        case SkBitmapScaler::RESIZE_HAMMING:
            fBitmapFilter = SkNEW(SkHammingFilter);
            break;
        case SkBitmapScaler::RESIZE_LANCZOS3:
            fBitmapFilter = SkNEW(SkLanczosFilter);
            break;
        default:
            
            fBitmapFilter = SkNEW_ARGS(SkMitchellFilter, (1.f/3.f, 1.f/3.f));
            break;
    }


    float scaleX = static_cast<float>(destWidth) /
                   static_cast<float>(srcFullWidth);
    float scaleY = static_cast<float>(destHeight) /
                   static_cast<float>(srcFullHeight);

    this->computeFilters(srcFullWidth, destSubset.fLeft, destSubset.width(),
                         scaleX, &fXFilter, convolveProcs);
    this->computeFilters(srcFullHeight, destSubset.fTop, destSubset.height(),
                         scaleY, &fYFilter, convolveProcs);
}












void SkResizeFilter::computeFilters(int srcSize,
                                  int destSubsetLo, int destSubsetSize,
                                  float scale,
                                  SkConvolutionFilter1D* output,
                                  const SkConvolutionProcs& convolveProcs) {
  int destSubsetHi = destSubsetLo + destSubsetSize;  

  
  
  
  
  
  float clampedScale = SkTMin(1.0f, scale);

  
  
  float srcSupport = fBitmapFilter->width() / clampedScale;

  
  float invScale = 1.0f / scale;

  SkTArray<float> filterValues(64);
  SkTArray<short> fixedFilterValues(64);

  
  
  
  for (int destSubsetI = destSubsetLo; destSubsetI < destSubsetHi;
       destSubsetI++) {
    
    
    filterValues.reset();
    fixedFilterValues.reset();

    
    
    
    
    
    
    float srcPixel = (static_cast<float>(destSubsetI) + 0.5f) * invScale;

    
    int srcBegin = SkTMax(0, SkScalarFloorToInt(srcPixel - srcSupport));
    int srcEnd = SkTMin(srcSize - 1, SkScalarCeilToInt(srcPixel + srcSupport));

    
    
    float filterSum = 0.0f;  
    for (int curFilterPixel = srcBegin; curFilterPixel <= srcEnd;
         curFilterPixel++) {
      
      
      
      
      
      
      float srcFilterDist =
          ((static_cast<float>(curFilterPixel) + 0.5f) - srcPixel);

      
      float destFilterDist = srcFilterDist * clampedScale;

      
      float filterValue = fBitmapFilter->evaluate(destFilterDist);
      filterValues.push_back(filterValue);

      filterSum += filterValue;
    }
    SkASSERT(!filterValues.empty());

    
    
    short fixedSum = 0;
    for (int i = 0; i < filterValues.count(); i++) {
      short curFixed = output->FloatToFixed(filterValues[i] / filterSum);
      fixedSum += curFixed;
      fixedFilterValues.push_back(curFixed);
    }

    
    
    
    
    
    short leftovers = output->FloatToFixed(1.0f) - fixedSum;
    fixedFilterValues[fixedFilterValues.count() / 2] += leftovers;

    
    output->AddFilter(srcBegin, &fixedFilterValues[0],
                      static_cast<int>(fixedFilterValues.count()));
  }

  if (convolveProcs.fApplySIMDPadding) {
      convolveProcs.fApplySIMDPadding( output );
  }
}

static SkBitmapScaler::ResizeMethod ResizeMethodToAlgorithmMethod(
                                    SkBitmapScaler::ResizeMethod method) {
    
    if (method >= SkBitmapScaler::RESIZE_FIRST_ALGORITHM_METHOD &&
    method <= SkBitmapScaler::RESIZE_LAST_ALGORITHM_METHOD) {
        return method;
    }
    
    
    
    switch (method) {
        
        
        
        
        case SkBitmapScaler::RESIZE_GOOD:
            return SkBitmapScaler::RESIZE_TRIANGLE;
        
        
        
        
        
        
        case SkBitmapScaler::RESIZE_BETTER:
            return SkBitmapScaler::RESIZE_HAMMING;
        default:
            return SkBitmapScaler::RESIZE_MITCHELL;
    }
}


bool SkBitmapScaler::Resize(SkBitmap* resultPtr,
                            const SkBitmap& source,
                            ResizeMethod method,
                            int destWidth, int destHeight,
                            const SkIRect& destSubset,
                            const SkConvolutionProcs& convolveProcs,
                            SkBitmap::Allocator* allocator) {
  
    SkASSERT(((RESIZE_FIRST_QUALITY_METHOD <= method) &&
        (method <= RESIZE_LAST_QUALITY_METHOD)) ||
        ((RESIZE_FIRST_ALGORITHM_METHOD <= method) &&
        (method <= RESIZE_LAST_ALGORITHM_METHOD)));

    SkIRect dest = { 0, 0, destWidth, destHeight };
    if (!dest.contains(destSubset)) {
        SkErrorInternals::SetError( kInvalidArgument_SkError,
                                    "Sorry, you passed me a bitmap resize "
                                    " method I have never heard of: %d",
                                    method );
    }

    
    
    if (source.width() < 1 || source.height() < 1 ||
        destWidth < 1 || destHeight < 1) {
        
        
        return false;
    }

    method = ResizeMethodToAlgorithmMethod(method);

    
    SkASSERT((SkBitmapScaler::RESIZE_FIRST_ALGORITHM_METHOD <= method) &&
        (method <= SkBitmapScaler::RESIZE_LAST_ALGORITHM_METHOD));

    SkAutoLockPixels locker(source);
    if (!source.readyToDraw() ||
        source.config() != SkBitmap::kARGB_8888_Config) {
        return false;
    }

    SkResizeFilter filter(method, source.width(), source.height(),
                          destWidth, destHeight, destSubset, convolveProcs);

    
    
    
    const unsigned char* sourceSubset =
        reinterpret_cast<const unsigned char*>(source.getPixels());

    
    SkBitmap result;
    result.setConfig(SkBitmap::kARGB_8888_Config,
                     destSubset.width(), destSubset.height(), 0,
                     source.alphaType());
    result.allocPixels(allocator, NULL);
    if (!result.readyToDraw()) {
        return false;
    }

    BGRAConvolve2D(sourceSubset, static_cast<int>(source.rowBytes()),
        !source.isOpaque(), filter.xFilter(), filter.yFilter(),
        static_cast<int>(result.rowBytes()),
        static_cast<unsigned char*>(result.getPixels()),
        convolveProcs, true);

    *resultPtr = result;
    resultPtr->lockPixels();
    SkASSERT(NULL != resultPtr->getPixels());
    return true;
}


bool SkBitmapScaler::Resize(SkBitmap* resultPtr,
                            const SkBitmap& source,
                            ResizeMethod method,
                            int destWidth, int destHeight,
                            const SkConvolutionProcs& convolveProcs,
                            SkBitmap::Allocator* allocator) {
    SkIRect destSubset = { 0, 0, destWidth, destHeight };
    return Resize(resultPtr, source, method, destWidth, destHeight, destSubset,
                  convolveProcs, allocator);
}
