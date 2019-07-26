






#ifndef SkPixelXorXfermode_DEFINED
#define SkPixelXorXfermode_DEFINED

#include "SkXfermode.h"






class SK_API SkPixelXorXfermode : public SkXfermode {
public:
    SkPixelXorXfermode(SkColor opColor) : fOpColor(opColor) {}

    SK_DEVELOPER_TO_STRING()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkPixelXorXfermode)

protected:
    SkPixelXorXfermode(SkReadBuffer& rb);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    virtual SkPMColor xferColor(SkPMColor src, SkPMColor dst) const;

private:
    SkColor fOpColor;

    typedef SkXfermode INHERITED;
};

#endif
