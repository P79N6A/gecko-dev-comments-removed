












#include <fstream>
#include <string>
#include <iostream>
#include <ctime>

#include "event_wrapper.h"
#include "trace.h"

#include "BWEStabilityTest.h"
#include "BWEConvergenceTest.h"
#include "BWETwoWayLimitFinding.h"

#include "MatlabPlot.h"



#ifdef MATLAB
MatlabEngine eng;
#endif


class testContainer
{
public:
    testContainer(BWETest *test, bool waitForKeyStroke, int delayStartSec,
        std::string instruction) :
    _test(test),
        _waitMaster(waitForKeyStroke),
        _waitSlave(waitForKeyStroke),
        _delayMaster(delayStartSec),
        _delaySlave(delayStartSec),
        _instr(instruction) {};

    testContainer(BWETest *test,
        bool waitForKeyStrokeMaster,
        bool waitForKeyStrokeSlave,
        int delayStartSecMaster,
        int delayStartSecSlave,
        std::string instruction) :
    _test(test),
        _waitMaster(waitForKeyStrokeMaster),
        _waitSlave(waitForKeyStrokeSlave),
        _delayMaster(delayStartSecMaster),
        _delaySlave(delayStartSecSlave),
        _instr(instruction) {};

    ~testContainer() { if(_test) delete _test; _test = NULL; };

    BWETest *_test;
    bool _waitMaster;
    bool _waitSlave;
    int _delayMaster;
    int _delaySlave;
    std::string _instr;
};

























void PopulateTests(std::vector<testContainer *>* tests, bool isMaster)
{
    tests->push_back(new testContainer(
        new BWEStabilityTest("Stability", 400, 5*60),
        true, true,
        0, 0,
        "Set bandwidth limit to 512 kbps"));

    tests->push_back(new testContainer(
        new BWEStabilityTest("Stability", 4000, 5*60),
        true, true,
        0, 0,
        "Set bandwidth limit to 5120 kbps"));

    tests->push_back(new testContainer(
        new BWEStabilityTest("Stability", 400, 5*60),
        true, true,
        0, 0,
        "Set bandwidth limit to 512 kbps and a normal distributed delay\
        with mean 100 ms and std dev 15 ms"));

    tests->push_back(new testContainer(
        new BWEConvergenceTestUp("Convergence 256->512", 256, 512),
        true,
        0,
        "Set bandwith limit to 512 kbps"));

        tests->push_back(new testContainer(
        new BWEConvergenceTestUp("Convergence 1024->5120", 1024, 5120),
        true,
        0,
        "Set bandwith limit to 5120 kbps"));

    tests->push_back(new testContainer(
        new BWETwoWayLimitFinding("Asymmetric limit finding {1024, 2048} kbps",
        500, 1024,
        500, 2048,
        isMaster),
        true,
        0,
        "Set bandwith limit to {1024, 2048} kbps asymmetric"));

    tests->push_back(new testContainer(
        new BWETwoWayLimitFinding("Symmetric limit finding {1024, 1024} kbps",
        500, 1024,
        500, 1024,
        isMaster),
        true,
        0,
        "Set bandwith limit to 1024 kbps symmetric"));
}


int main(int argc, char* argv[])
{

    bool isMaster = false;
    WebRtc_UWord16 port;
    std::string ip;
    std::fstream log;
    log.open("TestLog.txt", std::fstream::out | std::fstream::app);

    log << "\n\nBWE TESTER\n";

    time_t t = time(0);   
    struct tm * now = localtime( & t );
    log << (now->tm_year + 1900) << '-'
        << (now->tm_mon + 1) << '-'
        <<  now->tm_mday << " "
        <<  now->tm_hour << ":" << now->tm_min
        << "\n";

    if (argc == 4)
    {
        
        ip = argv[1];

        
        port = atoi(argv[2]);

        
        isMaster = (atoi(argv[3]) != 0);

        std::cout << "Destination: " << ip << "\n";
        log << "Destination: " << ip << "\n";
        std::cout << "Port: " << port << "\n";
        log << "Port: " << port << "\n";
        if (isMaster)
        {
            std::cout << "Master\n";
            log << "Master\n";
        }
        else
        {
            std::cout << "Slave\n";
            log << "Slave\n";
        }

    }
    else
    {
        printf("Usage\nBWETester dstIP port master\n");
        exit(1);
    }

    std::vector<testContainer*> tests;
    PopulateTests(&tests, isMaster);

    int testIndex = 0;
    EventWrapper* event = EventWrapper::Create();
    std::vector<testContainer*>::iterator it;
    for (it=tests.begin() ; it < tests.end(); it++)
    {
        ++testIndex;

        BWETest *theTest = (*it)->_test;

        if (theTest)
        {
            std::cout << "\nTest " << testIndex << ": " << theTest->TestName() << "\n";
        }

        
        std::cout << "--> " << (*it)->_instr << std::endl;

        if ((isMaster && (*it)->_waitMaster)
            || (!isMaster && (*it)->_waitSlave))
        {
            
            std::cout << "Press enter to start test\n";
            getc(stdin);
        }

        if (isMaster)
        {
            if ((*it)->_delayMaster > 0)
            {
                
                std::cout << "Test starting in "
                    << (*it)->_delayMaster
                    << " seconds" << std::endl;
                event->Wait((*it)->_delayMaster * 1000);
            }
        }
        else
        {
            if ((*it)->_delaySlave > 0)
            {
                
                std::cout << "Test starting in "
                    << (*it)->_delaySlave
                    << " seconds" << std::endl;
                event->Wait((*it)->_delaySlave * 1000);
            }
        }

        
        if (theTest)
        {
            theTest->SetMaster(isMaster);
            if (theTest->Init(ip, port) != 0)
            {
                throw "Error initializing sender";
                exit (1);
            }

            theTest->Start();
            theTest->ProcLoop();
            theTest->Stop();
            theTest->Report(log);
            log << std::flush;
        }

        delete (*it); 
    }
    delete event;
    event = NULL;

    log.close();
    return (0);
}
