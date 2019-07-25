



















#include "settings.h"
#include "filterbank_tables.h"
#include "codec.h"





static void WebRtcIsac_AllPassFilter2Float(float *InOut, const float *APSectionFactors,
                                           int lengthInOut, int NumberOfSections,
                                           float *FilterState)
{
  int n, j;
  float temp;
  for (j=0; j<NumberOfSections; j++){
    for (n=0;n<lengthInOut;n++){
      temp = FilterState[j] + APSectionFactors[j] * InOut[n];
      FilterState[j] = -APSectionFactors[j] * temp + InOut[n];
      InOut[n] = temp;
    }
  }
}


static const float kHpStCoefInFloat[4] =
{-1.94895953203325f, 0.94984516000000f, -0.05101826139794f, 0.05015484000000f};




































void WebRtcIsac_SplitAndFilterFloat(float *pin, float *LP, float *HP,
                                    double *LP_la, double *HP_la,
                                    PreFiltBankstr *prefiltdata)
{
  int k,n;
  float CompositeAPFilterState[NUMBEROFCOMPOSITEAPSECTIONS];
  float ForTransform_CompositeAPFilterState[NUMBEROFCOMPOSITEAPSECTIONS];
  float ForTransform_CompositeAPFilterState2[NUMBEROFCOMPOSITEAPSECTIONS];
  float tempinoutvec[FRAMESAMPLES+MAX_AR_MODEL_ORDER];
  float tempin_ch1[FRAMESAMPLES+MAX_AR_MODEL_ORDER];
  float tempin_ch2[FRAMESAMPLES+MAX_AR_MODEL_ORDER];
  float in[FRAMESAMPLES];
  float ftmp;


  

  for (k=0;k<FRAMESAMPLES;k++) {
    in[k] = pin[k] + kHpStCoefInFloat[2] * prefiltdata->HPstates_float[0] +
        kHpStCoefInFloat[3] * prefiltdata->HPstates_float[1];
    ftmp = pin[k] - kHpStCoefInFloat[0] * prefiltdata->HPstates_float[0] -
        kHpStCoefInFloat[1] * prefiltdata->HPstates_float[1];
    prefiltdata->HPstates_float[1] = prefiltdata->HPstates_float[0];
    prefiltdata->HPstates_float[0] = ftmp;
  }

  





  




  

  
  for (k=0;k<NUMBEROFCOMPOSITEAPSECTIONS;k++){
    CompositeAPFilterState[k] = 0.0;
  }
  
  for (k=0;k<FRAMESAMPLES_HALF;k++) {
    tempinoutvec[k] = in[FRAMESAMPLES-1-2*k];
  }

  
  WebRtcIsac_AllPassFilter2Float(tempinoutvec, WebRtcIsac_kCompositeApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCOMPOSITEAPSECTIONS, CompositeAPFilterState);

  

  for (k=0;k<FRAMESAMPLES_HALF;k++) {
    tempin_ch1[FRAMESAMPLES_HALF+QLOOKAHEAD-1-k] = tempinoutvec[k];
  }

  

  for (k=0; k<NUMBEROFCOMPOSITEAPSECTIONS; k++) {
    ForTransform_CompositeAPFilterState[k] = CompositeAPFilterState[k];
  }

  


  WebRtcIsac_AllPassFilter2Float(prefiltdata->INLABUF1_float,
                                 WebRtcIsac_kCompositeApFactorsFloat, QLOOKAHEAD,
                                 NUMBEROFCOMPOSITEAPSECTIONS, CompositeAPFilterState);

  
  


  for (k=0;k<QLOOKAHEAD;k++) {
    tempin_ch1[QLOOKAHEAD-1-k]=prefiltdata->INLABUF1_float[k];
    prefiltdata->INLABUF1_float[k]=in[FRAMESAMPLES-1-2*k];
  }

  

  for (k=0;k<NUMBEROFCOMPOSITEAPSECTIONS;k++){
    CompositeAPFilterState[k] = 0.0;
  }

  for (k=0;k<FRAMESAMPLES_HALF;k++) {
    tempinoutvec[k] = in[FRAMESAMPLES-2-2*k];
  }

  WebRtcIsac_AllPassFilter2Float(tempinoutvec, WebRtcIsac_kCompositeApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCOMPOSITEAPSECTIONS, CompositeAPFilterState);

  for (k=0;k<FRAMESAMPLES_HALF;k++) {
    tempin_ch2[FRAMESAMPLES_HALF+QLOOKAHEAD-1-k] = tempinoutvec[k];
  }

  for (k=0; k<NUMBEROFCOMPOSITEAPSECTIONS; k++) {
    ForTransform_CompositeAPFilterState2[k] = CompositeAPFilterState[k];
  }


  WebRtcIsac_AllPassFilter2Float(prefiltdata->INLABUF2_float,
                                 WebRtcIsac_kCompositeApFactorsFloat, QLOOKAHEAD,NUMBEROFCOMPOSITEAPSECTIONS,
                                 CompositeAPFilterState);

  for (k=0;k<QLOOKAHEAD;k++) {
    tempin_ch2[QLOOKAHEAD-1-k]=prefiltdata->INLABUF2_float[k];
    prefiltdata->INLABUF2_float[k]=in[FRAMESAMPLES-2-2*k];
  }

  
  




  



  for (k=0;k<NUMBEROFCHANNELAPSECTIONS;k++){ 
    for (n=0; n<NUMBEROFCOMPOSITEAPSECTIONS;n++){
      prefiltdata->INSTAT1_float[k] += ForTransform_CompositeAPFilterState[n]*
          WebRtcIsac_kTransform1Float[k*NUMBEROFCHANNELAPSECTIONS+n];
      prefiltdata->INSTAT2_float[k] += ForTransform_CompositeAPFilterState2[n]*
          WebRtcIsac_kTransform2Float[k*NUMBEROFCHANNELAPSECTIONS+n];
    }
  }

  
  
  

  WebRtcIsac_AllPassFilter2Float(tempin_ch1,WebRtcIsac_kUpperApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTAT1_float);
  WebRtcIsac_AllPassFilter2Float(tempin_ch2,WebRtcIsac_kLowerApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTAT2_float);

  
  for (k=0; k<FRAMESAMPLES_HALF; k++) {
    LP[k] = 0.5f*(tempin_ch1[k] + tempin_ch2[k]);
    HP[k] = 0.5f*(tempin_ch1[k] - tempin_ch2[k]);
  }

  
  





  
  for (k=0; k<FRAMESAMPLES_HALF; k++) {
    tempin_ch1[k]=in[2*k+1];
    tempin_ch2[k]=in[2*k];
  }

  

  WebRtcIsac_AllPassFilter2Float(tempin_ch1,WebRtcIsac_kUpperApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTATLA1_float);
  WebRtcIsac_AllPassFilter2Float(tempin_ch2,WebRtcIsac_kLowerApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTATLA2_float);

  for (k=0; k<FRAMESAMPLES_HALF; k++) {
    LP_la[k] = (float)(0.5f*(tempin_ch1[k] + tempin_ch2[k])); 
    HP_la[k] = (double)(0.5f*(tempin_ch1[k] - tempin_ch2[k])); 
  }


}





