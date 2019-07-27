



#ifndef __inSearchLoop_h__
#define __inSearchLoop_h__

#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "inISearchProcess.h"

class inSearchLoop
{
public:
  explicit inSearchLoop(inISearchProcess* aSearchProcess);
  virtual ~inSearchLoop();

  nsresult Start();
  nsresult Step();
  nsresult Stop();
  static void TimerCallback(nsITimer *aTimer, void *aClosure);

protected:
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<inISearchProcess> mSearchProcess;
};

#endif
