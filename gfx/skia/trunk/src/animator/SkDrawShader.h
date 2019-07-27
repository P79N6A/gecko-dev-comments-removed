






#ifndef SkDrawShader_DEFINED
#define SkDrawShader_DEFINED

#include "SkPaintPart.h"
#include "SkShader.h"

class SkBaseBitmap;

class SkDrawBitmapShader : public SkDrawShader {
    DECLARE_DRAW_MEMBER_INFO(BitmapShader);
    SkDrawBitmapShader();
    virtual bool add();
    virtual SkShader* getShader();
protected:
    SkBool filterBitmap;
    SkBaseBitmap* image;
private:
    typedef SkDrawShader INHERITED;
};

#endif 
