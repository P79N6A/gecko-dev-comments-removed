





#ifndef nsWindowMemoryReporter_h__
#define nsWindowMemoryReporter_h__

#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsDataHashtable.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "nsArenaMemoryStats.h"
#include "mozilla/Attributes.h"





#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

class nsWindowSizes {
public:
  nsWindowSizes(mozilla::MallocSizeOf aMallocSizeOf) {
    memset(this, 0, sizeof(nsWindowSizes));
    mMallocSizeOf = aMallocSizeOf;
  }
  mozilla::MallocSizeOf mMallocSizeOf;
  nsArenaMemoryStats mArenaStats;
  size_t mDOMElementNodesSize;
  size_t mDOMTextNodesSize;
  size_t mDOMCDATANodesSize;
  size_t mDOMCommentNodesSize;
  size_t mDOMEventTargetsSize;
  size_t mDOMOtherSize;
  size_t mStyleSheetsSize;
  size_t mLayoutPresShellSize;
  size_t mLayoutStyleSetsSize;
  size_t mLayoutTextRunsSize;
  size_t mLayoutPresContextSize;
  size_t mPropertyTablesSize;

  uint32_t mDOMEventTargetsCount;
  uint32_t mDOMEventListenersCount;
};
  































































class nsWindowMemoryReporter MOZ_FINAL : public nsIMemoryReporter,
                                         public nsIObserver,
                                         public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEMORYREPORTER
  NS_DECL_NSIOBSERVER

  static void Init();

private:
  




  class GhostURLsReporter MOZ_FINAL : public nsIMemoryReporter
  {
  public:
    GhostURLsReporter(nsWindowMemoryReporter* aWindowReporter);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMEMORYREPORTER

  private:
    nsRefPtr<nsWindowMemoryReporter> mWindowReporter;
  };

  



  class NumGhostsReporter MOZ_FINAL : public mozilla::MemoryUniReporter
  {
  public:
    NumGhostsReporter(nsWindowMemoryReporter* aWindowReporter)
      : MemoryUniReporter("ghost-windows", KIND_OTHER, UNITS_COUNT,
"The number of ghost windows present (the number of nodes underneath "
"explicit/window-objects/top(none)/ghost, modulo race conditions).  A ghost "
"window is not shown in any tab, does not share a domain with any non-detached "
"windows, and has met these criteria for at least "
"memory.ghost_window_timeout_seconds, or has survived a round of "
"about:memory's minimize memory usage button.\n\n"
"Ghost windows can happen legitimately, but they are often indicative of "
"leaks in the browser or add-ons.")
      , mWindowReporter(aWindowReporter)
    {}

  private:
    int64_t Amount() MOZ_OVERRIDE;

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

