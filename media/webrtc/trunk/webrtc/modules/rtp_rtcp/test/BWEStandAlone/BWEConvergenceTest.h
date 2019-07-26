









#ifndef WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWECONVERGENCETEST_H_
#define WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWECONVERGENCETEST_H_

#include <string>

#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/BWETestBase.h"
#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/TestSenderReceiver.h"
#include "webrtc/typedefs.h"

class BWEConvergenceTestUp : public BWEOneWayTest
{
public:
    BWEConvergenceTestUp(std::string testName, int startRateKbps, int availBWkbps);
    virtual ~BWEConvergenceTestUp();

    virtual int Init(std::string ip, uint16_t port);

protected:
    virtual bool StoppingCriterionMaster();

private:
    int _availBWkbps;
};


#endif 
