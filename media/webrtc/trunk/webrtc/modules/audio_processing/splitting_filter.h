









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_SPLITTING_FILTER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_SPLITTING_FILTER_H_

#include "typedefs.h"
#include "signal_processing_library.h"

namespace webrtc {
















void SplittingFilterAnalysis(const int16_t* in_data,
                             int16_t* low_band,
                             int16_t* high_band,
                             int32_t* filt_state1,
                             int32_t* filt_state2);

















void SplittingFilterSynthesis(const int16_t* low_band,
                              const int16_t* high_band,
                              int16_t* out_data,
                              int32_t* filt_state1,
                              int32_t* filt_state2);
}  

#endif  
