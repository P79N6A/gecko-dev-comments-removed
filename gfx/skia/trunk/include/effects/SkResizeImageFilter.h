






#ifndef SkResizeImageFilter_DEFINED
#define SkResizeImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkRect.h"
#include "SkPoint.h"
#include "SkPaint.h"






class SK_API SkResizeImageFilter : public SkImageFilter {
public:
    







    SkResizeImageFilter(SkScalar sx, SkScalar sy, SkPaint::FilterLevel filterLevel,
                        SkImageFilter* input = NULL);
    virtual ~SkResizeImageFilter();

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkResizeImageFilter)

protected:
    SkResizeImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const SkMatrix&,
                               SkBitmap* result, SkIPoint* loc) const SK_OVERRIDE;

private:
    SkScalar              fSx, fSy;
    SkPaint::FilterLevel  fFilterLevel;
    typedef SkImageFilter INHERITED;
};

#endif
