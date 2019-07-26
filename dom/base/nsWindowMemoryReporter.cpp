





#include "amIAddonManager.h"
#include "nsWindowMemoryReporter.h"
#include "nsGlobalWindow.h"
#include "nsIDocument.h"
#include "nsIDOMWindowCollection.h"
#include "nsIEffectiveTLDService.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsNetCID.h"
#include "nsPrintfCString.h"
#include "XPCJSMemoryReporter.h"
#include "js/MemoryMetrics.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla;

StaticRefPtr<nsWindowMemoryReporter> sWindowReporter;

nsWindowMemoryReporter::nsWindowMemoryReporter()
  : mCheckForGhostWindowsCallbackPending(false)
{
}

NS_IMPL_ISUPPORTS3(nsWindowMemoryReporter, nsIMemoryReporter, nsIObserver,
                   nsSupportsWeakReference)

static nsresult
AddNonJSSizeOfWindowAndItsDescendents(nsGlobalWindow* aWindow,
                                      nsTabSizes* aSizes)
{
  
  nsWindowSizes windowSizes(moz_malloc_size_of);
  aWindow->AddSizeOfIncludingThis(&windowSizes);
  windowSizes.addToTabSizes(aSizes);

  
  nsWindowSizes innerWindowSizes(moz_malloc_size_of);
  nsGlobalWindow* inner = aWindow->GetCurrentInnerWindowInternal();
  if (inner) {
    inner->AddSizeOfIncludingThis(&innerWindowSizes);
    innerWindowSizes.addToTabSizes(aSizes);
  }

  nsCOMPtr<nsIDOMWindowCollection> frames;
  nsresult rv = aWindow->GetFrames(getter_AddRefs(frames));
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t length;
  rv = frames->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (uint32_t i = 0; i < length; i++) {
      nsCOMPtr<nsIDOMWindow> child;
      rv = frames->Item(i, getter_AddRefs(child));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(child);

      nsGlobalWindow* childWin =
        static_cast<nsGlobalWindow*>(static_cast<nsIDOMWindow *>(child.get()));

      rv = AddNonJSSizeOfWindowAndItsDescendents(childWin, aSizes);
      NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

static nsresult
NonJSSizeOfTab(nsPIDOMWindow* aWindow, size_t* aDomSize, size_t* aStyleSize, size_t* aOtherSize)
{
  nsGlobalWindow* window = static_cast<nsGlobalWindow*>(aWindow);

  nsTabSizes sizes;
  nsresult rv = AddNonJSSizeOfWindowAndItsDescendents(window, &sizes);
  NS_ENSURE_SUCCESS(rv, rv);

  *aDomSize   = sizes.mDom;
  *aStyleSize = sizes.mStyle;
  *aOtherSize = sizes.mOther;
  return NS_OK;
}

 void
nsWindowMemoryReporter::Init()
{
  MOZ_ASSERT(!sWindowReporter);
  sWindowReporter = new nsWindowMemoryReporter();
  ClearOnShutdown(&sWindowReporter);
  NS_RegisterMemoryReporter(sWindowReporter);
  RegisterNonJSSizeOfTab(NonJSSizeOfTab);

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    
    
    os->AddObserver(sWindowReporter, DOM_WINDOW_DESTROYED_TOPIC,
                     true);
    os->AddObserver(sWindowReporter, "after-minimize-memory-usage",
                     true);
  }

  NS_RegisterMemoryReporter(new GhostWindowsReporter());
  RegisterGhostWindowsDistinguishedAmount(GhostWindowsReporter::DistinguishedAmount);
}

static already_AddRefed<nsIURI>
GetWindowURI(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> pWindow = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(pWindow, nullptr);

  nsCOMPtr<nsIDocument> doc = pWindow->GetExtantDoc();
  nsCOMPtr<nsIURI> uri;

  if (doc) {
    uri = doc->GetDocumentURI();
  }

  if (!uri) {
    nsCOMPtr<nsIScriptObjectPrincipal> scriptObjPrincipal =
      do_QueryInterface(aWindow);
    NS_ENSURE_TRUE(scriptObjPrincipal, nullptr);

    
    
    
    
    if (pWindow->GetOuterWindow()) {
      nsIPrincipal* principal = scriptObjPrincipal->GetPrincipal();
      if (principal) {
        principal->GetURI(getter_AddRefs(uri));
      }
    }
  }

  return uri.forget();
}

static void
AppendWindowURI(nsGlobalWindow *aWindow, nsACString& aStr)
{
  nsCOMPtr<nsIURI> uri = GetWindowURI(aWindow);

  if (uri) {
    nsCString spec;
    uri->GetSpec(spec);

    
    
    
    spec.ReplaceChar('/', '\\');

    aStr += spec;
  } else {
    
    
    aStr += NS_LITERAL_CSTRING("[system]");
  }
}

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(WindowsMallocSizeOf)


typedef nsDataHashtable<nsUint64HashKey, nsCString> WindowPaths;

static nsresult
CollectWindowReports(nsGlobalWindow *aWindow,
                     amIAddonManager *addonManager,
                     nsWindowSizes *aWindowTotalSizes,
                     nsTHashtable<nsUint64HashKey> *aGhostWindowIDs,
                     WindowPaths *aWindowPaths,
                     WindowPaths *aTopWindowPaths,
                     nsIMemoryReporterCallback *aCb,
                     nsISupports *aClosure)
{
  nsAutoCString windowPath("explicit/");

  
  
  nsGlobalWindow *top = NULL;
  nsCOMPtr<nsIURI> location;
  if (aWindow->GetOuterWindow()) {
    
    MOZ_ASSERT(!!aWindow->GetTop() == !!aWindow->GetDocShell());
    top = aWindow->GetTop();
    if (top) {
      location = GetWindowURI(top);
    }
  }
  if (!location) {
    location = GetWindowURI(aWindow);
  }

  if (addonManager && location) {
    bool ok;
    nsAutoCString id;
    if (NS_SUCCEEDED(addonManager->MapURIToAddonID(location, id, &ok)) && ok) {
      windowPath += NS_LITERAL_CSTRING("add-ons/") + id +
                    NS_LITERAL_CSTRING("/");
    }
  }

  windowPath += NS_LITERAL_CSTRING("window-objects/");

  if (top) {
    windowPath += NS_LITERAL_CSTRING("top(");
    AppendWindowURI(top, windowPath);
    windowPath += NS_LITERAL_CSTRING(", id=");
    windowPath.AppendInt(top->WindowID());
    windowPath += NS_LITERAL_CSTRING(")");

    aTopWindowPaths->Put(aWindow->WindowID(), windowPath);

    windowPath += aWindow->IsFrozen() ? NS_LITERAL_CSTRING("/cached/")
                                      : NS_LITERAL_CSTRING("/active/");
  } else {
    if (aGhostWindowIDs->Contains(aWindow->WindowID())) {
      windowPath += NS_LITERAL_CSTRING("top(none)/ghost/");
    } else {
      windowPath += NS_LITERAL_CSTRING("top(none)/detached/");
    }
  }

  windowPath += NS_LITERAL_CSTRING("window(");
  AppendWindowURI(aWindow, windowPath);
  windowPath += NS_LITERAL_CSTRING(")");

  
  aWindowPaths->Put(aWindow->WindowID(), windowPath);

#define REPORT(_pathTail, _amount, _desc)                                     \
  do {                                                                        \
    if (_amount > 0) {                                                        \
        nsAutoCString path(windowPath);                                       \
        path += _pathTail;                                                    \
        nsresult rv;                                                          \
        rv = aCb->Callback(EmptyCString(), path, nsIMemoryReporter::KIND_HEAP,\
                      nsIMemoryReporter::UNITS_BYTES, _amount,                \
                      NS_LITERAL_CSTRING(_desc), aClosure);                   \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
    }                                                                         \
  } while (0)

  nsWindowSizes windowSizes(WindowsMallocSizeOf);
  aWindow->AddSizeOfIncludingThis(&windowSizes);

  REPORT("/dom/element-nodes", windowSizes.mDOMElementNodes,
         "Memory used by the element nodes in a window's DOM.");
  aWindowTotalSizes->mDOMElementNodes += windowSizes.mDOMElementNodes;

  REPORT("/dom/text-nodes", windowSizes.mDOMTextNodes,
         "Memory used by the text nodes in a window's DOM.");
  aWindowTotalSizes->mDOMTextNodes += windowSizes.mDOMTextNodes;

  REPORT("/dom/cdata-nodes", windowSizes.mDOMCDATANodes,
         "Memory used by the CDATA nodes in a window's DOM.");
  aWindowTotalSizes->mDOMCDATANodes += windowSizes.mDOMCDATANodes;

  REPORT("/dom/comment-nodes", windowSizes.mDOMCommentNodes,
         "Memory used by the comment nodes in a window's DOM.");
  aWindowTotalSizes->mDOMCommentNodes += windowSizes.mDOMCommentNodes;

  REPORT("/dom/event-targets", windowSizes.mDOMEventTargets,
         "Memory used by the event targets table in a window's DOM, and the "
         "objects it points to, which include XHRs.");
  aWindowTotalSizes->mDOMEventTargets += windowSizes.mDOMEventTargets;

  REPORT("/dom/other", windowSizes.mDOMOther,
         "Memory used by a window's DOM that isn't measured by the other "
         "'dom/' numbers.");
  aWindowTotalSizes->mDOMOther += windowSizes.mDOMOther;

  REPORT("/property-tables",
         windowSizes.mPropertyTables,
         "Memory used for the property tables within a window.");
  aWindowTotalSizes->mPropertyTables += windowSizes.mPropertyTables;

  REPORT("/style-sheets", windowSizes.mStyleSheets,
         "Memory used by style sheets within a window.");
  aWindowTotalSizes->mStyleSheets += windowSizes.mStyleSheets;

  REPORT("/layout/pres-shell", windowSizes.mLayoutPresShell,
         "Memory used by layout's PresShell, along with any structures "
         "allocated in its arena and not measured elsewhere, "
         "within a window.");
  aWindowTotalSizes->mLayoutPresShell += windowSizes.mLayoutPresShell;

  REPORT("/layout/line-boxes", windowSizes.mArenaStats.mLineBoxes,
         "Memory used by line boxes within a window.");
  aWindowTotalSizes->mArenaStats.mLineBoxes
    += windowSizes.mArenaStats.mLineBoxes;

  REPORT("/layout/rule-nodes", windowSizes.mArenaStats.mRuleNodes,
         "Memory used by CSS rule nodes within a window.");
  aWindowTotalSizes->mArenaStats.mRuleNodes
    += windowSizes.mArenaStats.mRuleNodes;

  REPORT("/layout/style-contexts", windowSizes.mArenaStats.mStyleContexts,
         "Memory used by style contexts within a window.");
  aWindowTotalSizes->mArenaStats.mStyleContexts
    += windowSizes.mArenaStats.mStyleContexts;

  REPORT("/layout/style-sets", windowSizes.mLayoutStyleSets,
         "Memory used by style sets within a window.");
  aWindowTotalSizes->mLayoutStyleSets += windowSizes.mLayoutStyleSets;

  REPORT("/layout/text-runs", windowSizes.mLayoutTextRuns,
         "Memory used for text-runs (glyph layout) in the PresShell's frame "
         "tree, within a window.");
  aWindowTotalSizes->mLayoutTextRuns += windowSizes.mLayoutTextRuns;

  REPORT("/layout/pres-contexts", windowSizes.mLayoutPresContext,
         "Memory used for the PresContext in the PresShell's frame "
         "within a window.");
  aWindowTotalSizes->mLayoutPresContext += windowSizes.mLayoutPresContext;

  
  
  
  const size_t FRAME_SUNDRIES_THRESHOLD =
    js::MemoryReportingSundriesThreshold();

  size_t frameSundriesSize = 0;
#define FRAME_ID(classname)                                             \
  {                                                                     \
    size_t frameSize                                                    \
      = windowSizes.mArenaStats.FRAME_ID_STAT_FIELD(classname);         \
    if (frameSize < FRAME_SUNDRIES_THRESHOLD) {                         \
      frameSundriesSize += frameSize;                                   \
    } else {                                                            \
      REPORT("/layout/frames/" # classname, frameSize,                  \
             "Memory used by frames of "                                \
             "type " #classname " within a window.");                   \
    }                                                                   \
    aWindowTotalSizes->mArenaStats.FRAME_ID_STAT_FIELD(classname)       \
      += frameSize;                                                     \
  }
#include "nsFrameIdList.h"
#undef FRAME_ID

  if (frameSundriesSize > 0) {
    REPORT("/layout/frames/sundries", frameSundriesSize,
           "The sum of all memory used by frames which were too small "
           "to be shown individually.");
  }

#undef REPORT

  return NS_OK;
}

typedef nsTArray< nsRefPtr<nsGlobalWindow> > WindowArray;

static
PLDHashOperator
GetWindows(const uint64_t& aId, nsGlobalWindow*& aWindow, void* aClosure)
{
  ((WindowArray *)aClosure)->AppendElement(aWindow);

  return PL_DHASH_NEXT;
}

struct ReportGhostWindowsEnumeratorData
{
  nsIMemoryReporterCallback* callback;
  nsISupports* closure;
  nsresult rv;
};

static PLDHashOperator
ReportGhostWindowsEnumerator(nsUint64HashKey* aIDHashKey, void* aClosure)
{
  ReportGhostWindowsEnumeratorData *data =
    static_cast<ReportGhostWindowsEnumeratorData*>(aClosure);

  nsGlobalWindow::WindowByIdTable* windowsById =
    nsGlobalWindow::GetWindowsTable();
  if (!windowsById) {
    NS_WARNING("Couldn't get window-by-id hashtable?");
    return PL_DHASH_NEXT;
  }

  nsGlobalWindow* window = windowsById->Get(aIDHashKey->GetKey());
  if (!window) {
    NS_WARNING("Could not look up window?");
    return PL_DHASH_NEXT;
  }

  nsAutoCString path;
  path.AppendLiteral("ghost-windows/");
  AppendWindowURI(window, path);

  nsresult rv = data->callback->Callback(
     EmptyCString(),
    path,
    nsIMemoryReporter::KIND_OTHER,
    nsIMemoryReporter::UNITS_COUNT,
     1,
     NS_LITERAL_CSTRING("A ghost window."),
    data->closure);

  if (NS_FAILED(rv) && NS_SUCCEEDED(data->rv)) {
    data->rv = rv;
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsWindowMemoryReporter::GetName(nsACString &aName)
{
  aName.AssignLiteral("window-objects");
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMemoryReporter::CollectReports(nsIMemoryReporterCallback* aCb,
                                       nsISupports* aClosure)
{
  nsGlobalWindow::WindowByIdTable* windowsById =
    nsGlobalWindow::GetWindowsTable();
  NS_ENSURE_TRUE(windowsById, NS_OK);

  
  
  WindowArray windows;
  windowsById->Enumerate(GetWindows, &windows);

  
  
  nsTHashtable<nsUint64HashKey> ghostWindows;
  CheckForGhostWindows(&ghostWindows);
  ReportGhostWindowsEnumeratorData reportGhostWindowsEnumData =
    { aCb, aClosure, NS_OK };
  ghostWindows.EnumerateEntries(ReportGhostWindowsEnumerator,
                                &reportGhostWindowsEnumData);
  nsresult rv = reportGhostWindowsEnumData.rv;
  NS_ENSURE_SUCCESS(rv, rv);

  WindowPaths windowPaths;
  WindowPaths topWindowPaths;

  
  nsWindowSizes windowTotalSizes(NULL);
  nsCOMPtr<amIAddonManager> addonManager;
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    addonManager = do_GetService("@mozilla.org/addons/integration;1");
  }
  for (uint32_t i = 0; i < windows.Length(); i++) {
    rv = CollectWindowReports(windows[i], addonManager,
                              &windowTotalSizes, &ghostWindows,
                              &windowPaths, &topWindowPaths, aCb,
                              aClosure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  rv = xpc::JSReporter::CollectReports(&windowPaths, &topWindowPaths,
                                       aCb, aClosure);
  NS_ENSURE_SUCCESS(rv, rv);

#define REPORT(_path, _amount, _desc)                                         \
  do {                                                                        \
    nsresult rv;                                                              \
    rv = aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path),             \
                       nsIMemoryReporter::KIND_OTHER,                         \
                       nsIMemoryReporter::UNITS_BYTES, _amount,               \
                       NS_LITERAL_CSTRING(_desc), aClosure);                  \
    NS_ENSURE_SUCCESS(rv, rv);                                                \
  } while (0)

  REPORT("window-objects/dom/element-nodes", windowTotalSizes.mDOMElementNodes,
         "This is the sum of all windows' 'dom/element-nodes' numbers.");

  REPORT("window-objects/dom/text-nodes", windowTotalSizes.mDOMTextNodes,
         "This is the sum of all windows' 'dom/text-nodes' numbers.");

  REPORT("window-objects/dom/cdata-nodes", windowTotalSizes.mDOMCDATANodes,
         "This is the sum of all windows' 'dom/cdata-nodes' numbers.");

  REPORT("window-objects/dom/comment-nodes", windowTotalSizes.mDOMCommentNodes,
         "This is the sum of all windows' 'dom/comment-nodes' numbers.");

  REPORT("window-objects/dom/event-targets", windowTotalSizes.mDOMEventTargets,
         "This is the sum of all windows' 'dom/event-targets' numbers.");

  REPORT("window-objects/dom/other", windowTotalSizes.mDOMOther,
         "This is the sum of all windows' 'dom/other' numbers.");

  REPORT("window-objects/property-tables",
         windowTotalSizes.mPropertyTables,
         "This is the sum of all windows' 'property-tables' numbers.");

  REPORT("window-objects/style-sheets", windowTotalSizes.mStyleSheets,
         "This is the sum of all windows' 'style-sheets' numbers.");

  REPORT("window-objects/layout/pres-shell", windowTotalSizes.mLayoutPresShell,
         "This is the sum of all windows' 'layout/arenas' numbers.");

  REPORT("window-objects/layout/line-boxes",
         windowTotalSizes.mArenaStats.mLineBoxes,
         "This is the sum of all windows' 'layout/line-boxes' numbers.");

  REPORT("window-objects/layout/rule-nodes",
         windowTotalSizes.mArenaStats.mRuleNodes,
         "This is the sum of all windows' 'layout/rule-nodes' numbers.");

  REPORT("window-objects/layout/style-contexts",
         windowTotalSizes.mArenaStats.mStyleContexts,
         "This is the sum of all windows' 'layout/style-contexts' numbers.");

  REPORT("window-objects/layout/style-sets", windowTotalSizes.mLayoutStyleSets,
         "This is the sum of all windows' 'layout/style-sets' numbers.");

  REPORT("window-objects/layout/text-runs", windowTotalSizes.mLayoutTextRuns,
         "This is the sum of all windows' 'layout/text-runs' numbers.");

  REPORT("window-objects/layout/pres-contexts", windowTotalSizes.mLayoutPresContext,
         "This is the sum of all windows' 'layout/pres-contexts' numbers.");

  size_t frameTotal = 0;
#define FRAME_ID(classname)                \
  frameTotal += windowTotalSizes.mArenaStats.FRAME_ID_STAT_FIELD(classname);
#include "nsFrameIdList.h"
#undef FRAME_ID

  REPORT("window-objects/layout/frames", frameTotal,
         "Memory used for layout frames within windows. "
         "This is the sum of all windows' 'layout/frames/' numbers.");

#undef REPORT

  return NS_OK;
}

uint32_t
nsWindowMemoryReporter::GetGhostTimeout()
{
  return Preferences::GetUint("memory.ghost_window_timeout_seconds", 60);
}

NS_IMETHODIMP
nsWindowMemoryReporter::Observe(nsISupports *aSubject, const char *aTopic,
                                const PRUnichar *aData)
{
  if (!strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC)) {
    ObserveDOMWindowDetached(aSubject);
  } else if (!strcmp(aTopic, "after-minimize-memory-usage")) {
    ObserveAfterMinimizeMemoryUsage();
  } else {
    MOZ_ASSERT(false);
  }

  return NS_OK;
}

void
nsWindowMemoryReporter::ObserveDOMWindowDetached(nsISupports* aWindow)
{
  nsWeakPtr weakWindow = do_GetWeakReference(aWindow);
  if (!weakWindow) {
    NS_WARNING("Couldn't take weak reference to a window?");
    return;
  }

  mDetachedWindows.Put(weakWindow, TimeStamp());

  if (!mCheckForGhostWindowsCallbackPending) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this,
                           &nsWindowMemoryReporter::CheckForGhostWindowsCallback);
    NS_DispatchToCurrentThread(runnable);
    mCheckForGhostWindowsCallbackPending = true;
  }
}

static PLDHashOperator
BackdateTimeStampsEnumerator(nsISupports *aKey, TimeStamp &aTimeStamp,
                             void* aClosure)
{
  TimeStamp *minTimeStamp = static_cast<TimeStamp*>(aClosure);

  if (!aTimeStamp.IsNull() && aTimeStamp > *minTimeStamp) {
    aTimeStamp = *minTimeStamp;
  }

  return PL_DHASH_NEXT;
}

void
nsWindowMemoryReporter::ObserveAfterMinimizeMemoryUsage()
{
  
  
  
  
  

  TimeStamp minTimeStamp = TimeStamp::Now() -
                           TimeDuration::FromSeconds(GetGhostTimeout());

  mDetachedWindows.Enumerate(BackdateTimeStampsEnumerator,
                             &minTimeStamp);
}

void
nsWindowMemoryReporter::CheckForGhostWindowsCallback()
{
  mCheckForGhostWindowsCallbackPending = false;
  CheckForGhostWindows();
}

struct CheckForGhostWindowsEnumeratorData
{
  nsTHashtable<nsCStringHashKey> *nonDetachedDomains;
  nsTHashtable<nsUint64HashKey> *ghostWindowIDs;
  nsIEffectiveTLDService *tldService;
  uint32_t ghostTimeout;
  TimeStamp now;
};

static PLDHashOperator
CheckForGhostWindowsEnumerator(nsISupports *aKey, TimeStamp& aTimeStamp,
                               void* aClosure)
{
  CheckForGhostWindowsEnumeratorData *data =
    static_cast<CheckForGhostWindowsEnumeratorData*>(aClosure);

  nsWeakPtr weakKey = do_QueryInterface(aKey);
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(weakKey);
  if (!window) {
    
    
    return PL_DHASH_REMOVE;
  }

  
  
  
  nsCOMPtr<nsIDOMWindow> top;
  if (window->GetOuterWindow()) {
    window->GetTop(getter_AddRefs(top));
  }

  if (top) {
    
    return PL_DHASH_REMOVE;
  }

  nsCOMPtr<nsIURI> uri = GetWindowURI(window);

  nsAutoCString domain;
  if (uri) {
    
    
    data->tldService->GetBaseDomain(uri, 0, domain);
  }

  if (data->nonDetachedDomains->Contains(domain)) {
    
    
    aTimeStamp = TimeStamp();
  } else {
    
    
    if (aTimeStamp.IsNull()) {
      
      aTimeStamp = data->now;
    } else if ((data->now - aTimeStamp).ToSeconds() > data->ghostTimeout) {
      
      
      if (data->ghostWindowIDs) {
        nsCOMPtr<nsPIDOMWindow> pWindow = do_QueryInterface(window);
        if (pWindow) {
          data->ghostWindowIDs->PutEntry(pWindow->WindowID());
        }
      }
    }
  }

  return PL_DHASH_NEXT;
}

struct GetNonDetachedWindowDomainsEnumeratorData
{
  nsTHashtable<nsCStringHashKey> *nonDetachedDomains;
  nsIEffectiveTLDService *tldService;
};

static PLDHashOperator
GetNonDetachedWindowDomainsEnumerator(const uint64_t& aId, nsGlobalWindow* aWindow,
                                      void* aClosure)
{
  GetNonDetachedWindowDomainsEnumeratorData *data =
    static_cast<GetNonDetachedWindowDomainsEnumeratorData*>(aClosure);

  
  
  if (!aWindow->GetOuterWindow() || !aWindow->GetTop()) {
    
    return PL_DHASH_NEXT;
  }

  nsCOMPtr<nsIURI> uri = GetWindowURI(aWindow);

  nsAutoCString domain;
  if (uri) {
    data->tldService->GetBaseDomain(uri, 0, domain);
  }

  data->nonDetachedDomains->PutEntry(domain);
  return PL_DHASH_NEXT;
}


















void
nsWindowMemoryReporter::CheckForGhostWindows(
  nsTHashtable<nsUint64HashKey> *aOutGhostIDs )
{
  nsCOMPtr<nsIEffectiveTLDService> tldService = do_GetService(
    NS_EFFECTIVETLDSERVICE_CONTRACTID);
  if (!tldService) {
    NS_WARNING("Couldn't get TLDService.");
    return;
  }

  nsGlobalWindow::WindowByIdTable *windowsById =
    nsGlobalWindow::GetWindowsTable();
  if (!windowsById) {
    NS_WARNING("GetWindowsTable returned null");
    return;
  }

  nsTHashtable<nsCStringHashKey> nonDetachedWindowDomains;

  
  GetNonDetachedWindowDomainsEnumeratorData nonDetachedEnumData =
    { &nonDetachedWindowDomains, tldService };
  windowsById->EnumerateRead(GetNonDetachedWindowDomainsEnumerator,
                             &nonDetachedEnumData);

  
  
  CheckForGhostWindowsEnumeratorData ghostEnumData =
    { &nonDetachedWindowDomains, aOutGhostIDs, tldService,
      GetGhostTimeout(), TimeStamp::Now() };
  mDetachedWindows.Enumerate(CheckForGhostWindowsEnumerator,
                             &ghostEnumData);
}

 int64_t
nsWindowMemoryReporter::GhostWindowsReporter::DistinguishedAmount()
{
  nsTHashtable<nsUint64HashKey> ghostWindows;
  sWindowReporter->CheckForGhostWindows(&ghostWindows);
  return ghostWindows.Count();
}

