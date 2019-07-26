








#include "SkPixelXorXfermode.h"
#include "SkColorPriv.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkString.h"



SkPMColor SkPixelXorXfermode::xferColor(SkPMColor src, SkPMColor dst) const {
    SkPMColor res = src ^ dst ^ fOpColor;
    res |= (SK_A32_MASK << SK_A32_SHIFT);   
    return res;
}

void SkPixelXorXfermode::flatten(SkWriteBuffer& wb) const {
    this->INHERITED::flatten(wb);
    wb.writeColor(fOpColor);
}

SkPixelXorXfermode::SkPixelXorXfermode(SkReadBuffer& rb)
        : INHERITED(rb) {
    fOpColor = rb.readColor();
}

#ifndef SK_IGNORE_TO_STRING
void SkPixelXorXfermode::toString(SkString* str) const {
    str->append("SkPixelXorXfermode: ");
    str->appendHex(fOpColor);
}
#endif