static const float kHpStCoefOut1Float[4] =
{-1.99701049409000f, 0.99714204490000f, 0.01701049409000f, -0.01704204490000f};


static const float kHpStCoefOut2Float[4] =
{-1.98645294509837f, 0.98672435560000f, 0.00645294509837f, -0.00662435560000f};





















void WebRtcIsac_FilterAndCombineFloat(float *InLP,
                                      float *InHP,
                                      float *Out,
                                      PostFiltBankstr *postfiltdata)
{
  int k;
  float tempin_ch1[FRAMESAMPLES+MAX_AR_MODEL_ORDER];
  float tempin_ch2[FRAMESAMPLES+MAX_AR_MODEL_ORDER];
  float ftmp, ftmp2;

  
  for (k=0;k<FRAMESAMPLES_HALF;k++) {
    tempin_ch1[k]=InLP[k]+InHP[k]; 
    tempin_ch2[k]=InLP[k]-InHP[k]; 
  }


  


  WebRtcIsac_AllPassFilter2Float(tempin_ch1, WebRtcIsac_kLowerApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCHANNELAPSECTIONS,postfiltdata->STATE_0_UPPER_float);

  



  WebRtcIsac_AllPassFilter2Float(tempin_ch2, WebRtcIsac_kUpperApFactorsFloat,
                                 FRAMESAMPLES_HALF, NUMBEROFCHANNELAPSECTIONS,postfiltdata->STATE_0_LOWER_float);


  
  for (k=0;k<FRAMESAMPLES_HALF;k++) {
    Out[2*k]=tempin_ch2[k];
    Out[2*k+1]=tempin_ch1[k];
  }


  

  for (k=0;k<FRAMESAMPLES;k++) {
    ftmp2 = Out[k] + kHpStCoefOut1Float[2] * postfiltdata->HPstates1_float[0] +
        kHpStCoefOut1Float[3] * postfiltdata->HPstates1_float[1];
    ftmp = Out[k] - kHpStCoefOut1Float[0] * postfiltdata->HPstates1_float[0] -
        kHpStCoefOut1Float[1] * postfiltdata->HPstates1_float[1];
    postfiltdata->HPstates1_float[1] = postfiltdata->HPstates1_float[0];
    postfiltdata->HPstates1_float[0] = ftmp;
    Out[k] = ftmp2;
  }

  for (k=0;k<FRAMESAMPLES;k++) {
    ftmp2 = Out[k] + kHpStCoefOut2Float[2] * postfiltdata->HPstates2_float[0] +
        kHpStCoefOut2Float[3] * postfiltdata->HPstates2_float[1];
    ftmp = Out[k] - kHpStCoefOut2Float[0] * postfiltdata->HPstates2_float[0] -
        kHpStCoefOut2Float[1] * postfiltdata->HPstates2_float[1];
    postfiltdata->HPstates2_float[1] = postfiltdata->HPstates2_float[0];
    postfiltdata->HPstates2_float[0] = ftmp;
    Out[k] = ftmp2;
  }
}
