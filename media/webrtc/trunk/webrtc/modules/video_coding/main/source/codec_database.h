









#ifndef WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_CODEC_DATABASE_H_
#define WEBRTC_MODULES_VIDEO_CODING_MAIN_SOURCE_CODEC_DATABASE_H_

#include <map>

#include "modules/video_coding/codecs/interface/video_codec_interface.h"
#include "modules/video_coding/main/interface/video_coding.h"
#include "modules/video_coding/main/source/generic_decoder.h"
#include "modules/video_coding/main/source/generic_encoder.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"

namespace webrtc {

enum VCMCodecDBProperties {
  kDefaultPayloadSize = 1440
};

struct VCMDecoderMapItem {
 public:
  VCMDecoderMapItem(VideoCodec* settings,
                    int number_of_cores,
                    bool require_key_frame);

  scoped_ptr<VideoCodec> settings;
  int number_of_cores;
  bool require_key_frame;
};

struct VCMExtDecoderMapItem {
 public:
  VCMExtDecoderMapItem(VideoDecoder* external_decoder_instance,
                       uint8_t payload_type,
                       bool internal_render_timing);

  uint8_t payload_type;
  VideoDecoder* external_decoder_instance;
  bool internal_render_timing;
};

class VCMCodecDataBase {
 public:
  explicit VCMCodecDataBase(int id);
  ~VCMCodecDataBase();

  
  
  static int NumberOfCodecs();

  
  static bool Codec(int list_id, VideoCodec* settings);

  
  static bool Codec(VideoCodecType codec_type, VideoCodec* settings);

  void ResetSender();

  
  
  
  bool RegisterSendCodec(const VideoCodec* send_codec,
                         int number_of_cores,
                         int max_payload_size);

  
  
  bool SendCodec(VideoCodec* current_send_codec) const;

  
  
  VideoCodecType SendCodec() const;

  
  
  
  
  void RegisterExternalEncoder(VideoEncoder* external_encoder,
                               uint8_t payload_type,
                               bool internal_source);

  
  
  
  bool DeregisterExternalEncoder(uint8_t payload_type, bool* was_send_codec);

  
  
  
  
  
  
  VCMGenericEncoder* GetEncoder(
      const VideoCodec* settings,
      VCMEncodedFrameCallback* encoded_frame_callback);

  bool SetPeriodicKeyFrames(bool enable);

  
  void ResetReceiver();

  
  bool DeregisterExternalDecoder(uint8_t payload_type);

  
  
  
  
  bool RegisterExternalDecoder(VideoDecoder* external_decoder,
                               uint8_t payload_type,
                               bool internal_render_timing);

  bool DecoderRegistered() const;

  bool RegisterReceiveCodec(const VideoCodec* receive_codec,
                            int number_of_cores,
                            bool require_key_frame);

  bool DeregisterReceiveCodec(uint8_t payload_type);

  
  bool ReceiveCodec(VideoCodec* current_receive_codec) const;

  
  VideoCodecType ReceiveCodec() const;

  
  
  
  
  
  VCMGenericDecoder* GetDecoder(
      uint8_t payload_type, VCMDecodedFrameCallback* decoded_frame_callback);

  
  VCMGenericDecoder* CreateDecoderCopy() const;

  
  
  void ReleaseDecoder(VCMGenericDecoder* decoder) const;

  
  
  void CopyDecoder(const VCMGenericDecoder& decoder);

  
  
  
  bool SupportsRenderScheduling() const;

 private:
  typedef std::map<uint8_t, VCMDecoderMapItem*> DecoderMap;
  typedef std::map<uint8_t, VCMExtDecoderMapItem*> ExternalDecoderMap;

  VCMGenericDecoder* CreateAndInitDecoder(uint8_t payload_type,
                                          VideoCodec* new_codec,
                                          bool* external) const;

  
  VCMGenericEncoder* CreateEncoder(const VideoCodecType type) const;

  void DeleteEncoder();

  
  VCMGenericDecoder* CreateDecoder(VideoCodecType type) const;

  const VCMDecoderMapItem* FindDecoderItem(uint8_t payload_type) const;

  const VCMExtDecoderMapItem* FindExternalDecoderItem(
      uint8_t payload_type) const;

  int id_;
  int number_of_cores_;
  int max_payload_size_;
  bool periodic_key_frames_;
  bool current_enc_is_external_;
  VideoCodec send_codec_;
  VideoCodec receive_codec_;
  uint8_t external_payload_type_;
  VideoEncoder* external_encoder_;
  bool internal_source_;
  VCMGenericEncoder* ptr_encoder_;
  VCMGenericDecoder* ptr_decoder_;
  bool current_dec_is_external_;
  DecoderMap dec_map_;
  ExternalDecoderMap dec_external_map_;
};  

}  

#endif  
