









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H

#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/interface/module.h"
#include "webrtc/typedefs.h"

namespace webrtc {


struct CodecInst;
struct WebRtcRTPHeader;
class AudioFrame;
class RTPFragmentationHeader;

#define WEBRTC_10MS_PCM_AUDIO 960 // 16 bits super wideband 48 kHz


class AudioPacketizationCallback {
 public:
  virtual ~AudioPacketizationCallback() {}

  virtual WebRtc_Word32 SendData(
      FrameType frame_type,
      WebRtc_UWord8 payload_type,
      WebRtc_UWord32 timestamp,
      const WebRtc_UWord8* payload_data,
      WebRtc_UWord16 payload_len_bytes,
      const RTPFragmentationHeader* fragmentation) = 0;
};


class AudioCodingFeedback {
 public:
  virtual ~AudioCodingFeedback() {}

  virtual WebRtc_Word32 IncomingDtmf(const WebRtc_UWord8 digit_dtmf,
                                     const bool end) = 0;
};


class ACMVADCallback {
 public:
  virtual ~ACMVADCallback() {}

  virtual WebRtc_Word32 InFrameType(WebRtc_Word16 frameType) = 0;
};


class ACMVQMonCallback {
 public:
  virtual ~ACMVQMonCallback() {}

  virtual WebRtc_Word32 NetEqStatistics(
      const WebRtc_Word32 id, 
      const WebRtc_UWord16 MIUsValid, 
      const WebRtc_UWord16 MIUsReplaced, 
      const WebRtc_UWord8 eventFlags, 
      const WebRtc_UWord16 delayMS) = 0; 
};

class AudioCodingModule: public Module {
 protected:
  AudioCodingModule() {}
  virtual ~AudioCodingModule() {}

 public:
  
  
  
  static AudioCodingModule* Create(const WebRtc_Word32 id);

  static void Destroy(AudioCodingModule* module);

  
  
  

  
  
  
  
  
  
  
  static WebRtc_UWord8 NumberOfCodecs();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static WebRtc_Word32 Codec(const WebRtc_UWord8 list_id, CodecInst& codec);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static WebRtc_Word32 Codec(const char* payload_name, CodecInst& codec,
                             int sampling_freq_hz, int channels);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static WebRtc_Word32 Codec(const char* payload_name, int sampling_freq_hz,
                             int channels);

  
  
  
  
  
  
  
  
  
  
  
  
  static bool IsCodecValid(const CodecInst& codec);

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 InitializeSender() = 0;

  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ResetEncoder() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterSendCodec(const CodecInst& send_codec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual int RegisterSecondarySendCodec(const CodecInst& send_codec) = 0;

  
  
  
  
  virtual void UnregisterSecondarySendCodec() = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SendCodec(CodecInst& current_send_codec) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual int SecondarySendCodec(CodecInst* secondary_codec) const = 0;

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SendFrequency() const = 0;

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SendBitrate() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetReceivedEstimatedBandwidth(
      const WebRtc_Word32 bw) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterTransportCallback(
      AudioPacketizationCallback* transport) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 Add10MsData(const AudioFrame& audio_frame) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetFECStatus(const bool enable_fec) = 0;

  
  
  
  
  
  
  
  
  virtual bool FECStatus() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetVAD(const bool enable_dtx = true,
                               const bool enable_vad = false,
                               const ACMVADMode vad_mode = VADNormal) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 VAD(bool& dtx_enabled, bool& vad_enabled,
                            ACMVADMode& vad_mode) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ReplaceInternalDTXWithWebRtc(
      const bool use_webrtc_dtx = false) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 IsInternalDTXReplacedWithWebRtc(
      bool& uses_webrtc_dtx) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterVADCallback(ACMVADCallback* vad_callback) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 InitializeReceiver() = 0;

  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ResetDecoder() = 0;

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ReceiveFrequency() const = 0;

  
  
  
  
  
  
  
  virtual WebRtc_Word32 PlayoutFrequency() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterReceiveCodec(
      const CodecInst& receive_codec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 UnregisterReceiveCodec(
      const WebRtc_Word16 payload_type) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ReceiveCodec(CodecInst& curr_receive_codec) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incoming_payload,
                                       const WebRtc_Word32 payload_len_bytes,
                                       const WebRtcRTPHeader& rtp_info) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 IncomingPayload(const WebRtc_UWord8* incoming_payload,
                                        const WebRtc_Word32 payload_len_byte,
                                        const WebRtc_UWord8 payload_type,
                                        const WebRtc_UWord32 timestamp = 0) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetMinimumPlayoutDelay(const WebRtc_Word32 time_ms) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32
      RegisterIncomingMessagesCallback(
          AudioCodingFeedback* in_message_callback,
          const ACMCountries cpt = ACMDisableCountryDetection) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetDtmfPlayoutStatus(const bool enable) = 0;

  
  
  
  
  
  
  
  
  virtual bool DtmfPlayoutStatus() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetBackgroundNoiseMode(
      const ACMBackgroundNoiseMode mode) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 BackgroundNoiseMode(ACMBackgroundNoiseMode& mode) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 PlayoutTimestamp(WebRtc_UWord32& timestamp) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 DecoderEstimatedBandwidth() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetPlayoutMode(const AudioPlayoutMode mode) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual AudioPlayoutMode PlayoutMode() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32
      PlayoutData10Ms(const WebRtc_Word32 desired_freq_hz,
                      AudioFrame &audio_frame) = 0;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 SetReceiveVADMode(const ACMVADMode mode) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual ACMVADMode ReceiveVADMode() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetISACMaxRate(
      const WebRtc_UWord32 max_rate_bps) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetISACMaxPayloadSize(
      const WebRtc_UWord16 max_payload_len_bytes) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ConfigISACBandwidthEstimator(
      const WebRtc_UWord8 init_frame_size_ms,
      const WebRtc_UWord16 init_rate_bps,
      const bool enforce_frame_size = false) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 NetworkStatistics(
      ACMNetworkStatistics& network_statistics) const = 0;
};

}  

#endif  
