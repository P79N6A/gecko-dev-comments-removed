









#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"

class CallReportTest : public AfterStreamingFixture {
};

TEST_F(CallReportTest, ResetCallReportStatisticsFailsForBadInput) {
  EXPECT_EQ(-1, voe_call_report_->ResetCallReportStatistics(-2));
  EXPECT_EQ(-1, voe_call_report_->ResetCallReportStatistics(1));
}

TEST_F(CallReportTest, ResetCallReportStatisticsSucceedsWithCorrectInput) {
  EXPECT_EQ(0, voe_call_report_->ResetCallReportStatistics(channel_));
  EXPECT_EQ(0, voe_call_report_->ResetCallReportStatistics(-1));
}

TEST_F(CallReportTest, EchoMetricSummarySucceeds) {
  EXPECT_EQ(0, voe_apm_->SetEcMetricsStatus(true));
  Sleep(1000);

  webrtc::EchoStatistics echo_statistics;
  EXPECT_EQ(0, voe_call_report_->GetEchoMetricSummary(echo_statistics));
}

TEST_F(CallReportTest, GetRoundTripTimeSummaryReturnsAllMinusOnesIfRtcpIsOff) {
  voe_rtp_rtcp_->SetRTCPStatus(channel_, false);

  webrtc::StatVal delays;
  EXPECT_EQ(0, voe_call_report_->GetRoundTripTimeSummary(channel_, delays));
  EXPECT_EQ(-1, delays.average);
  EXPECT_EQ(-1, delays.min);
  EXPECT_EQ(-1, delays.max);
}


TEST_F(CallReportTest, DISABLED_GetRoundTripTimesReturnsValuesIfRtcpIsOn) {
  voe_rtp_rtcp_->SetRTCPStatus(channel_, true);
  Sleep(1000);

  webrtc::StatVal delays;
  EXPECT_EQ(0, voe_call_report_->GetRoundTripTimeSummary(channel_, delays));
  EXPECT_NE(-1, delays.average);
  EXPECT_NE(-1, delays.min);
  EXPECT_NE(-1, delays.max);
}

TEST_F(CallReportTest, WriteReportToFileFailsOnBadInput) {
  EXPECT_EQ(-1, voe_call_report_->WriteReportToFile(NULL));
}

TEST_F(CallReportTest, WriteReportToFileSucceedsWithCorrectFilename) {
  std::string output_path = webrtc::test::OutputPath();
  std::string report_filename = output_path + "call_report.txt";

  EXPECT_EQ(0, voe_call_report_->WriteReportToFile(report_filename.c_str()));
}
