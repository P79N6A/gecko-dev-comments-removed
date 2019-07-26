






#ifndef SkMatrixConvolutionImageFilter_DEFINED
#define SkMatrixConvolutionImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkSize.h"
#include "SkPoint.h"







class SK_API SkMatrixConvolutionImageFilter : public SkImageFilter {
public:
    
    enum TileMode {
      kClamp_TileMode,         
      kRepeat_TileMode,        
      kClampToBlack_TileMode,  
    };

    




















    SkMatrixConvolutionImageFilter(const SkISize& kernelSize, const SkScalar* kernel, SkScalar gain, SkScalar bias, const SkIPoint& target, TileMode tileMode, bool convolveAlpha, SkImageFilter* input = NULL);
    virtual ~SkMatrixConvolutionImageFilter();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkMatrixConvolutionImageFilter)

protected:
    SkMatrixConvolutionImageFilter(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const SkMatrix&,
                               SkBitmap* result, SkIPoint* loc) SK_OVERRIDE;

#if SK_SUPPORT_GPU
    virtual bool asNewEffect(GrEffectRef**, GrTexture*) const SK_OVERRIDE;
#endif

private:
    SkISize   fKernelSize;
    SkScalar* fKernel;
    SkScalar  fGain;
    SkScalar  fBias;
    SkIPoint  fTarget;
    TileMode  fTileMode;
    bool      fConvolveAlpha;
    typedef SkImageFilter INHERITED;

    template <class PixelFetcher, bool convolveAlpha>
    void filterPixels(const SkBitmap& src, SkBitmap* result, const SkIRect& rect);
    template <class PixelFetcher>
    void filterPixels(const SkBitmap& src, SkBitmap* result, const SkIRect& rect);
    void filterInteriorPixels(const SkBitmap& src, SkBitmap* result, const SkIRect& rect);
    void filterBorderPixels(const SkBitmap& src, SkBitmap* result, const SkIRect& rect);
};

#endif
