









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_NETEQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_NETEQ_H_

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class RWLockWrapper;
struct CodecInst;

namespace acm1 {

#define MAX_NUM_SLAVE_NETEQ 1

class ACMNetEQ {
 public:
  enum JitterBuffer {
    kMasterJb = 0,
    kSlaveJb = 1
  };

  
  ACMNetEQ();

  
  ~ACMNetEQ();

  
  
  
  
  
  
  
  
  int32_t Init();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t RecIn(const uint8_t* incoming_payload,
                const int32_t length_payload,
                const WebRtcRTPHeader& rtp_info,
                uint32_t receive_timestamp);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int RecIn(const WebRtcRTPHeader& rtp_info, uint32_t receive_timestamp);

  
  
  
  
  
  
  
  
  
  
  
  int32_t RecOut(AudioFrame& audio_frame);

  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t AddCodec(WebRtcNetEQ_CodecDef *codec_def,
                   bool to_master = true);

  
  
  
  
  
  
  
  
  
  
  
  int32_t AllocatePacketBuffer(const WebRtcNetEQDecoder* used_codecs,
                               int16_t num_codecs);

  
  
  
  
  
  
  
  
  
  
  int32_t SetAVTPlayout(const bool enable);

  
  
  
  
  
  
  
  bool avt_playout() const;

  
  
  
  
  
  
  int32_t CurrentSampFreqHz() const;

  
  
  
  
  
  
  
  
  
  
  
  int32_t SetPlayoutMode(const AudioPlayoutMode mode);

  
  
  
  
  
  
  AudioPlayoutMode playout_mode() const;

  
  
  
  
  
  
  
  
  
  
  int32_t NetworkStatistics(ACMNetworkStatistics* statistics) const;

  
  
  
  
  
  
  ACMVADMode vad_mode() const;

  
  
  
  
  
  
  
  
  
  
  int16_t SetVADMode(const ACMVADMode mode);

  
  
  
  
  
  
  RWLockWrapper* DecodeLock() const {
    return decode_lock_;
  }

  
  
  
  
  
  
  
  int32_t FlushBuffers();

  
  
  
  
  
  
  
  
  
  
  int16_t RemoveCodec(WebRtcNetEQDecoder codec_idx,
                      bool is_stereo = false);

  
  
  
  
  
  
  
  
  
  
  
  int16_t SetBackgroundNoiseMode(const ACMBackgroundNoiseMode mode);

  
  
  
  
  
  
  int16_t BackgroundNoiseMode(ACMBackgroundNoiseMode& mode);

  void set_id(int32_t id);

  int32_t PlayoutTimestamp(uint32_t& timestamp);

  void set_received_stereo(bool received_stereo);

  uint8_t num_slaves();

  
  void RemoveSlaves();

  int16_t AddSlave(const WebRtcNetEQDecoder* used_codecs,
                   int16_t num_codecs);

  void BufferSpec(int& num_packets, int& size_bytes, int& overhead_bytes) {
    num_packets = min_of_max_num_packets_;
    size_bytes = min_of_buffer_size_bytes_;
    overhead_bytes = per_packet_overhead_bytes_;
  }

  
  
  
  void EnableAVSync(bool enable);

  
  
  
  bool DecodedRtpInfo(int* sequence_number, uint32_t* timestamp) const;

  
  
  
  
  int SetMinimumDelay(int minimum_delay_ms);

  
  
  
  int SetMaximumDelay(int maximum_delay_ms);

  
  
  
  
  
  
  int LeastRequiredDelayMs() const ;

 private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void RTPPack(int16_t* rtp_packet, const int8_t* payload,
                      const int32_t payload_length_bytes,
                      const WebRtcRTPHeader& rtp_info);

  void LogError(const char* neteq_func_name, const int16_t idx) const;

  int16_t InitByIdxSafe(const int16_t idx);

  
  
  
  
  
  
  
  int16_t EnableVAD();

  int16_t EnableVADByIdxSafe(const int16_t idx);

  int16_t AllocatePacketBufferByIdxSafe(
      const WebRtcNetEQDecoder* used_codecs,
      int16_t num_codecs,
      const int16_t idx);

  
  void RemoveNetEQSafe(int index);

  void RemoveSlavesSafe();

  void* inst_[MAX_NUM_SLAVE_NETEQ + 1];
  void* inst_mem_[MAX_NUM_SLAVE_NETEQ + 1];

  int16_t* neteq_packet_buffer_[MAX_NUM_SLAVE_NETEQ + 1];

  int32_t id_;
  float current_samp_freq_khz_;
  bool avt_playout_;
  AudioPlayoutMode playout_mode_;
  CriticalSectionWrapper* neteq_crit_sect_;

  WebRtcVadInst* ptr_vadinst_[MAX_NUM_SLAVE_NETEQ + 1];

  bool vad_status_;
  ACMVADMode vad_mode_;
  RWLockWrapper* decode_lock_;
  bool is_initialized_[MAX_NUM_SLAVE_NETEQ + 1];
  uint8_t num_slaves_;
  bool received_stereo_;
  void* master_slave_info_;
  AudioFrame::VADActivity previous_audio_activity_;

  CriticalSectionWrapper* callback_crit_sect_;
  
  int min_of_max_num_packets_;
  
  int min_of_buffer_size_bytes_;
  int per_packet_overhead_bytes_;

  
  bool av_sync_;

  int minimum_delay_ms_;
  int maximum_delay_ms_;
};

}  

}  

#endif
