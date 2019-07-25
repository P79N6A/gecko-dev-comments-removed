









#include "acm_dtmf_detection.h"
#include "audio_coding_module_typedefs.h"

namespace webrtc {

ACMDTMFDetection::ACMDTMFDetection()
    : _init(0) {}

ACMDTMFDetection::~ACMDTMFDetection() {}

WebRtc_Word16 ACMDTMFDetection::Enable(ACMCountries ) {
  return -1;
}

WebRtc_Word16 ACMDTMFDetection::Disable() {
  return -1;
}

WebRtc_Word16 ACMDTMFDetection::Detect(
    const WebRtc_Word16* ,
    const WebRtc_UWord16 ,
    const WebRtc_Word32 ,
    bool& ,
    WebRtc_Word16& ) {
  return -1;
}

WebRtc_Word16 ACMDTMFDetection::GetVersion(
    WebRtc_Word8* ,
    WebRtc_UWord32& ,
    WebRtc_UWord32& ) {
  return -1;
}

} 
