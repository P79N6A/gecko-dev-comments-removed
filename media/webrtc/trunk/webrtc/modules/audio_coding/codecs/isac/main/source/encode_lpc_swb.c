

















#include "encode_lpc_swb.h"
#include "typedefs.h"
#include "settings.h"

#include "lpc_shape_swb12_tables.h"
#include "lpc_shape_swb16_tables.h"
#include "lpc_gain_swb_tables.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

















WebRtc_Word16
WebRtcIsac_RemoveLarMean(
    double* lar,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 coeffCntr;
  WebRtc_Word16 vecCntr;
  WebRtc_Word16 numVec;
  const double* meanLAR;
  switch(bandwidth)
  {
    case isac12kHz:
      {
        numVec = UB_LPC_VEC_PER_FRAME;
        meanLAR = WebRtcIsac_kMeanLarUb12;
        break;
      }
    case isac16kHz:
      {
        numVec = UB16_LPC_VEC_PER_FRAME;
        meanLAR = WebRtcIsac_kMeanLarUb16;
        break;
      }
    default:
      return -1;
  }

  for(vecCntr = 0; vecCntr < numVec; vecCntr++)
  {
    for(coeffCntr = 0; coeffCntr < UB_LPC_ORDER; coeffCntr++)
    {
      
      *lar++ -= meanLAR[coeffCntr];
    }
  }
  return 0;
}

















WebRtc_Word16
WebRtcIsac_DecorrelateIntraVec(
    const double* data,
    double*       out,
    WebRtc_Word16 bandwidth)
{
  const double* ptrData;
  const double* ptrRow;
  WebRtc_Word16 rowCntr;
  WebRtc_Word16 colCntr;
  WebRtc_Word16 larVecCntr;
  WebRtc_Word16 numVec;
  const double* decorrMat;
  switch(bandwidth)
  {
    case isac12kHz:
      {
        decorrMat = &WebRtcIsac_kIntraVecDecorrMatUb12[0][0];
        numVec = UB_LPC_VEC_PER_FRAME;
        break;
      }
    case isac16kHz:
      {
        decorrMat = &WebRtcIsac_kIintraVecDecorrMatUb16[0][0];
        numVec = UB16_LPC_VEC_PER_FRAME;
        break;
      }
    default:
      return -1;
  }

  
  
  
  
  
  
  

  ptrData = data;
  for(larVecCntr = 0; larVecCntr < numVec; larVecCntr++)
  {
    for(rowCntr = 0; rowCntr < UB_LPC_ORDER; rowCntr++)
    {
      ptrRow = &decorrMat[rowCntr * UB_LPC_ORDER];
      *out = 0;
      for(colCntr = 0; colCntr < UB_LPC_ORDER; colCntr++)
      {
        *out += ptrData[colCntr] * ptrRow[colCntr];
      }
      out++;
    }
    ptrData += UB_LPC_ORDER;
  }
  return 0;
}


















WebRtc_Word16
WebRtcIsac_DecorrelateInterVec(
    const double* data,
    double* out,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 coeffCntr;
  WebRtc_Word16 rowCntr;
  WebRtc_Word16 colCntr;
  const double* decorrMat;
  WebRtc_Word16 interVecDim;

  switch(bandwidth)
  {
    case isac12kHz:
      {
        decorrMat = &WebRtcIsac_kInterVecDecorrMatUb12[0][0];
        interVecDim = UB_LPC_VEC_PER_FRAME;
        break;
      }
    case isac16kHz:
      {
        decorrMat = &WebRtcIsac_kInterVecDecorrMatUb16[0][0];
        interVecDim = UB16_LPC_VEC_PER_FRAME;
        break;
      }
    default:
      return -1;
  }

  
  
  
  
  
  
  
  

  for(coeffCntr = 0; coeffCntr < UB_LPC_ORDER; coeffCntr++)
  {
    for(colCntr = 0; colCntr < interVecDim; colCntr++)
    {
      out[coeffCntr + colCntr * UB_LPC_ORDER] = 0;
      for(rowCntr = 0; rowCntr < interVecDim; rowCntr++)
      {
        out[coeffCntr + colCntr * UB_LPC_ORDER] +=
            data[coeffCntr + rowCntr * UB_LPC_ORDER] *
            decorrMat[rowCntr * interVecDim + colCntr];
      }
    }
  }
  return 0;
}















