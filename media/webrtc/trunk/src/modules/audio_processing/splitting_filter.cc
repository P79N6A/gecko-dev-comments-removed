









#include "splitting_filter.h"
#include "signal_processing_library.h"

namespace webrtc {

void SplittingFilterAnalysis(const WebRtc_Word16* in_data,
                             WebRtc_Word16* low_band,
                             WebRtc_Word16* high_band,
                             WebRtc_Word32* filter_state1,
                             WebRtc_Word32* filter_state2)
{
    WebRtcSpl_AnalysisQMF(in_data, low_band, high_band, filter_state1, filter_state2);
}

void SplittingFilterSynthesis(const WebRtc_Word16* low_band,
                              const WebRtc_Word16* high_band,
                              WebRtc_Word16* out_data,
                              WebRtc_Word32* filt_state1,
                              WebRtc_Word32* filt_state2)
{
    WebRtcSpl_SynthesisQMF(low_band, high_band, out_data, filt_state1, filt_state2);
}
}  
