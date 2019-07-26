









#ifndef WEBRTC_MODULES_RTP_RTCP_INTERFACE_RECEIVE_STATISTICS_H_
#define WEBRTC_MODULES_RTP_RTCP_INTERFACE_RECEIVE_STATISTICS_H_

#include <map>

#include "webrtc/modules/interface/module.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class Clock;

class StreamStatistician {
 public:
  virtual ~StreamStatistician();

  virtual bool GetStatistics(RtcpStatistics* statistics, bool reset) = 0;
  virtual void GetDataCounters(uint32_t* bytes_received,
                               uint32_t* packets_received) const = 0;
  virtual uint32_t BitrateReceived() const = 0;

  
  virtual void ResetStatistics() = 0;

  
  
  virtual bool IsRetransmitOfOldPacket(const RTPHeader& header,
                                       int min_rtt) const = 0;

  
  virtual bool IsPacketInOrder(uint16_t sequence_number) const = 0;
};

typedef std::map<uint32_t, StreamStatistician*> StatisticianMap;

class ReceiveStatistics : public Module {
 public:
  virtual ~ReceiveStatistics() {}

  static ReceiveStatistics* Create(Clock* clock);

  
  virtual void IncomingPacket(const RTPHeader& rtp_header,
                              size_t bytes,
                              bool retransmitted) = 0;

  
  virtual void FecPacketReceived(uint32_t ssrc) = 0;

  
  
  virtual StatisticianMap GetActiveStatisticians() const = 0;

  
  virtual StreamStatistician* GetStatistician(uint32_t ssrc) const = 0;

  
  virtual void SetMaxReorderingThreshold(int max_reordering_threshold) = 0;

  
  virtual void RegisterRtcpStatisticsCallback(
      RtcpStatisticsCallback* callback) = 0;

  
  virtual void RegisterRtpStatisticsCallback(
      StreamDataCountersCallback* callback) = 0;
};

class NullReceiveStatistics : public ReceiveStatistics {
 public:
  virtual void IncomingPacket(const RTPHeader& rtp_header,
                              size_t bytes,
                              bool retransmitted) OVERRIDE;
  virtual void FecPacketReceived(uint32_t ssrc) OVERRIDE;
  virtual StatisticianMap GetActiveStatisticians() const OVERRIDE;
  virtual StreamStatistician* GetStatistician(uint32_t ssrc) const OVERRIDE;
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;
  virtual void SetMaxReorderingThreshold(int max_reordering_threshold) OVERRIDE;
  virtual void RegisterRtcpStatisticsCallback(RtcpStatisticsCallback* callback)
      OVERRIDE;
  virtual void RegisterRtpStatisticsCallback(
      StreamDataCountersCallback* callback) OVERRIDE;
};

}  
#endif  
