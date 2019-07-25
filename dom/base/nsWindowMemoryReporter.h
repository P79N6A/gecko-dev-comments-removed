




#ifndef nsWindowMemoryReporter_h__
#define nsWindowMemoryReporter_h__

#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsDataHashtable.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "mozilla/TimeStamp.h"
#include "nsArenaMemoryStats.h"
#include "mozilla/Attributes.h"





#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

class nsWindowSizes {
public:
  nsWindowSizes(nsMallocSizeOfFun aMallocSizeOf) {
    memset(this, 0, sizeof(nsWindowSizes));
    mMallocSizeOf = aMallocSizeOf;
  }
  nsMallocSizeOfFun mMallocSizeOf;
  nsArenaMemoryStats mArenaStats;
  size_t mDOMElementNodes;
  size_t mDOMTextNodes;
  size_t mDOMCDATANodes;
  size_t mDOMCommentNodes;
  size_t mDOMOther;
  size_t mStyleSheets;
  size_t mLayoutPresShell;
  size_t mLayoutStyleSets;
  size_t mLayoutTextRuns;
  size_t mLayoutPresContext;
  size_t mPropertyTables;
};































































class nsWindowMemoryReporter MOZ_FINAL : public nsIMemoryMultiReporter,
                                         public nsIObserver,
                                         public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYMULTIREPORTER
  NS_DECL_NSIOBSERVER

  static void Init();

private:
  





  class GhostURLsReporter MOZ_FINAL : public nsIMemoryMultiReporter
  {
  public:
    GhostURLsReporter(nsWindowMemoryReporter* aWindowReporter);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYMULTIREPORTER

  private:
    nsRefPtr<nsWindowMemoryReporter> mWindowReporter;
  };

  



  class NumGhostsReporter MOZ_FINAL : public nsIMemoryReporter
  {
  public:
    NumGhostsReporter(nsWindowMemoryReporter* aWindowReporter);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER

  private:
    nsRefPtr<nsWindowMemoryReporter> mWindowReporter;
  };

  
  nsWindowMemoryReporter();

  



  uint32_t GetGhostTimeout();

  void ObserveDOMWindowDetached(nsISupports* aWindow);
  void ObserveAfterMinimizeMemoryUsage();

  




  void CheckForGhostWindowsCallback();

  












  void CheckForGhostWindows(nsTHashtable<nsUint64HashKey> *aOutGhostIDs = NULL);

  









  nsDataHashtable<nsISupportsHashKey, mozilla::TimeStamp> mDetachedWindows;

  


  bool mCheckForGhostWindowsCallbackPending;
};

#endif 

