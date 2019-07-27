








#include "./vpx_config.h"
#define RTCD_C
#include "./vp9_rtcd.h"
#include "vpx_ports/vpx_once.h"

void vpx_scale_rtcd(void);

void vp9_rtcd() {
    vpx_scale_rtcd();
    
    
    once(setup_rtcd_internal);
}
