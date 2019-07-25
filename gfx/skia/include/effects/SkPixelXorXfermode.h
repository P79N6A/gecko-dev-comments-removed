








#ifndef SkPixelXorXfermode_DEFINED
#define SkPixelXorXfermode_DEFINED

#include "SkXfermode.h"






class SkPixelXorXfermode : public SkXfermode {
public:
    SkPixelXorXfermode(SkColor opColor) : fOpColor(opColor) {}

    
    virtual Factory getFactory();
    virtual void flatten(SkFlattenableWriteBuffer&);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkPixelXorXfermode, (buffer));
    }

protected:
    
    virtual SkPMColor xferColor(SkPMColor src, SkPMColor dst);

private:
    SkColor fOpColor;

    SkPixelXorXfermode(SkFlattenableReadBuffer& rb);
    
    static SkFlattenable* Create(SkFlattenableReadBuffer&);

    typedef SkXfermode INHERITED;
};

#endif
