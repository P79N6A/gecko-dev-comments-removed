






#ifndef SkPixelXorXfermode_DEFINED
#define SkPixelXorXfermode_DEFINED

#include "SkXfermode.h"






class SK_API SkPixelXorXfermode : public SkXfermode {
public:
    static SkPixelXorXfermode* Create(SkColor opColor) {
        return SkNEW_ARGS(SkPixelXorXfermode, (opColor));
    }

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPixelXorXfermode)

protected:
    SkPixelXorXfermode(SkReadBuffer& rb);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    virtual SkPMColor xferColor(SkPMColor src, SkPMColor dst) const;

#ifdef SK_SUPPORT_LEGACY_PUBLICEFFECTCONSTRUCTORS
public:
#endif
    SkPixelXorXfermode(SkColor opColor) : fOpColor(opColor) {}

private:
    SkColor fOpColor;

    typedef SkXfermode INHERITED;
};

#endif
