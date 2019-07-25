








#include "SkFontHost.h"

static SkFontHost::LCDOrientation gLCDOrientation = SkFontHost::kHorizontal_LCDOrientation;
static SkFontHost::LCDOrder gLCDOrder = SkFontHost::kRGB_LCDOrder;


SkFontHost::LCDOrientation SkFontHost::GetSubpixelOrientation()
{
    return gLCDOrientation;
}


void SkFontHost::SetSubpixelOrientation(LCDOrientation orientation)
{
    gLCDOrientation = orientation;
}


SkFontHost::LCDOrder SkFontHost::GetSubpixelOrder()
{
    return gLCDOrder;
}


void SkFontHost::SetSubpixelOrder(LCDOrder order)
{
    gLCDOrder = order;
}
