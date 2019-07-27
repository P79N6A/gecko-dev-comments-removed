









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_INTERFACE_NETEQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_INTERFACE_NETEQ_H_

#include <string.h>  

#include <vector>

#include "webrtc/base/constructormagic.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/audio_coding/neteq/interface/audio_decoder.h"
#include "webrtc/typedefs.h"

namespace webrtc {


struct WebRtcRTPHeader;

struct NetEqNetworkStatistics {
  uint16_t current_buffer_size_ms;  
  uint16_t preferred_buffer_size_ms;  
  uint16_t jitter_peaks_found;  
                                
  uint16_t packet_loss_rate;  
  uint16_t packet_discard_rate;  
  uint16_t expand_rate;  
                         
  uint16_t preemptive_rate;  
                             
  uint16_t accelerate_rate;  
                             
  int32_t clockdrift_ppm;  
                           
  int added_zero_samples;  
};

enum NetEqOutputType {
  kOutputNormal,
  kOutputPLC,
  kOutputCNG,
  kOutputPLCtoCNG,
  kOutputVADPassive
};

enum NetEqPlayoutMode {
  kPlayoutOn,
  kPlayoutOff,
  kPlayoutFax,
  kPlayoutStreaming
};


class NetEq {
 public:
  enum BackgroundNoiseMode {
    kBgnOn,    
    kBgnFade,  
    kBgnOff    
  };

  struct Config {
    Config()
        : sample_rate_hz(16000),
          enable_audio_classifier(false),
          max_packets_in_buffer(50),
          
          max_delay_ms(2000),
          background_noise_mode(kBgnOff),
          playout_mode(kPlayoutOn) {}

    int sample_rate_hz;  
    bool enable_audio_classifier;
    int max_packets_in_buffer;
    int max_delay_ms;
    BackgroundNoiseMode background_noise_mode;
    NetEqPlayoutMode playout_mode;
  };

  enum ReturnCodes {
    kOK = 0,
    kFail = -1,
    kNotImplemented = -2
  };

  enum ErrorCodes {
    kNoError = 0,
    kOtherError,
    kInvalidRtpPayloadType,
    kUnknownRtpPayloadType,
    kCodecNotSupported,
    kDecoderExists,
    kDecoderNotFound,
    kInvalidSampleRate,
    kInvalidPointer,
    kAccelerateError,
    kPreemptiveExpandError,
    kComfortNoiseErrorCode,
    kDecoderErrorCode,
    kOtherDecoderError,
    kInvalidOperation,
    kDtmfParameterError,
    kDtmfParsingError,
    kDtmfInsertError,
    kStereoNotSupported,
    kSampleUnderrun,
    kDecodedTooMuch,
    kFrameSplitError,
    kRedundancySplitError,
    kPacketBufferCorruption,
    kSyncPacketNotAccepted
  };

  
  
  
  static NetEq* Create(const NetEq::Config& config);

  virtual ~NetEq() {}

  
  
  
  
  virtual int InsertPacket(const WebRtcRTPHeader& rtp_header,
                           const uint8_t* payload,
                           int length_bytes,
                           uint32_t receive_timestamp) = 0;

  
  
  
  
  
  
  
  
  
  virtual int InsertSyncPacket(const WebRtcRTPHeader& rtp_header,
                               uint32_t receive_timestamp) = 0;

  
  
  
  
  
  
  
  
  virtual int GetAudio(size_t max_length, int16_t* output_audio,
                       int* samples_per_channel, int* num_channels,
                       NetEqOutputType* type) = 0;

  
  
  virtual int RegisterPayloadType(enum NetEqDecoder codec,
                                  uint8_t rtp_payload_type) = 0;

  
  
  
  
  virtual int RegisterExternalDecoder(AudioDecoder* decoder,
                                      enum NetEqDecoder codec,
                                      uint8_t rtp_payload_type) = 0;

  
  
  virtual int RemovePayloadType(uint8_t rtp_payload_type) = 0;

  
  
  
  
  virtual bool SetMinimumDelay(int delay_ms) = 0;

  
  
  
  
  virtual bool SetMaximumDelay(int delay_ms) = 0;

  
  
  
  
  virtual int LeastRequiredDelayMs() const = 0;

  
  virtual int SetTargetDelay() = 0;

  
  virtual int TargetDelay() = 0;

  
  virtual int CurrentDelay() = 0;

  
  
  
  virtual void SetPlayoutMode(NetEqPlayoutMode mode) = 0;

  
  
  
  virtual NetEqPlayoutMode PlayoutMode() const = 0;

  
  
  virtual int NetworkStatistics(NetEqNetworkStatistics* stats) = 0;

  
  
  
  virtual void WaitingTimes(std::vector<int>* waiting_times) = 0;

  
  
  virtual void GetRtcpStatistics(RtcpStatistics* stats) = 0;

  
  virtual void GetRtcpStatisticsNoReset(RtcpStatistics* stats) = 0;

  
  
  virtual void EnableVad() = 0;

  
  virtual void DisableVad() = 0;

  
  
  virtual bool GetPlayoutTimestamp(uint32_t* timestamp) = 0;

  
  virtual int SetTargetNumberOfChannels() = 0;

  
  virtual int SetTargetSampleRate() = 0;

  
  
  virtual int LastError() const = 0;

  
  
  
  virtual int LastDecoderError() = 0;

  
  virtual void FlushBuffers() = 0;

  
  virtual void PacketBufferStatistics(int* current_num_packets,
                                      int* max_num_packets) const = 0;

  
  
  virtual int DecodedRtpInfo(int* sequence_number,
                             uint32_t* timestamp) const = 0;

 protected:
  NetEq() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(NetEq);
};

}  
#endif
