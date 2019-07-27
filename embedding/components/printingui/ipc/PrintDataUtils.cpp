





#include "PrintDataUtils.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSettings.h"
#include "nsIServiceManager.h"
#include "nsIWebBrowserPrint.h"
#include "nsXPIDLString.h"

namespace mozilla {
namespace embedding {







NS_IMPL_ISUPPORTS(MockWebBrowserPrint, nsIWebBrowserPrint);

MockWebBrowserPrint::MockWebBrowserPrint(PrintData aData)
  : mData(aData)
{
  MOZ_COUNT_CTOR(MockWebBrowserPrint);
}

MockWebBrowserPrint::~MockWebBrowserPrint()
{
  MOZ_COUNT_DTOR(MockWebBrowserPrint);
}

NS_IMETHODIMP
MockWebBrowserPrint::GetGlobalPrintSettings(nsIPrintSettings **aGlobalPrintSettings)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetCurrentPrintSettings(nsIPrintSettings **aCurrentPrintSettings)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetCurrentChildDOMWindow(nsIDOMWindow **aCurrentPrintSettings)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetDoingPrint(bool *aDoingPrint)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetDoingPrintPreview(bool *aDoingPrintPreview)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetIsFramesetDocument(bool *aIsFramesetDocument)
{
  *aIsFramesetDocument = mData.isFramesetDocument();
  return NS_OK;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetIsFramesetFrameSelected(bool *aIsFramesetFrameSelected)
{
  *aIsFramesetFrameSelected = mData.isFramesetFrameSelected();
  return NS_OK;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetIsIFrameSelected(bool *aIsIFrameSelected)
{
  *aIsIFrameSelected = mData.isIFrameSelected();
  return NS_OK;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetIsRangeSelection(bool *aIsRangeSelection)
{
  *aIsRangeSelection = mData.isRangeSelection();
  return NS_OK;
}

NS_IMETHODIMP
MockWebBrowserPrint::GetPrintPreviewNumPages(int32_t *aPrintPreviewNumPages)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::Print(nsIPrintSettings* aThePrintSettings,
                           nsIWebProgressListener* aWPListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::PrintPreview(nsIPrintSettings* aThePrintSettings,
                                  nsIDOMWindow* aChildDOMWin,
                                  nsIWebProgressListener* aWPListener)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::PrintPreviewNavigate(int16_t aNavType,
                                          int32_t aPageNum)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::Cancel()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::EnumerateDocumentNames(uint32_t* aCount,
                                            char16_t*** aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MockWebBrowserPrint::ExitPrintPreview()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 

