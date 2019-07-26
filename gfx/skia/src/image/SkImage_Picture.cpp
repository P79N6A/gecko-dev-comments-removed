






#include "SkImage_Base.h"
#include "SkImagePriv.h"
#include "SkPicture.h"

class SkImage_Picture : public SkImage_Base {
public:
    SkImage_Picture(SkPicture*);
    virtual ~SkImage_Picture();

    virtual void onDraw(SkCanvas*, SkScalar, SkScalar, const SkPaint*) SK_OVERRIDE;

private:
    SkPicture*  fPicture;

    typedef SkImage_Base INHERITED;
};



SkImage_Picture::SkImage_Picture(SkPicture* pict) : INHERITED(pict->width(), pict->height()) {
    pict->endRecording();
    pict->ref();
    fPicture = pict;
}

SkImage_Picture::~SkImage_Picture() {
    fPicture->unref();
}

void SkImage_Picture::onDraw(SkCanvas* canvas, SkScalar x, SkScalar y,
                             const SkPaint* paint) {
    SkImagePrivDrawPicture(canvas, fPicture, x, y, paint);
}

SkImage* SkNewImageFromPicture(const SkPicture* srcPicture) {
    







    SkAutoTUnref<SkPicture> playback(SkNEW_ARGS(SkPicture, (*srcPicture)));

    return SkNEW_ARGS(SkImage_Picture, (playback));
}

