




































#include "nsWindowMemoryReporter.h"
#include "nsGlobalWindow.h"


nsWindowMemoryReporter::nsWindowMemoryReporter()
{
}

NS_IMPL_ISUPPORTS1(nsWindowMemoryReporter, nsIMemoryMultiReporter)


void
nsWindowMemoryReporter::Init()
{
  
  NS_RegisterMemoryMultiReporter(new nsWindowMemoryReporter());
}

static bool
AppendWindowURI(nsGlobalWindow *aWindow, nsACString& aStr)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aWindow->GetExtantDocument());
  nsCOMPtr<nsIURI> uri;

  if (doc) {
    uri = doc->GetDocumentURI();
  }

  if (!uri) {
    nsIPrincipal *principal = aWindow->GetPrincipal();

    if (principal) {
      principal->GetURI(getter_AddRefs(uri));
    }
  }

  if (!uri) {
    return false;
  }

  nsCString spec;
  uri->GetSpec(spec);

  
  
  
  spec.ReplaceChar('/', '\\');

  aStr += spec;

  return true;
}

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(DOMStyleMallocSizeOf, "windows")

static void
CollectWindowReports(nsGlobalWindow *aWindow,
                     nsWindowSizes *aWindowTotalSizes,
                     nsIMemoryMultiReporterCallback *aCb,
                     nsISupports *aClosure)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCAutoString windowPath("explicit/window-objects/");

  nsIDocShell *docShell = aWindow->GetDocShell();

  nsGlobalWindow *top = aWindow->GetTop();
  nsWindowSizes windowSizes(DOMStyleMallocSizeOf);
  aWindow->SizeOfIncludingThis(&windowSizes);

  if (docShell && aWindow->IsFrozen()) {
    windowPath += NS_LITERAL_CSTRING("cached/");
  } else if (docShell) {
    windowPath += NS_LITERAL_CSTRING("active/");
  } else {
    windowPath += NS_LITERAL_CSTRING("other/");
  }

  if (aWindow->IsInnerWindow()) {
    windowPath += NS_LITERAL_CSTRING("top=");

    if (top) {
      windowPath.AppendInt(top->WindowID());

      nsGlobalWindow *topInner = top->GetCurrentInnerWindowInternal();
      if (topInner) {
        windowPath += NS_LITERAL_CSTRING(" (inner=");
        windowPath.AppendInt(topInner->WindowID());
        windowPath += NS_LITERAL_CSTRING(")");
      }
    } else {
      windowPath += NS_LITERAL_CSTRING("none");
    }

    windowPath += NS_LITERAL_CSTRING("/inner-window(id=");
    windowPath.AppendInt(aWindow->WindowID());
    windowPath += NS_LITERAL_CSTRING(", uri=");

    if (!AppendWindowURI(aWindow, windowPath)) {
      windowPath += NS_LITERAL_CSTRING("[system]");
    }

    windowPath += NS_LITERAL_CSTRING(")");
  } else {
    
    
    

    windowPath += NS_LITERAL_CSTRING("outer-windows");
  }

#define REPORT(_path1, _path2, _amount, _desc)                                \
  do {                                                                        \
    if (_amount > 0) {                                                        \
        nsCAutoString path(_path1);                                           \
        path += _path2;                                                       \
        aCb->Callback(EmptyCString(), path, nsIMemoryReporter::KIND_HEAP,     \
                      nsIMemoryReporter::UNITS_BYTES, _amount,                \
                      NS_LITERAL_CSTRING(_desc), aClosure);                   \
    }                                                                         \
  } while (0)

  REPORT(windowPath, "/dom", windowSizes.mDOM,
         "Memory used by a window and the DOM within it.");
  aWindowTotalSizes->mDOM += windowSizes.mDOM;

  REPORT(windowPath, "/style-sheets", windowSizes.mStyleSheets,
         "Memory used by style sheets within a window.");
  aWindowTotalSizes->mStyleSheets += windowSizes.mStyleSheets;

  REPORT(windowPath, "/layout/arenas", windowSizes.mLayoutArenas,
         "Memory used by layout PresShell, PresContext, and other related "
         "areas within a window.");
  aWindowTotalSizes->mLayoutArenas += windowSizes.mLayoutArenas;

  REPORT(windowPath, "/layout/style-sets", windowSizes.mLayoutStyleSets,
         "Memory used by style sets within a window.");
  aWindowTotalSizes->mLayoutStyleSets += windowSizes.mLayoutStyleSets;

  REPORT(windowPath, "/layout/text-runs", windowSizes.mLayoutTextRuns,
         "Memory used for text-runs (glyph layout) in the PresShell's frame "
         "tree, within a window.");
  aWindowTotalSizes->mLayoutTextRuns += windowSizes.mLayoutTextRuns;

#undef REPORT
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

  
  nsRefPtr<nsGlobalWindow> *w = windows.Elements();
  nsRefPtr<nsGlobalWindow> *end = w + windows.Length();
  nsWindowSizes windowTotalSizes(NULL);
  for (; w != end; ++w) {
    CollectWindowReports(*w, &windowTotalSizes, aCb, aClosure);
  }

#define REPORT(_path, _amount, _desc)                                         \
  do {                                                                        \
    aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING(_path),                  \
                  nsIMemoryReporter::KIND_OTHER,                              \
                  nsIMemoryReporter::UNITS_BYTES, _amount,                    \
                  NS_LITERAL_CSTRING(_desc), aClosure);                       \
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


