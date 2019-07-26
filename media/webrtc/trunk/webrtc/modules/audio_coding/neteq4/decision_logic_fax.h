









#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DECISION_LOGIC_FAX_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ4_DECISION_LOGIC_FAX_H_

#include "webrtc/modules/audio_coding/neteq4/decision_logic.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class DecisionLogicFax : public DecisionLogic {
 public:
  
  DecisionLogicFax(int fs_hz,
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

  
  virtual ~DecisionLogicFax() {}

 protected:
  
  
  
  
  
  
  
  
  
  virtual Operations GetDecisionSpecialized(const SyncBuffer& sync_buffer,
                                            const Expand& expand,
                                            int decoder_frame_length,
                                            const RTPHeader* packet_header,
                                            Modes prev_mode,
                                            bool play_dtmf,
                                            bool* reset_decoder) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(DecisionLogicFax);
};

}  
#endif  
