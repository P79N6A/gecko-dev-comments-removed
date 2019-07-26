





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
  size_t mDOMElementNodes;
  size_t mDOMTextNodes;
  size_t mDOMCDATANodes;
  size_t mDOMCommentNodes;
  size_t mDOMEventTargets;
  size_t mDOMOther;
  size_t mStyleSheets;
  size_t mLayoutPresShell;
  size_t mLayoutStyleSets;
  size_t mLayoutTextRuns;
  size_t mLayoutPresContext;
  size_t mPropertyTables;
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
  



  class GhostWindowsReporter MOZ_FINAL : public mozilla::MemoryUniReporter
  {
  public:
    GhostWindowsReporter()
      : MemoryUniReporter("ghost-windows", KIND_OTHER, UNITS_COUNT,
"The number of ghost windows present (the number of nodes underneath "
"explicit/window-objects/top(none)/ghost, modulo race conditions).  A ghost "
"window is not shown in any tab, does not share a domain with any non-detached "
"windows, and has met these criteria for at least "
"memory.ghost_window_timeout_seconds, or has survived a round of "
"about:memory's minimize memory usage button.\n\n"
"Ghost windows can happen legitimately, but they are often indicative of "
"leaks in the browser or add-ons.")
    {}

    static int64_t DistinguishedAmount();

  private:
    int64_t Amount() MOZ_OVERRIDE { return DistinguishedAmount(); }
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

