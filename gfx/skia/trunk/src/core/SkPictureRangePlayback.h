







#ifndef SkPictureRangePlayback_DEFINED
#define SkPictureRangePlayback_DEFINED

#include "SkPicturePlayback.h"







class SkPictureRangePlayback : public SkPicturePlayback {
public:
    
    
    
    SkPictureRangePlayback(const SkPicture* picture, size_t start, size_t stop)
    : INHERITED(picture)
    , fStart(start)
    , fStop(stop) {
    }

    virtual void draw(SkCanvas* canvas, SkDrawPictureCallback*) SK_OVERRIDE;

private:
    size_t fStart;
    size_t fStop;

    typedef SkPicturePlayback INHERITED;
};


#endif
