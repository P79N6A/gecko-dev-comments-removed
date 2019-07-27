






#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkBlitMask.h"
#include "SkUtilsArm.h"
#include "SkBlitMask_opts_arm_neon.h"

SkBlitMask::ColorProc SkBlitMask::PlatformColorProcs(SkColorType dstCT,
                                                     SkMask::Format maskFormat,
                                                     SkColor color) {
#if SK_ARM_NEON_IS_NONE
    return NULL;
#else












#endif

    
    

    return NULL;
}

SkBlitMask::BlitLCD16RowProc SkBlitMask::PlatformBlitRowProcs16(bool isOpaque) {
    if (isOpaque) {
        return SK_ARM_NEON_WRAP(SkBlitLCD16OpaqueRow);
    } else {
        return SK_ARM_NEON_WRAP(SkBlitLCD16Row);
    }
}

SkBlitMask::RowProc SkBlitMask::PlatformRowProcs(SkColorType dstCT,
                                                 SkMask::Format maskFormat,
                                                 RowFlags flags) {
    return NULL;
}
