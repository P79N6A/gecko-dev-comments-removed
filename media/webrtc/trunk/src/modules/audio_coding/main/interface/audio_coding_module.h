









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_INTERFACE_AUDIO_CODING_MODULE_H

#include "audio_coding_module_typedefs.h"
#include "module.h"
#include "module_common_types.h"

namespace webrtc {


struct CodecInst;

#define WEBRTC_10MS_PCM_AUDIO 960 // 16 bits super wideband 48 Khz


class AudioPacketizationCallback {
 public:
  virtual ~AudioPacketizationCallback() {}

  virtual WebRtc_Word32 SendData(
      FrameType frameType, WebRtc_UWord8 payloadType, WebRtc_UWord32 timeStamp,
      const WebRtc_UWord8* payloadData, WebRtc_UWord16 payloadSize,
      const RTPFragmentationHeader* fragmentation) = 0;
};


class AudioCodingFeedback {
 public:
  virtual ~AudioCodingFeedback() {}

  virtual WebRtc_Word32 IncomingDtmf(const WebRtc_UWord8 digitDtmf,
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static WebRtc_Word32 Codec(const WebRtc_UWord8 listId, CodecInst& codec);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static WebRtc_Word32 Codec(const char* payload_name, CodecInst& codec,
                             int sampling_freq_hz, int channels);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static WebRtc_Word32 Codec(const char* payload_name, int sampling_freq_hz,
                             int channels);

  
  
  
  
  
  
  
  
  
  
  
  
  static bool IsCodecValid(const CodecInst& codec);

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 InitializeSender() = 0;

  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ResetEncoder() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterSendCodec(const CodecInst& sendCodec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SendCodec(CodecInst& currentSendCodec) const = 0;

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SendFrequency() const = 0;

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SendBitrate() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetReceivedEstimatedBandwidth(
      const WebRtc_Word32 bw) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterTransportCallback(
      AudioPacketizationCallback* transport) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 Add10MsData(const AudioFrame& audioFrame) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetFECStatus(const bool enableFEC) = 0;

  
  
  
  
  
  
  
  
  virtual bool FECStatus() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetVAD(const bool enableDTX = true,
                               const bool enableVAD = false,
                               const ACMVADMode vadMode = VADNormal) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 VAD(bool& dtxEnabled, bool& vadEnabled,
                            ACMVADMode& vadMode) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ReplaceInternalDTXWithWebRtc(
      const bool useWebRtcDTX = false) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 IsInternalDTXReplacedWithWebRtc(
      bool& usesWebRtcDTX) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterVADCallback(ACMVADCallback* vadCallback) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 InitializeReceiver() = 0;

  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ResetDecoder() = 0;

  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ReceiveFrequency() const = 0;

  
  
  
  
  
  
  
  virtual WebRtc_Word32 PlayoutFrequency() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 RegisterReceiveCodec(const CodecInst& receiveCodec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 UnregisterReceiveCodec(
      const WebRtc_Word16 receiveCodec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ReceiveCodec(CodecInst& currRcvCodec) const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incomingPayload,
                                       const WebRtc_Word32 payloadLengthByte,
                                       const WebRtcRTPHeader& rtpInfo) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 IncomingPayload(const WebRtc_UWord8* incomingPayload,
                                        const WebRtc_Word32 payloadLengthByte,
                                        const WebRtc_UWord8 payloadType,
                                        const WebRtc_UWord32 timestamp = 0) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetMinimumPlayoutDelay(const WebRtc_Word32 timeMs) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32
      RegisterIncomingMessagesCallback(
          AudioCodingFeedback* inMsgCallback,
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
      PlayoutData10Ms(const WebRtc_Word32 desiredFreqHz,
                      AudioFrame &audioFrame) = 0;

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word16 SetReceiveVADMode(const ACMVADMode mode) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual ACMVADMode ReceiveVADMode() const = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetISACMaxRate(
      const WebRtc_UWord32 maxRateBitPerSec) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 SetISACMaxPayloadSize(
      const WebRtc_UWord16 maxPayloadLenBytes) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 ConfigISACBandwidthEstimator(
      const WebRtc_UWord8 initFrameSizeMsec,
      const WebRtc_UWord16 initRateBitPerSec,
      const bool enforceFrameSize = false) = 0;

  
  
  

  
  
  
  
  
  
  
  
  
  
  
  virtual WebRtc_Word32 NetworkStatistics(
      ACMNetworkStatistics& networkStatistics) const = 0;
};

}  

#endif  
