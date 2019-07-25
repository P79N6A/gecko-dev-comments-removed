









#ifndef WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWETWOWAYLIMITFINDING_H_
#define WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWETWOWAYLIMITFINDING_H_

#include "BWETestBase.h"

class BWETwoWayLimitFinding : public BWETest
{
public:
    BWETwoWayLimitFinding(std::string testName,
        int masterStartRateKbps, int masterAvailBWkbps,
        int slaveStartRateKbps, int slaveAvailBWkbps,
        bool isMaster = false);

    virtual ~BWETwoWayLimitFinding();

    virtual int Init(std::string ip, WebRtc_UWord16 port);

protected:
    virtual bool StoppingCriterionMaster();
    

private:
    int _availBWkbps;
    int _incomingAvailBWkbps;
    bool _forwLimitReached;
    bool _revLimitReached;

};


#endif 
