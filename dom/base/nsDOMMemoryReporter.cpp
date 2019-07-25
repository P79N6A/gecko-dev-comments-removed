




































#include "nsDOMMemoryReporter.h"
#include "nsGlobalWindow.h"


nsDOMMemoryMultiReporter::nsDOMMemoryMultiReporter()
{
}

NS_IMPL_ISUPPORTS1(nsDOMMemoryMultiReporter, nsIMemoryMultiReporter)


void
nsDOMMemoryMultiReporter::Init()
{
  
  NS_RegisterMemoryMultiReporter(new nsDOMMemoryMultiReporter());
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

struct WindowTotals
{
  WindowTotals() : mDom(0), mStyleSheets(0) {}
  size_t mDom;
  size_t mStyleSheets;
};

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(WindowStyleSheetsMallocSizeOf,
                                     "window/style-sheets")

static void
CollectWindowReports(nsGlobalWindow *aWindow,
                     WindowTotals *aWindowTotals,
                     nsIMemoryMultiReporterCallback *aCb,
                     nsISupports *aClosure)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCAutoString windowPath("explicit/dom+style/window-objects/");

  nsIDocShell *docShell = aWindow->GetDocShell();

  nsGlobalWindow *top = aWindow->GetTop();
  PRInt64 windowDOMSize = aWindow->SizeOf();
  PRInt64 styleSheetsSize = aWindow->SizeOfStyleSheets(WindowStyleSheetsMallocSizeOf);

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

  if (windowDOMSize > 0) {
    nsCAutoString domPath(windowPath);
    domPath += "/dom";
    NS_NAMED_LITERAL_CSTRING(kWindowDesc,
                             "Memory used by a window and the DOM within it.");
    aCb->Callback(EmptyCString(), domPath, nsIMemoryReporter::KIND_HEAP,
                  nsIMemoryReporter::UNITS_BYTES, windowDOMSize, kWindowDesc,
                  aClosure);
    aWindowTotals->mDom += windowDOMSize;
  }

  if (styleSheetsSize > 0) {
    nsCAutoString styleSheetsPath(windowPath);
    styleSheetsPath += "/style-sheets";
    NS_NAMED_LITERAL_CSTRING(kStyleSheetsDesc,
                             "Memory used by style sheets within a window.");
    aCb->Callback(EmptyCString(), styleSheetsPath,
                  nsIMemoryReporter::KIND_HEAP,
                  nsIMemoryReporter::UNITS_BYTES, styleSheetsSize,
                  kStyleSheetsDesc, aClosure);
    aWindowTotals->mStyleSheets += styleSheetsSize;
  }
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
nsDOMMemoryMultiReporter::CollectReports(nsIMemoryMultiReporterCallback* aCb,
                                         nsISupports* aClosure)
{
  nsGlobalWindow::WindowByIdTable* windowsById =
    nsGlobalWindow::GetWindowsTable();
  NS_ENSURE_TRUE(windowsById, NS_OK);

  
  
  WindowArray windows;
  windowsById->Enumerate(GetWindows, &windows);

  
  nsRefPtr<nsGlobalWindow> *w = windows.Elements();
  nsRefPtr<nsGlobalWindow> *end = w + windows.Length();
  WindowTotals windowTotals;
  for (; w != end; ++w) {
    CollectWindowReports(*w, &windowTotals, aCb, aClosure);
  }

  NS_NAMED_LITERAL_CSTRING(kDomTotalWindowsDesc,
    "Memory used for the DOM within windows.  This is the sum of all windows' "
    "'dom' numbers.");
  aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING("dom-total-window"),
                nsIMemoryReporter::KIND_OTHER,
                nsIMemoryReporter::UNITS_BYTES, windowTotals.mDom,
                kDomTotalWindowsDesc, aClosure);

  NS_NAMED_LITERAL_CSTRING(kLayoutTotalWindowStyleSheetsDesc,
    "Memory used for style sheets within windows.  This is the sum of all windows' "
    "'style-sheets' numbers.");
  aCb->Callback(EmptyCString(), NS_LITERAL_CSTRING("style-sheets-total-window"),
                nsIMemoryReporter::KIND_OTHER,
                nsIMemoryReporter::UNITS_BYTES, windowTotals.mStyleSheets,
                kLayoutTotalWindowStyleSheetsDesc, aClosure);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryMultiReporter::GetExplicitNonHeap(PRInt64* aAmount)
{
  
  *aAmount = 0;
  return NS_OK;
}


