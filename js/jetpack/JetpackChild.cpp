




































#include "mozilla/jetpack/JetpackChild.h"

#include <stdio.h>

namespace mozilla {
namespace jetpack {

JetpackChild::JetpackChild()
{
  NS_ASSERTION(!gInstance, "Something terribly wrong here!");
  gInstance = this;
}

JetpackChild::~JetpackChild()
{
  NS_ASSERTION(gInstance == this, "Something terribly wrong here!");
  gInstance = nsnull;
}

bool
JetpackChild::Init(base::ProcessHandle aParentProcessHandle,
                   MessageLoop* aIOLoop,
                   IPC::Channel* aChannel)
{
  if (!Open(aChannel, aParentProcessHandle, aIOLoop))
    return false;

  return true;
}

void
JetpackChild::CleanUp()
{
}

bool
JetpackChild::RecvLoadImplementation(const nsCString& script)
{
  printf("Received LoadImplementation message: '%s'\n", script.get());
  return true;
}

JetpackChild* JetpackChild::gInstance;

} 
} 
