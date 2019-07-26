









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_AUDIO_CODING_MODULE_IMPL_H_

#include "acm_codec_database.h"
#include "acm_neteq.h"
#include "acm_resampler.h"
#include "common_types.h"
#include "engine_configurations.h"

namespace webrtc {

class ACMDTMFDetection;
class ACMGenericCodec;
class CriticalSectionWrapper;
class RWLockWrapper;

#ifdef ACM_QA_TEST
#   include <stdio.h>
#endif

class AudioCodingModuleImpl : public AudioCodingModule {
 public:
  
  AudioCodingModuleImpl(const WebRtc_Word32 id);

  
  ~AudioCodingModuleImpl();

  
  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

  
  
  WebRtc_Word32 TimeUntilNextProcess();

  
  WebRtc_Word32 Process();

  
  
  

  
  WebRtc_Word32 InitializeSender();

  
  WebRtc_Word32 ResetEncoder();

  
  WebRtc_Word32 RegisterSendCodec(const CodecInst& send_codec);

  
  WebRtc_Word32 SendCodec(CodecInst& current_codec) const;

  
  WebRtc_Word32 SendFrequency() const;

  
  
  
  WebRtc_Word32 SendBitrate() const;

  
  
  virtual WebRtc_Word32 SetReceivedEstimatedBandwidth(const WebRtc_Word32 bw);

  
  
  WebRtc_Word32 RegisterTransportCallback(
      AudioPacketizationCallback* transport);

  
  
  WebRtc_Word32 RegisterIncomingMessagesCallback(
      AudioCodingFeedback* incoming_message, const ACMCountries cpt);

  
  WebRtc_Word32 Add10MsData(const AudioFrame& audio_frame);

  
  WebRtc_Word32 SetBackgroundNoiseMode(const ACMBackgroundNoiseMode mode);

  
  WebRtc_Word32 BackgroundNoiseMode(ACMBackgroundNoiseMode& mode);

  
  
  

  
  WebRtc_Word32 SetFECStatus(const bool enable_fec);

  
  bool FECStatus() const;

  
  
  
  
  

  WebRtc_Word32 SetVAD(const bool enable_dtx = true,
                       const bool enable_vad = false,
                       const ACMVADMode mode = VADNormal);

  WebRtc_Word32 VAD(bool& dtx_enabled, bool& vad_enabled,
                    ACMVADMode& mode) const;

  WebRtc_Word32 RegisterVADCallback(ACMVADCallback* vadCallback);

  
  ACMVADMode ReceiveVADMode() const;

  
  WebRtc_Word16 SetReceiveVADMode(const ACMVADMode mode);

  
  
  

  
  WebRtc_Word32 InitializeReceiver();

  
  WebRtc_Word32 ResetDecoder();

  
  WebRtc_Word32 ReceiveFrequency() const;

  
  WebRtc_Word32 PlayoutFrequency() const;

  
  
  WebRtc_Word32 RegisterReceiveCodec(const CodecInst& receive_codec);

  
  WebRtc_Word32 ReceiveCodec(CodecInst& current_codec) const;

  
  WebRtc_Word32 IncomingPacket(const WebRtc_UWord8* incoming_payload,
                               const WebRtc_Word32 payload_length,
                               const WebRtcRTPHeader& rtp_info);

  
  
  WebRtc_Word32 IncomingPayload(const WebRtc_UWord8* incoming_payload,
                                const WebRtc_Word32 payload_length,
                                const WebRtc_UWord8 payload_type,
                                const WebRtc_UWord32 timestamp = 0);

  
  WebRtc_Word32 SetMinimumPlayoutDelay(const WebRtc_Word32 time_ms);

  
  
  WebRtc_Word32 SetDtmfPlayoutStatus(const bool enable);

  
  bool DtmfPlayoutStatus() const;

  
  
  
  WebRtc_Word32 DecoderEstimatedBandwidth() const;

  
  WebRtc_Word32 SetPlayoutMode(const AudioPlayoutMode mode);

  
  AudioPlayoutMode PlayoutMode() const;

  
  WebRtc_Word32 PlayoutTimestamp(WebRtc_UWord32& timestamp);

  
  
  WebRtc_Word32 PlayoutData10Ms(const WebRtc_Word32 desired_freq_hz,
                                AudioFrame &audio_frame);

  
  
  

  WebRtc_Word32 NetworkStatistics(ACMNetworkStatistics& statistics) const;

  void DestructEncoderInst(void* inst);

  WebRtc_Word16 AudioBuffer(WebRtcACMAudioBuff& buffer);

  
  
  WebRtc_Word32 REDPayloadISAC(const WebRtc_Word32 isac_rate,
                               const WebRtc_Word16 isac_bw_estimate,
                               WebRtc_UWord8* payload,
                               WebRtc_Word16* length_bytes);

