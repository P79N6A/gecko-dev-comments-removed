

















#include "defines.h"
#include "constants.h"





void WebRtcIlbcfix_EnhUpsample(
    WebRtc_Word32 *useq1, 
    WebRtc_Word16 *seq1 
                                ){
  int j;
  WebRtc_Word32 *pu1, *pu11;
  WebRtc_Word16 *ps, *w16tmp;
  const WebRtc_Word16 *pp;

  
  pu1=useq1;
  for (j=0;j<ENH_UPS0; j++) {
    pu11=pu1;
    
    pp=WebRtcIlbcfix_kEnhPolyPhaser[j]+1;
    ps=seq1+2;
    (*pu11) = WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    pu11+=ENH_UPS0;
    
    pp=WebRtcIlbcfix_kEnhPolyPhaser[j]+1;
    ps=seq1+3;
    (*pu11) = WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    pu11+=ENH_UPS0;
    
    pp=WebRtcIlbcfix_kEnhPolyPhaser[j]+1;
    ps=seq1+4;
    (*pu11) = WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--,*pp++);
    pu1++;
  }

  



  

  
















  pu1 = useq1 + 12;
  w16tmp = seq1+4;
  for (j=0;j<ENH_UPS0; j++) {
    pu11 = pu1;
    
    pp = WebRtcIlbcfix_kEnhPolyPhaser[j]+2;
    ps = w16tmp;
    (*pu11) = WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    pu11+=ENH_UPS0;
    
    pp = WebRtcIlbcfix_kEnhPolyPhaser[j]+3;
    ps = w16tmp;
    (*pu11) = WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    (*pu11) += WEBRTC_SPL_MUL_16_16(*ps--, *pp++);
    pu11+=ENH_UPS0;

    pu1++;
  }
}