double
WebRtcIsac_QuantizeUncorrLar(
    double* data,
    int* recIdx,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 cntr;
  WebRtc_Word32 idx;
  WebRtc_Word16 interVecDim;
  const double* leftRecPoint;
  double quantizationStepSize;
  const WebRtc_Word16* numQuantCell;
  switch(bandwidth)
  {
    case isac12kHz:
      {
        leftRecPoint         = WebRtcIsac_kLpcShapeLeftRecPointUb12;
        quantizationStepSize = WebRtcIsac_kLpcShapeQStepSizeUb12;
        numQuantCell         = WebRtcIsac_kLpcShapeNumRecPointUb12;
        interVecDim          = UB_LPC_VEC_PER_FRAME;
        break;
      }
    case isac16kHz:
      {
        leftRecPoint         = WebRtcIsac_kLpcShapeLeftRecPointUb16;
        quantizationStepSize = WebRtcIsac_kLpcShapeQStepSizeUb16;
        numQuantCell         = WebRtcIsac_kLpcShapeNumRecPointUb16;
        interVecDim          = UB16_LPC_VEC_PER_FRAME;
        break;
      }
    default:
      return -1;
  }

  
  
  
  for(cntr = 0; cntr < UB_LPC_ORDER * interVecDim; cntr++)
  {
    idx = (WebRtc_Word32)floor((*data - leftRecPoint[cntr]) /
                               quantizationStepSize + 0.5);
    if(idx < 0)
    {
      idx = 0;
    }
    else if(idx >= numQuantCell[cntr])
    {
      idx = numQuantCell[cntr] - 1;
    }

    *data++ = leftRecPoint[cntr] + idx * quantizationStepSize;
    *recIdx++ = idx;
  }
  return 0;
}















WebRtc_Word16
WebRtcIsac_DequantizeLpcParam(
    const int* idx,
    double*    out,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 cntr;
  WebRtc_Word16 interVecDim;
  const double* leftRecPoint;
  double quantizationStepSize;

  switch(bandwidth)
  {
    case isac12kHz:
      {
        leftRecPoint =         WebRtcIsac_kLpcShapeLeftRecPointUb12;
        quantizationStepSize = WebRtcIsac_kLpcShapeQStepSizeUb12;
        interVecDim =          UB_LPC_VEC_PER_FRAME;
        break;
      }
    case isac16kHz:
      {
        leftRecPoint =         WebRtcIsac_kLpcShapeLeftRecPointUb16;
        quantizationStepSize = WebRtcIsac_kLpcShapeQStepSizeUb16;
        interVecDim =          UB16_LPC_VEC_PER_FRAME;
        break;
      }
    default:
      return -1;
  }

  
  
  

  for(cntr = 0; cntr < UB_LPC_ORDER * interVecDim; cntr++)
  {
    *out++ = leftRecPoint[cntr] + *idx++ * quantizationStepSize;
  }
  return 0;
}















WebRtc_Word16
WebRtcIsac_CorrelateIntraVec(
    const double* data,
    double*       out,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 vecCntr;
  WebRtc_Word16 rowCntr;
  WebRtc_Word16 colCntr;
  WebRtc_Word16 numVec;
  const double* ptrData;
  const double* intraVecDecorrMat;

  switch(bandwidth)
  {
    case isac12kHz:
      {
        numVec            = UB_LPC_VEC_PER_FRAME;
        intraVecDecorrMat = &WebRtcIsac_kIntraVecDecorrMatUb12[0][0];
        break;
      }
    case isac16kHz:
      {
        numVec            = UB16_LPC_VEC_PER_FRAME;
        intraVecDecorrMat = &WebRtcIsac_kIintraVecDecorrMatUb16[0][0];
        break;
      }
    default:
      return -1;
  }


  ptrData = data;
  for(vecCntr = 0; vecCntr < numVec; vecCntr++)
  {
    for(colCntr = 0; colCntr < UB_LPC_ORDER; colCntr++)
    {
      *out = 0;
      for(rowCntr = 0; rowCntr < UB_LPC_ORDER; rowCntr++)
      {
        *out += ptrData[rowCntr] *
            intraVecDecorrMat[rowCntr * UB_LPC_ORDER + colCntr];
      }
      out++;
    }
    ptrData += UB_LPC_ORDER;
  }
  return 0;
}














WebRtc_Word16
WebRtcIsac_CorrelateInterVec(
    const double* data,
    double*       out,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 coeffCntr;
  WebRtc_Word16 rowCntr;
  WebRtc_Word16 colCntr;
  WebRtc_Word16 interVecDim;
  double myVec[UB16_LPC_VEC_PER_FRAME];
  const double* interVecDecorrMat;

  switch(bandwidth)
  {
    case isac12kHz:
      {
        interVecDim       = UB_LPC_VEC_PER_FRAME;
        interVecDecorrMat = &WebRtcIsac_kInterVecDecorrMatUb12[0][0];
        break;
      }
    case isac16kHz:
      {
        interVecDim       = UB16_LPC_VEC_PER_FRAME;
        interVecDecorrMat = &WebRtcIsac_kInterVecDecorrMatUb16[0][0];
        break;
      }
    default:
      return -1;
  }

  for(coeffCntr = 0; coeffCntr < UB_LPC_ORDER; coeffCntr++)
  {
    for(rowCntr = 0; rowCntr < interVecDim; rowCntr++)
    {
      myVec[rowCntr] = 0;
      for(colCntr = 0; colCntr < interVecDim; colCntr++)
      {
        myVec[rowCntr] += data[coeffCntr + colCntr * UB_LPC_ORDER] * 
            interVecDecorrMat[rowCntr * interVecDim + colCntr];
        
      }
    }

    for(rowCntr = 0; rowCntr < interVecDim; rowCntr++)
    {
      out[coeffCntr + rowCntr * UB_LPC_ORDER] = myVec[rowCntr];
    }
  }
  return 0;
}














