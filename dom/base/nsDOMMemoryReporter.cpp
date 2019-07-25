




































#include "nsDOMMemoryReporter.h"
#include "nsGlobalWindow.h"


nsDOMMemoryReporter::nsDOMMemoryReporter()
{
}

NS_IMPL_ISUPPORTS1(nsDOMMemoryReporter, nsIMemoryReporter)


void
nsDOMMemoryReporter::Init()
{
  
  NS_RegisterMemoryReporter(new nsDOMMemoryReporter());
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetProcess(char** aProcess)
{
  
  *aProcess = strdup("");
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetPath(char** aMemoryPath)
{
  *aMemoryPath = strdup("explicit/dom");
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetKind(int* aKind)
{
  *aKind = MR_HEAP;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetDescription(char** aDescription)
{
  *aDescription = strdup("Memory used by the DOM.");
  return NS_OK;
}

static
PLDHashOperator
GetWindowsMemoryUsage(const PRUint64& aId, nsGlobalWindow*& aWindow,
                      void* aClosure)
{
  *(PRInt64*)aClosure += aWindow->SizeOf();
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetMemoryUsed(PRInt64* aMemoryUsed) {
  *aMemoryUsed = 0;

  nsGlobalWindow::WindowByIdTable* windows = nsGlobalWindow::GetWindowsTable();
  windows->Enumerate(GetWindowsMemoryUsage, aMemoryUsed);

  return NS_OK;
}

