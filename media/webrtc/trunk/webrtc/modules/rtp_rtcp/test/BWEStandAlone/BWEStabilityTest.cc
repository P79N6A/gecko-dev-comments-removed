









#include <fstream>
#include <math.h>

#include "BWEStabilityTest.h"
#include "TestLoadGenerator.h"
#include "tick_util.h"
#include "critical_section_wrapper.h"


BWEStabilityTest::BWEStabilityTest(std::string testName, int rateKbps, int testDurationSeconds)
:
_testDurationSeconds(testDurationSeconds),
BWEOneWayTest(testName, rateKbps)
{
}


BWEStabilityTest::~BWEStabilityTest()
{
    if (_gen)
    {
        delete _gen;
        _gen = NULL;
    }
}


int BWEStabilityTest::Init(std::string ip, WebRtc_UWord16 port)
{
    
    const int rtpSampleRate = 90000;
    const int frameRate = 30;
    const double spreadFactor = 0.2;
    const double keyToDeltaRatio = 7;
    const int keyFramePeriod = 300;

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


void BWEStabilityTest::Report(std::fstream &log)
{
    
    if(_running) return;

    BWETest::Report(log);

    CriticalSectionScoped cs(_statCritSect);

    log << "Bitrate statistics\n";
    log << "\tAverage = " <<  _rateVecKbps.Mean() << " kbps\n";
    log << "\tMin     = " <<  _rateVecKbps.Min() << " kbps\n";
    log << "\tMax     = " <<  _rateVecKbps.Max() << " kbps\n";
    log << "\tStd     = " <<  _rateVecKbps.Std() << " kbps\n";

}


bool BWEStabilityTest::StoppingCriterionMaster()
{
    return (TickTime::MillisecondTimestamp() - _startTimeMs >= _testDurationSeconds * 1000);
}
