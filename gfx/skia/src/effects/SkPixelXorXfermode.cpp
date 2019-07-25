








#include "SkPixelXorXfermode.h"
#include "SkColorPriv.h"



SkPMColor SkPixelXorXfermode::xferColor(SkPMColor src, SkPMColor dst) {
    SkPMColor res = src ^ dst ^ fOpColor;
    res |= (SK_A32_MASK << SK_A32_SHIFT);   
    return res;
}

void SkPixelXorXfermode::flatten(SkFlattenableWriteBuffer& wb) const {
    this->INHERITED::flatten(wb);
    wb.write32(fOpColor);
}

SkPixelXorXfermode::SkPixelXorXfermode(SkFlattenableReadBuffer& rb)
        : INHERITED(rb) {
    fOpColor = rb.readU32();
}

SK_DEFINE_FLATTENABLE_REGISTRAR(SkPixelXorXfermode)
