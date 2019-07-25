




































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

static void
CollectWindowMemoryUsage(nsGlobalWindow *aWindow,
                         nsIMemoryMultiReporterCallback *aCb,
                         nsISupports *aClosure)
{
  NS_NAMED_LITERAL_CSTRING(kWindowDesc,
                           "Memory used by a window and the DOM within it.");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCAutoString str("explicit/dom/window-objects/");

  nsIDocShell *docShell = aWindow->GetDocShell();

  nsGlobalWindow *top = aWindow->GetTop();
  PRInt64 windowSize = aWindow->SizeOf();

  if (docShell && aWindow->IsFrozen()) {
    str += NS_LITERAL_CSTRING("cached/");
  } else if (docShell) {
    str += NS_LITERAL_CSTRING("active/");
  } else {
    str += NS_LITERAL_CSTRING("other/");
  }

  if (aWindow->IsInnerWindow()) {
    str += NS_LITERAL_CSTRING("top=");

    if (top) {
      str.AppendInt(top->WindowID());

      nsGlobalWindow *topInner = top->GetCurrentInnerWindowInternal();
      if (topInner) {
        str += NS_LITERAL_CSTRING(" (inner=");
        str.AppendInt(topInner->WindowID());
        str += NS_LITERAL_CSTRING(")");
      }
    } else {
      str += NS_LITERAL_CSTRING("none");
    }

    str += NS_LITERAL_CSTRING("/inner-window(id=");
    str.AppendInt(aWindow->WindowID());
    str += NS_LITERAL_CSTRING(", uri=");

    if (!AppendWindowURI(aWindow, str)) {
      str += NS_LITERAL_CSTRING("[system]");
    }

    str += NS_LITERAL_CSTRING(")");
  } else {
    
    
    

    str += NS_LITERAL_CSTRING("outer-windows");
  }

  aCb->Callback(EmptyCString(), str, nsIMemoryReporter::KIND_HEAP,
                nsIMemoryReporter::UNITS_BYTES, windowSize, kWindowDesc,
                aClosure);
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
  for (; w != end; ++w) {
    CollectWindowMemoryUsage(*w, aCb, aClosure);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryMultiReporter::GetExplicitNonHeap(PRInt64* aAmount)
{
  
  *aAmount = 0;
  return NS_OK;
}


