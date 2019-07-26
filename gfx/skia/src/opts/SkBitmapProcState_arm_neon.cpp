






#include "SkBitmapProcState.h"
#include "SkBitmapProcState_filter.h"
#include "SkColorPriv.h"
#include "SkFilterProc.h"
#include "SkPaint.h"
#include "SkShader.h"   
#include "SkUtilsArm.h"


extern const SkBitmapProcState::SampleProc32 gSkBitmapProcStateSample32_neon[];
extern const SkBitmapProcState::SampleProc16 gSkBitmapProcStateSample16_neon[];

#define   NAME_WRAP(x)  x ## _neon
#include "SkBitmapProcState_filter_neon.h"
#include "SkBitmapProcState_procs.h"

const SkBitmapProcState::SampleProc32 gSkBitmapProcStateSample32_neon[] = {
    S32_opaque_D32_nofilter_DXDY_neon,
    S32_alpha_D32_nofilter_DXDY_neon,
    S32_opaque_D32_nofilter_DX_neon,
    S32_alpha_D32_nofilter_DX_neon,
    S32_opaque_D32_filter_DXDY_neon,
    S32_alpha_D32_filter_DXDY_neon,
    S32_opaque_D32_filter_DX_neon,
    S32_alpha_D32_filter_DX_neon,

    S16_opaque_D32_nofilter_DXDY_neon,
    S16_alpha_D32_nofilter_DXDY_neon,
    S16_opaque_D32_nofilter_DX_neon,
    S16_alpha_D32_nofilter_DX_neon,
    S16_opaque_D32_filter_DXDY_neon,
    S16_alpha_D32_filter_DXDY_neon,
    S16_opaque_D32_filter_DX_neon,
    S16_alpha_D32_filter_DX_neon,

    SI8_opaque_D32_nofilter_DXDY_neon,
    SI8_alpha_D32_nofilter_DXDY_neon,
    SI8_opaque_D32_nofilter_DX_neon,
    SI8_alpha_D32_nofilter_DX_neon,
    SI8_opaque_D32_filter_DXDY_neon,
    SI8_alpha_D32_filter_DXDY_neon,
    SI8_opaque_D32_filter_DX_neon,
    SI8_alpha_D32_filter_DX_neon,

    S4444_opaque_D32_nofilter_DXDY_neon,
    S4444_alpha_D32_nofilter_DXDY_neon,
    S4444_opaque_D32_nofilter_DX_neon,
    S4444_alpha_D32_nofilter_DX_neon,
    S4444_opaque_D32_filter_DXDY_neon,
    S4444_alpha_D32_filter_DXDY_neon,
    S4444_opaque_D32_filter_DX_neon,
    S4444_alpha_D32_filter_DX_neon,

    
    SA8_alpha_D32_nofilter_DXDY_neon,
    SA8_alpha_D32_nofilter_DXDY_neon,
    SA8_alpha_D32_nofilter_DX_neon,
    SA8_alpha_D32_nofilter_DX_neon,
    SA8_alpha_D32_filter_DXDY_neon,
    SA8_alpha_D32_filter_DXDY_neon,
    SA8_alpha_D32_filter_DX_neon,
    SA8_alpha_D32_filter_DX_neon
};

const SkBitmapProcState::SampleProc16 gSkBitmapProcStateSample16_neon[] = {
    S32_D16_nofilter_DXDY_neon,
    S32_D16_nofilter_DX_neon,
    S32_D16_filter_DXDY_neon,
    S32_D16_filter_DX_neon,

    S16_D16_nofilter_DXDY_neon,
    S16_D16_nofilter_DX_neon,
    S16_D16_filter_DXDY_neon,
    S16_D16_filter_DX_neon,

    SI8_D16_nofilter_DXDY_neon,
    SI8_D16_nofilter_DX_neon,
    SI8_D16_filter_DXDY_neon,
    SI8_D16_filter_DX_neon,

    
    NULL, NULL, NULL, NULL,
    
    NULL, NULL, NULL, NULL
};
