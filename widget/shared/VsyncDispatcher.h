




#ifndef mozilla_widget_VsyncDispatcher_h
#define mozilla_widget_VsyncDispatcher_h

#include "nsISupportsImpl.h"

namespace mozilla {
class TimeStamp;

class VsyncObserver
{
public:
  
  
  virtual bool NotifyVsync(TimeStamp aVsyncTimestamp) = 0;

protected:
  virtual ~VsyncObserver() { }
};


class VsyncDispatcher
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncDispatcher)

public:
  static VsyncDispatcher* GetInstance();

  
  void AddCompositorVsyncObserver(VsyncObserver* aVsyncObserver);
  void RemoveCompositorVsyncObserver(VsyncObserver* aVsyncObserver);

private:
  VsyncDispatcher();
  virtual ~VsyncDispatcher();
};

} 

#endif 
