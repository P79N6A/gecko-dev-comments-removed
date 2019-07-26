






#ifndef SkTileImageFilter_DEFINED
#define SkTileImageFilter_DEFINED

#include "SkImageFilter.h"

class SK_API SkTileImageFilter : public SkImageFilter {
    typedef SkImageFilter INHERITED;

public:
    




    SkTileImageFilter(const SkRect& srcRect, const SkRect& dstRect, SkImageFilter* input)
        : INHERITED(input), fSrcRect(srcRect), fDstRect(dstRect) {}

    virtual bool onFilterImage(Proxy* proxy, const SkBitmap& src, const SkMatrix& ctm,
                               SkBitmap* dst, SkIPoint* offset) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkTileImageFilter)

protected:
    explicit SkTileImageFilter(SkReadBuffer& buffer);

    virtual void flatten(SkWriteBuffer& buffer) const SK_OVERRIDE;

private:
    SkRect fSrcRect;
    SkRect fDstRect;
};

#endif
