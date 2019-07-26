






#ifndef SkXfermodeImageFilter_DEFINED
#define SkXfermodeImageFilter_DEFINED

#include "SkImageFilter.h"

class SkBitmap;
class SkXfermode;

class SK_API SkXfermodeImageFilter : public SkImageFilter {
    





public:
    virtual ~SkXfermodeImageFilter();

    static SkXfermodeImageFilter* Create(SkXfermode* mode, SkImageFilter* background,
                                         SkImageFilter* foreground = NULL,
                                         const CropRect* cropRect = NULL) {
        return SkNEW_ARGS(SkXfermodeImageFilter, (mode, background, foreground, cropRect));
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkXfermodeImageFilter)

    virtual bool onFilterImage(Proxy* proxy,
                               const SkBitmap& src,
                               const Context& ctx,
                               SkBitmap* dst,
                               SkIPoint* offset) const SK_OVERRIDE;
#if SK_SUPPORT_GPU
    virtual bool canFilterImageGPU() const SK_OVERRIDE { return !cropRectIsSet(); }
    virtual bool filterImageGPU(Proxy* proxy, const SkBitmap& src, const Context& ctx,
                                SkBitmap* result, SkIPoint* offset) const SK_OVERRIDE;
#endif

protected:
    explicit SkXfermodeImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkXfermodeImageFilter(SkXfermode* mode, SkImageFilter* background,
                          SkImageFilter* foreground = NULL, const CropRect* cropRect = NULL);

private:
    SkXfermode* fMode;
    typedef SkImageFilter INHERITED;
};

#endif
