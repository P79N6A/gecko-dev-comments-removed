




























#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_FFT_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_FFT_H_


#include "structs.h"





int WebRtcIsac_Fftns (unsigned int ndim, const int dims[], double Re[], double Im[],
                     int isign, double scaling, FFTstr *fftstate);



#endif 
