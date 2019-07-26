









#include "webrtc/modules/audio_coding/main/acm2/call_statistics.h"

#include <cassert>

namespace webrtc {

namespace acm2 {

void CallStatistics::DecodedByNetEq(AudioFrame::SpeechType speech_type) {
  ++decoding_stat_.calls_to_neteq;
  switch (speech_type) {
    case AudioFrame::kNormalSpeech: {
      ++decoding_stat_.decoded_normal;
      break;
    }
    case AudioFrame::kPLC: {
      ++decoding_stat_.decoded_plc;
      break;
    }
    case AudioFrame::kCNG: {
      ++decoding_stat_.decoded_cng;
      break;
    }
    case AudioFrame::kPLCCNG: {
      ++decoding_stat_.decoded_plc_cng;
      break;
    }
    case AudioFrame::kUndefined: {
      
      assert(false);
    }
  }
}

void CallStatistics::DecodedBySilenceGenerator() {
  ++decoding_stat_.calls_to_silence_generator;
}

const AudioDecodingCallStats& CallStatistics::GetDecodingStatistics() const {
  return decoding_stat_;
}

}  

}  
