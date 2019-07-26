









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_VIDEOPROCESSOR_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_VIDEOPROCESSOR_H_

#include <string>

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "common_video/libyuv/include/scaler.h"
#include "common_video/interface/i420_video_frame.h"
#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "modules/video_coding/codecs/test/packet_manipulator.h"
#include "modules/video_coding/codecs/test/stats.h"
#include "system_wrappers/interface/tick_util.h"
#include "testsupport/frame_reader.h"
#include "testsupport/frame_writer.h"

namespace webrtc {
namespace test {


enum ExcludeFrameTypes {
  
  
  kExcludeOnlyFirstKeyFrame,
  
  
  kExcludeAllKeyFrames
};

const char* ExcludeFrameTypesToStr(ExcludeFrameTypes e);


struct TestConfig {
  TestConfig()
    : name(""), description(""), test_number(0),
      input_filename(""), output_filename(""), output_dir("out"),
      networking_config(), exclude_frame_types(kExcludeOnlyFirstKeyFrame),
      frame_length_in_bytes(-1), use_single_core(false), keyframe_interval(0),
      codec_settings(NULL), verbose(true) {
  };

  
  
  std::string name;

  
  
  std::string description;

  
  
  int test_number;

  
  std::string input_filename;

  
  
  std::string output_filename;

  
  
  std::string output_dir;

  
  NetworkingConfig networking_config;

  
  
  ExcludeFrameTypes exclude_frame_types;

  
  
  
  int frame_length_in_bytes;

  
  
  
  
  
  
  
  bool use_single_core;

  
  
  
  
  
  
  
  int keyframe_interval;

  
  
  
  webrtc::VideoCodec* codec_settings;

  
  bool verbose;
};


const char* VideoCodecTypeToStr(webrtc::VideoCodecType e);



















class VideoProcessor {
 public:
  virtual ~VideoProcessor() {}

  
  
  virtual bool Init() = 0;

  
  
  
  virtual bool ProcessFrame(int frame_number) = 0;

  
  virtual void SetRates(int bit_rate, int frame_rate) = 0;

  
  
  virtual int EncodedFrameSize() = 0;

  
  virtual int NumberDroppedFrames() = 0;

  
  virtual int NumberSpatialResizes() = 0;
};

class VideoProcessorImpl : public VideoProcessor {
 public:
  VideoProcessorImpl(webrtc::VideoEncoder* encoder,
                     webrtc::VideoDecoder* decoder,
                     FrameReader* frame_reader,
                     FrameWriter* frame_writer,
                     PacketManipulator* packet_manipulator,
                     const TestConfig& config,
                     Stats* stats);
  virtual ~VideoProcessorImpl();
  virtual bool Init();
  virtual bool ProcessFrame(int frame_number);

 private:
  
  void FrameEncoded(webrtc::EncodedImage* encodedImage);
  
  void FrameDecoded(const webrtc::I420VideoFrame& image);
  
  
  int GetElapsedTimeMicroseconds(const webrtc::TickTime& start,
                                 const webrtc::TickTime& stop);
  
  void SetRates(int bit_rate, int frame_rate);
  
  int EncodedFrameSize();
  
  int NumberDroppedFrames();
  
  int NumberSpatialResizes();

  webrtc::VideoEncoder* encoder_;
  webrtc::VideoDecoder* decoder_;
  FrameReader* frame_reader_;
  FrameWriter* frame_writer_;
  PacketManipulator* packet_manipulator_;
  const TestConfig& config_;
  Stats* stats_;

  EncodedImageCallback* encode_callback_;
  DecodedImageCallback* decode_callback_;
  
  WebRtc_UWord8* source_buffer_;
  
  
  WebRtc_UWord8* last_successful_frame_buffer_;
  webrtc::I420VideoFrame source_frame_;
  
  bool first_key_frame_has_been_excluded_;
  
  bool last_frame_missing_;
  
  bool initialized_;
  int encoded_frame_size_;
  int prev_time_stamp_;
  int num_dropped_frames_;
  int num_spatial_resizes_;
  int last_encoder_frame_width_;
  int last_encoder_frame_height_;
  Scaler scaler_;

  
  double bit_rate_factor_;  
  webrtc::TickTime encode_start_;
  webrtc::TickTime decode_start_;

  
  class VideoProcessorEncodeCompleteCallback
    : public webrtc::EncodedImageCallback {
   public:
      explicit VideoProcessorEncodeCompleteCallback(VideoProcessorImpl* vp)
        : video_processor_(vp) {
    }
    WebRtc_Word32 Encoded(
        webrtc::EncodedImage& encoded_image,
        const webrtc::CodecSpecificInfo* codec_specific_info = NULL,
        const webrtc::RTPFragmentationHeader* fragmentation = NULL);

   private:
    VideoProcessorImpl* video_processor_;
  };

  
  class VideoProcessorDecodeCompleteCallback
    : public webrtc::DecodedImageCallback {
   public:
      explicit VideoProcessorDecodeCompleteCallback(VideoProcessorImpl* vp)
      : video_processor_(vp) {
    }
    WebRtc_Word32 Decoded(webrtc::I420VideoFrame& image);

   private:
    VideoProcessorImpl* video_processor_;
  };
};

}  
}  

#endif  
