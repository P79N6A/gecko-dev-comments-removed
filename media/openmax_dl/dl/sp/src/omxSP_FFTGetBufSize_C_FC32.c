









#include "dl/api/armOMX.h"
#include "dl/api/omxtypes.h"
#include "dl/sp/api/armSP.h"
#include "dl/sp/api/omxSP.h"




























OMXResult omxSP_FFTGetBufSize_C_FC32(OMX_INT order, OMX_INT *pSize) {
  if (!pSize || (order < 1) || (order > TWIDDLE_TABLE_ORDER))
    return OMX_Sts_BadArgErr;
  




  return omxSP_FFTGetBufSize_C_SC32(order, pSize);
}
