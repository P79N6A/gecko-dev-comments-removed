




































#include "nsWindowMemoryReporter.h"
#include "nsGlobalWindow.h"
#include "nsIEffectiveTLDService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsNetCID.h"
#include "nsPrintfCString.h"

using namespace mozilla;

nsWindowMemoryReporter::nsWindowMemoryReporter()
  : mCheckForGhostWindowsCallbackPending(false)
{
  mDetachedWindows.Init();
}

NS_IMPL_ISUPPORTS3(nsWindowMemoryReporter, nsIMemoryMultiReporter, nsIObserver,
                   nsSupportsWeakReference)


void
nsWindowMemoryReporter::Init()
{
  
  nsWindowMemoryReporter *reporter = new nsWindowMemoryReporter();
  NS_RegisterMemoryMultiReporter(reporter);

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    
    
    os->AddObserver(reporter, DOM_WINDOW_DESTROYED_TOPIC,  true);
  }
}

static already_AddRefed<nsIURI>
GetWindowURI(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> pWindow = do_QueryInterface(aWindow);
  NS_ENSURE_TRUE(pWindow, NULL);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(pWindow->GetExtantDocument());
  nsCOMPtr<nsIURI> uri;

  if (doc) {
    uri = doc->GetDocumentURI();
  }

  if (!uri) {
    nsCOMPtr<nsIScriptObjectPrincipal> scriptObjPrincipal =
      do_QueryInterface(aWindow);
    NS_ENSURE_TRUE(scriptObjPrincipal, NULL);

    nsIPrincipal *principal = scriptObjPrincipal->GetPrincipal();

    if (principal) {
      principal->GetURI(getter_AddRefs(uri));
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

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(DOMStyleMallocSizeOf, "windows")

static nsresult
CollectWindowReports(nsGlobalWindow *aWindow,
                     nsWindowSizes *aWindowTotalSizes,
                     nsTHashtable<nsUint64HashKey> *aGhostWindowIDs,
                     nsIMemoryMultiReporterCallback *aCb,
                     nsISupports *aClosure)
{
  nsCAutoString windowPath("explicit/window-objects/");

  
  MOZ_ASSERT(!!aWindow->GetTop() == !!aWindow->GetDocShell());

  nsGlobalWindow *top = aWindow->GetTop();
  if (top) {
    windowPath += NS_LITERAL_CSTRING("top(");
    AppendWindowURI(top, windowPath);
    windowPath += NS_LITERAL_CSTRING(", id=");
    windowPath.AppendInt(top->WindowID());
    windowPath += NS_LITERAL_CSTRING(")/");

    windowPath += aWindow->IsFrozen() ? NS_LITERAL_CSTRING("cached/")
                                      : NS_LITERAL_CSTRING("active/");
  } else {
    nsCOMPtr<nsIURI> uri = GetWindowURI(aWindow);

    if (aGhostWindowIDs->Contains(aWindow->WindowID())) {
      windowPath += NS_LITERAL_CSTRING("top(none)/ghost/");
    } else {
      windowPath += NS_LITERAL_CSTRING("top(none)/detached/");
    }
  }

  windowPath += NS_LITERAL_CSTRING("window(");
  AppendWindowURI(aWindow, windowPath);
  windowPath += NS_LITERAL_CSTRING(")");

#define REPORT(_pathTail, _amount, _desc)                                     \
  do {                                                                        \
    if (_amount > 0) {                                                        \
        nsCAutoString path(windowPath);                                       \
        path += _pathTail;                                                    \
        nsresult rv;                                                          \
        rv = aCb->Callback(EmptyCString(), path, nsIMemoryReporter::KIND_HEAP,\
                      nsIMemoryReporter::UNITS_BYTES, _amount,                \
                      NS_LITERAL_CSTRING(_desc), aClosure);                   \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
    }                                                                         \
  } while (0)

  nsWindowSizes windowSizes(DOMStyleMallocSizeOf);
  aWindow->SizeOfIncludingThis(&windowSizes);

  REPORT("/dom", windowSizes.mDOM,
         "Memory used by a window and the DOM within it.");
  aWindowTotalSizes->mDOM += windowSizes.mDOM;

  REPORT("/style-sheets", windowSizes.mStyleSheets,
         "Memory used by style sheets within a window.");
  aWindowTotalSizes->mStyleSheets += windowSizes.mStyleSheets;

  REPORT("/layout/arenas", windowSizes.mLayoutArenas,
         "Memory used by layout PresShell, PresContext, and other related "
         "areas within a window.");
  aWindowTotalSizes->mLayoutArenas += windowSizes.mLayoutArenas;

  REPORT("/layout/style-sets", windowSizes.mLayoutStyleSets,
         "Memory used by style sets within a window.");
  aWindowTotalSizes->mLayoutStyleSets += windowSizes.mLayoutStyleSets;

  REPORT("/layout/text-runs", windowSizes.mLayoutTextRuns,
         "Memory used for text-runs (glyph layout) in the PresShell's frame "
         "tree, within a window.");
  aWindowTotalSizes->mLayoutTextRuns += windowSizes.mLayoutTextRuns;

#undef REPORT

  return NS_OK;
}

typedef nsTArray< nsRefPtr<nsGlobalWindow> > WindowArray;

static
PLDHashOperator
GetWindows(const PRUint64& aId, nsGlobalWindow*& aWindow, void* aClosure)
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
nsWindowMemoryReporter::CollectReports(nsIMemoryMultiReporterCallback* aCb,
                                       nsISupports* aClosure)
{
  nsGlobalWindow::WindowByIdTable* windowsById =
    nsGlobalWindow::GetWindowsTable();
  NS_ENSURE_TRUE(windowsById, NS_OK);

  
  
  WindowArray windows;
  windowsById->Enumerate(GetWindows, &windows);

  
  nsTHashtable<nsUint64HashKey> ghostWindows;
  ghostWindows.Init();
  CheckForGhostWindows(&ghostWindows);

  nsCOMPtr<nsIEffectiveTLDService> tldService = do_GetService(
    NS_EFFECTIVETLDSERVICE_CONTRACTID);
  NS_ENSURE_STATE(tldService);

  
  nsWindowSizes windowTotalSizes(NULL);
  for (PRUint32 i = 0; i < windows.Length(); i++) {
    nsresult rv = CollectWindowReports(windows[i], &windowTotalSizes,
                                       &ghostWindows, aCb, aClosure);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#define REPORT(_path, _amount, _desc)                                         \
  do {                                                                        \
    nsresult rv;                                                              \
    rv = aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path),             \
                       nsIMemoryReporter::KIND_OTHER,                         \
                       nsIMemoryReporter::UNITS_BYTES, _amount,               \
                       NS_LITERAL_CSTRING(_desc), aClosure);                  \
    NS_ENSURE_SUCCESS(rv, rv);                                                \
  } while (0)

  REPORT("window-objects-dom", windowTotalSizes.mDOM, 
         "Memory used for the DOM within windows. "
         "This is the sum of all windows' 'dom' numbers.");
    
  REPORT("window-objects-style-sheets", windowTotalSizes.mStyleSheets, 
         "Memory used for style sheets within windows. "
         "This is the sum of all windows' 'style-sheets' numbers.");
    
  REPORT("window-objects-layout-arenas", windowTotalSizes.mLayoutArenas, 
         "Memory used by layout PresShell, PresContext, and other related "
         "areas within windows. This is the sum of all windows' "
         "'layout/arenas' numbers.");
    
  REPORT("window-objects-layout-style-sets", windowTotalSizes.mLayoutStyleSets, 
         "Memory used for style sets within windows. "
         "This is the sum of all windows' 'layout/style-sets' numbers.");
    
  REPORT("window-objects-layout-text-runs", windowTotalSizes.mLayoutTextRuns, 
         "Memory used for text runs within windows. "
         "This is the sum of all windows' 'layout/text-runs' numbers.");

#undef REPORT
    
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMemoryReporter::GetExplicitNonHeap(PRInt64* aAmount)
{
  
  *aAmount = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsWindowMemoryReporter::Observe(nsISupports *aSubject, const char *aTopic,
                                const PRUnichar *aData)
{
  
  

  MOZ_ASSERT(!strcmp(aTopic, DOM_WINDOW_DESTROYED_TOPIC));

void
nsWindowMemoryReporter::ObserveDOMWindowDetached(nsISupports *aWindow)
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

  return NS_OK;
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
  PRUint32 ghostTimeout;
  TimeStamp now;
};

static PLDHashOperator
CheckForGhostWindowsEnumerator(nsISupports *aKey, TimeStamp& aTimeStamp,
                               void* aClosure)
{
  CheckForGhostWindowsEnumeratorData *data =
    static_cast<CheckForGhostWindowsEnumeratorData*>(aClosure);

  nsWeakPtr weakKey = do_QueryInterface(aKey);
  nsCOMPtr<nsIDOMWindow> window = do_QueryReferent(weakKey);
  if (!window) {
    
    
    return PL_DHASH_REMOVE;
  }

  nsCOMPtr<nsIDOMWindow> top;
  window->GetTop(getter_AddRefs(top));
  if (top) {
    
    return PL_DHASH_REMOVE;
  }

  nsCOMPtr<nsIURI> uri = GetWindowURI(window);

  nsCAutoString domain;
  data->tldService->GetBaseDomain(uri, 0, domain);

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
GetNonDetachedWindowDomainsEnumerator(const PRUint64& aId, nsGlobalWindow* aWindow,
                                      void* aClosure)
{
  GetNonDetachedWindowDomainsEnumeratorData *data =
    static_cast<GetNonDetachedWindowDomainsEnumeratorData*>(aClosure);

  if (!aWindow->GetTop()) {
    
    return PL_DHASH_NEXT;
  }

  nsCOMPtr<nsIURI> uri = GetWindowURI(aWindow);

  nsCAutoString domain;
  data->tldService->GetBaseDomain(uri, 0, domain);

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
  nonDetachedWindowDomains.Init();

  
  GetNonDetachedWindowDomainsEnumeratorData nonDetachedEnumData =
    { &nonDetachedWindowDomains, tldService };
  windowsById->EnumerateRead(GetNonDetachedWindowDomainsEnumerator,
                             &nonDetachedEnumData);

  PRUint32 ghostTimeout =
    Preferences::GetUint("memory.ghost_window_timeout_seconds", 60);

  
  
  CheckForGhostWindowsEnumeratorData ghostEnumData =
    { &nonDetachedWindowDomains, aOutGhostIDs, tldService,
      ghostTimeout, TimeStamp::Now() };
  mDetachedWindows.Enumerate(CheckForGhostWindowsEnumerator,
                             &ghostEnumData);
}
