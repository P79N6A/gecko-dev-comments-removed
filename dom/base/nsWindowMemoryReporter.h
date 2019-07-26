





#ifndef nsWindowMemoryReporter_h__
#define nsWindowMemoryReporter_h__

#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsDataHashtable.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/TimeStamp.h"
#include "nsArenaMemoryStats.h"

class nsWindowSizes {
#define FOR_EACH_SIZE(macro) \
  macro(DOM,   mDOMElementNodesSize) \
  macro(DOM,   mDOMTextNodesSize) \
  macro(DOM,   mDOMCDATANodesSize) \
  macro(DOM,   mDOMCommentNodesSize) \
  macro(DOM,   mDOMEventTargetsSize) \
  macro(DOM,   mDOMOtherSize) \
  macro(Style, mStyleSheetsSize) \
  macro(Other, mLayoutPresShellSize) \
  macro(Style, mLayoutStyleSetsSize) \
  macro(Other, mLayoutTextRunsSize) \
  macro(Other, mLayoutPresContextSize) \
  macro(Other, mPropertyTablesSize) \

public:
  nsWindowSizes(mozilla::MallocSizeOf aMallocSizeOf)
    :
      #define ZERO_SIZE(kind, mSize)  mSize(0),
      FOR_EACH_SIZE(ZERO_SIZE)
      #undef ZERO_SIZE
      mDOMEventTargetsCount(0),
      mDOMEventListenersCount(0),
      mArenaStats(),
      mMallocSizeOf(aMallocSizeOf)
  {}

  void addToTabSizes(nsTabSizes *sizes) const {
    #define ADD_TO_TAB_SIZES(kind, mSize) sizes->add(nsTabSizes::kind, mSize);
    FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
    #undef ADD_TO_TAB_SIZES
    mArenaStats.addToTabSizes(sizes);
  }

  size_t getTotalSize() const
  {
    size_t total = 0;
    #define ADD_TO_TOTAL_SIZE(kind, mSize) total += mSize;
    FOR_EACH_SIZE(ADD_TO_TOTAL_SIZE)
    #undef ADD_TO_TOTAL_SIZE
    total += mArenaStats.getTotalSize();
    return total;
  }

  #define DECL_SIZE(kind, mSize) size_t mSize;
  FOR_EACH_SIZE(DECL_SIZE);
  #undef DECL_SIZE

  uint32_t mDOMEventTargetsCount;
  uint32_t mDOMEventListenersCount;

  nsArenaMemoryStats mArenaStats;
  mozilla::MallocSizeOf mMallocSizeOf;

#undef FOR_EACH_SIZE
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

#ifdef DEBUG
  



  static void UnlinkGhostWindows();
#endif

private:
  ~nsWindowMemoryReporter();

  



  class GhostWindowsReporter MOZ_FINAL : public nsIMemoryReporter
  {
    ~GhostWindowsReporter() {}
  public:
    NS_DECL_ISUPPORTS

    static int64_t DistinguishedAmount();

    NS_IMETHOD
    CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                   bool aAnonymize)
    {
      return MOZ_COLLECT_REPORT(
        "ghost-windows", KIND_OTHER, UNITS_COUNT, DistinguishedAmount(),
"The number of ghost windows present (the number of nodes underneath "
"explicit/window-objects/top(none)/ghost, modulo race conditions).  A ghost "
"window is not shown in any tab, does not share a domain with any non-detached "
"windows, and has met these criteria for at least "
"memory.ghost_window_timeout_seconds, or has survived a round of "
"about:memory's minimize memory usage button.\n\n"
"Ghost windows can happen legitimately, but they are often indicative of "
"leaks in the browser or add-ons.");
    }
  };

  
  nsWindowMemoryReporter();

  



  uint32_t GetGhostTimeout();

  void ObserveDOMWindowDetached(nsISupports* aWindow);
  void ObserveAfterMinimizeMemoryUsage();

  












  void CheckForGhostWindows(nsTHashtable<nsUint64HashKey> *aOutGhostIDs = nullptr);

  



  void AsyncCheckForGhostWindows();

  


  void KillCheckTimer();

  static void CheckTimerFired(nsITimer* aTimer, void* aClosure);

  









  nsDataHashtable<nsISupportsHashKey, mozilla::TimeStamp> mDetachedWindows;

  



  mozilla::TimeStamp mLastCheckForGhostWindows;

  nsCOMPtr<nsITimer> mCheckTimer;

  bool mCycleCollectorIsRunning;

  bool mCheckTimerWaitingForCCEnd;
};

#endif 

