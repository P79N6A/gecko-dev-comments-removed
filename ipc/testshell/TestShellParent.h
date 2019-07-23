



































#ifndef _IPC_TESTSHELL_TESTSHELLPARENT_H_
#define _IPC_TESTSHELL_TESTSHELLPARENT_H_

#include "mozilla/ipc/TestShellProtocolParent.h"

namespace mozilla {
namespace ipc {

class TestShellParent : public TestShellProtocolParent
{
public:
  TestShellParent();
  ~TestShellParent();
};

} 
} 

#endif 
