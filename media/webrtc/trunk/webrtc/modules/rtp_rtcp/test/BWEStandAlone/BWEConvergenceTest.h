









#ifndef WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWECONVERGENCETEST_H_
#define WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWECONVERGENCETEST_H_

#include <string>

#include "BWETestBase.h"

#include "typedefs.h"

#include "TestSenderReceiver.h"

class BWEConvergenceTestUp : public BWEOneWayTest
{
public:
    BWEConvergenceTestUp(std::string testName, int startRateKbps, int availBWkbps);
    virtual ~BWEConvergenceTestUp();

    virtual int Init(std::string ip, WebRtc_UWord16 port);

protected:
    virtual bool StoppingCriterionMaster();

private:
    int _availBWkbps;
};


#endif 
