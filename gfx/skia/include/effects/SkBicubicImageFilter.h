






#ifndef SkBicubicImageFilter_DEFINED
#define SkBicubicImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkSize.h"
#include "SkPoint.h"






class SK_API SkBicubicImageFilter : public SkImageFilter {
public:
    






    SkBicubicImageFilter(const SkSize& scale, const SkScalar coefficients[16], SkImageFilter* input = NULL);
    static SkBicubicImageFilter* CreateMitchell(const SkSize& scale, SkImageFilter* input = NULL);
    virtual ~SkBicubicImageFilter();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkBicubicImageFilter)

protected:
    SkBicubicImageFilter(SkFlattenableReadBuffer& buffer);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const SkMatrix&,
                               SkBitmap* result, SkIPoint* loc) SK_OVERRIDE;

#if SK_SUPPORT_GPU
    virtual bool canFilterImageGPU() const SK_OVERRIDE { return true; }
    virtual bool filterImageGPU(Proxy* proxy, const SkBitmap& src, SkBitmap* result) SK_OVERRIDE;
#endif

private:
    SkSize    fScale;
    SkScalar  fCoefficients[16];
    typedef SkImageFilter INHERITED;
};

#endif
