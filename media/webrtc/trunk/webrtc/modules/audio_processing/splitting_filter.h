









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_SPLITTING_FILTER_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_MAIN_SOURCE_SPLITTING_FILTER_H_

#include "typedefs.h"
#include "signal_processing_library.h"

namespace webrtc {
















void SplittingFilterAnalysis(const WebRtc_Word16* in_data,
                             WebRtc_Word16* low_band,
                             WebRtc_Word16* high_band,
                             WebRtc_Word32* filt_state1,
                             WebRtc_Word32* filt_state2);

















void SplittingFilterSynthesis(const WebRtc_Word16* low_band,
                              const WebRtc_Word16* high_band,
                              WebRtc_Word16* out_data,
                              WebRtc_Word32* filt_state1,
                              WebRtc_Word32* filt_state2);
}  

#endif  
