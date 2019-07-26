









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/typedefs.h"

namespace webrtc {


struct CodecInst;
struct WebRtcRTPHeader;
class AudioFrame;
class RTPFragmentationHeader;
class Clock;

#define WEBRTC_10MS_PCM_AUDIO 960  // 16 bits super wideband 48 kHz


class AudioPacketizationCallback {
 public:
  virtual ~AudioPacketizationCallback() {}

  virtual int32_t SendData(
      FrameType frame_type,
      uint8_t payload_type,
      uint32_t timestamp,
      const uint8_t* payload_data,
      uint16_t payload_len_bytes,
      const RTPFragmentationHeader* fragmentation) = 0;
};


class AudioCodingFeedback {
 public:
  virtual ~AudioCodingFeedback() {}

  virtual int32_t IncomingDtmf(const uint8_t digit_dtmf,
                               const bool end) = 0;
};


class ACMVADCallback {
 public:
  virtual ~ACMVADCallback() {}

  virtual int32_t InFrameType(int16_t frameType) = 0;
};


class ACMVQMonCallback {
 public:
  virtual ~ACMVQMonCallback() {}

  virtual int32_t NetEqStatistics(
      const int32_t id,  
      const uint16_t MIUsValid,  
      const uint16_t MIUsReplaced,  
      const uint8_t eventFlags,  
      const uint16_t delayMS) = 0;  
};


extern const char kLegacyAcmVersion[];
extern const char kExperimentalAcmVersion[];

class AudioCodingModule: public Module {
 protected:
  AudioCodingModule() {}

 public:
  
  
  
  
  
  
  
  static AudioCodingModule* Create(int id);
  static AudioCodingModule* Create(int id, Clock* clock);
  virtual ~AudioCodingModule() {};

  
  
  

  
  
  
  
  
  
  
  static int NumberOfCodecs();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int Codec(int list_id, CodecInst* codec);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int Codec(const char* payload_name, CodecInst* codec,
                       int sampling_freq_hz, int channels);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int Codec(const char* payload_name, int sampling_freq_hz,
                             int channels);

  
  
  
  
  
  
  
  
  
  
  
  
  static bool IsCodecValid(const CodecInst& codec);

  
  
  
  virtual const char* Version() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t InitializeSender() = 0;

  
  
  
  
  
  
  
  
  
  virtual int32_t ResetEncoder() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t RegisterSendCodec(const CodecInst& send_codec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int RegisterSecondarySendCodec(const CodecInst& send_codec) = 0;

  
  
  
  
  virtual void UnregisterSecondarySendCodec() = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SendCodec(CodecInst* current_send_codec) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int SecondarySendCodec(CodecInst* secondary_codec) const = 0;

  
  
  
  
  
  
  
  
  virtual int32_t SendFrequency() const = 0;

  
  
  
  
  
  
  
  
  virtual int32_t SendBitrate() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetReceivedEstimatedBandwidth(
      const int32_t bw) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t RegisterTransportCallback(
      AudioPacketizationCallback* transport) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t Add10MsData(const AudioFrame& audio_frame) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetFECStatus(const bool enable_fec) = 0;

  
  
  
  
  
  
  
  
  virtual bool FECStatus() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetVAD(const bool enable_dtx = true,
                               const bool enable_vad = false,
                               const ACMVADMode vad_mode = VADNormal) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t VAD(bool* dtx_enabled, bool* vad_enabled,
                            ACMVADMode* vad_mode) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t ReplaceInternalDTXWithWebRtc(
      const bool use_webrtc_dtx = false) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t IsInternalDTXReplacedWithWebRtc(
      bool* uses_webrtc_dtx) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t RegisterVADCallback(ACMVADCallback* vad_callback) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t InitializeReceiver() = 0;

  
  
  
  
  
  
  
  
  
  virtual int32_t ResetDecoder() = 0;

  
  
  
  
  
  
  
  
  virtual int32_t ReceiveFrequency() const = 0;

  
  
  
  
  
  
  
  virtual int32_t PlayoutFrequency() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t RegisterReceiveCodec(
      const CodecInst& receive_codec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int UnregisterReceiveCodec(
      uint8_t payload_type) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t ReceiveCodec(CodecInst* curr_receive_codec) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t IncomingPacket(const uint8_t* incoming_payload,
                                       const int32_t payload_len_bytes,
                                       const WebRtcRTPHeader& rtp_info) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t IncomingPayload(const uint8_t* incoming_payload,
                                        const int32_t payload_len_byte,
                                        const uint8_t payload_type,
                                        const uint32_t timestamp = 0) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetMinimumPlayoutDelay(int time_ms) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int SetMaximumPlayoutDelay(int time_ms) = 0;

  
  
  
  
  
  
  virtual int LeastRequiredDelayMs() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetDtmfPlayoutStatus(const bool enable) = 0;

  
  
  
  
  
  
  
  
  virtual bool DtmfPlayoutStatus() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t PlayoutTimestamp(uint32_t* timestamp) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t DecoderEstimatedBandwidth() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetPlayoutMode(const AudioPlayoutMode mode) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual AudioPlayoutMode PlayoutMode() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t PlayoutData10Ms(int32_t desired_freq_hz,
                                        AudioFrame* audio_frame) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetISACMaxRate(int max_rate_bps) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetISACMaxPayloadSize(int max_payload_len_bytes) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t ConfigISACBandwidthEstimator(
      int init_frame_size_ms,
      int init_rate_bps,
      bool enforce_frame_size = false) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t NetworkStatistics(
      ACMNetworkStatistics* network_statistics) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetInitialPlayoutDelay(int delay_ms) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int EnableNack(size_t max_nack_list_size) = 0;

  
  virtual void DisableNack() = 0;

  
  
  
  
  
  
  
  
  
  virtual std::vector<uint16_t> GetNackList(int round_trip_time_ms) const = 0;

  virtual void GetDecodingCallStatistics(
      AudioDecodingCallStats* call_stats) const = 0;
};

struct AudioCodingModuleFactory {
  AudioCodingModuleFactory() {}
  virtual ~AudioCodingModuleFactory() {}

  virtual AudioCodingModule* Create(int id) const;
};

struct NewAudioCodingModuleFactory : AudioCodingModuleFactory {
  NewAudioCodingModuleFactory() {}
  virtual ~NewAudioCodingModuleFactory() {}

  virtual AudioCodingModule* Create(int id) const;
};

}  

#endif  
