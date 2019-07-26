

















#include "defines.h"
#include "constants.h"
#include "vq3.h"
#include "vq4.h"





void WebRtcIlbcfix_SplitVq(
    WebRtc_Word16 *qX,  
    WebRtc_Word16 *index, 

    WebRtc_Word16 *X,  
    WebRtc_Word16 *CB,  
    WebRtc_Word16 *dim, 
    WebRtc_Word16 *cbsize 
                           ) {

  WebRtc_Word16 *qXPtr, *indexPtr, *CBPtr, *XPtr;

  

  qXPtr=qX;
  indexPtr=index;
  CBPtr=CB;
  XPtr=X;
  WebRtcIlbcfix_Vq3(qXPtr, indexPtr, CBPtr, XPtr, cbsize[0]);

  qXPtr+=3;
  indexPtr+=1;
  CBPtr+=(dim[0]*cbsize[0]);
  XPtr+=3;
  WebRtcIlbcfix_Vq3(qXPtr, indexPtr, CBPtr, XPtr, cbsize[1]);

  qXPtr+=3;
  indexPtr+=1;
  CBPtr+=(dim[1]*cbsize[1]);
  XPtr+=3;
  WebRtcIlbcfix_Vq4(qXPtr, indexPtr, CBPtr, XPtr, cbsize[2]);

  return;
}
