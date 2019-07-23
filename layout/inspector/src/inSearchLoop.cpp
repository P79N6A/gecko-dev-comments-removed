




































#include "inSearchLoop.h"

#include "nsITimer.h"
#include "nsIServiceManager.h"


inSearchLoop::inSearchLoop(inISearchProcess* aSearchProcess)
{
  mSearchProcess = aSearchProcess;
  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
}

inSearchLoop::~inSearchLoop()
{
}




nsresult
inSearchLoop::Start()
{
  mTimer->InitWithFuncCallback(inSearchLoop::TimerCallback, (void*)this, 0, nsITimer::TYPE_REPEATING_SLACK);

  return NS_OK;
}

nsresult
inSearchLoop::Step()
{
  PRBool done = PR_FALSE;
  mSearchProcess->SearchStep(&done);

  if (done)
    Stop();

  return NS_OK;
}

nsresult
inSearchLoop::Stop()
{
  mTimer->Cancel();
  
  return NS_OK;
}

void 
inSearchLoop::TimerCallback(nsITimer *aTimer, void *aClosure)
{
  inSearchLoop* loop = (inSearchLoop*) aClosure;
  loop->Step();
}
