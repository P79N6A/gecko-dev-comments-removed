








#ifndef SkAvoidXfermode_DEFINED
#define SkAvoidXfermode_DEFINED

#include "SkXfermode.h"






class SkAvoidXfermode : public SkXfermode {
public:
    enum Mode {
        kAvoidColor_Mode,   
        kTargetColor_Mode   
    };

    














    SkAvoidXfermode(SkColor opColor, U8CPU tolerance, Mode mode);

    
    virtual void xfer32(SkPMColor dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) SK_OVERRIDE;
    virtual void xfer16(uint16_t dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) SK_OVERRIDE;
    virtual void xfer4444(uint16_t dst[], const SkPMColor src[], int count,
                          const SkAlpha aa[]) SK_OVERRIDE;
    virtual void xferA8(SkAlpha dst[], const SkPMColor src[], int count,
                        const SkAlpha aa[]) SK_OVERRIDE;

    
    virtual Factory getFactory() SK_OVERRIDE;
    virtual void flatten(SkFlattenableWriteBuffer&) SK_OVERRIDE;

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) {
        return SkNEW_ARGS(SkAvoidXfermode, (buffer));
    }

    SK_DECLARE_FLATTENABLE_REGISTRAR()

protected:
    SkAvoidXfermode(SkFlattenableReadBuffer&);

private:
    SkColor     fOpColor;
    uint32_t    fDistMul;   
    Mode        fMode;

    static SkFlattenable* Create(SkFlattenableReadBuffer&);

    typedef SkXfermode INHERITED;
};

#endif
