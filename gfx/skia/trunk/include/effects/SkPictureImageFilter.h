






#ifndef SkPictureImageFilter_DEFINED
#define SkPictureImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkPicture.h"

class SK_API SkPictureImageFilter : public SkImageFilter {
public:
    


    static SkPictureImageFilter* Create(const SkPicture* picture) {
        return SkNEW_ARGS(SkPictureImageFilter, (picture));
    }

    



    static SkPictureImageFilter* Create(const SkPicture* picture, const SkRect& cropRect) {
        return SkNEW_ARGS(SkPictureImageFilter, (picture, cropRect));
    }

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPictureImageFilter)

protected:
    explicit SkPictureImageFilter(const SkPicture* picture);
    SkPictureImageFilter(const SkPicture* picture, const SkRect& cropRect);
    virtual ~SkPictureImageFilter();
    





    explicit SkPictureImageFilter(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const Context&,
                               SkBitmap* result, SkIPoint* offset) const SK_OVERRIDE;
    virtual bool onFilterBounds(const SkIRect& src, const SkMatrix&,
                                SkIRect* dst) const SK_OVERRIDE;

private:
    const SkPicture* fPicture;
    SkRect           fCropRect;
    typedef SkImageFilter INHERITED;
};

#endif
