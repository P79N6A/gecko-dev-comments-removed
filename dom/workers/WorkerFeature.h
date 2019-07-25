





































#ifndef mozilla_dom_workers_workerfeature_h__
#define mozilla_dom_workers_workerfeature_h__

#include "mozilla/dom/workers/Workers.h"

BEGIN_WORKERS_NAMESPACE



















enum Status
{
  
  Pending = 0,

  
  Running,

  
  
  
  
  Closing,

  
  
  
  
  Terminating,

  
  
  
  
  Canceling,

  
  
  Killing,

  
  Dead
};

class WorkerFeature
{
public:
  virtual ~WorkerFeature() { }

  virtual bool Suspend(JSContext* aCx) { return true; }
  virtual bool Resume(JSContext* aCx) { return true; }

  virtual bool Notify(JSContext* aCx, Status aStatus) = 0;
};

END_WORKERS_NAMESPACE

#endif 
