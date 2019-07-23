






































#include "nsPrintingPromptService.h"

#include "nsCOMPtr.h"

#include "nsIPrintingPromptService.h"
#include "nsIFactory.h"
#include "nsIDOMWindow.h"
#include "nsReadableUtils.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIServiceManager.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWindowWatcher.h"
#include "nsIPrintSettingsX.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAutoBuffer.h"

#include "nsPDECommon.h"


#include "nsPrintProgress.h"
#include "nsPrintProgressParams.h"
#include "nsIWebProgressListener.h"


#include <PMApplication.h>
#include <CFPlugIn.h>





static nsresult LoadPDEPlugIn()
{
    static CFPlugInRef gPDEPlugIn = nsnull;

    if (!gPDEPlugIn) {

        CFURLRef pluginsURL = ::CFBundleCopyBuiltInPlugInsURL(CFBundleGetMainBundle());
        if (pluginsURL) {
            CFURLRef thePluginURL = ::CFURLCreateCopyAppendingPathComponent(nsnull,
                                                                          pluginsURL,
                                                                          CFSTR("PrintPDE.plugin"),
                                                                          PR_FALSE);
            if (thePluginURL) {
                gPDEPlugIn = ::CFPlugInCreate(nsnull, thePluginURL);
                ::CFRelease(thePluginURL);
            }
            ::CFRelease(pluginsURL);
        }
    }
    return gPDEPlugIn ? NS_OK : NS_ERROR_FAILURE;
}

static CFDictionaryRef ExtractCustomSettingsDict(PMPrintSettings nativePrintSettings)
{
    CFDictionaryRef resultDict = nsnull;
    UInt32 bytesNeeded;
    
    OSStatus status = ::PMGetPrintSettingsExtendedData(nativePrintSettings, kAppPrintDialogAppOnlyKey, &bytesNeeded, NULL);
    if (status == noErr) {
        nsAutoBuffer<UInt8, 512> dataBuffer;
        if (dataBuffer.EnsureElemCapacity(bytesNeeded)) {           
            status = ::PMGetPrintSettingsExtendedData(nativePrintSettings, kAppPrintDialogAppOnlyKey, &bytesNeeded, dataBuffer.get());
            if (status == noErr) {
                CFDataRef xmlData = ::CFDataCreate(kCFAllocatorDefault, dataBuffer.get(), bytesNeeded);
                if (xmlData) {
                    resultDict = (CFDictionaryRef)::CFPropertyListCreateFromXMLData(
                                                        kCFAllocatorDefault,
                                                        xmlData,
                                                        kCFPropertyListImmutable,
                                                        NULL);
                    CFRelease(xmlData);
                }
            }
        }
    }
    NS_ASSERTION(resultDict, "Failed to get custom print settings dict");
    return resultDict;
}

static PRBool
GetDictionaryStringValue(CFDictionaryRef aDictionary, CFStringRef aKey, nsAString& aResult)
{
    aResult.Truncate();
    CFTypeRef dictValue;
    if ((dictValue = CFDictionaryGetValue(aDictionary, aKey)) &&
        (CFGetTypeID(dictValue) == CFStringGetTypeID()))
    {
        CFIndex stringLen = CFStringGetLength((CFStringRef)dictValue);

        nsAutoBuffer<UniChar, 256> stringBuffer;
        if (stringBuffer.EnsureElemCapacity(stringLen + 1)) {
            ::CFStringGetCharacters((CFStringRef)dictValue, CFRangeMake(0, stringLen), stringBuffer.get());
            aResult.Assign(stringBuffer.get(), stringLen);
            return PR_TRUE;
        }
    }
    return PR_FALSE;
}


static PRBool
GetDictionaryBooleanValue(CFDictionaryRef aDictionary, CFStringRef aKey, PRBool& aResult)
{
    aResult = PR_FALSE;
    CFTypeRef dictValue;
    if ((dictValue = CFDictionaryGetValue(aDictionary, aKey)) &&
        (CFGetTypeID(dictValue) == CFBooleanGetTypeID()))
    {
      aResult = CFBooleanGetValue((CFBooleanRef)dictValue);
      return PR_TRUE;
    }
    return PR_FALSE;
}

static void
SetDictionaryStringValue(CFMutableDictionaryRef aDictionary, CFStringRef aKey, const nsXPIDLString& aValue)
{
    CFStringRef cfString = CFStringCreateWithCharacters(NULL, aValue.get(), aValue.Length());
    if (cfString) {
        CFDictionaryAddValue(aDictionary, aKey, cfString);
        CFRelease(cfString);
    }
}

