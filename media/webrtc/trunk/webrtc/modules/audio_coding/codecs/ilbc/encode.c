

















#include "defines.h"
#include "lpc_encode.h"
#include "frame_classify.h"
#include "state_search.h"
#include "state_construct.h"
#include "constants.h"
#include "cb_search.h"
#include "cb_construct.h"
#include "index_conv_enc.h"
#include "pack_bits.h"
#include "hp_input.h"

#ifdef SPLIT_10MS
#include "unpack_bits.h"
#include "index_conv_dec.h"
#endif
#ifndef WEBRTC_BIG_ENDIAN
#include "swap_bytes.h"
#endif





void WebRtcIlbcfix_EncodeImpl(
    WebRtc_UWord16 *bytes,     
    const WebRtc_Word16 *block, 
    iLBC_Enc_Inst_t *iLBCenc_inst 

                          ){
  int n, meml_gotten, Nfor, Nback;
  WebRtc_Word16 diff, start_pos;
  int index;
  int subcount, subframe;
  WebRtc_Word16 start_count, end_count;
  WebRtc_Word16 *residual;
  WebRtc_Word32 en1, en2;
  WebRtc_Word16 scale, max;
  WebRtc_Word16 *syntdenum;
  WebRtc_Word16 *decresidual;
  WebRtc_Word16 *reverseResidual;
  WebRtc_Word16 *reverseDecresidual;
  
  WebRtc_Word16 weightdenum[(LPC_FILTERORDER + 1)*NSUB_MAX];
  WebRtc_Word16 dataVec[BLOCKL_MAX + LPC_FILTERORDER];
  WebRtc_Word16 memVec[CB_MEML+CB_FILTERLEN];
  WebRtc_Word16 bitsMemory[sizeof(iLBC_bits)/sizeof(WebRtc_Word16)];
  iLBC_bits *iLBCbits_inst = (iLBC_bits*)bitsMemory;


#ifdef SPLIT_10MS
  WebRtc_Word16 *weightdenumbuf = iLBCenc_inst->weightdenumbuf;
  WebRtc_Word16 last_bit;
#endif

  WebRtc_Word16 *data = &dataVec[LPC_FILTERORDER];
  WebRtc_Word16 *mem = &memVec[CB_HALFFILTERLEN];

  
  residual = &iLBCenc_inst->lpc_buffer[LPC_LOOKBACK+BLOCKL_MAX-iLBCenc_inst->blockl];
  syntdenum = mem;      
  decresidual = residual;     
  reverseResidual = data;     
  reverseDecresidual = reverseResidual; 

#ifdef SPLIT_10MS

  WebRtcSpl_MemSetW16 (  (WebRtc_Word16 *) iLBCbits_inst, 0,
                         (WebRtc_Word16) (sizeof(iLBC_bits) / sizeof(WebRtc_Word16))  );

  start_pos = iLBCenc_inst->start_pos;
  diff = iLBCenc_inst->diff;

  if (iLBCenc_inst->section != 0){
    WEBRTC_SPL_MEMCPY_W16 (weightdenum, weightdenumbuf,
                           SCRATCH_ENCODE_DATAVEC - SCRATCH_ENCODE_WEIGHTDENUM);
    
    last_bit = WebRtcIlbcfix_UnpackBits (iLBCenc_inst->bytes, iLBCbits_inst, iLBCenc_inst->mode);
    if (last_bit)
      return;
    
    WebRtcIlbcfix_IndexConvDec (iLBCbits_inst->cb_index);

    if (iLBCenc_inst->section == 1){
      
      WEBRTC_SPL_MEMCPY_W16 (iLBCenc_inst->past_samples, block, 80);
    }
    else{ 
      
      WEBRTC_SPL_MEMCPY_W16 (iLBCenc_inst->past_samples + 80, block, 80);
    }
  }
  else{ 
    
    WEBRTC_SPL_MEMCPY_W16 (data + (iLBCenc_inst->mode * 8) - 80, block, 80);
    WEBRTC_SPL_MEMCPY_W16 (data, iLBCenc_inst->past_samples,
                           (iLBCenc_inst->mode * 8) - 80);
    iLBCenc_inst->Nfor_flag = 0;
    iLBCenc_inst->Nback_flag = 0;
#else

    WEBRTC_SPL_MEMCPY_W16(data,block,iLBCenc_inst->blockl);
#endif


    WebRtcIlbcfix_HpInput(data, (WebRtc_Word16*)WebRtcIlbcfix_kHpInCoefs,
                          iLBCenc_inst->hpimemy, iLBCenc_inst->hpimemx,
                          iLBCenc_inst->blockl);

    
    WebRtcIlbcfix_LpcEncode(syntdenum, weightdenum, iLBCbits_inst->lsf, data,
                            iLBCenc_inst);

    
    WEBRTC_SPL_MEMCPY_W16(dataVec, iLBCenc_inst->anaMem, LPC_FILTERORDER);

    
    for (n=0; n<iLBCenc_inst->nsub; n++ ) {
      WebRtcSpl_FilterMAFastQ12(
          &data[n*SUBL], &residual[n*SUBL],
          &syntdenum[n*(LPC_FILTERORDER+1)],
          LPC_FILTERORDER+1, SUBL);
    }

    
    WEBRTC_SPL_MEMCPY_W16(iLBCenc_inst->anaMem, &data[iLBCenc_inst->blockl-LPC_FILTERORDER], LPC_FILTERORDER);

    

    iLBCbits_inst->startIdx = WebRtcIlbcfix_FrameClassify(iLBCenc_inst,residual);

    


    index = (iLBCbits_inst->startIdx-1)*SUBL;
    max=WebRtcSpl_MaxAbsValueW16(&residual[index], 2*SUBL);
    scale=WebRtcSpl_GetSizeInBits(WEBRTC_SPL_MUL_16_16(max,max));

    
    scale = scale - 25;
    if(scale < 0) {
      scale = 0;
    }

    diff = STATE_LEN - iLBCenc_inst->state_short_len;
    en1=WebRtcSpl_DotProductWithScale(&residual[index], &residual[index],
                                      iLBCenc_inst->state_short_len, scale);
    index += diff;
    en2=WebRtcSpl_DotProductWithScale(&residual[index], &residual[index],
                                      iLBCenc_inst->state_short_len, scale);
    if (en1 > en2) {
      iLBCbits_inst->state_first = 1;
      start_pos = (iLBCbits_inst->startIdx-1)*SUBL;
    } else {
      iLBCbits_inst->state_first = 0;
      start_pos = (iLBCbits_inst->startIdx-1)*SUBL + diff;
    }

    

    WebRtcIlbcfix_StateSearch(iLBCenc_inst, iLBCbits_inst, &residual[start_pos],
                              &syntdenum[(iLBCbits_inst->startIdx-1)*(LPC_FILTERORDER+1)],
                              &weightdenum[(iLBCbits_inst->startIdx-1)*(LPC_FILTERORDER+1)]);

    WebRtcIlbcfix_StateConstruct(iLBCbits_inst->idxForMax, iLBCbits_inst->idxVec,
                                 &syntdenum[(iLBCbits_inst->startIdx-1)*(LPC_FILTERORDER+1)],
                                 &decresidual[start_pos], iLBCenc_inst->state_short_len
                                 );

    

    if (iLBCbits_inst->state_first) { 

      

      WebRtcSpl_MemSetW16(mem, 0, (WebRtc_Word16)(CB_MEML-iLBCenc_inst->state_short_len));
      WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-iLBCenc_inst->state_short_len,
                            decresidual+start_pos, iLBCenc_inst->state_short_len);

      

      WebRtcIlbcfix_CbSearch(iLBCenc_inst, iLBCbits_inst->cb_index, iLBCbits_inst->gain_index,
                             &residual[start_pos+iLBCenc_inst->state_short_len],
                             mem+CB_MEML-ST_MEM_L_TBL, ST_MEM_L_TBL, diff,
                             &weightdenum[iLBCbits_inst->startIdx*(LPC_FILTERORDER+1)], 0);

      

      WebRtcIlbcfix_CbConstruct(&decresidual[start_pos+iLBCenc_inst->state_short_len],
                                iLBCbits_inst->cb_index, iLBCbits_inst->gain_index,
                                mem+CB_MEML-ST_MEM_L_TBL, ST_MEM_L_TBL,
                                diff
                                );

    }
    else { 

      

      WebRtcSpl_MemCpyReversedOrder(&reverseResidual[diff-1],
                                    &residual[(iLBCbits_inst->startIdx+1)*SUBL-STATE_LEN], diff);

      

      meml_gotten = iLBCenc_inst->state_short_len;
      WebRtcSpl_MemCpyReversedOrder(&mem[CB_MEML-1], &decresidual[start_pos], meml_gotten);
      WebRtcSpl_MemSetW16(mem, 0, (WebRtc_Word16)(CB_MEML-iLBCenc_inst->state_short_len));

      
      WebRtcIlbcfix_CbSearch(iLBCenc_inst, iLBCbits_inst->cb_index, iLBCbits_inst->gain_index,
                             reverseResidual, mem+CB_MEML-ST_MEM_L_TBL, ST_MEM_L_TBL, diff,
                             &weightdenum[(iLBCbits_inst->startIdx-1)*(LPC_FILTERORDER+1)],
                             0);

      

      WebRtcIlbcfix_CbConstruct(reverseDecresidual,
                                iLBCbits_inst->cb_index, iLBCbits_inst->gain_index,
                                mem+CB_MEML-ST_MEM_L_TBL, ST_MEM_L_TBL,
                                diff
                                );

      

      WebRtcSpl_MemCpyReversedOrder(&decresidual[start_pos-1], reverseDecresidual, diff);
    }

#ifdef SPLIT_10MS
    iLBCenc_inst->start_pos = start_pos;
    iLBCenc_inst->diff = diff;
    iLBCenc_inst->section++;
    
    WebRtcIlbcfix_IndexConvEnc (iLBCbits_inst->cb_index);
    
    WebRtcIlbcfix_PackBits (iLBCenc_inst->bytes, iLBCbits_inst, iLBCenc_inst->mode);
    WEBRTC_SPL_MEMCPY_W16 (weightdenumbuf, weightdenum,
                           SCRATCH_ENCODE_DATAVEC - SCRATCH_ENCODE_WEIGHTDENUM);
    return;
  }
