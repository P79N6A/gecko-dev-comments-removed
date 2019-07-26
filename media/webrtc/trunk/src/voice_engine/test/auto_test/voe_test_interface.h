













#ifndef WEBRTC_VOICE_ENGINE_VOE_TEST_INTERFACE_H
#define WEBRTC_VOICE_ENGINE_VOE_TEST_INTERFACE_H

#include "common_types.h"

namespace voetest {

using namespace webrtc;


enum TestType {
  Invalid = -1, Standard = 0, Extended = 1, Stress = 2, Unit = 3, CPU = 4
};


enum ExtendedSelection {
  XSEL_Invalid = -1,
  XSEL_None = 0,
  XSEL_All,
  XSEL_Base,
  XSEL_CallReport,
  XSEL_Codec,
  XSEL_DTMF,
  XSEL_Encryption,
  XSEL_ExternalMedia,
  XSEL_File,
  XSEL_Hardware,
  XSEL_NetEqStats,
  XSEL_Network,
  XSEL_RTP_RTCP,
  XSEL_VideoSync,
  XSEL_VolumeControl,
};


int runAutoTest(TestType testType, ExtendedSelection extendedSel);

} 
#endif 
