









#include "modules/video_coding/codecs/test/videoprocessor.h"

#include <cassert>
#include <cstring>
#include <limits>

#include "system_wrappers/interface/cpu_info.h"

namespace webrtc {
namespace test {

VideoProcessorImpl::VideoProcessorImpl(webrtc::VideoEncoder* encoder,
                                       webrtc::VideoDecoder* decoder,
                                       FrameReader* frame_reader,
                                       FrameWriter* frame_writer,
                                       PacketManipulator* packet_manipulator,
                                       const TestConfig& config,
                                       Stats* stats)
    : encoder_(encoder),
      decoder_(decoder),
      frame_reader_(frame_reader),
      frame_writer_(frame_writer),
      packet_manipulator_(packet_manipulator),
      config_(config),
      stats_(stats),
      encode_callback_(NULL),
      decode_callback_(NULL),
      source_buffer_(NULL),
      first_key_frame_has_been_excluded_(false),
      last_frame_missing_(false),
      initialized_(false),
      encoded_frame_size_(0),
      prev_time_stamp_(0),
      num_dropped_frames_(0),
      num_spatial_resizes_(0),
      last_encoder_frame_width_(0),
      last_encoder_frame_height_(0),
      scaler_() {
  assert(encoder);
  assert(decoder);
  assert(frame_reader);
  assert(frame_writer);
  assert(packet_manipulator);
  assert(stats);
}

bool VideoProcessorImpl::Init() {
  
  bit_rate_factor_ = config_.codec_settings->maxFramerate * 0.001 * 8;  

  
  int frame_length_in_bytes = frame_reader_->FrameLength();
  source_buffer_ = new WebRtc_UWord8[frame_length_in_bytes];
  last_successful_frame_buffer_ = new WebRtc_UWord8[frame_length_in_bytes];
  
  
  last_encoder_frame_width_ = config_.codec_settings->width;
  last_encoder_frame_height_ = config_.codec_settings->height;

  
  encode_callback_ = new VideoProcessorEncodeCompleteCallback(this);
  decode_callback_ = new VideoProcessorDecodeCompleteCallback(this);
  WebRtc_Word32 register_result =
      encoder_->RegisterEncodeCompleteCallback(encode_callback_);
  if (register_result != WEBRTC_VIDEO_CODEC_OK) {
    fprintf(stderr, "Failed to register encode complete callback, return code: "
        "%d\n", register_result);
    return false;
  }
  register_result = decoder_->RegisterDecodeCompleteCallback(decode_callback_);
  if (register_result != WEBRTC_VIDEO_CODEC_OK) {
    fprintf(stderr, "Failed to register decode complete callback, return code: "
            "%d\n", register_result);
    return false;
  }
  
  WebRtc_UWord32 nbr_of_cores = 1;
  if (!config_.use_single_core) {
    nbr_of_cores = CpuInfo::DetectNumberOfCores();
  }
  WebRtc_Word32 init_result =
      encoder_->InitEncode(config_.codec_settings, nbr_of_cores,
                           config_.networking_config.max_payload_size_in_bytes);
  if (init_result != WEBRTC_VIDEO_CODEC_OK) {
    fprintf(stderr, "Failed to initialize VideoEncoder, return code: %d\n",
            init_result);
    return false;
  }
  init_result = decoder_->InitDecode(config_.codec_settings, nbr_of_cores);
  if (init_result != WEBRTC_VIDEO_CODEC_OK) {
    fprintf(stderr, "Failed to initialize VideoDecoder, return code: %d\n",
            init_result);
    return false;
  }

  if (config_.verbose) {
    printf("Video Processor:\n");
    printf("  #CPU cores used  : %d\n", nbr_of_cores);
    printf("  Total # of frames: %d\n", frame_reader_->NumberOfFrames());
    printf("  Codec settings:\n");
    printf("    Start bitrate  : %d kbps\n",
           config_.codec_settings->startBitrate);
    printf("    Width          : %d\n", config_.codec_settings->width);
    printf("    Height         : %d\n", config_.codec_settings->height);
  }
  initialized_ = true;
  return true;
}

VideoProcessorImpl::~VideoProcessorImpl() {
  delete[] source_buffer_;
  delete[] last_successful_frame_buffer_;
  encoder_->RegisterEncodeCompleteCallback(NULL);
  delete encode_callback_;
  decoder_->RegisterDecodeCompleteCallback(NULL);
  delete decode_callback_;
}


void VideoProcessorImpl::SetRates(int bit_rate, int frame_rate) {
  int set_rates_result = encoder_->SetRates(bit_rate, frame_rate);
  assert(set_rates_result >= 0);
  if (set_rates_result < 0) {
    fprintf(stderr, "Failed to update encoder with new rate %d, "
            "return code: %d\n", bit_rate, set_rates_result);
  }
  num_dropped_frames_ = 0;
  num_spatial_resizes_ = 0;
}

int VideoProcessorImpl::EncodedFrameSize() {
  return encoded_frame_size_;
}

int VideoProcessorImpl::NumberDroppedFrames() {
  return num_dropped_frames_;
}

int VideoProcessorImpl::NumberSpatialResizes() {
  return num_spatial_resizes_;
}

bool VideoProcessorImpl::ProcessFrame(int frame_number) {
  assert(frame_number >=0);
  if (!initialized_) {
    fprintf(stderr, "Attempting to use uninitialized VideoProcessor!\n");
    return false;
  }
  
  if (frame_number == 0) {
    prev_time_stamp_ = -1;
  }
  if (frame_reader_->ReadFrame(source_buffer_)) {
    
    int size_y = config_.codec_settings->width * config_.codec_settings->height;
    int half_width = (config_.codec_settings->width + 1) / 2;
    int half_height = (config_.codec_settings->height + 1) / 2;
    int size_uv = half_width * half_height;
    source_frame_.CreateFrame(size_y, source_buffer_,
                              size_uv, source_buffer_ + size_y,
                              size_uv, source_buffer_ + size_y + size_uv,
                              config_.codec_settings->width,
                              config_.codec_settings->height,
                              config_.codec_settings->width,
                              half_width, half_width);

    
    FrameStatistic& stat = stats_->NewFrame(frame_number);

    encode_start_ = TickTime::Now();
    
    source_frame_.set_timestamp(frame_number);

    
    std::vector<VideoFrameType> frame_types(1, kDeltaFrame);
    if (config_.keyframe_interval > 0 &&
        frame_number % config_.keyframe_interval == 0) {
      frame_types[0] = kKeyFrame;
    }

    
    encoded_frame_size_ = 0;

    WebRtc_Word32 encode_result = encoder_->Encode(source_frame_, NULL,
                                                   &frame_types);

    if (encode_result != WEBRTC_VIDEO_CODEC_OK) {
      fprintf(stderr, "Failed to encode frame %d, return code: %d\n",
              frame_number, encode_result);
    }
    stat.encode_return_code = encode_result;
    return true;
  } else {
    return false;  
  }
}

void VideoProcessorImpl::FrameEncoded(EncodedImage* encoded_image) {
  
  int num_dropped_from_prev_encode =  encoded_image->_timeStamp -
      prev_time_stamp_ - 1;
  num_dropped_frames_ +=  num_dropped_from_prev_encode;
  prev_time_stamp_ =  encoded_image->_timeStamp;
  if (num_dropped_from_prev_encode > 0) {
    
    
    for (int i = 0; i < num_dropped_from_prev_encode; i++) {
      frame_writer_->WriteFrame(last_successful_frame_buffer_);
    }
  }
  
  
  encoded_frame_size_ = encoded_image->_length;

  TickTime encode_stop = TickTime::Now();
  int frame_number = encoded_image->_timeStamp;
  FrameStatistic& stat = stats_->stats_[frame_number];
  stat.encode_time_in_us = GetElapsedTimeMicroseconds(encode_start_,
                                                      encode_stop);
  stat.encoding_successful = true;
  stat.encoded_frame_length_in_bytes = encoded_image->_length;
  stat.frame_number = encoded_image->_timeStamp;
  stat.frame_type = encoded_image->_frameType;
  stat.bit_rate_in_kbps = encoded_image->_length * bit_rate_factor_;
  stat.total_packets = encoded_image->_length /
      config_.networking_config.packet_size_in_bytes + 1;

  
  bool exclude_this_frame = false;
  
  if (encoded_image->_frameType == kKeyFrame) {
    switch (config_.exclude_frame_types) {
      case kExcludeOnlyFirstKeyFrame:
        if (!first_key_frame_has_been_excluded_) {
          first_key_frame_has_been_excluded_ = true;
          exclude_this_frame = true;
        }
        break;
      case kExcludeAllKeyFrames:
        exclude_this_frame = true;
        break;
      default:
        assert(false);
    }
  }
  if (!exclude_this_frame) {
    stat.packets_dropped =
          packet_manipulator_->ManipulatePackets(encoded_image);
  }

  
  
  decode_start_ = TickTime::Now();
  
  
  WebRtc_Word32 decode_result = decoder_->Decode(*encoded_image,
                                                 last_frame_missing_, NULL);
  stat.decode_return_code = decode_result;
  if (decode_result != WEBRTC_VIDEO_CODEC_OK) {
    
    
    frame_writer_->WriteFrame(last_successful_frame_buffer_);
  }
  
  last_frame_missing_ = encoded_image->_length == 0;
}

void VideoProcessorImpl::FrameDecoded(const I420VideoFrame& image) {
  TickTime decode_stop = TickTime::Now();
  int frame_number = image.timestamp();
  
  FrameStatistic& stat = stats_->stats_[frame_number];
  stat.decode_time_in_us = GetElapsedTimeMicroseconds(decode_start_,
                                                      decode_stop);
  stat.decoding_successful = true;

  
  if (static_cast<int>(image.width()) != last_encoder_frame_width_ ||
      static_cast<int>(image.height()) != last_encoder_frame_height_ ) {
    ++num_spatial_resizes_;
    last_encoder_frame_width_ = image.width();
    last_encoder_frame_height_ = image.height();
  }
  
  
  if (image.width() !=  config_.codec_settings->width ||
      image.height() != config_.codec_settings->height) {
    I420VideoFrame up_image;
    int ret_val = scaler_.Set(image.width(), image.height(),
                              config_.codec_settings->width,
                              config_.codec_settings->height,
                              kI420, kI420, kScaleBilinear);
    assert(ret_val >= 0);
    if (ret_val < 0) {
      fprintf(stderr, "Failed to set scalar for frame: %d, return code: %d\n",
              frame_number, ret_val);
    }
    ret_val = scaler_.Scale(image, &up_image);
    assert(ret_val >= 0);
    if (ret_val < 0) {
      fprintf(stderr, "Failed to scale frame: %d, return code: %d\n",
              frame_number, ret_val);
    }
    
    int length = CalcBufferSize(kI420, up_image.width(), up_image.height());
    scoped_array<uint8_t> image_buffer(new uint8_t[length]);
    length = ExtractBuffer(up_image, length, image_buffer.get());
    
    memcpy(last_successful_frame_buffer_, image_buffer.get(), length);
    bool write_success = frame_writer_->WriteFrame(image_buffer.get());
    assert(write_success);
    if (!write_success) {
      fprintf(stderr, "Failed to write frame %d to disk!", frame_number);
    }
  } else {  
    
    
    int length = CalcBufferSize(kI420,image.width(), image.height());
    scoped_array<uint8_t> image_buffer(new uint8_t[length]);
    length = ExtractBuffer(image, length, image_buffer.get());
    assert(length > 0);
    memcpy(last_successful_frame_buffer_, image_buffer.get(), length);

    bool write_success = frame_writer_->WriteFrame(image_buffer.get());
    assert(write_success);
    if (!write_success) {
      fprintf(stderr, "Failed to write frame %d to disk!", frame_number);
    }
  }
}

int VideoProcessorImpl::GetElapsedTimeMicroseconds(
    const webrtc::TickTime& start, const webrtc::TickTime& stop) {
  WebRtc_UWord64 encode_time = (stop - start).Microseconds();
  assert(encode_time <
         static_cast<unsigned int>(std::numeric_limits<int>::max()));
  return static_cast<int>(encode_time);
}

const char* ExcludeFrameTypesToStr(ExcludeFrameTypes e) {
  switch (e) {
    case kExcludeOnlyFirstKeyFrame:
      return "ExcludeOnlyFirstKeyFrame";
    case kExcludeAllKeyFrames:
      return "ExcludeAllKeyFrames";
    default:
      assert(false);
      return "Unknown";
  }
}

const char* VideoCodecTypeToStr(webrtc::VideoCodecType e) {
  switch (e) {
    case kVideoCodecVP8:
      return "VP8";
    case kVideoCodecI420:
      return "I420";
    case kVideoCodecRED:
      return "RED";
    case kVideoCodecULPFEC:
      return "ULPFEC";
    case kVideoCodecUnknown:
      return "Unknown";
    default:
      assert(false);
      return "Unknown";
  }
}


WebRtc_Word32
VideoProcessorImpl::VideoProcessorEncodeCompleteCallback::Encoded(
    EncodedImage& encoded_image,
    const webrtc::CodecSpecificInfo* codec_specific_info,
    const webrtc::RTPFragmentationHeader* fragmentation) {
  video_processor_->FrameEncoded(&encoded_image);  
  return 0;
}
WebRtc_Word32
VideoProcessorImpl::VideoProcessorDecodeCompleteCallback::Decoded(
    I420VideoFrame& image) {
  video_processor_->FrameDecoded(image);  
  return 0;
}

}  
}  
