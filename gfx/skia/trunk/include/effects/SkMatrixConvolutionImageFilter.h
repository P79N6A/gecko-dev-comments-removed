






#ifndef SkMatrixConvolutionImageFilter_DEFINED
#define SkMatrixConvolutionImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkSize.h"
#include "SkPoint.h"







class SK_API SkMatrixConvolutionImageFilter : public SkImageFilter {
public:
    
    enum TileMode {
      kClamp_TileMode = 0,         
      kRepeat_TileMode,        
      kClampToBlack_TileMode,  
      kMax_TileMode = kClampToBlack_TileMode
    };

    virtual ~SkMatrixConvolutionImageFilter();

    




















    static SkMatrixConvolutionImageFilter* Create(const SkISize& kernelSize,
                                                  const SkScalar* kernel,
                                                  SkScalar gain,
                                                  SkScalar bias,
                                                  const SkIPoint& kernelOffset,
                                                  TileMode tileMode,
                                                  bool convolveAlpha,
                                                  SkImageFilter* input = NULL,
                                                  const CropRect* cropRect = NULL) {
        return SkNEW_ARGS(SkMatrixConvolutionImageFilter, (kernelSize, kernel, gain, bias,
                                                           kernelOffset, tileMode, convolveAlpha,
                                                           input, cropRect));
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkMatrixConvolutionImageFilter)

protected:
    SkMatrixConvolutionImageFilter(const SkISize& kernelSize,
                                   const SkScalar* kernel,
                                   SkScalar gain,
                                   SkScalar bias,
                                   const SkIPoint& kernelOffset,
                                   TileMode tileMode,
                                   bool convolveAlpha,
                                   SkImageFilter* input,
                                   const CropRect* cropRect);
    explicit SkMatrixConvolutionImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* loc) const SK_OVERRIDE;
    virtual bool onFilterBounds(const SkIRect&, const SkMatrix&, SkIRect*) const SK_OVERRIDE;


#if SK_SUPPORT_GPU
    virtual bool asNewEffect(GrEffect** effect,
                             GrTexture*,
                             const SkMatrix& ctm,
                             const SkIRect& bounds) const SK_OVERRIDE;
#endif

private:
    SkISize   fKernelSize;
    SkScalar* fKernel;
    SkScalar  fGain;
    SkScalar  fBias;
    SkIPoint  fKernelOffset;
    TileMode  fTileMode;
    bool      fConvolveAlpha;
    typedef SkImageFilter INHERITED;

    template <class PixelFetcher, bool convolveAlpha>
    void filterPixels(const SkBitmap& src,
                      SkBitmap* result,
                      const SkIRect& rect,
                      const SkIRect& bounds) const;
    template <class PixelFetcher>
    void filterPixels(const SkBitmap& src,
                      SkBitmap* result,
                      const SkIRect& rect,
                      const SkIRect& bounds) const;
    void filterInteriorPixels(const SkBitmap& src,
                              SkBitmap* result,
                              const SkIRect& rect,
                              const SkIRect& bounds) const;
    void filterBorderPixels(const SkBitmap& src,
                            SkBitmap* result,
                            const SkIRect& rect,
                            const SkIRect& bounds) const;
};

#endif