WebRtc_Word16
WebRtcIsac_AddLarMean(
    double* data,
    WebRtc_Word16 bandwidth)
{
  WebRtc_Word16 coeffCntr;
  WebRtc_Word16 vecCntr;
  WebRtc_Word16 numVec;
  const double* meanLAR;

  switch(bandwidth)
  {
    case isac12kHz:
      {
        numVec = UB_LPC_VEC_PER_FRAME;
        meanLAR = WebRtcIsac_kMeanLarUb12;
        break;
      }
    case isac16kHz:
      {
        numVec = UB16_LPC_VEC_PER_FRAME;
        meanLAR = WebRtcIsac_kMeanLarUb16;
        break;
      }
    default:
      return -1;
  }

  for(vecCntr = 0; vecCntr < numVec; vecCntr++)
  {
    for(coeffCntr = 0; coeffCntr < UB_LPC_ORDER; coeffCntr++)
    {
      *data++ += meanLAR[coeffCntr];
    }
  }
  return 0;
}












WebRtc_Word16
WebRtcIsac_ToLogDomainRemoveMean(
    double* data)
{
  WebRtc_Word16 coeffCntr;
  for(coeffCntr = 0; coeffCntr < UB_LPC_GAIN_DIM; coeffCntr++)
  {
    data[coeffCntr] = log(data[coeffCntr]) - WebRtcIsac_kMeanLpcGain;
  }
  return 0;
}














WebRtc_Word16 WebRtcIsac_DecorrelateLPGain(
    const double* data,
    double* out)
{
  WebRtc_Word16 rowCntr;
  WebRtc_Word16 colCntr;

  for(colCntr = 0; colCntr < UB_LPC_GAIN_DIM; colCntr++)
  {
    *out = 0;
    for(rowCntr = 0; rowCntr < UB_LPC_GAIN_DIM; rowCntr++)
    {
      *out += data[rowCntr] * WebRtcIsac_kLpcGainDecorrMat[rowCntr][colCntr];
    }
    out++;
  }
  return 0;
}













double WebRtcIsac_QuantizeLpcGain(
    double* data,
    int*    idx)
{
  WebRtc_Word16 coeffCntr;
  for(coeffCntr = 0; coeffCntr < UB_LPC_GAIN_DIM; coeffCntr++)
  {
    *idx = (int)floor((*data - WebRtcIsac_kLeftRecPointLpcGain[coeffCntr]) /
                                WebRtcIsac_kQSizeLpcGain + 0.5);

    if(*idx < 0)
    {
      *idx = 0;
    }
    else if(*idx >= WebRtcIsac_kNumQCellLpcGain[coeffCntr])
    {
      *idx = WebRtcIsac_kNumQCellLpcGain[coeffCntr] - 1;
    }
    *data = WebRtcIsac_kLeftRecPointLpcGain[coeffCntr] + *idx *
        WebRtcIsac_kQSizeLpcGain;

    data++;
    idx++;
  }
  return 0;
}












WebRtc_Word16 WebRtcIsac_DequantizeLpcGain(
    const int* idx,
    double*    out)
{
  WebRtc_Word16 coeffCntr;
  for(coeffCntr = 0; coeffCntr < UB_LPC_GAIN_DIM; coeffCntr++)
  {
    *out = WebRtcIsac_kLeftRecPointLpcGain[coeffCntr] + *idx *
        WebRtcIsac_kQSizeLpcGain;
    out++;
    idx++;
  }
  return 0;
}












WebRtc_Word16 WebRtcIsac_CorrelateLpcGain(
    const double* data,
    double* out)
{
  WebRtc_Word16 rowCntr;
  WebRtc_Word16 colCntr;

  for(rowCntr = 0; rowCntr < UB_LPC_GAIN_DIM; rowCntr++)
  {
    *out = 0;
    for(colCntr = 0; colCntr < UB_LPC_GAIN_DIM; colCntr++)
    {
      *out += WebRtcIsac_kLpcGainDecorrMat[rowCntr][colCntr] * data[colCntr];
    }
    out++;
  }

  return 0;
}













WebRtc_Word16 WebRtcIsac_AddMeanToLinearDomain(
    double* lpcGains)
{
  WebRtc_Word16 coeffCntr;
  for(coeffCntr = 0; coeffCntr < UB_LPC_GAIN_DIM; coeffCntr++)
  {
    lpcGains[coeffCntr] = exp(lpcGains[coeffCntr] + WebRtcIsac_kMeanLpcGain);
  }
  return 0;
}
