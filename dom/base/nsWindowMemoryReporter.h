





#ifndef nsWindowMemoryReporter_h__
#define nsWindowMemoryReporter_h__

#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsDataHashtable.h"
#include "nsWeakReference.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/PodOperations.h"
#include "mozilla/TimeStamp.h"
#include "nsArenaMemoryStats.h"





#define NS_DECL_SIZEOF_EXCLUDING_THIS \
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

class nsWindowSizes {
#define FOR_EACH_SIZE(macro) \
  macro(DOM,   mDOMElementNodes) \
  macro(DOM,   mDOMTextNodes) \
  macro(DOM,   mDOMCDATANodes) \
  macro(DOM,   mDOMCommentNodes) \
  macro(DOM,   mDOMEventTargets) \
  macro(DOM,   mDOMOther) \
  macro(Style, mStyleSheets) \
  macro(Other, mLayoutPresShell) \
  macro(Style, mLayoutStyleSets) \
  macro(Other, mLayoutTextRuns) \
  macro(Other, mLayoutPresContext) \
  macro(Other, mPropertyTables) \

public:
  nsWindowSizes(mozilla::MallocSizeOf aMallocSizeOf)
    :
      #define ZERO_SIZE(kind, mSize)  mSize(0),
      FOR_EACH_SIZE(ZERO_SIZE)
      #undef ZERO_SIZE
      mArenaStats(),
      mMallocSizeOf(aMallocSizeOf)
  {}

  void addToTabSizes(nsTabSizes *sizes) const {
    #define ADD_TO_TAB_SIZES(kind, mSize) sizes->add(nsTabSizes::kind, mSize);
    FOR_EACH_SIZE(ADD_TO_TAB_SIZES)
    #undef ADD_TO_TAB_SIZES
    mArenaStats.addToTabSizes(sizes);
  }

  #define DECL_SIZE(kind, mSize) size_t mSize;
  FOR_EACH_SIZE(DECL_SIZE);
  #undef DECL_SIZE
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

