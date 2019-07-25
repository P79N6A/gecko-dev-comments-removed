









#include "BWETestBase.h"

#include <algorithm> 
#include <fstream>
#include <string>
#include <vector>
#include <math.h>

#include "TestSenderReceiver.h"
#include "TestLoadGenerator.h"
#include "event_wrapper.h"
#include "thread_wrapper.h"
#include "tick_util.h"
#include "critical_section_wrapper.h"


double StatVec::Mean()
{
    double sum = 0;

    
    if (size() <= 0) return (0);

    std::vector<double>::iterator it;
    for (it = begin(); it < end(); ++it)
    {
        sum += (*it);
    }

    return (sum / size());
}

double StatVec::Variance()
{
    double sumSqaure = 0;
    double sum = 0;

    std::vector<double>::iterator it;
    for (it = begin(); it < end(); ++it)
    {
        sum += (*it);
        sumSqaure += (*it) * (*it);
    }

    
    
    int M = static_cast<int> (size() - 1);

    if (M > 0)
    {
        double var = (sumSqaure / M) - (sum / (M+1)) * (sum / M);
        assert(var >= 0);
        return (var);
    }
    else
    {
        return (0);
    }
}

double StatVec::Std()
{
    return (sqrt(Variance()));
}

double StatVec::Max()
{
    
    if (size() <= 0) return (0);

    std::vector<double>::iterator it = begin();
    double maxVal = (*it);
    ++it;

    for (; it < end(); ++it)
    {
        if ((*it) > maxVal) maxVal = (*it);
    }

    return (maxVal);
}

double StatVec::Min()
{
    
    if (size() <= 0) return (0);

    std::vector<double>::iterator it = begin();
    double minVal = (*it);
    ++it;

    for (; it < end(); ++it)
    {
        if ((*it) < minVal) minVal = (*it);
    }

    return (minVal);
}

double StatVec::Median()
{
    double median;

    
    if (size() <= 0) return (0);

    
    sort(begin(), end());

    if ((size() % 2) == 0)
    {
        
        median = (at(size()/2 - 1) + at(size()/2)) / 2.0;
    }
    else
    {
        
        median = at(size()/2);
    }

    return (median);
}

double StatVec::Percentile(double p)
{
    
    if (size() <= 0) return (0);

    
    sort(begin(), end());

    int rank = static_cast<int> (((size() - 1) * p) / 100 + 0.5); 
    rank -= 1; 

    assert(rank >= 0);
    assert(rank < static_cast<int>(size()));

    return (at(rank));
}

void StatVec::Export(std::fstream &file, bool colVec )
{
    
    if (size() <= 0) return;

    std::string separator;
    if (colVec) separator = "\n";
    else separator = ", ";

    std::vector<double>::iterator it = begin();
    file << (*it);
    ++it;

    for (; it < end(); ++it)
    {
        file << separator << (*it);
    }

    file << std::endl;
}


bool BWETestProcThreadFunction(void *obj)
{
    if (obj == NULL)
    {
        return false;
    }
    BWETest *theObj = static_cast<BWETest *>(obj);

    theObj->ProcLoop();

    theObj->Stop();

    return(true);
}


BWETest::BWETest(std::string testName, int startRateKbps):
_testName(testName),
_startRateKbps(startRateKbps),
_master(false),
_sendrec(NULL),
_gen(NULL),
_initialized(false),
_started(false),
_running(false),
_eventPtr(NULL),
_procThread(NULL),
_startTimeMs(-1),
_stopTimeMs(-1),
_statCritSect(CriticalSectionWrapper::CreateCriticalSection())
{
    _sendrec = new TestSenderReceiver();
}


BWETest::~BWETest()
{
    if (_running)
    {
        Stop();
    }

    _statCritSect->Enter();
    delete &_statCritSect;

    if (_sendrec)
    {
        delete _sendrec;
        _sendrec = NULL;
    }
}


bool BWETest::SetMaster(bool isMaster )
{
    if (!_initialized)
    {
        
        _master = isMaster;
    }

    return (_master);
}


