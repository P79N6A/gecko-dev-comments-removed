














#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cassert>
#if defined(_WIN32)
#include <conio.h>
#endif

#include "webrtc/voice_engine/test/auto_test/voe_stress_test.h"
#include "webrtc/voice_engine/test/auto_test/voe_standard_test.h"

#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/voice_engine/voice_engine_defines.h"  

using namespace webrtc;

namespace voetest {

#define VALIDATE_STRESS(expr)                                   \
    if (expr)                                                   \
    {                                                           \
        printf("Error at line: %i, %s \n", __LINE__, #expr);    \
        printf("Error code: %i \n", base->LastError());  \
    }

#ifdef _WIN32

#define PAUSE_OR_SLEEP(x) PAUSE;
#else

#define PAUSE_OR_SLEEP(x) SleepMs(x);
#endif

const char* VoEStressTest::_key = "====YUtFWRAAAAADBtIHgAAAAAEAAAAcAAAAAQBHU0ds"
  "b2JhbCBJUCBTb3VuZAAC\nAAAAIwAAAExpY2Vuc2VkIHRvIE5vcnRlbCBOZXR3cm9rcwAAAAA"
  "xAAAAZxZ7/u0M\niFYyTwSwko5Uutf7mh8S0O4rYZYTFidbzQeuGonuL17F/2oD/2pfDp3jL4"
  "Rf3z/A\nnlJsEJgEtASkDNFuwLILjGY0pzjjAYQp3pCl6z6k2MtE06AirdjGLYCjENpq/opX"
  "\nOrs3sIuwdYK5va/aFcsjBDmlsGCUM48RDYG9s23bIHYafXUC4ofOaubbZPWiPTmL\nEVJ8WH"
  "4F9pgNjALc14oJXfON7r/3\n=EsLx";

int VoEStressTest::DoTest() {
  int test(-1);
  while (test != 0) {
    test = MenuSelection();
    switch (test) {
      case 0:
        
        break;
      case 1:
        
        StartStopTest();
        CreateDeleteChannelsTest();
        MultipleThreadsTest();
        break;
      case 2:
        StartStopTest();
        break;
      case 3:
        CreateDeleteChannelsTest();
        break;
      case 4:
        MultipleThreadsTest();
        break;
      default:
        
        printf("Invalid selection! (Test code error)\n");
        assert(false);
    } 
  } 

  return 0;
}

int VoEStressTest::MenuSelection() {
  printf("------------------------------------------------\n");
  printf("Select stress test\n\n");
  printf(" (0)  Quit\n");
  printf(" (1)  All\n");
  printf("- - - - - - - - - - - - - - - - - - - - - - - - \n");
  printf(" (2)  Start/stop\n");
  printf(" (3)  Create/delete channels\n");
  printf(" (4)  Multiple threads\n");

  const int maxMenuSelection = 4;
  int selection(-1);

  while ((selection < 0) || (selection > maxMenuSelection)) {
    printf("\n: ");
    int retval = scanf("%d", &selection);
    if ((retval != 1) || (selection < 0) || (selection > maxMenuSelection)) {
      printf("Invalid selection!\n");
    }
  }

  return selection;
}

int VoEStressTest::StartStopTest() {
  printf("------------------------------------------------\n");
  printf("Running start/stop test\n");
  printf("------------------------------------------------\n");

  printf("\nNOTE: this thest will fail after a while if Core audio is used\n");
  printf("because MS returns AUDCLNT_E_CPUUSAGE_EXCEEDED (VoE Error 10013).\n");

  
  VoEBase* base = _mgr.BasePtr();

  
  
  
  
  
  
  
  
  
  VALIDATE_STRESS(base->Init());
  VALIDATE_STRESS(base->CreateChannel());

  

  int numberOfLoops(2000);
  int loopSleep(200);
  int i(0);
  int markInterval(20);

  printf("Running %d loops with %d ms sleep. Mark every %d loop. \n",
         numberOfLoops, loopSleep, markInterval);
  printf("Test will take approximately %d minutes. \n",
         numberOfLoops * loopSleep / 1000 / 60 + 1);

  for (i = 0; i < numberOfLoops; ++i) {
    VALIDATE_STRESS(base->SetLocalReceiver(0, 4800));
    VALIDATE_STRESS(base->SetSendDestination(0, 4800, "127.0.0.1"));
    VALIDATE_STRESS(base->StartReceive(0));
    VALIDATE_STRESS(base->StartPlayout(0));
    VALIDATE_STRESS(base->StartSend(0));
    if (!(i % markInterval))
      MARK();
    SleepMs(loopSleep);
    VALIDATE_STRESS(base->StopSend(0));
    VALIDATE_STRESS(base->StopPlayout(0));
    VALIDATE_STRESS(base->StopReceive(0));
  }
  ANL();

  VALIDATE_STRESS(base->SetLocalReceiver(0, 4800));
  VALIDATE_STRESS(base->SetSendDestination(0, 4800, "127.0.0.1"));
  VALIDATE_STRESS(base->StartReceive(0));
  VALIDATE_STRESS(base->StartPlayout(0));
  VALIDATE_STRESS(base->StartSend(0));
  printf("Verify that audio is good. \n");
  PAUSE_OR_SLEEP(20000);
  VALIDATE_STRESS(base->StopSend(0));
  VALIDATE_STRESS(base->StopPlayout(0));
  VALIDATE_STRESS(base->StopReceive(0));

  


  
  VALIDATE_STRESS(base->DeleteChannel(0));
  VALIDATE_STRESS(base->Terminate());

  printf("Test finished \n");

  return 0;
}

int VoEStressTest::CreateDeleteChannelsTest() {
  printf("------------------------------------------------\n");
  printf("Running create/delete channels test\n");
  printf("------------------------------------------------\n");

  
  VoEBase* base = _mgr.BasePtr();

  
  
  
  
  
  
  
  
  
  VALIDATE_STRESS(base->Init());

  

  int numberOfLoops(10000);
  int loopSleep(10);
  int i(0);
  int markInterval(200);

  printf("Running %d loops with %d ms sleep. Mark every %d loop. \n",
         numberOfLoops, loopSleep, markInterval);
  printf("Test will take approximately %d minutes. \n",
         numberOfLoops * loopSleep / 1000 / 60 + 1);

  
  
  
  
  
  

  
  const int maxChannels = base->MaxNumOfChannels();
  VALIDATE_STRESS(maxChannels < 1); 
  bool* channelState = new bool[maxChannels];
  memset(channelState, 0, maxChannels * sizeof(bool));
  int channel(0);
  int noOfActiveChannels(0);
  for (i = 0; i < (maxChannels / 2); ++i) {
    channel = base->CreateChannel();
    VALIDATE_STRESS(channel < 0);
    if (channel >= 0) {
      channelState[channel] = true;
      ++noOfActiveChannels;
    }
  }
  srand((unsigned int) time(NULL));
  bool action(false);
  double rnd(0.0);
  int res(0);

  
  for (i = 0; i < numberOfLoops; ++i) {
    
    action = rand() <= (RAND_MAX / 2);
    if (action) {
      if (noOfActiveChannels < maxChannels) {
        
        channel = base->CreateChannel();
        VALIDATE_STRESS(channel < 0);
        if (channel >= 0) {
          channelState[channel] = true;
          ++noOfActiveChannels;
        }
      }
    } else {
      if (noOfActiveChannels > 0) {
        
        do {
          rnd = static_cast<double> (rand());
          channel = static_cast<int> (rnd /
                                      (static_cast<double> (RAND_MAX) + 1.0f) *
                                      maxChannels);
        } while (!channelState[channel]); 

        res = base->DeleteChannel(channel);
        VALIDATE_STRESS(0 != res);
        if (0 == res) {
          channelState[channel] = false;
          --noOfActiveChannels;
        }
      }
    }

    if (!(i % markInterval))
      MARK();
    SleepMs(loopSleep);
  }
  ANL();

  delete[] channelState;

  


  
  VALIDATE_STRESS(base->Terminate()); 

  printf("Test finished \n");

  return 0;
}

int VoEStressTest::MultipleThreadsTest() {
  printf("------------------------------------------------\n");
  printf("Running multiple threads test\n");
  printf("------------------------------------------------\n");

  
  VoEBase* base = _mgr.BasePtr();

  
  
  
  
  
  
  
  
  

  
  VALIDATE_STRESS(base->Init());
  VALIDATE_STRESS(base->CreateChannel());

  

  int numberOfLoops(10000);
  int loopSleep(0);
  int i(0);
  int markInterval(1000);

  printf("Running %d loops with %d ms sleep. Mark every %d loop. \n",
         numberOfLoops, loopSleep, markInterval);
  printf("Test will take approximately %d minutes. \n",
         numberOfLoops * loopSleep / 1000 / 60 + 1);

  srand((unsigned int) time(NULL));
  int rnd(0);

  
  const char* threadName = "StressTest Extra API Thread";
  _ptrExtraApiThread = ThreadWrapper::CreateThread(RunExtraApi, this,
                                                   kNormalPriority, threadName);
  unsigned int id(0);
  VALIDATE_STRESS(!_ptrExtraApiThread->Start(id));

  
  
  
  
  

  
  for (i = 0; i < numberOfLoops; ++i) {
    
    
    rnd = rand();
    if (rnd < (RAND_MAX / 2)) {
      
      base->StartPlayout(0);
    } else {
      
      base->StopPlayout(0);
    }
    

    if (!(i % markInterval))
      MARK();
    SleepMs(loopSleep);
  }
  ANL();

  
  VALIDATE_STRESS(!_ptrExtraApiThread->Stop());
  delete _ptrExtraApiThread;

  

  
  VALIDATE_STRESS(base->Terminate()); 

  printf("Test finished \n");

  return 0;
}



bool VoEStressTest::RunExtraApi(void* ptr) {
  return static_cast<VoEStressTest*> (ptr)->ProcessExtraApi();
}

bool VoEStressTest::ProcessExtraApi() {
  
  VoEBase* base = _mgr.BasePtr();
  int rnd(0);

  

  
  
  rnd = rand();
  if (rnd < (RAND_MAX / 2)) {
    
    base->StartPlayout(0);
  } else {
    
    base->StopPlayout(0);
  }
  

  return true;
}

} 
