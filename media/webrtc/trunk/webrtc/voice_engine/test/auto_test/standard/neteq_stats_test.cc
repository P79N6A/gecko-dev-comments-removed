









#include "voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"

class NetEQStatsTest : public AfterStreamingFixture {
};

TEST_F(NetEQStatsTest, ManualPrintStatisticsAfterRunningAWhile) {
  Sleep(5000);

  webrtc::NetworkStatistics network_statistics;

  EXPECT_EQ(0, voe_neteq_stats_->GetNetworkStatistics(
      channel_, network_statistics));

  TEST_LOG("Inspect these statistics and ensure they make sense.\n");

  TEST_LOG("    currentAccelerateRate     = %hu \n",
      network_statistics.currentAccelerateRate);
  TEST_LOG("    currentBufferSize         = %hu \n",
      network_statistics.currentBufferSize);
  TEST_LOG("    currentDiscardRate        = %hu \n",
      network_statistics.currentDiscardRate);
  TEST_LOG("    currentExpandRate         = %hu \n",
      network_statistics.currentExpandRate);
  TEST_LOG("    currentPacketLossRate     = %hu \n",
      network_statistics.currentPacketLossRate);
  TEST_LOG("    currentPreemptiveRate     = %hu \n",
      network_statistics.currentPreemptiveRate);
  TEST_LOG("    preferredBufferSize       = %hu \n",
      network_statistics.preferredBufferSize);
  TEST_LOG("    jitterPeaksFound          = %i \n",
      network_statistics.jitterPeaksFound);
  TEST_LOG("    clockDriftPPM             = %i \n",
      network_statistics.clockDriftPPM);
  TEST_LOG("    meanWaitingTimeMs         = %i \n",
      network_statistics.meanWaitingTimeMs);
  TEST_LOG("    medianWaitingTimeMs       = %i \n",
      network_statistics.medianWaitingTimeMs);
  TEST_LOG("    minWaitingTimeMs          = %i \n",
      network_statistics.minWaitingTimeMs);
  TEST_LOG("    maxWaitingTimeMs          = %i \n",
      network_statistics.maxWaitingTimeMs);

  
  EXPECT_EQ(0, network_statistics.addedSamples);
}
