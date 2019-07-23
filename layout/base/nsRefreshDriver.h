









































#ifndef nsRefreshDriver_h_
#define nsRefreshDriver_h_

#include "mozilla/TimeStamp.h"
#include "mozFlushType.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsTObserverArray.h"

class nsPresContext;






class nsARefreshObserver {
public:
  
  
  
  
  
  
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;

  virtual void WillRefresh(mozilla::TimeStamp aTime) = 0;
};

class nsRefreshDriver : public nsITimerCallback {
public:
  nsRefreshDriver(nsPresContext *aPresContext);
  ~nsRefreshDriver();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITIMERCALLBACK

  






  mozilla::TimeStamp MostRecentRefresh() const;

  















  PRBool AddRefreshObserver(nsARefreshObserver *aObserver,
                            mozFlushType aFlushType);
  PRBool RemoveRefreshObserver(nsARefreshObserver *aObserver,
                               mozFlushType aFlushType);

  




  void Disconnect() {
    StopTimer();
    mPresContext = nsnull;
  }

  



  void Freeze();

  



  void Thaw();

#ifdef DEBUG
  


  PRBool IsRefreshObserver(nsARefreshObserver *aObserver,
			   mozFlushType aFlushType);
#endif

private:
  typedef nsTObserverArray<nsARefreshObserver*> ObserverArray;

  void EnsureTimerStarted();
  void StopTimer();
  PRUint32 ObserverCount() const;
  void UpdateMostRecentRefresh();
  ObserverArray& ArrayFor(mozFlushType aFlushType);
  
  void DoRefresh();

  nsCOMPtr<nsITimer> mTimer;
  mozilla::TimeStamp mMostRecentRefresh; 

  nsPresContext *mPresContext; 
                               

  PRBool mFrozen;

  
  ObserverArray mObservers[3];
};

#endif 
