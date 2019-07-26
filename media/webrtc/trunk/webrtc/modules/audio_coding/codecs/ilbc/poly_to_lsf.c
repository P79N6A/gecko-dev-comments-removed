

















#include "defines.h"
#include "constants.h"
#include "poly_to_lsp.h"
#include "lsp_to_lsf.h"

void WebRtcIlbcfix_Poly2Lsf(
    WebRtc_Word16 *lsf,   
    WebRtc_Word16 *a    
                            ) {
  WebRtc_Word16 lsp[10];
  WebRtcIlbcfix_Poly2Lsp(a, lsp, (WebRtc_Word16*)WebRtcIlbcfix_kLspMean);
  WebRtcIlbcfix_Lsp2Lsf(lsp, lsf, 10);
}
