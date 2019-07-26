









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
  struct Statistics {
    Statistics()
        : fraction_lost(0),
          cumulative_lost(0),
          extended_max_sequence_number(0),
          jitter(0),
          max_jitter(0) {}

    uint8_t fraction_lost;
    uint32_t cumulative_lost;
    uint32_t extended_max_sequence_number;
    uint32_t jitter;
    uint32_t max_jitter;
  };

  virtual ~StreamStatistician();

  virtual bool GetStatistics(Statistics* statistics, bool reset) = 0;
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

  
  virtual void IncomingPacket(const RTPHeader& rtp_header, size_t bytes,
                              bool retransmitted) = 0;

  
  
  virtual StatisticianMap GetActiveStatisticians() const = 0;

  
  virtual StreamStatistician* GetStatistician(uint32_t ssrc) const = 0;

  
  virtual void SetMaxReorderingThreshold(int max_reordering_threshold) = 0;
};

class NullReceiveStatistics : public ReceiveStatistics {
 public:
  virtual void IncomingPacket(const RTPHeader& rtp_header, size_t bytes,
                              bool retransmitted) OVERRIDE;
  virtual StatisticianMap GetActiveStatisticians() const OVERRIDE;
  virtual StreamStatistician* GetStatistician(uint32_t ssrc) const OVERRIDE;
  virtual int32_t TimeUntilNextProcess() OVERRIDE;
  virtual int32_t Process() OVERRIDE;
  virtual void SetMaxReorderingThreshold(int max_reordering_threshold) OVERRIDE;
};

}  
#endif  
