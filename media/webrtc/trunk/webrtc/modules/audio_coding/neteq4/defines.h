









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DEFINES_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DEFINES_H_

namespace webrtc {

enum Operations {
  kNormal = 0,
  kMerge,
  kExpand,
  kAccelerate,
  kPreemptiveExpand,
  kRfc3389Cng,
  kRfc3389CngNoPacket,
  kCodecInternalCng,
  kDtmf,
  kAlternativePlc,
  kAlternativePlcIncreaseTimestamp,
  kAudioRepetition,
  kAudioRepetitionIncreaseTimestamp,
  kUndefined = -1
};

enum Modes {
  kModeNormal = 0,
  kModeExpand,
  kModeMerge,
  kModeAccelerateSuccess,
  kModeAccelerateLowEnergy,
  kModeAccelerateFail,
  kModePreemptiveExpandSuccess,
  kModePreemptiveExpandLowEnergy,
  kModePreemptiveExpandFail,
  kModeRfc3389Cng,
  kModeCodecInternalCng,
  kModeDtmf,
  kModeError,
  kModeUndefined = -1
};

#ifdef NETEQ4_VERBOSE_LOGGING
#define NETEQ_LOG_VERBOSE LOG(LS_VERBOSE)
#else
#define NETEQ_LOG_VERBOSE while(false)LOG(LS_VERBOSE)
#endif

}  
#endif  
