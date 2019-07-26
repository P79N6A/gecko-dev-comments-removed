









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_UNITTEST_HELPER_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_UNITTEST_HELPER_H_

#include <list>
#include <map>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {
namespace testing {

class TestBitrateObserver : public RemoteBitrateObserver {
 public:
  TestBitrateObserver() : updated_(false), latest_bitrate_(0) {}
  virtual ~TestBitrateObserver() {}

  virtual void OnReceiveBitrateChanged(const std::vector<unsigned int>& ssrcs,
                                       unsigned int bitrate) OVERRIDE;

  void Reset() { updated_ = false; }

  bool updated() const { return updated_; }

  unsigned int latest_bitrate() const { return latest_bitrate_; }

 private:
  bool updated_;
  unsigned int latest_bitrate_;
};

class RtpStream {
 public:
  struct RtpPacket {
    int64_t send_time;
    int64_t arrival_time;
    uint32_t rtp_timestamp;
    unsigned int size;
    unsigned int ssrc;
  };

  struct RtcpPacket {
    uint32_t ntp_secs;
    uint32_t ntp_frac;
    uint32_t timestamp;
    unsigned int ssrc;
  };

  typedef std::list<RtpPacket*> PacketList;

  enum { kSendSideOffsetUs = 1000000 };

  RtpStream(int fps, int bitrate_bps, unsigned int ssrc, unsigned int frequency,
      uint32_t timestamp_offset, int64_t rtcp_receive_time);
  void set_rtp_timestamp_offset(uint32_t offset);

  
  
  
  int64_t GenerateFrame(int64_t time_now_us, PacketList* packets);

  
  double next_rtp_time() const;

  
  RtcpPacket* Rtcp(int64_t time_now_us);

  void set_bitrate_bps(int bitrate_bps);

  int bitrate_bps() const;

  unsigned int ssrc() const;

  static bool Compare(const std::pair<unsigned int, RtpStream*>& left,
                      const std::pair<unsigned int, RtpStream*>& right);

 private:
  enum { kRtcpIntervalUs = 1000000 };

  int fps_;
  int bitrate_bps_;
  unsigned int ssrc_;
  unsigned int frequency_;
  int64_t next_rtp_time_;
  int64_t next_rtcp_time_;
  uint32_t rtp_timestamp_offset_;
  const double kNtpFracPerMs;

  DISALLOW_COPY_AND_ASSIGN(RtpStream);
};

class StreamGenerator {
 public:
  typedef std::list<RtpStream::RtcpPacket*> RtcpList;

  StreamGenerator(int capacity, double time_now);

  ~StreamGenerator();

  
  void AddStream(RtpStream* stream);

  
  void set_capacity_bps(int capacity_bps);

  
  
  void SetBitrateBps(int bitrate_bps);

  
  void set_rtp_timestamp_offset(unsigned int ssrc, uint32_t offset);

  
  
  int64_t GenerateFrame(RtpStream::PacketList* packets, int64_t time_now_us);

 private:
  typedef std::map<unsigned int, RtpStream*> StreamMap;

  
  int capacity_;
  
  int64_t prev_arrival_time_us_;
  
  StreamMap streams_;

  DISALLOW_COPY_AND_ASSIGN(StreamGenerator);
};
}  

class RemoteBitrateEstimatorTest : public ::testing::Test {
 public:
  RemoteBitrateEstimatorTest();
  virtual ~RemoteBitrateEstimatorTest();

 protected:
  virtual void SetUp() = 0;

  void AddDefaultStream();

  
  
  
  
  static uint32_t AbsSendTime(int64_t t, int64_t denom);

  
  static uint32_t AddAbsSendTime(uint32_t t1, uint32_t t2);

  
  
  
  void IncomingPacket(uint32_t ssrc,
                      uint32_t payload_size,
                      int64_t arrival_time,
                      uint32_t rtp_timestamp,
                      uint32_t absolute_send_time);

  
  
  
  
  
  
  bool GenerateAndProcessFrame(unsigned int ssrc, unsigned int bitrate_bps);

  
  
  
  
  unsigned int SteadyStateRun(unsigned int ssrc,
                              int number_of_frames,
                              unsigned int start_bitrate,
                              unsigned int min_bitrate,
                              unsigned int max_bitrate,
                              unsigned int target_bitrate);

  void InitialBehaviorTestHelper(unsigned int expected_converge_bitrate);
  void RateIncreaseReorderingTestHelper();
  void RateIncreaseRtpTimestampsTestHelper();
  void CapacityDropTestHelper(int number_of_streams,
                              bool wrap_time_stamp,
                              unsigned int expected_converge_bitrate,
                              unsigned int expected_bitrate_drop_delta);

  static const unsigned int kDefaultSsrc;

  SimulatedClock clock_;  
  scoped_ptr<testing::TestBitrateObserver> bitrate_observer_;
  scoped_ptr<RemoteBitrateEstimator> bitrate_estimator_;
  scoped_ptr<testing::StreamGenerator> stream_generator_;

  DISALLOW_COPY_AND_ASSIGN(RemoteBitrateEstimatorTest);
};
}  

#endif
