









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_DECISION_LOGIC_NORMAL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_DECISION_LOGIC_NORMAL_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/audio_coding/neteq/decision_logic.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class DecisionLogicNormal : public DecisionLogic {
 public:
  
  DecisionLogicNormal(int fs_hz,
                      int output_size_samples,
                      NetEqPlayoutMode playout_mode,
                      DecoderDatabase* decoder_database,
                      const PacketBuffer& packet_buffer,
                      DelayManager* delay_manager,
                      BufferLevelFilter* buffer_level_filter)
      : DecisionLogic(fs_hz, output_size_samples, playout_mode,
                      decoder_database, packet_buffer, delay_manager,
                      buffer_level_filter) {
  }

  
  virtual ~DecisionLogicNormal() {}

 protected:
  static const int kAllowMergeWithoutExpandMs = 20;  
  static const int kReinitAfterExpands = 100;
  static const int kMaxWaitForPacket = 10;

  
  
  
  
  
  
  
  
  
  virtual Operations GetDecisionSpecialized(const SyncBuffer& sync_buffer,
                                            const Expand& expand,
                                            int decoder_frame_length,
                                            const RTPHeader* packet_header,
                                            Modes prev_mode, bool play_dtmf,
                                            bool* reset_decoder);

  
  
  virtual Operations FuturePacketAvailable(
      const SyncBuffer& sync_buffer,
      const Expand& expand,
      int decoder_frame_length, Modes prev_mode,
      uint32_t target_timestamp,
      uint32_t available_timestamp,
      bool play_dtmf);

  
  virtual Operations ExpectedPacketAvailable(Modes prev_mode, bool play_dtmf);

  
  
  virtual Operations NoPacket(bool play_dtmf);

 private:
  
  
  Operations CngOperation(Modes prev_mode, uint32_t target_timestamp,
                          uint32_t available_timestamp);

  
  
  bool TimescaleAllowed() const { return timescale_hold_off_ == 0; }

  
  bool UnderTargetLevel() const;

  
  
  bool ReinitAfterExpands(uint32_t timestamp_leap) const;

  
  
  
  bool PacketTooEarly(uint32_t timestamp_leap) const;

  
  bool MaxWaitForPacket() const;

  DISALLOW_COPY_AND_ASSIGN(DecisionLogicNormal);
};

}  
#endif  