int BWETest::Init(std::string ip, WebRtc_UWord16 port)
{
    if (_initialized)
    {
        
        return (-1);
    }

    if (!_sendrec)
    {
        throw "SenderReceiver must be created";
        exit(1);
    }

    if (_started)
    {
        
        return (-1);
    }

    
    _sendrec->InitReceiver(port);

    
    _sendrec->SetLoadGenerator(_gen);
    _sendrec->InitSender(_startRateKbps, ip.c_str(), port);
    

    _sendrec->SetCallback(this);

    _initialized = true;

    return 0;
}


bool BWETest::Start()
{
    if (!_initialized)
    {
        
        return (false);
    }
    if (_started)
    {
        
        return (true);
    }

    if (_sendrec->Start() != 0)
    {
        
        return (false);
    }

    if (_gen)
    {
        if (_gen->Start() != 0)
        {
            
            return (false);
        }
    }

    _eventPtr = EventWrapper::Create();

    _startTimeMs = TickTime::MillisecondTimestamp();
    _started = true;
    _running = true;

    return (true);
}


bool BWETest::Stop()
{
    if (_procThread)
    {
        _stopTimeMs = TickTime::MillisecondTimestamp();
        _procThread->SetNotAlive();
        _running = false;
        _eventPtr->Set();

        while (!_procThread->Stop())
        {
            ;
        }

        delete _procThread;
        _procThread = NULL;

    }

    if (_eventPtr)
    {
        delete _eventPtr;
        _eventPtr = NULL;
    }

    _procThread = NULL;

    if(_gen)
    {
        _gen->Stop();
    }

    return(true);
}


bool BWETest::ProcLoop(void)
{
    bool receiving = false;

    
    while (_running)
    {

        
        if (_master && StoppingCriterionMaster())
        {
            printf("StoppingCriterionMaster()\n");
            _stopTimeMs = TickTime::MillisecondTimestamp();
            _running = false;
        }
        else if (!_master && StoppingCriterionSlave())
        {
            printf("StoppingCriterionSlave()\n");
            _running = false;
        }

        
        _eventPtr->Wait(1000); 

    }

    return true;
}


void BWETest::Report(std::fstream &log)
{
    
    if(_running) return;

    CriticalSectionScoped cs(_statCritSect);

    log << "\n\n*** Test name = " << _testName << "\n";
    log << "Execution time = " <<  static_cast<double>(_stopTimeMs - _startTimeMs) / 1000 << " s\n";
    log << "\n";
    log << "RTT statistics\n";
    log << "\tMin     = " << _rttVecMs.Min() << " ms\n";
    log << "\tMax     = " << _rttVecMs.Max() << " ms\n";
    log << "\n";
    log << "Loss statistics\n";
    log << "\tAverage = " << _lossVec.Mean() << "%\n";
    log << "\tMax     = " << _lossVec.Max() << "%\n";

    log << "\n" << "Rates" << "\n";
    _rateVecKbps.Export(log);

    log << "\n" << "RTT" << "\n";
    _rttVecMs.Export(log);

}



void BWETest::OnOnNetworkChanged(const WebRtc_UWord32 bitrateTargetBps,
                                 const WebRtc_UWord8 fractionLost,
                                 const WebRtc_UWord16 roundTripTimeMs,
                                 const WebRtc_UWord32 jitterMS,
                                 const WebRtc_UWord16 bwEstimateKbitMin,
                                 const WebRtc_UWord16 bwEstimateKbitMax)
{
    CriticalSectionScoped cs(_statCritSect);

    
    WebRtc_Word32 newBitrateKbps = bitrateTargetBps/1000;

    _rateVecKbps.push_back(newBitrateKbps);
    _rttVecMs.push_back(roundTripTimeMs);
    _lossVec.push_back(static_cast<double>(fractionLost) / 255.0);
}


int BWEOneWayTest::Init(std::string ip, WebRtc_UWord16 port)
{

    if (!_master)
    {
        
        UseRecvTimeout();
    }

    return (BWETest::Init(ip, port));

}


bool BWEOneWayTest::Start()
{
    bool ret = BWETest::Start();

    if (!_master)
    {
        
        const WebRtc_UWord8 dummy = 0;
        
        _sendrec->SendOutgoingData(
            static_cast<WebRtc_UWord32>(TickTime::MillisecondTimestamp()*90),
            &dummy, 1, webrtc::kVideoFrameDelta);
    }

    return ret;
}