static void
SetDictionaryBooleanvalue(CFMutableDictionaryRef aDictionary, CFStringRef aKey, PRBool aValue)
{
    CFDictionaryAddValue(aDictionary, aKey, aValue ? kCFBooleanTrue : kCFBooleanFalse);
}





NS_IMPL_ISUPPORTS2(nsPrintingPromptService, nsIPrintingPromptService, nsIWebProgressListener)

nsPrintingPromptService::nsPrintingPromptService() :
    mWatcher(do_GetService(NS_WINDOWWATCHER_CONTRACTID))
{
}

nsPrintingPromptService::~nsPrintingPromptService()
{
}

nsresult nsPrintingPromptService::Init()
{
    return NS_OK;
}





NS_IMETHODIMP 
nsPrintingPromptService::ShowPrintDialog(nsIDOMWindow *parent, nsIWebBrowserPrint *webBrowserPrint, nsIPrintSettings *printSettings)
{
    nsresult rv;
    OSStatus status;
      
    nsCOMPtr<nsIPrintSettingsX> printSettingsX(do_QueryInterface(printSettings));
    if (!printSettingsX)
        return NS_ERROR_NO_INTERFACE;
    
    PMPrintSession  printSession;
    rv = printSettingsX->GetNativePrintSession(&printSession);
    if (NS_FAILED(rv))
        return rv;
    
    PMPageFormat    pageFormat = kPMNoPageFormat;
    rv = printSettingsX->GetPMPageFormat(&pageFormat);
    if (NS_FAILED(rv))
        return rv;

    PMPrintSettings nativePrintSettings = kPMNoPrintSettings;
    rv = printSettingsX->GetPMPrintSettings(&nativePrintSettings);
    if (NS_FAILED(rv))
        return rv;
    
    status = ::PMSessionValidatePageFormat(printSession, pageFormat, kPMDontWantBoolean);
    if (status != noErr)
        return NS_ERROR_FAILURE;
        
    
    
    status = ::PMSessionDefaultPrintSettings(printSession, nativePrintSettings);
    if (status != noErr)
        return NS_ERROR_FAILURE;
        
    ::InitCursor();
        
    rv = ::LoadPDEPlugIn();
    NS_ASSERTION(NS_SUCCEEDED(rv), "LoadPDEPlugIn() failed");
    
    
    CFMutableDictionaryRef dictToPDE = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                    (const CFDictionaryKeyCallBacks *)&kCFTypeDictionaryKeyCallBacks,
                                    (const CFDictionaryValueCallBacks *)&kCFTypeDictionaryValueCallBacks);
    NS_ASSERTION(dictToPDE, "Failed to create a CFDictionary for print settings");
    if (dictToPDE) {
        PRBool  isOn;
        PRInt16 howToEnableFrameUI;

        printSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
        SetDictionaryBooleanvalue(dictToPDE, kPDEKeyHaveSelection, isOn);
        
        printSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);
        if (howToEnableFrameUI == nsIPrintSettings::kFrameEnableAll) {
            CFDictionaryAddValue(dictToPDE, kPDEKeyHaveFrames, kCFBooleanTrue);
            CFDictionaryAddValue(dictToPDE, kPDEKeyHaveFrameSelected, kCFBooleanTrue);
        }
        else if (howToEnableFrameUI == nsIPrintSettings::kFrameEnableAsIsAndEach) {
            CFDictionaryAddValue(dictToPDE, kPDEKeyHaveFrames, kCFBooleanTrue);
            CFDictionaryAddValue(dictToPDE, kPDEKeyHaveFrameSelected, kCFBooleanFalse);
        }
        else {
            CFDictionaryAddValue(dictToPDE, kPDEKeyHaveFrames, kCFBooleanFalse);
            CFDictionaryAddValue(dictToPDE, kPDEKeyHaveFrameSelected, kCFBooleanFalse);
        }

        
        printSettings->GetShrinkToFit(&isOn);
        SetDictionaryBooleanvalue(dictToPDE, kPDEKeyShrinkToFit, isOn);

        printSettings->GetPrintBGColors(&isOn);
        SetDictionaryBooleanvalue(dictToPDE, kPDEKeyPrintBGColors, isOn);

        printSettings->GetPrintBGImages(&isOn);
        SetDictionaryBooleanvalue(dictToPDE, kPDEKeyPrintBGImages, isOn);

        
        nsXPIDLString tempString;
        printSettings->GetHeaderStrRight(getter_Copies(tempString));
        SetDictionaryStringValue(dictToPDE, kPDEKeyHeaderRight, tempString);
        
        printSettings->GetHeaderStrCenter(getter_Copies(tempString));
        SetDictionaryStringValue(dictToPDE, kPDEKeyHeaderCenter, tempString);

        printSettings->GetHeaderStrLeft(getter_Copies(tempString));
        SetDictionaryStringValue(dictToPDE, kPDEKeyHeaderLeft, tempString);

        
        printSettings->GetFooterStrRight(getter_Copies(tempString));
        SetDictionaryStringValue(dictToPDE, kPDEKeyFooterRight, tempString);

        printSettings->GetFooterStrCenter(getter_Copies(tempString));
        SetDictionaryStringValue(dictToPDE, kPDEKeyFooterCenter, tempString);

        printSettings->GetFooterStrLeft(getter_Copies(tempString));
        SetDictionaryStringValue(dictToPDE, kPDEKeyFooterLeft, tempString);
        
        CFDataRef xmlData = CFPropertyListCreateXMLData(kCFAllocatorDefault, dictToPDE);
        NS_ASSERTION(xmlData, "Could not create print settings CFData from CFDictionary");
        if (xmlData) {
            status = ::PMSetPrintSettingsExtendedData(nativePrintSettings, kAppPrintDialogAppOnlyKey, CFDataGetLength(xmlData), (void *)CFDataGetBytePtr(xmlData));
            NS_ASSERTION(status == noErr, "PMSetPrintSettingsExtendedData() failed");
            CFRelease(xmlData);
        }
        CFRelease(dictToPDE);
    }

    Boolean accepted;
    status = ::PMSessionPrintDialog(printSession, nativePrintSettings, pageFormat, &accepted);
    if (status == noErr && accepted) {
        int pageRange = -1;
        
        CFDictionaryRef dictFromPDE = ExtractCustomSettingsDict(nativePrintSettings);
        if (dictFromPDE) {
            
            
            PRBool printSelectionOnly;
            if (GetDictionaryBooleanValue(dictFromPDE, kPDEKeyPrintSelection, printSelectionOnly)) {
                if (printSelectionOnly) {
                    printSettings->SetPrintRange(nsIPrintSettings::kRangeSelection);
                    pageRange = nsIPrintSettings::kRangeSelection;
                }
                else {
                    printSettings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
                    pageRange = nsIPrintSettings::kRangeAllPages;
                }
            }
            
            CFTypeRef dictValue;
            if ((dictValue = CFDictionaryGetValue(dictFromPDE, kPDEKeyPrintFrameType)) &&
                (CFGetTypeID(dictValue) == CFStringGetTypeID())) {
                if (CFEqual(dictValue, kPDEValueFramesAsIs))
                    printSettings->SetPrintFrameType(nsIPrintSettings::kFramesAsIs);
                else if (CFEqual(dictValue, kPDEValueSelectedFrame))
                    printSettings->SetPrintFrameType(nsIPrintSettings::kSelectedFrame);
                else if (CFEqual(dictValue, kPDEValueEachFrameSep))
                    printSettings->SetPrintFrameType(nsIPrintSettings::kEachFrameSep);
            }

            PRBool tempBool;
            if (GetDictionaryBooleanValue(dictFromPDE, kPDEKeyShrinkToFit, tempBool))
                printSettings->SetShrinkToFit(tempBool);
            
            if (GetDictionaryBooleanValue(dictFromPDE, kPDEKeyPrintBGColors, tempBool))
                printSettings->SetPrintBGColors(tempBool);

            if (GetDictionaryBooleanValue(dictFromPDE, kPDEKeyPrintBGImages, tempBool))
                printSettings->SetPrintBGImages(tempBool);

            nsAutoString stringFromDict;
            
            
            if (GetDictionaryStringValue(dictFromPDE, kPDEKeyHeaderLeft, stringFromDict))
                printSettings->SetHeaderStrLeft(stringFromDict.get());

            if (GetDictionaryStringValue(dictFromPDE, kPDEKeyHeaderCenter, stringFromDict))
                printSettings->SetHeaderStrCenter(stringFromDict.get());

            if (GetDictionaryStringValue(dictFromPDE, kPDEKeyHeaderRight, stringFromDict))
                printSettings->SetHeaderStrRight(stringFromDict.get());

            
            if (GetDictionaryStringValue(dictFromPDE, kPDEKeyFooterLeft, stringFromDict))
                printSettings->SetFooterStrLeft(stringFromDict.get());

            if (GetDictionaryStringValue(dictFromPDE, kPDEKeyFooterCenter, stringFromDict))
                printSettings->SetFooterStrCenter(stringFromDict.get());

            if (GetDictionaryStringValue(dictFromPDE, kPDEKeyFooterRight, stringFromDict))
                printSettings->SetFooterStrRight(stringFromDict.get());
        
            CFRelease(dictFromPDE);
        }
        
        if (pageRange == -1) {
            printSettings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
            pageRange = nsIPrintSettings::kRangeAllPages;
        }

        if (pageRange != nsIPrintSettings::kRangeSelection) {
            UInt32 firstPage, lastPage;
            status = ::PMGetFirstPage(nativePrintSettings, &firstPage);
            if (status == noErr) {
                status = ::PMGetLastPage(nativePrintSettings, &lastPage);
                if (status == noErr && lastPage != LONG_MAX) {
                    printSettings->SetPrintRange(nsIPrintSettings::kRangeSpecifiedPageRange);
                    printSettings->SetStartPageRange(firstPage);
                    printSettings->SetEndPageRange(lastPage);
                }
            }
        }
    }
        
    if (!accepted)
        return NS_ERROR_ABORT;

    if (status != noErr)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP 
