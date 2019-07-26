









#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/acm2/audio_coding_module_impl.h"
#include "webrtc/modules/audio_coding/main/source/audio_coding_module_impl.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {


AudioCodingModule* AudioCodingModule::Create(int id) {
  return new acm1::AudioCodingModuleImpl(id, Clock::GetRealTimeClock());
}

AudioCodingModule* AudioCodingModule::Create(int id, Clock* clock) {
  return new acm1::AudioCodingModuleImpl(id, clock);
}


int AudioCodingModule::NumberOfCodecs() {
  return ACMCodecDB::kNumCodecs;
}


int AudioCodingModule::Codec(int list_id, CodecInst* codec) {
  
  return ACMCodecDB::Codec(list_id, codec);
}


int AudioCodingModule::Codec(const char* payload_name,
                             CodecInst* codec,
                             int sampling_freq_hz,
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


int AudioCodingModule::Codec(const char* payload_name,
                             int sampling_freq_hz,
                             int channels) {
  return ACMCodecDB::CodecId(payload_name, sampling_freq_hz, channels);
}


bool AudioCodingModule::IsCodecValid(const CodecInst& codec) {
  int mirror_id;

  int codec_number = ACMCodecDB::CodecNumber(codec, &mirror_id);

  if (codec_number < 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceAudioCoding, -1,
                 "Invalid codec setting");
    return false;
  } else {
    return true;
  }
}

AudioCodingModule* AudioCodingModuleFactory::Create(int id) const {
  return new acm1::AudioCodingModuleImpl(static_cast<int32_t>(id),
                                         Clock::GetRealTimeClock());
}

AudioCodingModule* NewAudioCodingModuleFactory::Create(int id) const {
  return new AudioCodingModuleImpl(id);
}

}  
