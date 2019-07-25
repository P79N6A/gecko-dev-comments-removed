




































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
nsDOMMemoryReporter::GetProcess(nsACString &aProcess)
{
  
  aProcess.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetPath(nsACString &aMemoryPath)
{
  aMemoryPath.AssignLiteral("explicit/dom");
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetKind(PRInt32* aKind)
{
  *aKind = KIND_HEAP;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetDescription(nsACString &aDescription)
{
  aDescription.AssignLiteral("Memory used by the DOM.");
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMemoryReporter::GetUnits(PRInt32* aUnits)
{
  *aUnits = UNITS_BYTES;
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
nsDOMMemoryReporter::GetAmount(PRInt64* aAmount) {
  *aAmount = 0;

  nsGlobalWindow::WindowByIdTable* windows = nsGlobalWindow::GetWindowsTable();
  NS_ENSURE_TRUE(windows, NS_OK);

  windows->Enumerate(GetWindowsMemoryUsage, aAmount);

  return NS_OK;
}

