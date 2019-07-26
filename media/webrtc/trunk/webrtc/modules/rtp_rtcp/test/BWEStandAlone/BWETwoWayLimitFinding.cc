









#include "BWETwoWayLimitFinding.h"
#include "TestLoadGenerator.h"


BWETwoWayLimitFinding::BWETwoWayLimitFinding(
    std::string testName,
    int masterStartRateKbps, int masterAvailBWkbps,
    int slaveStartRateKbps, int slaveAvailBWkbps,
    bool isMaster )
    :
BWETest(testName, (isMaster ? masterStartRateKbps : slaveStartRateKbps)),
_availBWkbps(isMaster ? masterAvailBWkbps : slaveAvailBWkbps),
_incomingAvailBWkbps(isMaster ? slaveAvailBWkbps : masterAvailBWkbps),
_forwLimitReached(false),
_revLimitReached(false)
{
    _master = isMaster;
}


BWETwoWayLimitFinding::~BWETwoWayLimitFinding()
{
    if (_gen)
    {
        delete _gen;
        _gen = NULL;
    }
}


int BWETwoWayLimitFinding::Init(std::string ip, WebRtc_UWord16 port)
{
    
    const int rtpSampleRate = 90000;
    const int frameRate = 30;
    const double spreadFactor = 0.2;

    _gen = new CBRFixFRGenerator(_sendrec, _startRateKbps, rtpSampleRate, frameRate, spreadFactor);
    if (!_gen)
    {
        return (-1);
    }

    if (!_master) UseRecvTimeout(); 

    return BWETest::Init(ip, port);
}


bool BWETwoWayLimitFinding::StoppingCriterionMaster()
{
    if ((_sendrec->BitrateSent() / 1000.0) > (0.95 * _availBWkbps))
    {
        _forwLimitReached = true;
    }

    WebRtc_Word32 revRateKbps = _sendrec->ReceiveBitrateKbps();
    if (revRateKbps > (0.95 * _incomingAvailBWkbps))
    {
        _revLimitReached = true;
    }

    return (_forwLimitReached && _revLimitReached);
}

