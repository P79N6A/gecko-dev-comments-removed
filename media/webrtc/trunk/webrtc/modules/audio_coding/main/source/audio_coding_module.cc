









#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/source/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/source/acm_dtmf_detection.h"
#include "webrtc/modules/audio_coding/main/source/audio_coding_module_impl.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {


AudioCodingModule* AudioCodingModule::Create(const int32_t id) {
  return new AudioCodingModuleImpl(id, Clock::GetRealTimeClock());
}



AudioCodingModule* AudioCodingModule::Create(const int32_t id,
                                             Clock* clock) {
  return new AudioCodingModuleImpl(id, clock);
}


void AudioCodingModule::Destroy(AudioCodingModule* module) {
  delete static_cast<AudioCodingModuleImpl*>(module);
}


uint8_t AudioCodingModule::NumberOfCodecs() {
  return static_cast<uint8_t>(ACMCodecDB::kNumCodecs);
}


int32_t AudioCodingModule::Codec(uint8_t list_id,
                                 CodecInst* codec) {
  
  return ACMCodecDB::Codec(list_id, codec);
}


int32_t AudioCodingModule::Codec(const char* payload_name,
                                 CodecInst* codec, int sampling_freq_hz,
                                 int channels) {
  int codec_id;

  
  codec_id = ACMCodecDB::CodecId(payload_name, sampling_freq_hz, channels);
  if (codec_id < 0) {
    
    
    codec->plname[0] = '\0';
    codec->pltype = -1;
    codec->pacsize = 0;
    codec->rate = 0;
    codec->plfreq = 0;
    return -1;
  }

  
  ACMCodecDB::Codec(codec_id, codec);

  
  
  codec->channels = channels;

  return 0;
}


int32_t AudioCodingModule::Codec(const char* payload_name,
                                 int sampling_freq_hz, int channels) {
  return ACMCodecDB::CodecId(payload_name, sampling_freq_hz, channels);
}


bool AudioCodingModule::IsCodecValid(const CodecInst& codec) {
  int mirror_id;

  int codec_number = ACMCodecDB::CodecNumber(&codec, &mirror_id);

  if (codec_number < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, -1,
                 "Invalid codec settings.");
    return false;
  } else {
    return true;
  }
}

}  
