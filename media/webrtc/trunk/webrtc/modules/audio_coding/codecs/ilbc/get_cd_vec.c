

















#include "defines.h"
#include "constants.h"
#include "create_augmented_vec.h"





void WebRtcIlbcfix_GetCbVec(
    WebRtc_Word16 *cbvec,   
    WebRtc_Word16 *mem,   
    WebRtc_Word16 index,   
    WebRtc_Word16 lMem,   
    WebRtc_Word16 cbveclen   
                            ){
  WebRtc_Word16 k, base_size;
  WebRtc_Word16 lag;
  
  WebRtc_Word16 tempbuff2[SUBL+5];

  

  base_size=lMem-cbveclen+1;

  if (cbveclen==SUBL) {
    base_size+=WEBRTC_SPL_RSHIFT_W16(cbveclen,1);
  }

  

  if (index<lMem-cbveclen+1) {

    

    k=index+cbveclen;
    
    WEBRTC_SPL_MEMCPY_W16(cbvec, mem+lMem-k, cbveclen);

  } else if (index < base_size) {

    

    k=(WebRtc_Word16)WEBRTC_SPL_MUL_16_16(2, (index-(lMem-cbveclen+1)))+cbveclen;

    lag=WEBRTC_SPL_RSHIFT_W16(k, 1);

    WebRtcIlbcfix_CreateAugmentedVec(lag, mem+lMem, cbvec);

  }

  

  else {

    WebRtc_Word16 memIndTest;

    

    if (index-base_size<lMem-cbveclen+1) {

      

      memIndTest = lMem-(index-base_size+cbveclen);

      WebRtcSpl_MemSetW16(mem-CB_HALFFILTERLEN, 0, CB_HALFFILTERLEN);
      WebRtcSpl_MemSetW16(mem+lMem, 0, CB_HALFFILTERLEN);

      

      WebRtcSpl_FilterMAFastQ12(
          &mem[memIndTest+4], cbvec, (WebRtc_Word16*)WebRtcIlbcfix_kCbFiltersRev,
          CB_FILTERLEN, cbveclen);
    }

    

    else {
      
      memIndTest = lMem-cbveclen-CB_FILTERLEN;
      WebRtcSpl_MemSetW16(mem+lMem, 0, CB_HALFFILTERLEN);

      
      WebRtcSpl_FilterMAFastQ12(
          &mem[memIndTest+7], tempbuff2, (WebRtc_Word16*)WebRtcIlbcfix_kCbFiltersRev,
          CB_FILTERLEN, (WebRtc_Word16)(cbveclen+5));

      
      lag = (cbveclen<<1)-20+index-base_size-lMem-1;

      WebRtcIlbcfix_CreateAugmentedVec(lag, tempbuff2+SUBL+5, cbvec);
    }
  }
}