#endif

  

  Nfor = iLBCenc_inst->nsub-iLBCbits_inst->startIdx-1;

  
#ifdef SPLIT_10MS
  if (iLBCenc_inst->mode == 20)
  {
    subcount = 1;
  }
  if (iLBCenc_inst->mode == 30)
  {
    if (iLBCenc_inst->section == 1)
    {
      subcount = 1;
    }
    if (iLBCenc_inst->section == 2)
    {
      subcount = 3;
    }
  }
#else
  subcount=1;
#endif

  if( Nfor > 0 ){

    

    WebRtcSpl_MemSetW16(mem, 0, CB_MEML-STATE_LEN);
    WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-STATE_LEN,
                          decresidual+(iLBCbits_inst->startIdx-1)*SUBL, STATE_LEN);

#ifdef SPLIT_10MS
    if (iLBCenc_inst->Nfor_flag > 0)
    {
      for (subframe = 0; subframe < WEBRTC_SPL_MIN (Nfor, 2); subframe++)
      {
        
        WEBRTC_SPL_MEMCPY_W16 (mem, mem + SUBL, (CB_MEML - SUBL));
        WEBRTC_SPL_MEMCPY_W16 (mem + CB_MEML - SUBL,
                               &decresidual[(iLBCbits_inst->startIdx + 1 +
                                             subframe) * SUBL], SUBL);
      }
    }

    iLBCenc_inst->Nfor_flag++;

    if (iLBCenc_inst->mode == 20)
    {
      start_count = 0;
      end_count = Nfor;
    }
    if (iLBCenc_inst->mode == 30)
    {
      if (iLBCenc_inst->section == 1)
      {
        start_count = 0;
        end_count = WEBRTC_SPL_MIN (Nfor, 2);
      }
      if (iLBCenc_inst->section == 2)
      {
        start_count = WEBRTC_SPL_MIN (Nfor, 2);
        end_count = Nfor;
      }
    }
