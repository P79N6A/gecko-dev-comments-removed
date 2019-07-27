









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H_

#include <vector>

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/acm2/acm_codec_database.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/neteq/interface/neteq.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/typedefs.h"

namespace webrtc {


struct CodecInst;
struct WebRtcRTPHeader;
class AudioFrame;
class RTPFragmentationHeader;

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

class AudioCodingModule: public Module {
 protected:
  AudioCodingModule() {}

 public:
  struct Config {
    Config()
        : id(0),
          neteq_config(),
          clock(Clock::GetRealTimeClock()) {}

    int id;
    NetEq::Config neteq_config;
    Clock* clock;
  };

  
  
  
  
  
  
  
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

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t SetREDStatus(bool enable_red) = 0;

  
  
  
  
  
  
  
  
  virtual bool REDStatus() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetCodecFEC(bool enable_codec_fec) = 0;

  
  
  
  
  
  
  
  
  virtual bool CodecFEC() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetPacketLossRate(int packet_loss_rate) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetOpusMaxPlaybackRate(int frequency_hz) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  virtual int32_t NetworkStatistics(
      ACMNetworkStatistics* network_statistics) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int SetInitialPlayoutDelay(int delay_ms) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int EnableNack(size_t max_nack_list_size) = 0;

  
  virtual void DisableNack() = 0;

  
  
  
  
  
  
  
  
  
  virtual std::vector<uint16_t> GetNackList(int round_trip_time_ms) const = 0;

  virtual void GetDecodingCallStatistics(
      AudioDecodingCallStats* call_stats) const = 0;
};

class AudioEncoder;
class ReceiverInfo;

class AudioCoding {
 public:
  struct Config {
    Config()
        : neteq_config(),
          clock(Clock::GetRealTimeClock()),
          transport(NULL),
          vad_callback(NULL),
          play_dtmf(true),
          initial_playout_delay_ms(0),
          playout_channels(1),
          playout_frequency_hz(32000) {}

    AudioCodingModule::Config ToOldConfig() const {
      AudioCodingModule::Config old_config;
      old_config.id = 0;
      old_config.neteq_config = neteq_config;
      old_config.clock = clock;
      return old_config;
    }

    NetEq::Config neteq_config;
    Clock* clock;
    AudioPacketizationCallback* transport;
    ACMVADCallback* vad_callback;
    bool play_dtmf;
    int initial_playout_delay_ms;
    int playout_channels;
    int playout_frequency_hz;
  };

  static AudioCoding* Create(const Config& config);
  virtual ~AudioCoding() {};

  
  
  
  
  
  
  virtual bool RegisterSendCodec(AudioEncoder* send_codec) = 0;

  
  
  virtual bool RegisterSendCodec(int encoder_type,
                                 uint8_t payload_type,
                                 int frame_size_samples = 0) = 0;

  
  
  virtual const AudioEncoder* GetSenderInfo() const = 0;

  
  virtual const CodecInst* GetSenderCodecInst() = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int Add10MsAudio(const AudioFrame& audio_frame) = 0;

  
  virtual const ReceiverInfo* GetReceiverInfo() const = 0;

  
  
  
  
  virtual bool RegisterReceiveCodec(AudioDecoder* receive_codec) = 0;

  
  
  virtual bool RegisterReceiveCodec(int decoder_type, uint8_t payload_type) = 0;

  
  
  
  
  
  
  virtual bool InsertPacket(const uint8_t* incoming_payload,
                            int32_t payload_len_bytes,
                            const WebRtcRTPHeader& rtp_info) = 0;

  
  virtual bool InsertPayload(const uint8_t* incoming_payload,
                             int32_t payload_len_byte,
                             uint8_t payload_type,
                             uint32_t timestamp) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual bool SetMinimumPlayoutDelay(int time_ms) = 0;

  virtual bool SetMaximumPlayoutDelay(int time_ms) = 0;

  
  
  
  
  
  
  virtual int LeastRequiredDelayMs() const = 0;

  
  
  
  
  virtual bool PlayoutTimestamp(uint32_t* timestamp) = 0;

  
  
  virtual bool Get10MsAudio(AudioFrame* audio_frame) = 0;

  
  
  virtual bool NetworkStatistics(ACMNetworkStatistics* network_statistics) = 0;

  
  
  
  
  
  
  
  
  
  virtual bool EnableNack(size_t max_nack_list_size) = 0;

  
  virtual void DisableNack() = 0;


  
  
  
  
  
  
  
  
  
  
  
  virtual bool SetVad(bool enable_dtx,
                      bool enable_vad,
                      ACMVADMode vad_mode) = 0;

  
  
  
  
  
  
  virtual std::vector<uint16_t> GetNackList(int round_trip_time_ms) const = 0;

  
  virtual void GetDecodingCallStatistics(
      AudioDecodingCallStats* call_stats) const = 0;
};

}  

#endif  
