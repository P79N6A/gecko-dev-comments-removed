





































#ifndef mozilla__ipdltest_IPDLUnitTestTestSubprocess_h
#define mozilla__ipdltest_IPDLUnitTestTestSubprocess_h 1


#include "mozilla/ipc/GeckoChildProcessHost.h"

namespace mozilla {
namespace _ipdltest {


class IPDLUnitTestSubprocess : public mozilla::ipc::GeckoChildProcessHost
{
public:
  IPDLUnitTestSubprocess();
  ~IPDLUnitTestSubprocess();

  


  
  

private:
  DISALLOW_EVIL_CONSTRUCTORS(IPDLUnitTestSubprocess);
};


} 
} 


#endif 
