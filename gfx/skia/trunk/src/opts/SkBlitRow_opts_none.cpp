






#include "SkBlitRow.h"



SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
    return NULL;
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return NULL;
}

SkBlitRow::ColorProc SkBlitRow::PlatformColorProc() {
    return NULL;
}

SkBlitRow::ColorRectProc PlatformColorRectProcFactory() {
    return NULL;
}
