






































#ifndef __imgDiscardTracker_h__
#define __imgDiscardTracker_h__

class imgContainer;
class nsITimer;






struct imgDiscardTrackerNode
{
  
  imgContainer *curr;

  
  imgDiscardTrackerNode *prev, *next;
};











class imgDiscardTracker
{
  public:
    static nsresult Reset(struct imgDiscardTrackerNode *node);
    static void Remove(struct imgDiscardTrackerNode *node);
    static void Shutdown();
  private:
    static nsresult Initialize();
    static nsresult TimerOn();
    static void TimerOff();
    static void TimerCallback(nsITimer *aTimer, void *aClosure);
};

#endif 
