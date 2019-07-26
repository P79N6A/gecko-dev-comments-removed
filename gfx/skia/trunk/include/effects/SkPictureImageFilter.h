






#ifndef SkPictureImageFilter_DEFINED
#define SkPictureImageFilter_DEFINED

#include "SkImageFilter.h"
#include "SkPicture.h"

class SK_API SkPictureImageFilter : public SkImageFilter {
public:
    


    explicit SkPictureImageFilter(SkPicture* picture);

    



    SkPictureImageFilter(SkPicture* picture, const SkRect& rect);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPictureImageFilter)

protected:
    virtual ~SkPictureImageFilter();
    





    explicit SkPictureImageFilter(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;
    virtual bool onFilterImage(Proxy*, const SkBitmap& src, const SkMatrix&,
                               SkBitmap* result, SkIPoint* offset) const SK_OVERRIDE;

private:
    SkPicture* fPicture;
    SkRect     fRect;
    typedef SkImageFilter INHERITED;
};

#endif
