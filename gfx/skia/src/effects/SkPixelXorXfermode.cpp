








#include "SkPixelXorXfermode.h"
#include "SkColorPriv.h"



SkPMColor SkPixelXorXfermode::xferColor(SkPMColor src, SkPMColor dst) {
    SkPMColor res = src ^ dst ^ fOpColor;
    res |= (SK_A32_MASK << SK_A32_SHIFT);   
    return res;
}

void SkPixelXorXfermode::flatten(SkFlattenableWriteBuffer& wb) {
    this->INHERITED::flatten(wb);
    wb.write32(fOpColor);
}

SkPixelXorXfermode::SkPixelXorXfermode(SkFlattenableReadBuffer& rb)
        : SkXfermode(rb) {
    fOpColor = rb.readU32();
}

SkFlattenable::Factory SkPixelXorXfermode::getFactory() {
    return Create;
}

SkFlattenable* SkPixelXorXfermode::Create(SkFlattenableReadBuffer& rb) {
    return SkNEW_ARGS(SkPixelXorXfermode, (rb));
}

static SkFlattenable::Registrar
    gSkPixelXorXfermodeReg("SkPixelXorXfermode",
                           SkPixelXorXfermode::CreateProc);
