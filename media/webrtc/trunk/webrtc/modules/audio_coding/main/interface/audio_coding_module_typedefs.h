









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_TYPEDEFS_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_TYPEDEFS_H_

#include <map>

#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {























enum AudioPlayoutMode {
  voice = 0,
  fax = 1,
  streaming = 2,
  off = 3,
};


















enum ACMSpeechType {
  normal = 0,
  PLC = 1,
  CNG = 2,
  PLCCNG = 3,
  VADPassive = 4
};










enum ACMVADMode {
  VADNormal = 0,
  VADLowBitrate = 1,
  VADAggr = 2,
  VADVeryAggr = 3
};





enum ACMCountries {
  ACMDisableCountryDetection = -1, 
  ACMUSA = 0,
  ACMJapan,
  ACMCanada,
  ACMFrance,
  ACMGermany,
  ACMAustria,
  ACMBelgium,
  ACMUK,
  ACMCzech,
  ACMDenmark,
  ACMFinland,
  ACMGreece,
  ACMHungary,
  ACMIceland,
  ACMIreland,
  ACMItaly,
  ACMLuxembourg,
  ACMMexico,
  ACMNorway,
  ACMPoland,
  ACMPortugal,
  ACMSpain,
  ACMSweden,
  ACMTurkey,
  ACMChina,
  ACMHongkong,
  ACMTaiwan,
  ACMKorea,
  ACMSingapore,
  ACMNonStandard1

};











enum ACMAMRPackingFormat {
  AMRUndefined = -1,
  AMRBandwidthEfficient = 0,
  AMROctetAlligned = 1,
  AMRFileStorage = 2
};


























typedef struct {
  uint16_t currentBufferSize;
  uint16_t preferredBufferSize;
  bool jitterPeaksFound;
  uint16_t currentPacketLossRate;
  uint16_t currentDiscardRate;
  uint16_t currentExpandRate;
  uint16_t currentPreemptiveRate;
  uint16_t currentAccelerateRate;
  int32_t clockDriftPPM;
  int meanWaitingTimeMs;
  int medianWaitingTimeMs;
  int minWaitingTimeMs;
  int maxWaitingTimeMs;
  int addedSamples;
} ACMNetworkStatistics;









enum ACMBackgroundNoiseMode {
  On,
  Fade,
  Off
};

}  

#endif  
