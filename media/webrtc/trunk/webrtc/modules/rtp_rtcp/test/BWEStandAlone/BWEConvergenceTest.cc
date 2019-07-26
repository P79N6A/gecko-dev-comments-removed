









#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/BWEConvergenceTest.h"

#include <fstream>
#include <string>

#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/TestLoadGenerator.h"
#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/TestSenderReceiver.h"
#include "webrtc/system_wrappers/interface/event_wrapper.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/tick_util.h"


BWEConvergenceTestUp::BWEConvergenceTestUp(std::string testName, int startRateKbps, int availBWkbps)
:
_availBWkbps(availBWkbps),
BWEOneWayTest(testName, startRateKbps)
{
}


BWEConvergenceTestUp::~BWEConvergenceTestUp()
{
    if (_gen)
    {
        delete _gen;
        _gen = NULL;
    }
}


int BWEConvergenceTestUp::Init(std::string ip, uint16_t port)
{
    
    const int rtpSampleRate = 90000;
    const int frameRate = 30;
    const double spreadFactor = 0.2;

    if (_master)
    {
        _gen = new CBRFixFRGenerator(_sendrec, _startRateKbps, rtpSampleRate, frameRate, spreadFactor);
        if (!_gen)
        {
            return (-1);
        }
    }

    return BWEOneWayTest::Init(ip, port);
}


bool BWEConvergenceTestUp::StoppingCriterionMaster()
{
    return ((_sendrec->BitrateSent() / 1000.0) > (0.9 * _availBWkbps));
}