#else
    start_count = 0;
    end_count = (WebRtc_Word16)Nfor;
#endif

    

    for (subframe = start_count; subframe < end_count; subframe++){

      

      WebRtcIlbcfix_CbSearch(iLBCenc_inst, iLBCbits_inst->cb_index+subcount*CB_NSTAGES,
                             iLBCbits_inst->gain_index+subcount*CB_NSTAGES,
                             &residual[(iLBCbits_inst->startIdx+1+subframe)*SUBL],
                             mem, MEM_LF_TBL, SUBL,
                             &weightdenum[(iLBCbits_inst->startIdx+1+subframe)*(LPC_FILTERORDER+1)],
                             (WebRtc_Word16)subcount);

      

      WebRtcIlbcfix_CbConstruct(&decresidual[(iLBCbits_inst->startIdx+1+subframe)*SUBL],
                                iLBCbits_inst->cb_index+subcount*CB_NSTAGES,
                                iLBCbits_inst->gain_index+subcount*CB_NSTAGES,
                                mem, MEM_LF_TBL,
                                SUBL
                                );

      

      WEBRTC_SPL_MEMMOVE_W16(mem, mem+SUBL, (CB_MEML-SUBL));
      WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-SUBL,
                            &decresidual[(iLBCbits_inst->startIdx+1+subframe)*SUBL], SUBL);

      subcount++;
    }
  }

