









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_UNITTEST_HELPER_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_UNITTEST_HELPER_H_

#include <gtest/gtest.h>

#include <list>
#include <map>
#include <utility>

#include "modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h"
#include "system_wrappers/interface/constructor_magic.h"
#include "system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

enum { kMtu = 1200 };

namespace testing {

class TestBitrateObserver : public RemoteBitrateObserver {
 public:
  TestBitrateObserver() : updated_(false), latest_bitrate_(0) {}

  void OnReceiveBitrateChanged(std::vector<unsigned int>* ssrcs,
                               unsigned int bitrate) {
    latest_bitrate_ = bitrate;
    updated_ = true;
  }

  void Reset() {
    updated_ = false;
  }

  bool updated() const {
    return updated_;
  }

  unsigned int latest_bitrate() const {
    return latest_bitrate_;
  }

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

  enum { kSendSideOffsetMs = 1000 };

  RtpStream(int fps, int bitrate_bps, unsigned int ssrc, unsigned int frequency,
      uint32_t timestamp_offset, int64_t rtcp_receive_time);
  void set_rtp_timestamp_offset(uint32_t offset);

  
  
  
  int64_t GenerateFrame(double time_now, PacketList* packets);

  
  double next_rtp_time() const;

  
  RtcpPacket* Rtcp(double time_now);

  void set_bitrate_bps(int bitrate_bps);

  int bitrate_bps() const;

  unsigned int ssrc() const;

  static bool Compare(const std::pair<unsigned int, RtpStream*>& left,
                      const std::pair<unsigned int, RtpStream*>& right);

 private:
  enum { kRtcpIntervalMs = 1000 };

  int fps_;
  int bitrate_bps_;
  unsigned int ssrc_;
  unsigned int frequency_;
  double next_rtp_time_;
  double next_rtcp_time_;
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

  
  
  void set_bitrate_bps(int bitrate_bps);

  
  void set_rtp_timestamp_offset(unsigned int ssrc, uint32_t offset);

  
  
  double GenerateFrame(RtpStream::PacketList* packets, double time_now);

  void Rtcps(RtcpList* rtcps, double time_now) const;

 private:
  typedef std::map<unsigned int, RtpStream*> StreamMap;

  
  int capacity_;
  
  double prev_arrival_time_;
  
  StreamMap streams_;

  DISALLOW_COPY_AND_ASSIGN(StreamGenerator);
};
}  

class RemoteBitrateEstimatorTest : public ::testing::Test {
 public:
  RemoteBitrateEstimatorTest();
  explicit RemoteBitrateEstimatorTest(bool align_streams);

 protected:
  virtual void SetUp();

  void AddDefaultStream();

  
  
  
  
  
  
  bool GenerateAndProcessFrame(unsigned int ssrc, unsigned int bitrate_bps);

  
  
  
  unsigned int SteadyStateRun(unsigned int ssrc,
                              int number_of_frames,
                              unsigned int start_bitrate,
                              unsigned int min_bitrate,
                              unsigned int max_bitrate);

  enum { kDefaultSsrc = 1 };

  double time_now_;  
  OverUseDetectorOptions over_use_detector_options_;
  scoped_ptr<RemoteBitrateEstimator> bitrate_estimator_;
  scoped_ptr<testing::TestBitrateObserver> bitrate_observer_;
  scoped_ptr<testing::StreamGenerator> stream_generator_;
  const bool align_streams_;

  DISALLOW_COPY_AND_ASSIGN(RemoteBitrateEstimatorTest);
};

class RemoteBitrateEstimatorTestAlign : public RemoteBitrateEstimatorTest {
 public:
  RemoteBitrateEstimatorTestAlign() : RemoteBitrateEstimatorTest(true) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoteBitrateEstimatorTestAlign);
};
}  

#endif
