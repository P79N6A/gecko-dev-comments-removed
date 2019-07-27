








#include "./vpx_config.h"
#define RTCD_C
#include "./vpx_dsp_rtcd.h"
#include "vpx_ports/vpx_once.h"

void vpx_dsp_rtcd() {
  once(setup_rtcd_internal);
}