#ifdef SPLIT_10MS
  if ((iLBCenc_inst->section == 1) &&
      (iLBCenc_inst->mode == 30) && (Nfor > 0) && (end_count == 2))
  {
    iLBCenc_inst->section++;
    
    WebRtcIlbcfix_IndexConvEnc (iLBCbits_inst->cb_index);
    
    WebRtcIlbcfix_PackBits (iLBCenc_inst->bytes, iLBCbits_inst, iLBCenc_inst->mode);
    WEBRTC_SPL_MEMCPY_W16 (weightdenumbuf, weightdenum,
                           SCRATCH_ENCODE_DATAVEC - SCRATCH_ENCODE_WEIGHTDENUM);
    return;
  }
#endif

  

  Nback = iLBCbits_inst->startIdx-1;

  if( Nback > 0 ){

    




    WebRtcSpl_MemCpyReversedOrder(&reverseResidual[Nback*SUBL-1], residual, Nback*SUBL);

    

    meml_gotten = SUBL*(iLBCenc_inst->nsub+1-iLBCbits_inst->startIdx);
    if( meml_gotten > CB_MEML ) {
      meml_gotten=CB_MEML;
    }

    WebRtcSpl_MemCpyReversedOrder(&mem[CB_MEML-1], &decresidual[Nback*SUBL], meml_gotten);
    WebRtcSpl_MemSetW16(mem, 0, (WebRtc_Word16)(CB_MEML-meml_gotten));

#ifdef SPLIT_10MS
    if (iLBCenc_inst->Nback_flag > 0)
    {
      for (subframe = 0; subframe < WEBRTC_SPL_MAX (2 - Nfor, 0); subframe++)
      {
        
        WEBRTC_SPL_MEMCPY_W16 (mem, mem + SUBL, (CB_MEML - SUBL));
        WEBRTC_SPL_MEMCPY_W16 (mem + CB_MEML - SUBL,
                               &reverseDecresidual[subframe * SUBL], SUBL);
      }
    }

    iLBCenc_inst->Nback_flag++;


    if (iLBCenc_inst->mode == 20)
    {
      start_count = 0;
      end_count = Nback;
    }
    if (iLBCenc_inst->mode == 30)
    {
      if (iLBCenc_inst->section == 1)
      {
        start_count = 0;
        end_count = WEBRTC_SPL_MAX (2 - Nfor, 0);
      }
      if (iLBCenc_inst->section == 2)
      {
        start_count = WEBRTC_SPL_MAX (2 - Nfor, 0);
        end_count = Nback;
      }
    }
#else
    start_count = 0;
    end_count = (WebRtc_Word16)Nback;
#endif

    

    for (subframe = start_count; subframe < end_count; subframe++){

      

      WebRtcIlbcfix_CbSearch(iLBCenc_inst, iLBCbits_inst->cb_index+subcount*CB_NSTAGES,
                             iLBCbits_inst->gain_index+subcount*CB_NSTAGES, &reverseResidual[subframe*SUBL],
                             mem, MEM_LF_TBL, SUBL,
                             &weightdenum[(iLBCbits_inst->startIdx-2-subframe)*(LPC_FILTERORDER+1)],
                             (WebRtc_Word16)subcount);

      

      WebRtcIlbcfix_CbConstruct(&reverseDecresidual[subframe*SUBL],
                                iLBCbits_inst->cb_index+subcount*CB_NSTAGES,
                                iLBCbits_inst->gain_index+subcount*CB_NSTAGES,
                                mem, MEM_LF_TBL, SUBL
                                );

      

      WEBRTC_SPL_MEMMOVE_W16(mem, mem+SUBL, (CB_MEML-SUBL));
      WEBRTC_SPL_MEMCPY_W16(mem+CB_MEML-SUBL,
                            &reverseDecresidual[subframe*SUBL], SUBL);

      subcount++;

    }

    

    WebRtcSpl_MemCpyReversedOrder(&decresidual[SUBL*Nback-1], reverseDecresidual, SUBL*Nback);
  }
  

  

  WebRtcIlbcfix_IndexConvEnc(iLBCbits_inst->cb_index);

  

