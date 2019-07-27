






#ifndef SkAvoidXfermode_DEFINED
#define SkAvoidXfermode_DEFINED

#include "SkXfermode.h"






class SK_API SkAvoidXfermode : public SkXfermode {
public:
    enum Mode {
        kAvoidColor_Mode,   
        kTargetColor_Mode   
    };

    














    static SkAvoidXfermode* Create(SkColor opColor, U8CPU tolerance, Mode mode) {
        return SkNEW_ARGS(SkAvoidXfermode, (opColor, tolerance, mode));
    }

    
    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkAvoidXfermode)

protected:
    SkAvoidXfermode(SkColor opColor, U8CPU tolerance, Mode mode);
    explicit SkAvoidXfermode(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

private:
    SkColor     fOpColor;
    uint32_t    fDistMul;   
    Mode        fMode;

    typedef SkXfermode INHERITED;
};

#endif
