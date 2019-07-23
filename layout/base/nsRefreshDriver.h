









































#ifndef nsRefreshDriver_h_
#define nsRefreshDriver_h_

#include "mozilla/TimeStamp.h"
#include "mozFlushType.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"






class nsARefreshObserver {
public:
  virtual void WillRefresh(mozilla::TimeStamp aTime) = 0;
};






class nsRefreshDriver : private nsITimerCallback {
public:
  nsRefreshDriver();
  ~nsRefreshDriver();

  






  mozilla::TimeStamp MostRecentRefresh() const;

  












  PRBool AddRefreshObserver(nsARefreshObserver *aObserver,
                            mozFlushType aFlushType);
  PRBool RemoveRefreshObserver(nsARefreshObserver *aObserver,
                               mozFlushType aFlushType);
private:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD Notify(nsITimer *aTimer);

  typedef nsTObserverArray<nsARefreshObserver*> ObserverArray;

  void EnsureTimerStarted();
  void StopTimer();
  PRUint32 ObserverCount() const;
  void UpdateMostRecentRefresh();
  ObserverArray& ArrayFor(mozFlushType aFlushType);

  nsCOMPtr<nsITimer> mTimer;
  mozilla::TimeStamp mMostRecentRefresh; 

  
  ObserverArray mObservers[3];
};

#endif 
