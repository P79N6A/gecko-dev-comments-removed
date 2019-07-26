





#include "amIAddonManager.h"
#include "nsWindowMemoryReporter.h"
#include "nsGlobalWindow.h"
#include "nsIDocument.h"
#include "nsIEffectiveTLDService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsNetCID.h"
#include "nsPrintfCString.h"
#include "XPCJSMemoryReporter.h"
#include "js/MemoryMetrics.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla;

nsWindowMemoryReporter::nsWindowMemoryReporter()
  : mCheckForGhostWindowsCallbackPending(false)
{
}

NS_IMPL_ISUPPORTS3(nsWindowMemoryReporter, nsIMemoryReporter, nsIObserver,
                   nsSupportsWeakReference)


void
nsWindowMemoryReporter::Init()
{
  
  nsRefPtr<nsWindowMemoryReporter> windowReporter = new nsWindowMemoryReporter();
  NS_RegisterMemoryReporter(windowReporter);

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    
    
    os->AddObserver(windowReporter, DOM_WINDOW_DESTROYED_TOPIC,
                     true);
    os->AddObserver(windowReporter, "after-minimize-memory-usage",
                     true);
  }

  nsRefPtr<GhostURLsReporter> ghostURLsReporter =
    new GhostURLsReporter(windowReporter);
  NS_RegisterMemoryReporter(ghostURLsReporter);

  nsRefPtr<NumGhostsReporter> numGhostsReporter =
    new NumGhostsReporter(windowReporter);
  NS_RegisterMemoryReporter(numGhostsReporter);
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
  nsAutoCString windowPath;

  
  
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

  nsCString explicitWindowPath("explicit/");
  explicitWindowPath += windowPath;

  
  nsCString censusWindowPath("event-counts/");
  censusWindowPath += windowPath;

  
  aWindowPaths->Put(aWindow->WindowID(), explicitWindowPath);

#define REPORT_SIZE(_pathTail, _amount, _desc)                                \
  do {                                                                        \
    if (_amount > 0) {                                                        \
      nsAutoCString path(explicitWindowPath);                                 \
      path += _pathTail;                                                      \
      nsresult rv;                                                            \
      rv = aCb->Callback(EmptyCString(), path, nsIMemoryReporter::KIND_HEAP,  \
                    nsIMemoryReporter::UNITS_BYTES, _amount,                  \
                    NS_LITERAL_CSTRING(_desc), aClosure);                     \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    }                                                                         \
  } while (0)

