

















#include "defines.h"
#include "window32_w32.h"
#include "bw_expand.h"
#include "poly_to_lsf.h"
#include "constants.h"





void WebRtcIlbcfix_SimpleLpcAnalysis(
    WebRtc_Word16 *lsf,   
    WebRtc_Word16 *data,   
    iLBC_Enc_Inst_t *iLBCenc_inst
    
                                     ) {
  int k;
  int scale;
  WebRtc_Word16 is;
  WebRtc_Word16 stability;
  
  WebRtc_Word16 A[LPC_FILTERORDER + 1];
  WebRtc_Word32 R[LPC_FILTERORDER + 1];
  WebRtc_Word16 windowedData[BLOCKL_MAX];
  WebRtc_Word16 rc[LPC_FILTERORDER];

  is=LPC_LOOKBACK+BLOCKL_MAX-iLBCenc_inst->blockl;
  WEBRTC_SPL_MEMCPY_W16(iLBCenc_inst->lpc_buffer+is,data,iLBCenc_inst->blockl);

  

  for (k = 0; k < iLBCenc_inst->lpc_n; k++) {

    is = LPC_LOOKBACK;

    if (k < (iLBCenc_inst->lpc_n - 1)) {

      
      WebRtcSpl_ElementwiseVectorMult(windowedData, iLBCenc_inst->lpc_buffer, WebRtcIlbcfix_kLpcWin, BLOCKL_MAX, 15);
    } else {

      
      WebRtcSpl_ElementwiseVectorMult(windowedData, iLBCenc_inst->lpc_buffer+is, WebRtcIlbcfix_kLpcAsymWin, BLOCKL_MAX, 15);
    }

    
    WebRtcSpl_AutoCorrelation(windowedData, BLOCKL_MAX, LPC_FILTERORDER, R, &scale);

    
    WebRtcIlbcfix_Window32W32(R, R, WebRtcIlbcfix_kLpcLagWin, LPC_FILTERORDER + 1 );

    
    stability=WebRtcSpl_LevinsonDurbin(R, A, rc, LPC_FILTERORDER);

    



    if (stability!=1) {
      A[0]=4096;
      WebRtcSpl_MemSetW16(&A[1], 0, LPC_FILTERORDER);
    }

    
    WebRtcIlbcfix_BwExpand(A, A, (WebRtc_Word16*)WebRtcIlbcfix_kLpcChirpSyntDenum, LPC_FILTERORDER+1);

    
    WebRtcIlbcfix_Poly2Lsf(lsf + k*LPC_FILTERORDER, A);
  }

  is=LPC_LOOKBACK+BLOCKL_MAX-iLBCenc_inst->blockl;
  WEBRTC_SPL_MEMCPY_W16(iLBCenc_inst->lpc_buffer,
                        iLBCenc_inst->lpc_buffer+LPC_LOOKBACK+BLOCKL_MAX-is, is);

  return;
}
