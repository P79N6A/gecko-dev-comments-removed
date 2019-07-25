









#ifndef WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWESTABILITYTEST_H_
#define WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWESTABILITYTEST_H_

#include <string>

#include "BWETestBase.h"

#include "typedefs.h"

#include "TestSenderReceiver.h"

class BWEStabilityTest : public BWEOneWayTest
{
public:
    BWEStabilityTest(std::string testName, int rateKbps, int testDurationSeconds);
    virtual ~BWEStabilityTest();

    virtual int Init(std::string ip, WebRtc_UWord16 port);
    virtual void Report(std::fstream &log);

protected:
    virtual bool StoppingCriterionMaster();

private:
    int _testDurationSeconds;
};


#endif 
