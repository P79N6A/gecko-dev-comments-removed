









#ifndef WEBRTC_MODULES_VIDEO_CODING_QM_SELECT_H_
#define WEBRTC_MODULES_VIDEO_CODING_QM_SELECT_H_

#include "webrtc/common_types.h"
#include "webrtc/typedefs.h"





namespace webrtc {
struct VideoContentMetrics;

struct VCMResolutionScale {
  VCMResolutionScale()
      : codec_width(640),
        codec_height(480),
        frame_rate(30.0f),
        spatial_width_fact(1.0f),
        spatial_height_fact(1.0f),
        temporal_fact(1.0f),
        change_resolution_spatial(false),
        change_resolution_temporal(false) {
  }
  uint16_t codec_width;
  uint16_t codec_height;
  float frame_rate;
  float spatial_width_fact;
  float spatial_height_fact;
  float temporal_fact;
  bool change_resolution_spatial;
  bool change_resolution_temporal;
};






enum ImageType {
  kQCIF = 0,            
  kHCIF,                
  kQVGA,                
  kCIF,                 
  kHVGA,                
  kVGA,                 
  kQFULLHD,             
  kWHD,                 
  kFULLHD,              
  kNumImageTypes
};

const uint32_t kSizeOfImageType[kNumImageTypes] =
{ 25344, 57024, 76800, 101376, 172800, 307200, 518400, 921600, 2073600 };

enum FrameRateLevelClass {
  kFrameRateLow,
  kFrameRateMiddle1,
  kFrameRateMiddle2,
  kFrameRateHigh
};

enum ContentLevelClass {
  kLow,
  kHigh,
  kDefault
};

struct VCMContFeature {
  VCMContFeature()
      : value(0.0f),
        level(kDefault) {
  }
  void Reset() {
    value = 0.0f;
    level = kDefault;
  }
  float value;
  ContentLevelClass level;
};

enum UpDownAction {
  kUpResolution,
  kDownResolution
};

enum SpatialAction {
  kNoChangeSpatial,
  kOneHalfSpatialUniform,        
  kOneQuarterSpatialUniform,     
  kNumModesSpatial
};

enum TemporalAction {
  kNoChangeTemporal,
  kTwoThirdsTemporal,     
  kOneHalfTemporal,       
  kNumModesTemporal
};

struct ResolutionAction {
  ResolutionAction()
      : spatial(kNoChangeSpatial),
        temporal(kNoChangeTemporal) {
  }
  SpatialAction spatial;
  TemporalAction temporal;
};


const float kFactorWidthSpatial[kNumModesSpatial] =
    { 1.0f, 4.0f / 3.0f, 2.0f };

const float kFactorHeightSpatial[kNumModesSpatial] =
    { 1.0f, 4.0f / 3.0f, 2.0f };

const float kFactorTemporal[kNumModesTemporal] =
    { 1.0f, 1.5f, 2.0f };

enum EncoderState {
  kStableEncoding,    
  kStressedEncoding,  
                      
  kEasyEncoding       
};



class VCMQmMethod {
 public:
  VCMQmMethod();
  virtual ~VCMQmMethod();

  
  void ResetQM();
  virtual void Reset() = 0;

  
  uint8_t ComputeContentClass();

  
  void UpdateContent(const VideoContentMetrics* content_metrics);

  
  
  void ComputeSpatial();

  
  
  void ComputeMotionNFD();

  
  ImageType GetImageType(uint16_t width, uint16_t height);

  
  ImageType FindClosestImageType(uint16_t width, uint16_t height);

  
  FrameRateLevelClass FrameRateLevel(float frame_rate);

 protected:
  
  const VideoContentMetrics* content_metrics_;

  
  uint16_t width_;
  uint16_t height_;
  float user_frame_rate_;
  uint16_t native_width_;
  uint16_t native_height_;
  float native_frame_rate_;
  float aspect_ratio_;
  
