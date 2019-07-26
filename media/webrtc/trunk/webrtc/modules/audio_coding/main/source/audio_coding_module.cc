









#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_dtmf_detection.h"
#include "webrtc/modules/audio_coding/main/source/audio_coding_module_impl.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {


AudioCodingModule* AudioCodingModule::Create(const WebRtc_Word32 id) {
  return new AudioCodingModuleImpl(id);
}


void AudioCodingModule::Destroy(AudioCodingModule* module) {
  delete static_cast<AudioCodingModuleImpl*>(module);
}


WebRtc_UWord8 AudioCodingModule::NumberOfCodecs() {
  return static_cast<WebRtc_UWord8>(ACMCodecDB::kNumCodecs);
}


WebRtc_Word32 AudioCodingModule::Codec(const WebRtc_UWord8 list_id,
                                       CodecInst& codec) {
  
  return ACMCodecDB::Codec(list_id, &codec);
}


WebRtc_Word32 AudioCodingModule::Codec(const char* payload_name,
                                       CodecInst& codec, int sampling_freq_hz,
                                       int channels) {
  int codec_id;

  
  codec_id = ACMCodecDB::CodecId(payload_name, sampling_freq_hz, channels);
  if (codec_id < 0) {
    
    
    codec.plname[0] = '\0';
    codec.pltype = -1;
    codec.pacsize = 0;
    codec.rate = 0;
    codec.plfreq = 0;
    return -1;
  }

  
  ACMCodecDB::Codec(codec_id, &codec);

  
  
  codec.channels = channels;

  return 0;
}


WebRtc_Word32 AudioCodingModule::Codec(const char* payload_name,
                                       int sampling_freq_hz, int channels) {
  return ACMCodecDB::CodecId(payload_name, sampling_freq_hz, channels);
}


bool AudioCodingModule::IsCodecValid(const CodecInst& codec) {
  int mirror_id;
  char err_msg[500];

  int codec_number = ACMCodecDB::CodecNumber(&codec, &mirror_id, err_msg, 500);

  if (codec_number < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, -1, err_msg);
    return false;
  } else {
    return true;
  }
}

}  
