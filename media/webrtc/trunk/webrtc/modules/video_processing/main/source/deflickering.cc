









#include "webrtc/modules/video_processing/main/source/deflickering.h"

#include <math.h>
#include <stdlib.h>

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/system_wrappers/interface/sort.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {



enum { kFrequencyDeviation = 39 };

enum { kMinFrequencyToDetect = 32 };

enum { kNumFlickerBeforeDetect = 2 };
enum { kmean_valueScaling = 4 };  

enum { kZeroCrossingDeadzone = 10 };


enum { kDownsamplingFactor = 8 };
enum { kLog2OfDownsamplingFactor = 3 };







const uint16_t VPMDeflickering::prob_uw16_[kNumProbs] = {102, 205, 410, 614,
    819, 1024, 1229, 1434, 1638, 1843, 1946, 1987}; 






const uint16_t VPMDeflickering::weight_uw16_[kNumQuants - kMaxOnlyLength] =
    {16384, 18432, 20480, 22528, 24576, 26624, 28672, 30720, 32768}; 

VPMDeflickering::VPMDeflickering()
    : id_(0) {
  Reset();
}

VPMDeflickering::~VPMDeflickering() {}

int32_t VPMDeflickering::ChangeUniqueId(const int32_t id) {
  id_ = id;
  return 0;
}

void VPMDeflickering::Reset() {
  mean_buffer_length_ = 0;
  detection_state_ = 0;
  frame_rate_ = 0;

  memset(mean_buffer_, 0, sizeof(int32_t) * kMeanBufferLength);
  memset(timestamp_buffer_, 0, sizeof(int32_t) * kMeanBufferLength);

  
  quant_hist_uw8_[0][0] = 0;
  quant_hist_uw8_[0][kNumQuants - 1] = 255;
  for (int32_t i = 0; i < kNumProbs; i++) {
    quant_hist_uw8_[0][i + 1] = static_cast<uint8_t>((WEBRTC_SPL_UMUL_16_16(
        prob_uw16_[i], 255) + (1 << 10)) >> 11);  
  }

  for (int32_t i = 1; i < kFrameHistory_size; i++) {
    memcpy(quant_hist_uw8_[i], quant_hist_uw8_[0],
           sizeof(uint8_t) * kNumQuants);
  }
}

int32_t VPMDeflickering::ProcessFrame(I420VideoFrame* frame,
    VideoProcessingModule::FrameStats* stats) {
  assert(frame);
  uint32_t frame_memory;
  uint8_t quant_uw8[kNumQuants];
  uint8_t maxquant_uw8[kNumQuants];
  uint8_t minquant_uw8[kNumQuants];
  uint16_t target_quant_uw16[kNumQuants];
  uint16_t increment_uw16;
  uint8_t map_uw8[256];

  uint16_t tmp_uw16;
  uint32_t tmp_uw32;
  int width = frame->width();
  int height = frame->height();

  if (frame->IsZeroSize()) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, id_,
                 "Null frame pointer");
    return VPM_GENERAL_ERROR;
  }

  
  if (height < 2) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, id_,
                 "Invalid frame size");
    return VPM_GENERAL_ERROR;
  }

  if (!VideoProcessingModule::ValidFrameStats(*stats)) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, id_,
                 "Invalid frame stats");
    return VPM_GENERAL_ERROR;
  }

  if (PreDetection(frame->timestamp(), *stats) == -1) return VPM_GENERAL_ERROR;

  
  int32_t det_flicker = DetectFlicker();
  if (det_flicker < 0) {
    return VPM_GENERAL_ERROR;
  } else if (det_flicker != 1) {
    return 0;
  }

  
  const uint32_t y_size = height * width;

  const uint32_t y_sub_size = width * (((height - 1) >>
      kLog2OfDownsamplingFactor) + 1);
  uint8_t* y_sorted = new uint8_t[y_sub_size];
  uint32_t sort_row_idx = 0;
  for (int i = 0; i < height; i += kDownsamplingFactor) {
    memcpy(y_sorted + sort_row_idx * width,
        frame->buffer(kYPlane) + i * width, width);
    sort_row_idx++;
  }

  webrtc::Sort(y_sorted, y_sub_size, webrtc::TYPE_UWord8);

  uint32_t prob_idx_uw32 = 0;
  quant_uw8[0] = 0;
  quant_uw8[kNumQuants - 1] = 255;

  
  
  if (y_sub_size > (1 << 21) - 1) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, id_,
        "Subsampled number of pixels too large");
    return -1;
  }

  for (int32_t i = 0; i < kNumProbs; i++) {
    
    prob_idx_uw32 = WEBRTC_SPL_UMUL_32_16(y_sub_size, prob_uw16_[i]) >> 11;
    quant_uw8[i + 1] = y_sorted[prob_idx_uw32];
  }

  delete [] y_sorted;
  y_sorted = NULL;

  
  memmove(quant_hist_uw8_[1], quant_hist_uw8_[0],
      (kFrameHistory_size - 1) * kNumQuants * sizeof(uint8_t));
  
  memcpy(quant_hist_uw8_[0], quant_uw8, kNumQuants * sizeof(uint8_t));

  
  
  frame_memory = (frame_rate_ + (1 << 5)) >> 5;  
                                                 
  if (frame_memory > kFrameHistory_size) {
    frame_memory = kFrameHistory_size;
  }

  
  for (int32_t i = 0; i < kNumQuants; i++) {
    maxquant_uw8[i] = 0;
    minquant_uw8[i] = 255;
    for (uint32_t j = 0; j < frame_memory; j++) {
      if (quant_hist_uw8_[j][i] > maxquant_uw8[i]) {
        maxquant_uw8[i] = quant_hist_uw8_[j][i];
      }

      if (quant_hist_uw8_[j][i] < minquant_uw8[i]) {
        minquant_uw8[i] = quant_hist_uw8_[j][i];
      }
    }
  }

  
  for (int32_t i = 0; i < kNumQuants - kMaxOnlyLength; i++) {
    target_quant_uw16[i] = static_cast<uint16_t>((WEBRTC_SPL_UMUL_16_16(
        weight_uw16_[i], maxquant_uw8[i]) + WEBRTC_SPL_UMUL_16_16((1 << 15) -
        weight_uw16_[i], minquant_uw8[i])) >> 8);  
  }

  for (int32_t i = kNumQuants - kMaxOnlyLength; i < kNumQuants; i++) {
    target_quant_uw16[i] = ((uint16_t)maxquant_uw8[i]) << 7;
  }

  
  uint16_t mapUW16;  
  for (int32_t i = 1; i < kNumQuants; i++) {
    
    tmp_uw32 = static_cast<uint32_t>(target_quant_uw16[i] -
        target_quant_uw16[i - 1]);
    tmp_uw16 = static_cast<uint16_t>(quant_uw8[i] - quant_uw8[i - 1]);  

    if (tmp_uw16 > 0) {
      increment_uw16 = static_cast<uint16_t>(WebRtcSpl_DivU32U16(tmp_uw32,
          tmp_uw16)); 
    } else {
      
      increment_uw16 = 0;
    }

    mapUW16 = target_quant_uw16[i - 1];
    for (uint32_t j = quant_uw8[i - 1]; j < (uint32_t)(quant_uw8[i] + 1); j++) {
      
      map_uw8[j] = (uint8_t)((mapUW16 + (1 << 6)) >> 7);
      mapUW16 += increment_uw16;
    }
  }

  
  uint8_t* buffer = frame->buffer(kYPlane);
  for (uint32_t i = 0; i < y_size; i++) {
    buffer[i] = map_uw8[buffer[i]];
  }

  
  VideoProcessingModule::ClearFrameStats(stats);

  return VPM_OK;
}