  ImageType image_type_;
  FrameRateLevelClass framerate_level_;
  
  VCMContFeature motion_;
  VCMContFeature spatial_;
  uint8_t content_class_;
  bool init_;
};



class VCMQmResolution : public VCMQmMethod {
 public:
  VCMQmResolution();
  virtual ~VCMQmResolution();

  
  virtual void Reset();

  
  void ResetRates();

  
  void ResetDownSamplingState();

  
  EncoderState GetEncoderState();

  
  int Initialize(float bitrate,
                 float user_framerate,
                 uint16_t width,
                 uint16_t height,
                 int num_layers);

  
  void UpdateCodecParameters(float frame_rate, uint16_t width, uint16_t height);

  
  
  void UpdateEncodedSize(int encoded_size,
                         FrameType encoded_frame_type);

  
  
  void UpdateRates(float target_bitrate,
                   float encoder_sent_rate,
                   float incoming_framerate,
                   uint8_t packet_loss);

  
  
  
  int SelectResolution(VCMResolutionScale** qm);

  
  void SetCPULoadState(CPULoadState state);

 private:
  
  void SetDefaultAction();

  
  void ComputeRatesForSelection();

  
  void ComputeEncoderState();

  
  bool GoingUpResolution();

  
  bool GoingDownResolution();

  
  
  
  bool ConditionForGoingUp(float fac_width,
                           float fac_height,
                           float fac_temp,
                           float scale_fac);

  
  
  
  float GetTransitionRate(float fac_width,
                          float fac_height,
                          float fac_temp,
                          float scale_fac);

  
  void UpdateDownsamplingState(UpDownAction up_down);

  
  void UpdateCodecResolution();

  
  uint8_t RateClass(float transition_rate);

  
  void AdjustAction();

  
  void ConvertSpatialFractionalToWhole();

  
  
  bool EvenFrameSize();

  
  void InsertLatestDownAction();

  
  void RemoveLastDownAction();

  
  void ConstrainAmountOfDownSampling();

  
  
  void PickSpatialOrTemporal();

  
  void SelectSpatialDirectionMode(float transition_rate);

  enum { kDownActionHistorySize = 10};

  VCMResolutionScale* qm_;
  
  float target_bitrate_;
  float incoming_framerate_;
  float per_frame_bandwidth_;
  float buffer_level_;

  
  float sum_target_rate_;
  float sum_incoming_framerate_;
  float sum_rate_MM_;
  float sum_rate_MM_sgn_;
  float sum_packet_loss_;
  
  uint32_t frame_cnt_;
  uint32_t frame_cnt_delta_;
  uint32_t update_rate_cnt_;
  uint32_t low_buffer_cnt_;

  
  float state_dec_factor_spatial_;
  float state_dec_factor_temporal_;

  
  float avg_target_rate_;
  float avg_incoming_framerate_;
  float avg_ratio_buffer_low_;
  float avg_rate_mismatch_;
  float avg_rate_mismatch_sgn_;
  float avg_packet_loss_;
  EncoderState encoder_state_;
  ResolutionAction action_;
  
  
  
  
  ResolutionAction down_action_history_[kDownActionHistorySize];
  int num_layers_;
  CPULoadState loadstate_;
};



class VCMQmRobustness : public VCMQmMethod {
 public:
  VCMQmRobustness();
  ~VCMQmRobustness();

  virtual void Reset();

  
  
  float AdjustFecFactor(uint8_t code_rate_delta,
                        float total_rate,
                        float framerate,
                        uint32_t rtt_time,
                        uint8_t packet_loss);

  
  bool SetUepProtection(uint8_t code_rate_delta,
                        float total_rate,
                        uint8_t packet_loss,
                        bool frame_type);

 private:
  
  float prev_total_rate_;
  uint32_t prev_rtt_time_;
  uint8_t prev_packet_loss_;
  uint8_t prev_code_rate_delta_;
};
}  
#endif  
