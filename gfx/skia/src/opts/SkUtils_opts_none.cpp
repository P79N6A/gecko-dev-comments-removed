








#include "SkBlitRow.h"
#include "SkUtils.h"

SkMemset16Proc SkMemset16GetPlatformProc() {
    return NULL;
}

SkMemset32Proc SkMemset32GetPlatformProc() {
    return NULL;
}

SkBlitRow::ColorRectProc PlatformColorRectProcFactory() {
    return NULL;
}
