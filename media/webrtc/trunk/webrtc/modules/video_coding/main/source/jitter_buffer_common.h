









#ifndef WEBRTC_MODULES_VIDEO_CODING_JITTER_BUFFER_COMMON_H_
#define WEBRTC_MODULES_VIDEO_CODING_JITTER_BUFFER_COMMON_H_

#include "webrtc/typedefs.h"

namespace webrtc {

enum { kMaxNumberOfFrames     = 300 };
enum { kStartNumberOfFrames   = 6 };
enum { kMaxVideoDelayMs       = 10000 };

enum VCMJitterBufferEnum {
  kMaxConsecutiveOldFrames        = 60,
  kMaxConsecutiveOldPackets       = 300,
  kMaxPacketsInSession            = 800,
  kBufferIncStepSizeBytes         = 30000,   
  kMaxJBFrameSizeBytes            = 4000000  
};

enum VCMFrameBufferEnum {
  kStateError           = -4,
  kFlushIndicator       = -3,   
  kTimeStampError       = -2,
  kSizeError            = -1,
  kNoError              = 0,
  kIncomplete           = 1,    
  kFirstPacket          = 2,
  kCompleteSession      = 3,    
  kDecodableSession     = 4,    
  kDuplicatePacket      = 5     
};

enum VCMFrameBufferStateEnum {
  kStateFree,               
  kStateEmpty,              
  kStateIncomplete,         
  kStateComplete,           
  kStateDecoding,           
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
