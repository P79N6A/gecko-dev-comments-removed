




































#ifndef nsWindowMemoryReporter_h__
#define nsWindowMemoryReporter_h__

#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsDataHashtable.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "mozilla/TimeStamp.h"





#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

class nsWindowSizes {
public:
  nsWindowSizes(nsMallocSizeOfFun aMallocSizeOf) {
    memset(this, 0, sizeof(nsWindowSizes));
    mMallocSizeOf = aMallocSizeOf;
  }
  nsMallocSizeOfFun mMallocSizeOf;
  size_t mDOM;
  size_t mStyleSheets;
  size_t mLayoutArenas;
  size_t mLayoutStyleSets;
  size_t mLayoutTextRuns;
};































































class nsWindowMemoryReporter: public nsIMemoryMultiReporter,
                              public nsIObserver,
                              public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYMULTIREPORTER
  NS_DECL_NSIOBSERVER

  static void Init();

private:
  




  class nsGhostWindowMemoryReporter: public nsIMemoryMultiReporter
  {
  public:
    nsGhostWindowMemoryReporter(nsWindowMemoryReporter* aWindowReporter);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYMULTIREPORTER

  private:
    nsRefPtr<nsWindowMemoryReporter> mWindowReporter;
  };

  
  nsWindowMemoryReporter();

  



  PRUint32 GetGhostTimeout();

  void ObserveDOMWindowDetached(nsISupports* aWindow);
  void ObserveAfterMinimizeMemoryUsage();

  




  void CheckForGhostWindowsCallback();

  












  void CheckForGhostWindows(nsTHashtable<nsUint64HashKey> *aOutGhostIDs = NULL);

  









  nsDataHashtable<nsISupportsHashKey, mozilla::TimeStamp> mDetachedWindows;

  


  bool mCheckForGhostWindowsCallbackPending;
};

#endif 