#ifdef SPLIT_10MS
  if( (iLBCenc_inst->mode==30) && (iLBCenc_inst->section==1) ){
    WebRtcIlbcfix_PackBits(iLBCenc_inst->bytes, iLBCbits_inst, iLBCenc_inst->mode);
  }
  else{
    WebRtcIlbcfix_PackBits(bytes, iLBCbits_inst, iLBCenc_inst->mode);
  }
#else
  WebRtcIlbcfix_PackBits(bytes, iLBCbits_inst, iLBCenc_inst->mode);
#endif

#ifndef WEBRTC_BIG_ENDIAN
  

#ifdef SPLIT_10MS
  if (( (iLBCenc_inst->section == 1) && (iLBCenc_inst->mode == 20) ) ||
      ( (iLBCenc_inst->section == 2) && (iLBCenc_inst->mode == 30) )){
    WebRtcIlbcfix_SwapBytes(bytes, iLBCenc_inst->no_of_words, bytes);
  }
#else
  WebRtcIlbcfix_SwapBytes(bytes, iLBCenc_inst->no_of_words, bytes);
#endif
#endif

#ifdef SPLIT_10MS
  if (subcount == (iLBCenc_inst->nsub - 1))
  {
    iLBCenc_inst->section = 0;
  }
  else
  {
    iLBCenc_inst->section++;
    WEBRTC_SPL_MEMCPY_W16 (weightdenumbuf, weightdenum,
                           SCRATCH_ENCODE_DATAVEC - SCRATCH_ENCODE_WEIGHTDENUM);
  }
#endif

}
