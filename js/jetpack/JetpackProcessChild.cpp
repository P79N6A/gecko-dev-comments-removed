




































#include "mozilla/ipc/IOThreadChild.h"

#include "mozilla/jetpack/JetpackProcessChild.h"

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace jetpack {

bool
JetpackProcessChild::Init()
{
  mJetpack.Init(ParentHandle(),
                IOThreadChild::message_loop(),
                IOThreadChild::channel());
  return true;
}

void
JetpackProcessChild::CleanUp()
{
  mJetpack.CleanUp();
}

} 
} 
