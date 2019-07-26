








#include "vpx_config.h"
#define RTCD_C
#include "vp8_rtcd.h"
#include "vpx_ports/vpx_once.h"

extern void vpx_scale_rtcd(void);

void vp8_rtcd()
{
    vpx_scale_rtcd();
    once(setup_rtcd_internal);
}