  WebRtc_Word16 SetAudioBuffer(WebRtcACMAudioBuff& buffer);

  WebRtc_UWord32 EarliestTimestamp() const;

  WebRtc_Word32 LastEncodedTimestamp(WebRtc_UWord32& timestamp) const;

  WebRtc_Word32 ReplaceInternalDTXWithWebRtc(const bool use_webrtc_dtx);

  WebRtc_Word32 IsInternalDTXReplacedWithWebRtc(bool& uses_webrtc_dtx);

  WebRtc_Word32 SetISACMaxRate(const WebRtc_UWord32 max_bit_per_sec);

  WebRtc_Word32 SetISACMaxPayloadSize(const WebRtc_UWord16 max_size_bytes);

  WebRtc_Word32 ConfigISACBandwidthEstimator(
      const WebRtc_UWord8 frame_size_ms,
      const WebRtc_UWord16 rate_bit_per_sec,
      const bool enforce_frame_size = false);

  WebRtc_Word32 UnregisterReceiveCodec(const WebRtc_Word16 payload_type);

 protected:
  void UnregisterSendCodec();

  WebRtc_Word32 UnregisterReceiveCodecSafe(const WebRtc_Word16 id);

  ACMGenericCodec* CreateCodec(const CodecInst& codec);

  WebRtc_Word16 DecoderParamByPlType(const WebRtc_UWord8 payload_type,
                                     WebRtcACMCodecParams& codec_params) const;

  WebRtc_Word16 DecoderListIDByPlName(
      const char* name, const WebRtc_UWord16 frequency = 0) const;

  WebRtc_Word32 InitializeReceiverSafe();

  bool HaveValidEncoder(const char* caller_name) const;

  WebRtc_Word32 RegisterRecCodecMSSafe(const CodecInst& receive_codec,
                                       WebRtc_Word16 codec_id,
                                       WebRtc_Word16 mirror_id,
                                       ACMNetEQ::JB jitter_buffer);

 private:
  
  
  int UpdateUponReceivingCodec(int index);

  
  
  int InitStereoSlave();

  
  
  bool IsCodecForSlave(int index) const;

  
  bool IsCodecRED(const CodecInst* codec) const;
  
  bool IsCodecRED(int index) const;

  
  bool IsCodecCN(int index) const;
  
  bool IsCodecCN(const CodecInst* codec) const;

  AudioPacketizationCallback* _packetizationCallback;
  WebRtc_Word32 _id;
  WebRtc_UWord32 _lastTimestamp;
  WebRtc_UWord32 _lastInTimestamp;
  CodecInst _sendCodecInst;
  uint8_t _cng_nb_pltype;
  uint8_t _cng_wb_pltype;
  uint8_t _cng_swb_pltype;
  uint8_t _red_pltype;
  bool _vadEnabled;
  bool _dtxEnabled;
  ACMVADMode _vadMode;
  ACMGenericCodec* _codecs[ACMCodecDB::kMaxNumCodecs];
  ACMGenericCodec* _slaveCodecs[ACMCodecDB::kMaxNumCodecs];
  WebRtc_Word16 _mirrorCodecIdx[ACMCodecDB::kMaxNumCodecs];
  bool _stereoReceive[ACMCodecDB::kMaxNumCodecs];
  bool _stereoReceiveRegistered;
  bool _stereoSend;
  int _prev_received_channel;
  int _expected_channels;
  WebRtc_Word32 _currentSendCodecIdx;
  int _current_receive_codec_idx;
  bool _sendCodecRegistered;
  ACMResampler _inputResampler;
  ACMResampler _outputResampler;
  ACMNetEQ _netEq;
  CriticalSectionWrapper* _acmCritSect;
  ACMVADCallback* _vadCallback;
  WebRtc_UWord8 _lastRecvAudioCodecPlType;

  
  bool _isFirstRED;
  bool _fecEnabled;
  WebRtc_UWord8* _redBuffer;
  RTPFragmentationHeader* _fragmentation;
  WebRtc_UWord32 _lastFECTimestamp;
  
  
  WebRtc_UWord8 _receiveREDPayloadType;

  
  WebRtc_UWord8 _previousPayloadType;

  
  
  
  WebRtc_Word16 _registeredPlTypes[ACMCodecDB::kMaxNumCodecs];

  
  
  
  WebRtcRTPHeader* _dummyRTPHeader;
  WebRtc_UWord16 _recvPlFrameSizeSmpls;

  bool _receiverInitialized;
  ACMDTMFDetection* _dtmfDetector;

  AudioCodingFeedback* _dtmfCallback;
  WebRtc_Word16 _lastDetectedTone;
  CriticalSectionWrapper* _callbackCritSect;

  AudioFrame _audioFrame;

#ifdef ACM_QA_TEST
  FILE* _outgoingPL;
  FILE* _incomingPL;
#endif

};

}  

#endif  
