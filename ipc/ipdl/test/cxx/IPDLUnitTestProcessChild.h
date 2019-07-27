




#ifndef mozilla__ipdltest_IPDLUnitTestThreadChild_h
#define mozilla__ipdltest_IPDLUnitTestThreadChild_h 1

#include "mozilla/ipc/ProcessChild.h"

namespace mozilla {
namespace _ipdltest {

class IPDLUnitTestProcessChild : public mozilla::ipc::ProcessChild
{
  typedef mozilla::ipc::ProcessChild ProcessChild;

public:
  IPDLUnitTestProcessChild(ProcessId aParentPid) :
    ProcessChild(aParentPid)
  { }

  ~IPDLUnitTestProcessChild()
  { }

  virtual bool Init();
};

} 
} 

#endif 
