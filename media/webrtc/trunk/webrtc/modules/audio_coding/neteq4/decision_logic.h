









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DECISION_LOGIC_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DECISION_LOGIC_H_

#include "webrtc/modules/audio_coding/neteq4/defines.h"
#include "webrtc/modules/audio_coding/neteq4/interface/neteq.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class BufferLevelFilter;
class DecoderDatabase;
class DelayManager;
class Expand;
class PacketBuffer;
class SyncBuffer;
struct RTPHeader;



class DecisionLogic {
 public:
  
  
  static DecisionLogic* Create(int fs_hz,
                               int output_size_samples,
                               NetEqPlayoutMode playout_mode,
                               DecoderDatabase* decoder_database,
                               const PacketBuffer& packet_buffer,
                               DelayManager* delay_manager,
                               BufferLevelFilter* buffer_level_filter);

  
  DecisionLogic(int fs_hz,
                int output_size_samples,
                NetEqPlayoutMode playout_mode,
                DecoderDatabase* decoder_database,
                const PacketBuffer& packet_buffer,
                DelayManager* delay_manager,
                BufferLevelFilter* buffer_level_filter);

  
  virtual ~DecisionLogic() {}

  
  void Reset();

  
  void SoftReset();

  
  void SetSampleRate(int fs_hz, int output_size_samples);

  
  
  
  
  
  
  
  
  
  
  
  Operations GetDecision(const SyncBuffer& sync_buffer,
                         const Expand& expand,
                         int decoder_frame_length,
                         const RTPHeader* packet_header,
                         Modes prev_mode,
                         bool play_dtmf,
                         bool* reset_decoder);

  
  bool CngRfc3389On() const { return cng_state_ == kCngRfc3389On; }
  bool CngOff() const { return cng_state_ == kCngOff; }

  
  void SetCngOff() { cng_state_ = kCngOff; }

  
  
  
  
  void ExpandDecision(bool is_expand_decision);

  
  void AddSampleMemory(int32_t value) {
    sample_memory_ += value;
  }

  
  void set_sample_memory(int32_t value) { sample_memory_ = value; }
  int generated_noise_samples() const { return generated_noise_samples_; }
  void set_generated_noise_samples(int value) {
    generated_noise_samples_ = value;
  }
  int packet_length_samples() const { return packet_length_samples_; }
  void set_packet_length_samples(int value) {
    packet_length_samples_ = value;
  }
  void set_prev_time_scale(bool value) { prev_time_scale_ = value; }
  NetEqPlayoutMode playout_mode() const { return playout_mode_; }

 protected:
  
  static const int kMinTimescaleInterval = 6;

  enum CngState {
    kCngOff,
    kCngRfc3389On,
    kCngInternalOn
  };

  
  
  
  
  
  
  
  
  
  
  virtual Operations GetDecisionSpecialized(const SyncBuffer& sync_buffer,
                                            const Expand& expand,
                                            int decoder_frame_length,
                                            const RTPHeader* packet_header,
                                            Modes prev_mode,
                                            bool play_dtmf,
                                            bool* reset_decoder) = 0;

  
  
  void FilterBufferLevel(int buffer_size_packets, Modes prev_mode);

  DecoderDatabase* decoder_database_;
  const PacketBuffer& packet_buffer_;
  DelayManager* delay_manager_;
  BufferLevelFilter* buffer_level_filter_;
  int fs_mult_;
  int output_size_samples_;
  CngState cng_state_;  
                        
  int generated_noise_samples_;
  int packet_length_samples_;
  int sample_memory_;
  bool prev_time_scale_;
  int timescale_hold_off_;
  int num_consecutive_expands_;
  const NetEqPlayoutMode playout_mode_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DecisionLogic);
};

}  
#endif  
