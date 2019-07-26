









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_COMMON_DEFS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_COMMON_DEFS_H_

#include <string.h>

#include "webrtc/common_types.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/typedefs.h"



#if ((defined WEBRTC_CODEC_ISAC) && (defined WEBRTC_CODEC_ISACFX))
#error iSAC and iSACFX codecs cannot be enabled at the same time
#endif

#ifdef WIN32

#define STR_CASE_CMP(x, y) ::_stricmp(x, y)
#else

#define STR_CASE_CMP(x, y) ::strcasecmp(x, y)
#endif

namespace webrtc {




#define AUDIO_BUFFER_SIZE_W16  2560






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
  WebRtc_Word16 in_audio[AUDIO_BUFFER_SIZE_W16];
  WebRtc_Word16 in_audio_ix_read;
  WebRtc_Word16 in_audio_ix_write;
  WebRtc_UWord32 in_timestamp[TIMESTAMP_BUFFER_SIZE_W32];
  WebRtc_Word16 in_timestamp_ix_write;
  WebRtc_UWord32 last_timestamp;
  WebRtc_UWord32 last_in_timestamp;
};

}  

#endif  
