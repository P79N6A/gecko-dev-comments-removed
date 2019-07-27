








#include "./vpx_config.h"
#define RTCD_C
#include "./vpx_scale_rtcd.h"
#include "vpx_ports/vpx_once.h"

void vpx_scale_rtcd()
{
    once(setup_rtcd_internal);
}
