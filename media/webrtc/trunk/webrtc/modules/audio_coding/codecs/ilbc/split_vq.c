

















#include "defines.h"
#include "constants.h"
#include "vq3.h"
#include "vq4.h"





void WebRtcIlbcfix_SplitVq(
    int16_t *qX,  
    int16_t *index, 

    int16_t *X,  
    int16_t *CB,  
    int16_t *dim, 
    int16_t *cbsize 
                           ) {

  int16_t *qXPtr, *indexPtr, *CBPtr, *XPtr;

  

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
