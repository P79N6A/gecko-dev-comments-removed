









#include "webrtc/modules/audio_coding/main/source/acm_dtmf_detection.h"

#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"

namespace webrtc {

namespace acm1 {

ACMDTMFDetection::ACMDTMFDetection() {}

ACMDTMFDetection::~ACMDTMFDetection() {}

int16_t ACMDTMFDetection::Enable(ACMCountries ) {
  return -1;
}

int16_t ACMDTMFDetection::Disable() {
  return -1;
}

int16_t ACMDTMFDetection::Detect(
    const int16_t* ,
    const uint16_t ,
    const int32_t ,
    bool& ,
    int16_t& ) {
  return -1;
}

}  

}  
