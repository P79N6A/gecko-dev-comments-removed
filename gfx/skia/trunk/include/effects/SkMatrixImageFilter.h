






#ifndef SkMatrixImageFilter_DEFINED
#define SkMatrixImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkScalar.h"
#include "SkSize.h"
#include "SkPoint.h"
#include "SkPaint.h"






class SK_API SkMatrixImageFilter : public SkImageFilter {
public:
    






    static SkMatrixImageFilter* Create(const SkMatrix& transform,
                                       SkPaint::FilterLevel,
                                       SkImageFilter* input = NULL);
    virtual ~SkMatrixImageFilter();

    virtual void computeFastBounds(const SkRect&, SkRect*) const SK_OVERRIDE;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkMatrixImageFilter)

protected:
    SkMatrixImageFilter(const SkMatrix& transform,
                        SkPaint::FilterLevel,
                        SkImageFilter* input);
    SkMatrixImageFilter(SkReadBuffer& buffer);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* loc) const SK_OVERRIDE;
    virtual bool onFilterBounds(const SkIRect& src, const SkMatrix&,
                                SkIRect* dst) const SK_OVERRIDE;

private:
    SkMatrix              fTransform;
    SkPaint::FilterLevel  fFilterLevel;
    typedef SkImageFilter INHERITED;
};

#endif
