





































#ifndef mozilla__ipdltest_IPDLUnitTestThreadChild_h
#define mozilla__ipdltest_IPDLUnitTestThreadChild_h 1

#include "mozilla/ipc/MozillaChildThread.h"

namespace mozilla {
namespace _ipdltest {

class IPDLUnitTestThreadChild : public mozilla::ipc::MozillaChildThread
{
public:
  IPDLUnitTestThreadChild(ProcessHandle aParentHandle);
  ~IPDLUnitTestThreadChild();

protected:
  virtual void Init();
};

} 
} 

#endif 