#define REPORT_COUNT(_pathTail, _amount, _desc)                               \
  do {                                                                        \
    if (_amount > 0) {                                                        \
      nsAutoCString path(censusWindowPath);                                   \
      path += _pathTail;                                                      \
      nsresult rv;                                                            \
      rv = aCb->Callback(EmptyCString(), path, nsIMemoryReporter::KIND_OTHER, \
                    nsIMemoryReporter::UNITS_COUNT, _amount,                  \
                    NS_LITERAL_CSTRING(_desc), aClosure);                     \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
    }                                                                         \
  } while (0)

  nsWindowSizes windowSizes(WindowsMallocSizeOf);
  aWindow->SizeOfIncludingThis(&windowSizes);

  REPORT_SIZE("/dom/element-nodes", windowSizes.mDOMElementNodesSize,
              "Memory used by the element nodes in a window's DOM.");
  aWindowTotalSizes->mDOMElementNodesSize += windowSizes.mDOMElementNodesSize;

  REPORT_SIZE("/dom/text-nodes", windowSizes.mDOMTextNodesSize,
              "Memory used by the text nodes in a window's DOM.");
  aWindowTotalSizes->mDOMTextNodesSize += windowSizes.mDOMTextNodesSize;

  REPORT_SIZE("/dom/cdata-nodes", windowSizes.mDOMCDATANodesSize,
              "Memory used by the CDATA nodes in a window's DOM.");
  aWindowTotalSizes->mDOMCDATANodesSize += windowSizes.mDOMCDATANodesSize;

  REPORT_SIZE("/dom/comment-nodes", windowSizes.mDOMCommentNodesSize,
              "Memory used by the comment nodes in a window's DOM.");
  aWindowTotalSizes->mDOMCommentNodesSize += windowSizes.mDOMCommentNodesSize;

  REPORT_SIZE("/dom/event-targets", windowSizes.mDOMEventTargetsSize,
              "Memory used by the event targets table in a window's DOM, and "
              "the objects it points to, which include XHRs.");
  aWindowTotalSizes->mDOMEventTargetsSize += windowSizes.mDOMEventTargetsSize;

  REPORT_COUNT("/dom/event-targets", windowSizes.mDOMEventTargetsCount,
               "Number of non-node event targets in the event targets table in "
               " window's DOM, such as XHRs.");
  aWindowTotalSizes->mDOMEventTargetsCount += windowSizes.mDOMEventTargetsCount;

  REPORT_COUNT("/dom/event-listeners", windowSizes.mDOMEventListenersCount,
               "Number of event listeners in a window, including event "
               "listeners on nodes and other event targets.");
  aWindowTotalSizes->mDOMEventListenersCount += windowSizes.mDOMEventListenersCount;

  REPORT_SIZE("/dom/other", windowSizes.mDOMOtherSize,
              "Memory used by a window's DOM that isn't measured by the other "
               "'dom/' numbers.");
  aWindowTotalSizes->mDOMOtherSize += windowSizes.mDOMOtherSize;

  REPORT_SIZE("/property-tables", windowSizes.mPropertyTablesSize,
              "Memory used for the property tables within a window.");
  aWindowTotalSizes->mPropertyTablesSize += windowSizes.mPropertyTablesSize;

  REPORT_SIZE("/style-sheets", windowSizes.mStyleSheetsSize,
              "Memory used by style sheets within a window.");
  aWindowTotalSizes->mStyleSheetsSize += windowSizes.mStyleSheetsSize;

  REPORT_SIZE("/layout/pres-shell", windowSizes.mLayoutPresShellSize,
              "Memory used by layout's PresShell, along with any structures "
              "allocated in its arena and not measured elsewhere, "
              "within a window.");
  aWindowTotalSizes->mLayoutPresShellSize += windowSizes.mLayoutPresShellSize;

  REPORT_SIZE("/layout/line-boxes", windowSizes.mArenaStats.mLineBoxes,
              "Memory used by line boxes within a window.");
  aWindowTotalSizes->mArenaStats.mLineBoxes
    += windowSizes.mArenaStats.mLineBoxes;

  REPORT_SIZE("/layout/rule-nodes", windowSizes.mArenaStats.mRuleNodes,
              "Memory used by CSS rule nodes within a window.");
  aWindowTotalSizes->mArenaStats.mRuleNodes
    += windowSizes.mArenaStats.mRuleNodes;

  REPORT_SIZE("/layout/style-contexts", windowSizes.mArenaStats.mStyleContexts,
              "Memory used by style contexts within a window.");
  aWindowTotalSizes->mArenaStats.mStyleContexts
    += windowSizes.mArenaStats.mStyleContexts;

  REPORT_SIZE("/layout/style-sets", windowSizes.mLayoutStyleSetsSize,
              "Memory used by style sets within a window.");
  aWindowTotalSizes->mLayoutStyleSetsSize += windowSizes.mLayoutStyleSetsSize;

  REPORT_SIZE("/layout/text-runs", windowSizes.mLayoutTextRunsSize,
              "Memory used for text-runs (glyph layout) in the PresShell's "
              "frame tree, within a window.");
  aWindowTotalSizes->mLayoutTextRunsSize += windowSizes.mLayoutTextRunsSize;

  REPORT_SIZE("/layout/pres-contexts", windowSizes.mLayoutPresContextSize,
              "Memory used for the PresContext in the PresShell's frame "
              "within a window.");
  aWindowTotalSizes->mLayoutPresContextSize += windowSizes.mLayoutPresContextSize;

  
  
  
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
      REPORT_SIZE("/layout/frames/" # classname, frameSize,             \
                  "Memory used by frames of "                           \
                  "type " #classname " within a window.");              \
    }                                                                   \
    aWindowTotalSizes->mArenaStats.FRAME_ID_STAT_FIELD(classname)       \
      += frameSize;                                                     \
  }
#include "nsFrameIdList.h"
#undef FRAME_ID

  if (frameSundriesSize > 0) {
    REPORT_SIZE("/layout/frames/sundries", frameSundriesSize,
                "The sum of all memory used by frames which were too small "
                "to be shown individually.");
  }