int32_t VPMDeflickering::PreDetection(const uint32_t timestamp,
  const VideoProcessingModule::FrameStats& stats) {
  int32_t mean_val;  
  uint32_t frame_rate = 0;
  int32_t meanBufferLength;  

  mean_val = ((stats.sum << kmean_valueScaling) / stats.num_pixels);
  
  
  memmove(mean_buffer_ + 1, mean_buffer_,
      (kMeanBufferLength - 1) * sizeof(int32_t));
  mean_buffer_[0] = mean_val;

  
  
  memmove(timestamp_buffer_ + 1, timestamp_buffer_, (kMeanBufferLength - 1) *
      sizeof(uint32_t));
  timestamp_buffer_[0] = timestamp;


  if (timestamp_buffer_[kMeanBufferLength - 1] != 0) {
    frame_rate = ((90000 << 4) * (kMeanBufferLength - 1));
    frame_rate /=
        (timestamp_buffer_[0] - timestamp_buffer_[kMeanBufferLength - 1]);
  } else if (timestamp_buffer_[1] != 0) {
    frame_rate = (90000 << 4) / (timestamp_buffer_[0] - timestamp_buffer_[1]);
  }

  
  if (frame_rate == 0) {
    meanBufferLength = 1;
  } else {
    meanBufferLength =
        (kNumFlickerBeforeDetect * frame_rate) / kMinFrequencyToDetect;
  }
  
  if (meanBufferLength >= kMeanBufferLength) {
    


    mean_buffer_length_ = 0;
    return 2;
  }
  mean_buffer_length_ = meanBufferLength;

  if ((timestamp_buffer_[mean_buffer_length_ - 1] != 0) &&
      (mean_buffer_length_ != 1)) {
    frame_rate = ((90000 << 4) * (mean_buffer_length_ - 1));
    frame_rate /=
        (timestamp_buffer_[0] - timestamp_buffer_[mean_buffer_length_ - 1]);
  } else if (timestamp_buffer_[1] != 0) {
    frame_rate = (90000 << 4) / (timestamp_buffer_[0] - timestamp_buffer_[1]);
  }
  frame_rate_ = frame_rate;

  return VPM_OK;
}










