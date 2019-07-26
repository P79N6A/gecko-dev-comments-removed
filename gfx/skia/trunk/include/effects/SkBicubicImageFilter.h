






#ifndef SkBicubicImageFilter_DEFINED
#define SkBicubicImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkSize.h"
#include "SkPoint.h"






class SK_API SkBicubicImageFilter : public SkImageFilter {
public:
    virtual ~SkBicubicImageFilter();

    static SkBicubicImageFilter* CreateMitchell(const SkSize& scale, SkImageFilter* input = NULL);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkBicubicImageFilter)

protected:
    





    SkBicubicImageFilter(const SkSize& scale, const SkScalar coefficients[16],
                         SkImageFilter* input = NULL);
    SkBicubicImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* loc) const SK_OVERRIDE;

#if SK_SUPPORT_GPU
    virtual bool canFilterImageGPU() const SK_OVERRIDE { return true; }
    virtual bool filterImageGPU(Proxy* proxy, const SkBitmap& src, const Context& ctx,
                                SkBitmap* result, SkIPoint* offset) const SK_OVERRIDE;
#endif

private:
    SkSize    fScale;
    SkScalar  fCoefficients[16];
    typedef SkImageFilter INHERITED;
};

#endif
