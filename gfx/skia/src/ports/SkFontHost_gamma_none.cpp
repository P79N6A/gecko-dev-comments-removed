














#include "SkFontHost.h"

void SkFontHost::GetGammaTables(const uint8_t* tables[2])
{
    tables[0] = NULL;
    tables[1] = NULL;
}

int SkFontHost::ComputeGammaFlag(const SkPaint& paint)
{
    return 0;
}

