

















#include "defines.h"
#include "constants.h"
#include "create_augmented_vec.h"





void WebRtcIlbcfix_GetCbVec(
    int16_t *cbvec,   
    int16_t *mem,   
    int16_t index,   
    int16_t lMem,   
    int16_t cbveclen   
                            ){
  int16_t k, base_size;
  int16_t lag;
  
  int16_t tempbuff2[SUBL+5];

  

  base_size=lMem-cbveclen+1;

  if (cbveclen==SUBL) {
    base_size+=WEBRTC_SPL_RSHIFT_W16(cbveclen,1);
  }

  

  if (index<lMem-cbveclen+1) {

    

    k=index+cbveclen;
    
    WEBRTC_SPL_MEMCPY_W16(cbvec, mem+lMem-k, cbveclen);

  } else if (index < base_size) {

    

    k=(int16_t)WEBRTC_SPL_MUL_16_16(2, (index-(lMem-cbveclen+1)))+cbveclen;

    lag=WEBRTC_SPL_RSHIFT_W16(k, 1);

    WebRtcIlbcfix_CreateAugmentedVec(lag, mem+lMem, cbvec);

  }

  

  else {

    int16_t memIndTest;

    

    if (index-base_size<lMem-cbveclen+1) {

      

      memIndTest = lMem-(index-base_size+cbveclen);

      WebRtcSpl_MemSetW16(mem-CB_HALFFILTERLEN, 0, CB_HALFFILTERLEN);
      WebRtcSpl_MemSetW16(mem+lMem, 0, CB_HALFFILTERLEN);

      

      WebRtcSpl_FilterMAFastQ12(
          &mem[memIndTest+4], cbvec, (int16_t*)WebRtcIlbcfix_kCbFiltersRev,
          CB_FILTERLEN, cbveclen);
    }

    

    else {
      
      memIndTest = lMem-cbveclen-CB_FILTERLEN;
      WebRtcSpl_MemSetW16(mem+lMem, 0, CB_HALFFILTERLEN);

      
      WebRtcSpl_FilterMAFastQ12(
          &mem[memIndTest+7], tempbuff2, (int16_t*)WebRtcIlbcfix_kCbFiltersRev,
          CB_FILTERLEN, (int16_t)(cbveclen+5));

      
      lag = (cbveclen<<1)-20+index-base_size-lMem-1;

      WebRtcIlbcfix_CreateAugmentedVec(lag, tempbuff2+SUBL+5, cbvec);
    }
  }
}
