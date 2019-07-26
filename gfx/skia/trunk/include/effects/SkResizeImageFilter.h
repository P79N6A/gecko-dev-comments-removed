






#ifndef SkResizeImageFilter_DEFINED
#define SkResizeImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkRect.h"
#include "SkPoint.h"
#include "SkPaint.h"






class SK_API SkResizeImageFilter : public SkImageFilter {
public:
    virtual ~SkResizeImageFilter();

    






    static SkResizeImageFilter* Create(SkScalar sx, SkScalar sy, SkPaint::FilterLevel filterLevel,
                                       SkImageFilter* input = NULL) {
        return SkNEW_ARGS(SkResizeImageFilter, (sx, sy, filterLevel, input));
    }

    virtual void computeFastBounds(const SkRect&, SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkResizeImageFilter)

protected:
    SkResizeImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* loc) const SK_OVERRIDE;
    virtual bool onFilterBounds(const SkIRect& src, const SkMatrix&,
                                SkIRect* dst) const SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkResizeImageFilter(SkScalar sx, SkScalar sy, SkPaint::FilterLevel filterLevel,
                        SkImageFilter* input = NULL);

private:
    SkScalar              fSx, fSy;
    SkPaint::FilterLevel  fFilterLevel;
    typedef SkImageFilter INHERITED;
};

#endif
