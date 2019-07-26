









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_COMMON_DEFS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_ACM2_ACM_COMMON_DEFS_H_

#include <string.h>

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/typedefs.h"



#if ((defined WEBRTC_CODEC_ISAC) && (defined WEBRTC_CODEC_ISACFX))
#error iSAC and iSACFX codecs cannot be enabled at the same time
#endif


namespace webrtc {




#define AUDIO_BUFFER_SIZE_W16 7680






#define TIMESTAMP_BUFFER_SIZE_W32  (AUDIO_BUFFER_SIZE_W16/80)


#define MAX_PAYLOAD_SIZE_BYTE   7680


const int kIsacWbDefaultRate = 32000;
const int kIsacSwbDefaultRate = 56000;
const int kIsacPacSize480 = 480;
const int kIsacPacSize960 = 960;
const int kIsacPacSize1440 = 1440;










enum WebRtcACMEncodingType {
  kNoEncoding,
  kActiveNormalEncoded,
  kPassiveNormalEncoded,
  kPassiveDTXNB,
  kPassiveDTXWB,
  kPassiveDTXSWB,
  kPassiveDTXFB
};










struct WebRtcACMCodecParams {
  CodecInst codec_inst;
  bool enable_dtx;
  bool enable_vad;
  ACMVADMode vad_mode;
};


struct WebRtcACMAudioBuff {
  int16_t in_audio[AUDIO_BUFFER_SIZE_W16];
  int16_t in_audio_ix_read;
  int16_t in_audio_ix_write;
  uint32_t in_timestamp[TIMESTAMP_BUFFER_SIZE_W32];
  int16_t in_timestamp_ix_write;
  uint32_t last_timestamp;
  uint32_t last_in_timestamp;
};

}  

#endif  
