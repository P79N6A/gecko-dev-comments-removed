











#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/neteq/buffer_level_filter.h"
#include "webrtc/modules/audio_coding/neteq/decoder_database.h"
#include "webrtc/modules/audio_coding/neteq/decision_logic.h"
#include "webrtc/modules/audio_coding/neteq/delay_manager.h"
#include "webrtc/modules/audio_coding/neteq/delay_peak_detector.h"
#include "webrtc/modules/audio_coding/neteq/packet_buffer.h"

namespace webrtc {

TEST(DecisionLogic, CreateAndDestroy) {
  int fs_hz = 8000;
  int output_size_samples = fs_hz / 100;  
  DecoderDatabase decoder_database;
  PacketBuffer packet_buffer(10);
  DelayPeakDetector delay_peak_detector;
  DelayManager delay_manager(240, &delay_peak_detector);
  BufferLevelFilter buffer_level_filter;
  DecisionLogic* logic = DecisionLogic::Create(fs_hz, output_size_samples,
                                               kPlayoutOn, &decoder_database,
                                               packet_buffer, &delay_manager,
                                               &buffer_level_filter);
  delete logic;
  logic = DecisionLogic::Create(fs_hz, output_size_samples,
                                kPlayoutStreaming,
                                &decoder_database,
                                packet_buffer, &delay_manager,
                                &buffer_level_filter);
  delete logic;
  logic = DecisionLogic::Create(fs_hz, output_size_samples,
                                kPlayoutFax,
                                &decoder_database,
                                packet_buffer, &delay_manager,
                                &buffer_level_filter);
  delete logic;
  logic = DecisionLogic::Create(fs_hz, output_size_samples,
                                kPlayoutOff,
                                &decoder_database,
                                packet_buffer, &delay_manager,
                                &buffer_level_filter);
  delete logic;
}



}  
