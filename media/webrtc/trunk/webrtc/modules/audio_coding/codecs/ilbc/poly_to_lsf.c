

















#include "defines.h"
#include "constants.h"
#include "poly_to_lsp.h"
#include "lsp_to_lsf.h"

void WebRtcIlbcfix_Poly2Lsf(
    int16_t *lsf,   
    int16_t *a    
                            ) {
  int16_t lsp[10];
  WebRtcIlbcfix_Poly2Lsp(a, lsp, (int16_t*)WebRtcIlbcfix_kLspMean);
  WebRtcIlbcfix_Lsp2Lsf(lsp, lsf, 10);
}
