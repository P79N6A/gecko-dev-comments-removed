








#include "./vpx_config.h"
#define RTCD_C
#include "./vp9_rtcd.h"
#include "vpx_ports/vpx_once.h"

void vp9_rtcd() {
    
    
    once(setup_rtcd_internal);
}
