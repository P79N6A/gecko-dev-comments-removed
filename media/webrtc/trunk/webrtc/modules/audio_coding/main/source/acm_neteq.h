









#ifndef WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_NETEQ_H_
#define WEBRTC_MODULES_AUDIO_CODING_MAIN_SOURCE_ACM_NETEQ_H_

#include "webrtc/common_audio/vad/include/webrtc_vad.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/audio_coding/neteq/interface/webrtc_neteq.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class RWLockWrapper;
struct CodecInst;

#define MAX_NUM_SLAVE_NETEQ 1

class ACMNetEQ {
 public:
  enum JitterBuffer {
    kMasterJb = 0,
    kSlaveJb = 1
  };

  
  ACMNetEQ();

  
  ~ACMNetEQ();

  
  
  
  
  
  
  
  
  WebRtc_Word32 Init();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 RecIn(const WebRtc_UWord8* incoming_payload,
                      const WebRtc_Word32 length_payload,
                      const WebRtcRTPHeader& rtp_info);

  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 RecOut(AudioFrame& audio_frame);

  
  
  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 AddCodec(WebRtcNetEQ_CodecDef *codec_def,
                         bool to_master = true);

  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 AllocatePacketBuffer(const WebRtcNetEQDecoder* used_codecs,
                                     WebRtc_Word16 num_codecs);

  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 SetExtraDelay(const WebRtc_Word32 delay_in_ms);

  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 SetAVTPlayout(const bool enable);

  
  
  
  
  
  
  
  bool avt_playout() const;

  
  
  
  
  
  
  WebRtc_Word32 CurrentSampFreqHz() const;

  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 SetPlayoutMode(const AudioPlayoutMode mode);

  
  
  
  
  
  
  AudioPlayoutMode playout_mode() const;

  
  
  
  
  
  
  
  
  
  
  WebRtc_Word32 NetworkStatistics(ACMNetworkStatistics* statistics) const;

  
  
  
  
  
  
  ACMVADMode vad_mode() const;

  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 SetVADMode(const ACMVADMode mode);

  
  
  
  
  
  
  RWLockWrapper* DecodeLock() const {
    return decode_lock_;
  }

  
  
  
  
  
  
  
  WebRtc_Word32 FlushBuffers();

  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 RemoveCodec(WebRtcNetEQDecoder codec_idx,
                            bool is_stereo = false);

  
  
  
  
  
  
  
  
  
  
  
  WebRtc_Word16 SetBackgroundNoiseMode(const ACMBackgroundNoiseMode mode);

  
  
  
  
  
  
  WebRtc_Word16 BackgroundNoiseMode(ACMBackgroundNoiseMode& mode);

  void set_id(WebRtc_Word32 id);

  WebRtc_Word32 PlayoutTimestamp(WebRtc_UWord32& timestamp);

  void set_received_stereo(bool received_stereo);

  WebRtc_UWord8 num_slaves();

  
  void RemoveSlaves();

  WebRtc_Word16 AddSlave(const WebRtcNetEQDecoder* used_codecs,
                         WebRtc_Word16 num_codecs);

 private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void RTPPack(WebRtc_Word16* rtp_packet, const WebRtc_Word8* payload,
                      const WebRtc_Word32 payload_length_bytes,
                      const WebRtcRTPHeader& rtp_info);

  void LogError(const char* neteq_func_name, const WebRtc_Word16 idx) const;

  WebRtc_Word16 InitByIdxSafe(const WebRtc_Word16 idx);

  
  
  
  
  
  
  
  WebRtc_Word16 EnableVAD();

  WebRtc_Word16 EnableVADByIdxSafe(const WebRtc_Word16 idx);

  WebRtc_Word16 AllocatePacketBufferByIdxSafe(
      const WebRtcNetEQDecoder* used_codecs,
      WebRtc_Word16 num_codecs,
      const WebRtc_Word16 idx);

  
  void RemoveNetEQSafe(int index);

  void RemoveSlavesSafe();

  void* inst_[MAX_NUM_SLAVE_NETEQ + 1];
  void* inst_mem_[MAX_NUM_SLAVE_NETEQ + 1];

  WebRtc_Word16* neteq_packet_buffer_[MAX_NUM_SLAVE_NETEQ + 1];

  WebRtc_Word32 id_;
  float current_samp_freq_khz_;
  bool avt_playout_;
  AudioPlayoutMode playout_mode_;
  CriticalSectionWrapper* neteq_crit_sect_;

  WebRtcVadInst* ptr_vadinst_[MAX_NUM_SLAVE_NETEQ + 1];

  bool vad_status_;
  ACMVADMode vad_mode_;
  RWLockWrapper* decode_lock_;
  bool is_initialized_[MAX_NUM_SLAVE_NETEQ + 1];
  WebRtc_UWord8 num_slaves_;
  bool received_stereo_;
  void* master_slave_info_;
  AudioFrame::VADActivity previous_audio_activity_;
  WebRtc_Word32 extra_delay_;

  CriticalSectionWrapper* callback_crit_sect_;
};

}  

#endif
