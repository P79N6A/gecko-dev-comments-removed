






#ifndef SkLerpXfermode_DEFINED
#define SkLerpXfermode_DEFINED

#include "SkXfermode.h"

class SK_API SkLerpXfermode : public SkXfermode {
public:
    





    static SkXfermode* Create(SkScalar scale);

    
    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLerpXfermode)

protected:
    SkLerpXfermode(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkLerpXfermode(unsigned scale256);

    unsigned fScale256;  

    typedef SkXfermode INHERITED;
};

#endif
