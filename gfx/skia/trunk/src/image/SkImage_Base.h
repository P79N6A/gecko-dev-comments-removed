






#ifndef SkImage_Base_DEFINED
#define SkImage_Base_DEFINED

#include "SkImage.h"

class SkImage_Base : public SkImage {
public:
    SkImage_Base(int width, int height) : INHERITED(width, height) {}

    virtual void onDraw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*) = 0;
    virtual void onDrawRectToRect(SkCanvas*, const SkRect* src,
                                  const SkRect& dst, const SkPaint*) = 0;

    
    virtual bool onReadPixels(SkBitmap*, const SkIRect& subset) const;

    virtual const void* onPeekPixels(SkImageInfo*, size_t* ) const {
        return NULL;
    }

    virtual GrTexture* onGetTexture() { return NULL; }

    
    
    virtual bool getROPixels(SkBitmap*) const { return false; }

    virtual SkShader* onNewShader(SkShader::TileMode,
                                  SkShader::TileMode,
                                  const SkMatrix* localMatrix) const { return NULL; };
private:
    typedef SkImage INHERITED;
};

#endif