nsPrintingPromptService::ShowProgress(nsIDOMWindow*            parent, 
                                      nsIWebBrowserPrint*      webBrowserPrint,    
                                      nsIPrintSettings*        printSettings,      
                                      nsIObserver*             openDialogObserver, 
                                      PRBool                   isForPrinting,
                                      nsIWebProgressListener** webProgressListener,
                                      nsIPrintProgressParams** printProgressParams,
                                      PRBool*                  notifyOnOpen)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsPrintingPromptService::ShowPageSetup(nsIDOMWindow *parent, nsIPrintSettings *printSettings, nsIObserver *aObs)
{
  nsCOMPtr<nsIPrintSettingsX> printSettingsX(do_QueryInterface(printSettings));
  if (!printSettingsX)
    return NS_ERROR_NO_INTERFACE;
  
  OSStatus status;
    
  PMPrintSession printSession;
  status = ::PMCreateSession(&printSession);
  if (status != noErr)
    return NS_ERROR_FAILURE;
    
  PMPageFormat pageFormat;
  printSettingsX->GetPMPageFormat(&pageFormat);
  if (pageFormat == kPMNoPageFormat) {
    ::PMRelease(printSession);
    return NS_ERROR_FAILURE;
  }
    
  Boolean validated;
  ::PMSessionValidatePageFormat(printSession, pageFormat, &validated);

  ::InitCursor();

  Boolean   accepted = false;
  status = ::PMSessionPageSetupDialog(printSession, pageFormat, &accepted);
  OSStatus tempStatus = ::PMRelease(printSession);
  if (status == noErr)
    status = tempStatus;
    
  if (status != noErr)
    return NS_ERROR_FAILURE;    
  if (!accepted)
    return NS_ERROR_ABORT;

  return NS_OK;
}

NS_IMETHODIMP 
nsPrintingPromptService::ShowPrinterProperties(nsIDOMWindow *parent, const PRUnichar *printerName, nsIPrintSettings *printSettings)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP 
nsPrintingPromptService::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
    if ((aStateFlags & STATE_STOP) && mWebProgressListener) {
        mWebProgressListener->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus);
        if (mPrintProgress) 
          mPrintProgress->CloseProgressDialog(PR_TRUE);
        mPrintProgress       = nsnull;
        mWebProgressListener = nsnull;
    }
    return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
    if (mWebProgressListener) {
      return mWebProgressListener->OnProgressChange(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress);
    }
    return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    if (mWebProgressListener) {
        return mWebProgressListener->OnLocationChange(aWebProgress, aRequest, location);
    }
    return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
    if (mWebProgressListener) {
        return mWebProgressListener->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
    }
    return NS_OK;
}


NS_IMETHODIMP 
nsPrintingPromptService::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    if (mWebProgressListener) {
        return mWebProgressListener->OnSecurityChange(aWebProgress, aRequest, state);
    }
    return NS_OK;
}
