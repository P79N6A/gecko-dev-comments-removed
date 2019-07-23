



































#include "nsPrintSettingsWin.h"
#include "nsCRT.h"

NS_IMPL_ISUPPORTS_INHERITED1(nsPrintSettingsWin, 
                             nsPrintSettings, 
                             nsIPrintSettingsWin)





nsPrintSettingsWin::nsPrintSettingsWin() :
  nsPrintSettings(),
  mDeviceName(nsnull),
  mDriverName(nsnull),
  mDevMode(nsnull)
{

}





nsPrintSettingsWin::nsPrintSettingsWin(const nsPrintSettingsWin& aPS) :
  mDeviceName(nsnull),
  mDriverName(nsnull),
  mDevMode(nsnull)
{
  *this = aPS;
}





nsPrintSettingsWin::~nsPrintSettingsWin()
{
  if (mDeviceName) nsMemory::Free(mDeviceName);
  if (mDriverName) nsMemory::Free(mDriverName);
  if (mDevMode) ::HeapFree(::GetProcessHeap(), 0, mDevMode);
}


NS_IMETHODIMP nsPrintSettingsWin::SetDeviceName(char * aDeviceName)
{
  if (mDeviceName) {
    nsMemory::Free(mDeviceName);
  }
  mDeviceName = aDeviceName?nsCRT::strdup(aDeviceName):nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettingsWin::GetDeviceName(char * *aDeviceName)
{
  NS_ENSURE_ARG_POINTER(aDeviceName);
  *aDeviceName = mDeviceName?nsCRT::strdup(mDeviceName):nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsPrintSettingsWin::SetDriverName(char * aDriverName)
{
  if (mDriverName) {
    nsMemory::Free(mDriverName);
  }
  mDriverName = aDriverName?nsCRT::strdup(aDriverName):nsnull;
  return NS_OK;
}
NS_IMETHODIMP nsPrintSettingsWin::GetDriverName(char * *aDriverName)
{
  NS_ENSURE_ARG_POINTER(aDriverName);
  *aDriverName = mDriverName?nsCRT::strdup(mDriverName):nsnull;
  return NS_OK;
}

void nsPrintSettingsWin::CopyDevMode(DEVMODE* aInDevMode, DEVMODE *& aOutDevMode)
{
  aOutDevMode = nsnull;
  size_t size = aInDevMode->dmSize + aInDevMode->dmDriverExtra;
  aOutDevMode = (LPDEVMODE)::HeapAlloc (::GetProcessHeap(), HEAP_ZERO_MEMORY, size);
  if (aOutDevMode) {
    memcpy(aOutDevMode, aInDevMode, size);
  }

}


NS_IMETHODIMP nsPrintSettingsWin::GetDevMode(DEVMODE * *aDevMode)
{
  NS_ENSURE_ARG_POINTER(aDevMode);

  if (mDevMode) {
    CopyDevMode(mDevMode, *aDevMode);
  } else {
    *aDevMode = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsPrintSettingsWin::SetDevMode(DEVMODE * aDevMode)
{
  if (mDevMode) {
    ::HeapFree(::GetProcessHeap(), 0, mDevMode);
    mDevMode = NULL;
  }

  if (aDevMode) {
    CopyDevMode(aDevMode, mDevMode);
  }
  return NS_OK;
}


nsresult 
nsPrintSettingsWin::_Clone(nsIPrintSettings **_retval)
{
  nsPrintSettingsWin* printSettings = new nsPrintSettingsWin(*this);
  return printSettings->QueryInterface(NS_GET_IID(nsIPrintSettings), (void**)_retval); 
}


nsPrintSettingsWin& nsPrintSettingsWin::operator=(const nsPrintSettingsWin& rhs)
{
  if (this == &rhs) {
    return *this;
  }

  ((nsPrintSettings&) *this) = rhs;

  if (mDeviceName) {
    nsCRT::free(mDeviceName);
  }

  if (mDriverName) {
    nsCRT::free(mDriverName);
  }

  
  if (mDevMode) {
    ::HeapFree(::GetProcessHeap(), 0, mDevMode);
  }

  mDeviceName = rhs.mDeviceName?nsCRT::strdup(rhs.mDeviceName):nsnull;
  mDriverName = rhs.mDriverName?nsCRT::strdup(rhs.mDriverName):nsnull;

  if (rhs.mDevMode) {
    CopyDevMode(rhs.mDevMode, mDevMode);
  } else {
    mDevMode = nsnull;
  }

  return *this;
}



nsresult 
nsPrintSettingsWin::_Assign(nsIPrintSettings *aPS)
{
  nsPrintSettingsWin *psWin = NS_STATIC_CAST(nsPrintSettingsWin*, aPS);
  *this = *psWin;
  return NS_OK;
}





#ifdef DEBUG_rodsX
#include "nsIPrintOptions.h"
#include "nsIServiceManager.h"
class Tester {
public:
  Tester();
};
Tester::Tester()
{
  nsCOMPtr<nsIPrintSettings> ps;
  nsresult rv;
  nsCOMPtr<nsIPrintOptions> printService = do_GetService("@mozilla.org/gfx/printsettings-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = printService->CreatePrintSettings(getter_AddRefs(ps));
  }

  if (ps) {
    ps->SetPrintOptions(nsIPrintSettings::kPrintOddPages,  PR_TRUE);
    ps->SetPrintOptions(nsIPrintSettings::kPrintEvenPages,  PR_FALSE);
    ps->SetMarginTop(1.0);
    ps->SetMarginLeft(1.0);
    ps->SetMarginBottom(1.0);
    ps->SetMarginRight(1.0);
    ps->SetScaling(0.5);
    ps->SetPrintBGColors(PR_TRUE);
    ps->SetPrintBGImages(PR_TRUE);
    ps->SetPrintRange(15);
    ps->SetHeaderStrLeft(NS_ConvertUTF8toUTF16("Left").get());
    ps->SetHeaderStrCenter(NS_ConvertUTF8toUTF16("Center").get());
    ps->SetHeaderStrRight(NS_ConvertUTF8toUTF16("Right").get());
    ps->SetFooterStrLeft(NS_ConvertUTF8toUTF16("Left").get());
    ps->SetFooterStrCenter(NS_ConvertUTF8toUTF16("Center").get());
    ps->SetFooterStrRight(NS_ConvertUTF8toUTF16("Right").get());
    ps->SetPaperName(NS_ConvertUTF8toUTF16("Paper Name").get());
    ps->SetPaperSizeType(10);
    ps->SetPaperData(1);
    ps->SetPaperWidth(100.0);
    ps->SetPaperHeight(50.0);
    ps->SetPaperSizeUnit(nsIPrintSettings::kPaperSizeMillimeters);
    ps->SetPrintReversed(PR_TRUE);
    ps->SetPrintInColor(PR_TRUE);
    ps->SetPaperSize(5);
    ps->SetOrientation(nsIPrintSettings::kLandscapeOrientation);
    ps->SetPrintCommand(NS_ConvertUTF8toUTF16("Command").get());
    ps->SetNumCopies(2);
    ps->SetPrinterName(NS_ConvertUTF8toUTF16("Printer Name").get());
    ps->SetPrintToFile(PR_TRUE);
    ps->SetToFileName(NS_ConvertUTF8toUTF16("File Name").get());
    ps->SetPrintPageDelay(1000);

    nsCOMPtr<nsIPrintSettings> ps2;
    if (NS_SUCCEEDED(rv)) {
      rv = printService->CreatePrintSettings(getter_AddRefs(ps2));
    }

    ps2->Assign(ps);

    nsCOMPtr<nsIPrintSettings> psClone;
    ps2->Clone(getter_AddRefs(psClone));

  }

}
Tester gTester;
#endif

