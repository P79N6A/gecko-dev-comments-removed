






#ifndef SkTileImageFilter_DEFINED
#define SkTileImageFilter_DEFINED

#include "SkImageFilter.h"

class SK_API SkTileImageFilter : public SkImageFilter {
    typedef SkImageFilter INHERITED;

public:
    




    static SkTileImageFilter* Create(const SkRect& srcRect, const SkRect& dstRect,
                                     SkImageFilter* input) {
        return SkNEW_ARGS(SkTileImageFilter, (srcRect, dstRect, input));
    }

    virtual bool onFilterImage(Proxy* proxy, const SkBitmap& src, const Context& ctx,
                               SkBitmap* dst, SkIPoint* offset) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkTileImageFilter)

protected:
    explicit SkTileImageFilter(SkReadBuffer& buffer);

    virtual void flatten(SkWriteBuffer& buffer) const SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkTileImageFilter(const SkRect& srcRect, const SkRect& dstRect, SkImageFilter* input)
        : INHERITED(input), fSrcRect(srcRect), fDstRect(dstRect) {}

private:
    SkRect fSrcRect;
    SkRect fDstRect;
};

#endif
