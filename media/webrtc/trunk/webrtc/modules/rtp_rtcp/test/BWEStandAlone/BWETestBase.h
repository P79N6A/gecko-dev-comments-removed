









#ifndef WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWETESTBASE_H_
#define WEBRTC_MODULES_RTP_RTCP_TEST_BWESTANDALONE_BWETESTBASE_H_

#include <string>
#include <vector>

#include "webrtc/typedefs.h"

#include "webrtc/modules/rtp_rtcp/test/BWEStandAlone/TestSenderReceiver.h"


class StatVec : public std::vector<double>
{
public:
    double Mean();
    double Variance();
    double Std();
    double Max();
    double Min();
    double Median();
    double Percentile(double p); 
    void Export(std::fstream &file, bool colVec = false);
};


class BWETest : public SendRecCB
{
public:
    BWETest(std::string testName, int startRateKbps);
    virtual ~BWETest();

    bool SetMaster(bool isMaster = true);
    void UseRecvTimeout() { _sendrec->SetPacketTimeout(1000); };
    virtual int Init(std::string ip, uint16_t port);
    virtual bool Start();
    virtual bool Stop();
    bool ProcLoop(void);
    virtual void Report(std::fstream &log);
    std::string TestName() { return (_testName); };

    
    virtual void OnOnNetworkChanged(const uint32_t bitrateTargetBps,
        const uint8_t fractionLost,
        const uint16_t roundTripTimeMs,
        const uint32_t jitterMS,
        const uint16_t bwEstimateKbitMin,
        const uint16_t bwEstimateKbitMax);


protected:
    virtual bool StoppingCriterionMaster() = 0;
    virtual bool StoppingCriterionSlave() { return (_sendrec->timeOutTriggered()); };

    TestSenderReceiver * _sendrec;
    TestLoadGenerator * _gen;

    std::string _testName;
    int _startRateKbps;
    bool _master;
    bool _initialized;
    bool _started;
    bool _running;
    EventWrapper *_eventPtr;
    ThreadWrapper* _procThread;
    int64_t _startTimeMs;
    int64_t _stopTimeMs;

    
    CriticalSectionWrapper* _statCritSect;
    StatVec _rateVecKbps;
    StatVec _rttVecMs;
    StatVec _lossVec;
};


class BWEOneWayTest : public BWETest
{
public:
    BWEOneWayTest(std::string testName, int startRateKbps) :
      BWETest(testName, startRateKbps) {};

    virtual int Init(std::string ip, uint16_t port);
    virtual bool Start();

protected:
    virtual bool StoppingCriterionSlave() {return ( _sendrec->timeOutTriggered()); };

private:

};

#endif
