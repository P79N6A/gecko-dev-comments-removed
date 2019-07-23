







































#include "nsDeviceContextSpecX.h"

#include "prmem.h"
#include "plstr.h"
#include "nsCRT.h"
#include <unistd.h>

#include "nsIServiceManager.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSettingsX.h"

#ifdef MOZ_CAIRO_GFX
#include "gfxQuartzSurface.h"
#include "gfxQuartzPDFSurface.h"
#endif





nsDeviceContextSpecX::nsDeviceContextSpecX()
: mPrintSession(NULL)
, mPageFormat(kPMNoPageFormat)
, mPrintSettings(kPMNoPrintSettings)
, mSavedPort(0)
, mBeganPrinting(PR_FALSE)
{
}





nsDeviceContextSpecX::~nsDeviceContextSpecX()
{
  if (mPrintSession)
    ::PMRelease(mPrintSession);
  ClosePrintManager();
}

#ifdef MOZ_CAIRO_GFX
NS_IMPL_ISUPPORTS1(nsDeviceContextSpecX, nsIDeviceContextSpec)
#else
NS_IMPL_ISUPPORTS2(nsDeviceContextSpecX, nsIDeviceContextSpec, nsIPrintingContext)
#endif




NS_IMETHODIMP nsDeviceContextSpecX::Init(nsIWidget *aWidget,
                                         nsIPrintSettings* aPS,
                                         PRBool aIsPrintPreview)
{
  return Init(aPS, aIsPrintPreview);
}

NS_IMETHODIMP nsDeviceContextSpecX::Init(nsIPrintSettings* aPS, PRBool	aIsPrintPreview)
{
  nsresult rv;
    
  nsCOMPtr<nsIPrintSettingsX> printSettingsX(do_QueryInterface(aPS));
  if (!printSettingsX)
    return NS_ERROR_NO_INTERFACE;
  
  rv = printSettingsX->GetNativePrintSession(&mPrintSession);
  if (NS_FAILED(rv))
    return rv;  
  ::PMRetain(mPrintSession);

  rv = printSettingsX->GetPMPageFormat(&mPageFormat);
  if (NS_FAILED(rv))
    return rv;
  rv = printSettingsX->GetPMPrintSettings(&mPrintSettings);
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::PrintManagerOpen(PRBool* aIsOpen)
{
    *aIsOpen = mBeganPrinting;
    return NS_OK;
}





NS_IMETHODIMP nsDeviceContextSpecX::ClosePrintManager()
{
	return NS_OK;
}  

#ifndef MOZ_CAIRO_GFX
NS_IMETHODIMP nsDeviceContextSpecX::BeginDocument(PRUnichar*  aTitle, 
                                                  PRUnichar*  aPrintToFileName,
                                                  PRInt32     aStartPage, 
                                                  PRInt32     aEndPage)
{
    if (aTitle) {
      CFStringRef cfString = ::CFStringCreateWithCharacters(NULL, aTitle, nsCRT::strlen(aTitle));
      if (cfString) {
        ::PMSetJobNameCFString(mPrintSettings, cfString);
        ::CFRelease(cfString);
      }
    }

    OSStatus status;
    status = ::PMSetFirstPage(mPrintSettings, aStartPage, false);
    NS_ASSERTION(status == noErr, "PMSetFirstPage failed");
    status = ::PMSetLastPage(mPrintSettings, aEndPage, false);
    NS_ASSERTION(status == noErr, "PMSetLastPage failed");

    status = ::PMSessionBeginDocument(mPrintSession, mPrintSettings, mPageFormat);
    if (status != noErr) return NS_ERROR_ABORT;
    
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::EndDocument()
{
    ::PMSessionEndDocument(mPrintSession);
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::AbortDocument()
{
    return EndDocument();
}

NS_IMETHODIMP nsDeviceContextSpecX::BeginPage()
{
    OSStatus status = ::PMSessionBeginPage(mPrintSession, mPageFormat, NULL);
    if (status != noErr) return NS_ERROR_ABORT;
    
#ifndef MOZ_CAIRO_GFX
    ::GetPort(&mSavedPort);
    void *graphicsContext;
    status = ::PMSessionGetGraphicsContext(mPrintSession, kPMGraphicsContextQuickdraw, &graphicsContext);
    if (status != noErr)
      return NS_ERROR_ABORT;
    ::SetPort((CGrafPtr)graphicsContext);
#endif
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::EndPage()
{
    OSStatus status = ::PMSessionEndPage(mPrintSession);
    if (mSavedPort)
    {
        ::SetPort(mSavedPort);
        mSavedPort = 0;
    }
    if (status != noErr)
      return NS_ERROR_ABORT;
    return NS_OK;
}
#endif

NS_IMETHODIMP nsDeviceContextSpecX::GetPrinterResolution(double* aResolution)
{
    PMPrinter printer;
    OSStatus status = ::PMSessionGetCurrentPrinter(mPrintSession, &printer);
    if (status != noErr)
      return NS_ERROR_FAILURE;
      
    PMResolution defaultResolution;
    status = ::PMPrinterGetPrinterResolution(printer, kPMDefaultResolution, &defaultResolution);
    if (status != noErr)
      return NS_ERROR_FAILURE;
    
    *aResolution = defaultResolution.hRes;
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight)
{
    PMRect pageRect;
    ::PMGetAdjustedPageRect(mPageFormat, &pageRect);
    *aTop = pageRect.top, *aLeft = pageRect.left;
    *aBottom = pageRect.bottom, *aRight = pageRect.right;
    return NS_OK;
}

#ifdef MOZ_CAIRO_GFX
NS_IMETHODIMP nsDeviceContextSpecX::GetSurfaceForPrinter(gfxASurface **surface)
{
    
    char *filename;
    char tmpfilename[] = "/tmp/printing.XXXXXX";
    filename = mktemp(tmpfilename);

    double top, left, bottom, right;
    GetPageRect(&top, &left, &bottom, &right);

    const double width = right - left;
    const double height = bottom - top;

    nsRefPtr<gfxASurface> newSurface = new gfxQuartzPDFSurface(filename, gfxSize(width, height));

    if (!newSurface)
      return NS_ERROR_FAILURE;

    *surface = newSurface;
    NS_ADDREF(*surface);

    return NS_OK;
}
#endif
