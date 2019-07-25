




































#ifndef mozilla_imagelib_DiscardTracker_h_
#define mozilla_imagelib_DiscardTracker_h_

#define DISCARD_TIMEOUT_PREF "image.mem.min_discard_timeout_ms"

class imgContainer;
class nsITimer;

namespace mozilla {
namespace imagelib {






struct DiscardTrackerNode
{
  
  imgContainer *curr;

  
  DiscardTrackerNode *prev, *next;
};










class DiscardTracker
{
  public:
    static nsresult Reset(struct DiscardTrackerNode *node);
    static void Remove(struct DiscardTrackerNode *node);
    static void Shutdown();
    static void ReloadTimeout();
  private:
    static nsresult Initialize();
    static nsresult TimerOn();
    static void TimerOff();
    static void TimerCallback(nsITimer *aTimer, void *aClosure);
};

} 
} 

#endif 
