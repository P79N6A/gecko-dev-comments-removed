








#include "./vpx_config.h"
#define RTCD_C
#include "./vp8_rtcd.h"
#include "vpx_ports/vpx_once.h"


void vp8_rtcd()
{
    once(setup_rtcd_internal);
}