int32_t VPMDeflickering::DetectFlicker() {
  uint32_t  i;
  int32_t  freqEst;       
  int32_t  ret_val = -1;

  
  if (mean_buffer_length_ < 2) {
    
    return(2);
  }
  
  
  int32_t deadzone = (kZeroCrossingDeadzone << kmean_valueScaling);  
  int32_t meanOfBuffer = 0;  
  int32_t numZeros     = 0;  
  int32_t cntState     = 0;  
  int32_t cntStateOld  = 0;  

  for (i = 0; i < mean_buffer_length_; i++) {
    meanOfBuffer += mean_buffer_[i];
  }
  meanOfBuffer += (mean_buffer_length_ >> 1);  
  meanOfBuffer /= mean_buffer_length_;

  
  cntStateOld = (mean_buffer_[0] >= (meanOfBuffer + deadzone));
  cntStateOld -= (mean_buffer_[0] <= (meanOfBuffer - deadzone));
  for (i = 1; i < mean_buffer_length_; i++) {
    cntState = (mean_buffer_[i] >= (meanOfBuffer + deadzone));
    cntState -= (mean_buffer_[i] <= (meanOfBuffer - deadzone));
    if (cntStateOld == 0) {
      cntStateOld = -cntState;
    }
    if (((cntState + cntStateOld) == 0) && (cntState != 0)) {
      numZeros++;
      cntStateOld = cntState;
    }
  }
  

  




  freqEst = ((numZeros * 90000) << 3);
  freqEst /=
      (timestamp_buffer_[0] - timestamp_buffer_[mean_buffer_length_ - 1]);

  
  uint8_t freqState = 0;  
                          
                          
                          
  int32_t freqAlias = freqEst;
  if (freqEst > kMinFrequencyToDetect) {
    uint8_t aliasState = 1;
    while(freqState == 0) {
      
      freqAlias += (aliasState * frame_rate_);
      freqAlias += ((freqEst << 1) * (1 - (aliasState << 1)));
      
      freqState = (abs(freqAlias - (100 << 4)) <= kFrequencyDeviation);
      freqState += (abs(freqAlias - (120 << 4)) <= kFrequencyDeviation);
      freqState += 2 * (freqAlias > ((120 << 4) + kFrequencyDeviation));
      
      aliasState++;
      aliasState &= 0x01;
    }
  }
  
  if (freqState == 1) {
    ret_val = 1;
  } else if (freqState == 0) {
    ret_val = 2;
  } else {
    ret_val = 0;
  }
  return ret_val;
}

}  
