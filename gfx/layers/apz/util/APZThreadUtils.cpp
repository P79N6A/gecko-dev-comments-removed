




#include "mozilla/layers/APZThreadUtils.h"

#include "mozilla/layers/Compositor.h"

namespace mozilla {
namespace layers {

static bool sThreadAssertionsEnabled = true;
static PRThread* sControllerThread;

 void
APZThreadUtils::SetThreadAssertionsEnabled(bool aEnabled) {
  sThreadAssertionsEnabled = aEnabled;
}

 bool
APZThreadUtils::GetThreadAssertionsEnabled() {
  return sThreadAssertionsEnabled;
}

 void
APZThreadUtils::AssertOnControllerThread() {
  if (!GetThreadAssertionsEnabled()) {
    return;
  }

  static bool sControllerThreadDetermined = false;
  if (!sControllerThreadDetermined) {
    
    
    
    
    sControllerThread = PR_GetCurrentThread();
    sControllerThreadDetermined = true;
  }
  MOZ_ASSERT(sControllerThread == PR_GetCurrentThread());
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
#ifdef MOZ_WIDGET_GONK
  
  
  MessageLoop* loop = CompositorParent::CompositorLoop();
  MOZ_ASSERT(MessageLoop::current() != loop);
  loop->PostTask(FROM_HERE, aTask);
#else
  
  
  AssertOnControllerThread();
  aTask->Run();
  delete aTask;
#endif
}

} 
} 
