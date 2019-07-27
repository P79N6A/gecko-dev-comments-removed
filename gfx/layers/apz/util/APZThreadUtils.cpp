




#include "mozilla/layers/APZThreadUtils.h"

#include "mozilla/layers/Compositor.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace layers {

static bool sThreadAssertionsEnabled = true;
static MessageLoop* sControllerThread;

 void
APZThreadUtils::SetThreadAssertionsEnabled(bool aEnabled) {
  sThreadAssertionsEnabled = aEnabled;
}

 bool
APZThreadUtils::GetThreadAssertionsEnabled() {
  return sThreadAssertionsEnabled;
}

 void
APZThreadUtils::SetControllerThread(MessageLoop* aLoop)
{
  
  
  MOZ_ASSERT(!sControllerThread || !aLoop || sControllerThread == aLoop);
  sControllerThread = aLoop;
}

 void
APZThreadUtils::AssertOnControllerThread() {
  if (!GetThreadAssertionsEnabled()) {
    return;
  }

  MOZ_ASSERT(sControllerThread == MessageLoop::current());
}

 void
APZThreadUtils::AssertOnCompositorThread()
{
  if (GetThreadAssertionsEnabled()) {
    Compositor::AssertOnCompositorThread();
  }
}

 void
APZThreadUtils::RunOnControllerThread(Task* aTask)
{
#ifdef MOZ_ANDROID_APZ
  
  
  if (AndroidBridge::IsJavaUiThread()) {
    aTask->Run();
    delete aTask;
  } else {
    AndroidBridge::Bridge()->PostTaskToUiThread(aTask, 0);
  }
#else
  if (!sControllerThread) {
    
    NS_WARNING("Dropping task posted to controller thread\n");
    delete aTask;
    return;
  }

  if (sControllerThread == MessageLoop::current()) {
    aTask->Run();
    delete aTask;
  } else {
    sControllerThread->PostTask(FROM_HERE, aTask);
  }
#endif
}

} 
} 
