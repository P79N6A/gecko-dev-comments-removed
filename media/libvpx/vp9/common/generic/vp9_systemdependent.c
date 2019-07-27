










#include "./vpx_config.h"
#include "./vp9_rtcd.h"
#include "vp9/common/vp9_onyxc_int.h"

void vp9_machine_specific_config(VP9_COMMON *cm) {
  (void)cm;
  vp9_rtcd();
}
