









#ifndef WEBRTC_MODULES_VIDEO_CODING_JITTER_BUFFER_COMMON_H_
#define WEBRTC_MODULES_VIDEO_CODING_JITTER_BUFFER_COMMON_H_

#include "webrtc/typedefs.h"

namespace webrtc {


static const float kFastConvergeMultiplier = 0.4f;
static const float kNormalConvergeMultiplier = 0.2f;

enum { kMaxNumberOfFrames     = 300 };
enum { kStartNumberOfFrames   = 6 };
enum { kMaxVideoDelayMs       = 10000 };
enum { kPacketsPerFrameMultiplier = 5 };
enum { kFastConvergeThreshold = 5};

enum VCMJitterBufferEnum {
  kMaxConsecutiveOldFrames        = 60,
  kMaxConsecutiveOldPackets       = 300,
  kMaxPacketsInSession            = 800,
  kBufferIncStepSizeBytes         = 30000,   
  kMaxJBFrameSizeBytes            = 4000000  
};

enum VCMFrameBufferEnum {
  kOutOfBoundsPacket    = -7,
  kNotInitialized       = -6,
  kOldPacket            = -5,
  kGeneralError         = -4,
  kFlushIndicator       = -3,   
  kTimeStampError       = -2,
  kSizeError            = -1,
  kNoError              = 0,
  kIncomplete           = 1,    
  kCompleteSession      = 3,    
  kDecodableSession     = 4,    
  kDuplicatePacket      = 5     
};

enum VCMFrameBufferStateEnum {
  kStateEmpty,              
  kStateIncomplete,         
  kStateComplete,           
  kStateDecodable           
};

enum { kH264StartCodeLengthBytes = 4};


enum VCMNaluCompleteness {
  kNaluUnset = 0,       
  kNaluComplete = 1,    
  kNaluStart,           
  kNaluIncomplete,      
  kNaluEnd,             
};
}  

#endif  
