









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_COMMON_DEFS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_COMMON_DEFS_H_

#include <string.h>

#include "audio_coding_module_typedefs.h"
#include "common_types.h"
#include "engine_configurations.h"
#include "typedefs.h"



#if ((defined WEBRTC_CODEC_ISAC) && (defined WEBRTC_CODEC_ISACFX))
#error iSAC and iSACFX codecs cannot be enabled at the same time
#endif

#ifdef WIN32

#define STR_CASE_CMP(x,y) ::_stricmp(x,y)
#else

#define STR_CASE_CMP(x,y) ::strcasecmp(x,y)
#endif

namespace webrtc {




#define AUDIO_BUFFER_SIZE_W16  2560






#define TIMESTAMP_BUFFER_SIZE_W32  (AUDIO_BUFFER_SIZE_W16/80)


#define MAX_PAYLOAD_SIZE_BYTE   7680


const int kIsacWbDefaultRate = 32000;
const int kIsacSwbDefaultRate = 56000;
const int kIsacPacSize480 = 480;
const int kIsacPacSize960 = 960;










enum WebRtcACMEncodingType {
  kNoEncoding,
  kActiveNormalEncoded,
  kPassiveNormalEncoded,
  kPassiveDTXNB,
  kPassiveDTXWB,
  kPassiveDTXSWB
};










struct WebRtcACMCodecParams {
  CodecInst codecInstant;
  bool enableDTX;
  bool enableVAD;
  ACMVADMode vadMode;
};












struct WebRtcACMAudioBuff {
  WebRtc_Word16 inAudio[AUDIO_BUFFER_SIZE_W16];
  WebRtc_Word16 inAudioIxRead;
  WebRtc_Word16 inAudioIxWrite;
  WebRtc_UWord32 inTimestamp[TIMESTAMP_BUFFER_SIZE_W32];
  WebRtc_Word16 inTimestampIxWrite;
  WebRtc_UWord32 lastTimestamp;
  WebRtc_UWord32 lastInTimestamp;
};

} 

#endif  