#undef REPORT_SIZE
#undef REPORT_COUNT

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

  WindowPaths windowPaths;

  WindowPaths topWindowPaths;

  
  nsWindowSizes windowTotalSizes(NULL);
  nsCOMPtr<amIAddonManager> addonManager =
    do_GetService("@mozilla.org/addons/integration;1");
  for (uint32_t i = 0; i < windows.Length(); i++) {
    nsresult rv = CollectWindowReports(windows[i], addonManager,
                                       &windowTotalSizes, &ghostWindows,
                                       &windowPaths, &topWindowPaths, aCb,
                                       aClosure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  nsresult rv = xpc::JSReporter::CollectReports(&windowPaths, &topWindowPaths,
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

  REPORT("window-objects/dom/element-nodes", windowTotalSizes.mDOMElementNodesSize,
         "This is the sum of all windows' 'dom/element-nodes' numbers.");

  REPORT("window-objects/dom/text-nodes", windowTotalSizes.mDOMTextNodesSize,
         "This is the sum of all windows' 'dom/text-nodes' numbers.");

  REPORT("window-objects/dom/cdata-nodes", windowTotalSizes.mDOMCDATANodesSize,
         "This is the sum of all windows' 'dom/cdata-nodes' numbers.");

  REPORT("window-objects/dom/comment-nodes", windowTotalSizes.mDOMCommentNodesSize,
         "This is the sum of all windows' 'dom/comment-nodes' numbers.");

  REPORT("window-objects/dom/event-targets", windowTotalSizes.mDOMEventTargetsSize,
         "This is the sum of all windows' 'dom/event-targets' numbers.");

  REPORT("window-objects/dom/other", windowTotalSizes.mDOMOtherSize,
         "This is the sum of all windows' 'dom/other' numbers.");

  REPORT("window-objects/property-tables",
         windowTotalSizes.mPropertyTablesSize,
         "This is the sum of all windows' 'property-tables' numbers.");

  REPORT("window-objects/style-sheets", windowTotalSizes.mStyleSheetsSize,
         "This is the sum of all windows' 'style-sheets' numbers.");

  REPORT("window-objects/layout/pres-shell", windowTotalSizes.mLayoutPresShellSize,
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

  REPORT("window-objects/layout/style-sets", windowTotalSizes.mLayoutStyleSetsSize,
         "This is the sum of all windows' 'layout/style-sets' numbers.");

  REPORT("window-objects/layout/text-runs", windowTotalSizes.mLayoutTextRunsSize,
         "This is the sum of all windows' 'layout/text-runs' numbers.");

  REPORT("window-objects/layout/pres-contexts", windowTotalSizes.mLayoutPresContextSize,
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

NS_IMPL_ISUPPORTS1(nsWindowMemoryReporter::GhostURLsReporter,
                   nsIMemoryReporter)

nsWindowMemoryReporter::
GhostURLsReporter::GhostURLsReporter(
  nsWindowMemoryReporter* aWindowReporter)
  : mWindowReporter(aWindowReporter)
{
}

NS_IMETHODIMP
nsWindowMemoryReporter::
GhostURLsReporter::GetName(nsACString& aName)
{
  aName.AssignLiteral("ghost-windows-multi");
  return NS_OK;
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
nsWindowMemoryReporter::
GhostURLsReporter::CollectReports(
  nsIMemoryReporterCallback* aCb,
  nsISupports* aClosure)
{
  
  nsTHashtable<nsUint64HashKey> ghostWindows;
  mWindowReporter->CheckForGhostWindows(&ghostWindows);

  ReportGhostWindowsEnumeratorData reportGhostWindowsEnumData =
    { aCb, aClosure, NS_OK };

  
  ghostWindows.EnumerateEntries(ReportGhostWindowsEnumerator,
                                &reportGhostWindowsEnumData);

  return reportGhostWindowsEnumData.rv;
}

int64_t
nsWindowMemoryReporter::NumGhostsReporter::Amount()
{
  nsTHashtable<nsUint64HashKey> ghostWindows;
  mWindowReporter->CheckForGhostWindows(&ghostWindows);
  return ghostWindows.Count();
}
