

















#include "filterbank_tables.h"
#include "settings.h"






const WebRtc_Word16 WebRtcIsacfix_kHpStCoeffInQ30[8] = {
  -31932,  16189,  
  15562,  17243,  
  -26748, -17186,  
  26296, -27476   
};





const WebRtc_Word16 WebRtcIsacfix_kHPStCoeffOut1Q30[8] = {
  -32719, -1306,  
  16337, 11486,  
  8918, 26078,  
  -8935,  3956   
};





const WebRtc_Word16 WebRtcIsacfix_kHPStCoeffOut2Q30[8] = {
  -32546, -2953,  
  16166, 32233,  
  3383, 13217,  
  -3473, -4597   
};


const WebRtc_Word16 WebRtcIsacfix_kUpperApFactorsQ15[2] = {
  1137, 12537
};


const WebRtc_Word16 WebRtcIsacfix_kLowerApFactorsQ15[2] = {
  5059, 24379
};
