







































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
#include "nsIPrintSettingsMac.h"
#include "nsComponentResContext.h"
#include "nsWatchTask.h"


#include "nsPrintProgress.h"
#include "nsPrintProgressParams.h"
#include "nsIWebProgressListener.h"


#include <Printing.h>
#include <Dialogs.h>
#include <Appearance.h>
#include <Resources.h>


static const char *kPrintProgressDialogURL = "chrome://global/content/printProgress.xul";








#define DITL_ADDITIONS  128

enum {
  ePrintSelectionCheckboxID = 1,
  ePrintFrameAsIsCheckboxID,
  ePrintSelectedFrameCheckboxID,
  ePrintAllFramesCheckboxID,
  eDrawFrameID
};

typedef struct dialog_item_struct {
  Handle  handle;       
  Rect    bounds;         
  char    type;           
  char    data[1];        
} DialogItem, *DialogItemPtr, **DialogItemHandle;
 
typedef struct append_item_list_struct {
  short max_index;      
  DialogItem  items[1]; 
} ItemList, *ItemListPtr, **ItemListHandle;




static TPPrDlg          gPrtJobDialog;                 
static long             prFirstItem;                  
static PItemUPP         prPItemProc;                  
static PRBool           gPrintSelection;
static PItemUPP         gPrtJobDialogItemProc;
static UserItemUPP      gDrawListUPP = nsnull;
static nsIPrintSettings	*gPrintSettings = nsnull;







static pascal void MyBBoxDraw(WindowPtr theWindow, short aItemNo)
{
  short   itemType;
  Rect    itemBox;
  Handle  itemH;

  ::GetDialogItem((DialogPtr)gPrtJobDialog, prFirstItem + eDrawFrameID-1, &itemType, &itemH, &itemBox);
  
  
  if ((long)DrawThemeSecondaryGroup != kUnresolvedCFragSymbolAddress)
    ::DrawThemeSecondaryGroup(&itemBox, kThemeStateActive);
  else
    ::FrameRect(&itemBox);
}






static pascal void MyJobItems(DialogPtr aDialog, short aItemNo)
{
short   myItem, firstItem, i, itemType;
short   value;
Rect    itemBox;
Handle  itemH;

  firstItem = prFirstItem;
  
  myItem = aItemNo-firstItem+1;
  if (myItem>0) {
    switch (myItem) {
      case ePrintSelectionCheckboxID:
        ::GetDialogItem(aDialog, firstItem, &itemType, &itemH, &itemBox);
        gPrintSelection = !gPrintSelection;
        ::SetControlValue((ControlHandle)itemH, gPrintSelection);
        break;

      case ePrintFrameAsIsCheckboxID:
      case ePrintSelectedFrameCheckboxID:
      case ePrintAllFramesCheckboxID:
        for (i=ePrintFrameAsIsCheckboxID; i<=ePrintAllFramesCheckboxID; i++){
          ::GetDialogItem(aDialog, firstItem+i-1, &itemType, &itemH, &itemBox);
          ::SetControlValue((ControlHandle)itemH, i==myItem);
        }
        break;
        
      default:
        break;
    }
  } else {
    
    CallPItemProc(prPItemProc, aDialog, aItemNo);
    
    if (((TPPrDlg)aDialog)->fDone)
    {
      
      if (gPrintSettings)
      {
        
        ::GetDialogItem(aDialog, firstItem+ePrintSelectionCheckboxID-1, &itemType, &itemH, &itemBox);
        value = ::GetControlValue((ControlHandle)itemH);
        if (1==value){
          gPrintSettings->SetPrintRange(nsIPrintSettings::kRangeSelection);
        } else {
          gPrintSettings->SetPrintRange(nsIPrintSettings::kRangeAllPages);
        }
        
        
        ::GetDialogItem(aDialog, firstItem+ePrintFrameAsIsCheckboxID-1, &itemType, &itemH, &itemBox);
        value = ::GetControlValue((ControlHandle)itemH);
        if (1==value){
          gPrintSettings->SetPrintFrameType(nsIPrintSettings::kFramesAsIs);
        }
        
        
        ::GetDialogItem(aDialog, firstItem+ePrintSelectedFrameCheckboxID-1, &itemType, &itemH, &itemBox);
        value = ::GetControlValue((ControlHandle)itemH);
        if (1==value){
          gPrintSettings->SetPrintFrameType(nsIPrintSettings::kSelectedFrame);
        }
        
        
        ::GetDialogItem(aDialog, firstItem+ePrintAllFramesCheckboxID-1, &itemType, &itemH, &itemBox);
        value = ::GetControlValue((ControlHandle)itemH);
        if (1==value){
          gPrintSettings->SetPrintFrameType(nsIPrintSettings::kEachFrameSep);
        }        
      }
    }
  }
}





static PRInt32  AppendToDialog(TPPrDlg  aDialog, PRInt32  aDITLID)
{
  short           firstItem;
  ItemListHandle  myAppendDITLH;
  ItemListHandle  dlg_Item_List;

  dlg_Item_List = (ItemListHandle)((DialogPeek)aDialog)->items;
  firstItem = (**dlg_Item_List).max_index+2;

  nsComponentResourceContext resContext;
  if (resContext.BecomeCurrent()) { 
    myAppendDITLH = (ItemListHandle)::GetResource('DITL', aDITLID);
    NS_ASSERTION(myAppendDITLH, "Failed to get DITL items");
    if (myAppendDITLH) {
      ::AppendDITL((DialogPtr)aDialog, (Handle)myAppendDITLH, appendDITLBottom);
      ::ReleaseResource((Handle) myAppendDITLH);
    }
  }

  return firstItem;
}






static pascal TPPrDlg MyJobDlgInit(THPrint aHPrint)
{
  PRInt32 i;
  short   itemType;
  Handle  itemH;
  Rect    itemBox;
  PRBool  isOn;
  PRInt16 howToEnableFrameUI = nsIPrintSettings::kFrameEnableNone;

  prFirstItem = AppendToDialog(gPrtJobDialog, DITL_ADDITIONS);

  if (gPrintSettings) {
    gPrintSettings->GetPrintOptions(nsIPrintSettings::kEnableSelectionRB, &isOn);
    gPrintSettings->GetHowToEnableFrameUI(&howToEnableFrameUI);
  }

  ::GetDialogItem((DialogPtr) gPrtJobDialog, prFirstItem+ePrintSelectionCheckboxID-1, &itemType, &itemH, &itemBox);
  if ( isOn ) {
    ::HiliteControl((ControlHandle)itemH, 0);
  } else {
    ::HiliteControl((ControlHandle)itemH, 255); 
  }
  
  gPrintSelection = PR_FALSE;
  ::SetControlValue((ControlHandle) itemH, gPrintSelection);

  if (howToEnableFrameUI == nsIPrintSettings::kFrameEnableAll) {
    for (i = ePrintFrameAsIsCheckboxID; i <= ePrintAllFramesCheckboxID; i++){
      ::GetDialogItem((DialogPtr) gPrtJobDialog, prFirstItem+i-1, &itemType, &itemH, &itemBox);
      ::SetControlValue((ControlHandle) itemH, (i==4));
      ::HiliteControl((ControlHandle)itemH, 0);
    }
  }
  else if (howToEnableFrameUI == nsIPrintSettings::kFrameEnableAsIsAndEach) {
    for (i = ePrintFrameAsIsCheckboxID; i <= ePrintAllFramesCheckboxID; i++){
      ::GetDialogItem((DialogPtr) gPrtJobDialog, prFirstItem+i-1, &itemType, &itemH, &itemBox);
      ::SetControlValue((ControlHandle) itemH, (i==4));
      if ( i == 3){
        ::HiliteControl((ControlHandle)itemH, 255);
      }
    }
  }
  else {
    for (i = ePrintFrameAsIsCheckboxID; i <= ePrintAllFramesCheckboxID; i++){
      ::GetDialogItem((DialogPtr) gPrtJobDialog, prFirstItem+i-1, &itemType, &itemH, &itemBox);
      ::SetControlValue((ControlHandle) itemH, FALSE);
      ::HiliteControl((ControlHandle)itemH, 255); 
    }
  }
  
  
  prPItemProc = gPrtJobDialog->pItemProc;
  gPrtJobDialog->pItemProc = gPrtJobDialogItemProc = NewPItemUPP(MyJobItems);


  
  gDrawListUPP = NewUserItemProc(MyBBoxDraw);
  ::GetDialogItem((DialogPtr)gPrtJobDialog, prFirstItem+eDrawFrameID-1, &itemType, &itemH, &itemBox);
  ::SetDialogItem((DialogPtr)gPrtJobDialog, prFirstItem+eDrawFrameID-1, itemType, (Handle)gDrawListUPP, &itemBox);

  return gPrtJobDialog;
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
  THPrint     printRecH = nsnull;    
  GrafPtr     oldport;
  PDlgInitUPP theInitProcPtr;

	gPrintSettings = printSettings;

  ::GetPort(&oldport);
  
  nsresult rv;
  nsCOMPtr<nsIPrintSettingsMac> printSettingsMac(do_QueryInterface(printSettings));
  if (!printSettingsMac)
    return NS_ERROR_NO_INTERFACE;

  theInitProcPtr = NewPDlgInitProc(MyJobDlgInit);
  if (!theInitProcPtr)
    return NS_ERROR_FAILURE;
      
  
  rv = printSettingsMac->GetTHPrint(&printRecH);
  if (NS_FAILED(rv))
    return rv;

  
  ::PrOpen();
  if (::PrError() != noErr) {
    ::DisposeHandle((Handle)printRecH);
    return NS_ERROR_FAILURE;
  }
  
  
  ::PrValidate(printRecH);
  if (::PrError() != noErr) {
    ::DisposeHandle((Handle)printRecH);
    ::PrClose();
    return NS_ERROR_FAILURE;
  }
  
  
  gPrtJobDialog = ::PrJobInit(printRecH);
  if (::PrError() != noErr) {
    ::DisposeHandle((Handle)printRecH);
    ::PrClose();
    return NS_ERROR_FAILURE;
  }

  
  theInitProcPtr = NewPDlgInitProc(MyJobDlgInit);
  if (!theInitProcPtr)
    return NS_ERROR_FAILURE;      

  nsWatchTask::GetTask().Suspend();
  ::InitCursor();
	
  
  if (::PrDlgMain(printRecH, theInitProcPtr))
  {
    
    rv = NS_OK;
    printSettingsMac->SetTHPrint(printRecH);
  }
  else
  {
    
    ::SetPort(oldport); 
    rv = NS_ERROR_ABORT;
  }
  
  ::DisposeHandle((Handle)printRecH);
  
  
  DisposePItemUPP(gPrtJobDialogItemProc);
  gPrtJobDialogItemProc = nsnull;
  
  DisposePItemUPP(theInitProcPtr);
  DisposePItemUPP(gDrawListUPP);
  gDrawListUPP = nsnull;
      
  nsWatchTask::GetTask().Resume();
  return rv;
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
    NS_ENSURE_ARG(webProgressListener);
    NS_ENSURE_ARG(printProgressParams);
    NS_ENSURE_ARG(notifyOnOpen);

    *notifyOnOpen = PR_FALSE;

    nsPrintProgress* prtProgress = new nsPrintProgress();
    nsresult rv = prtProgress->QueryInterface(NS_GET_IID(nsIPrintProgress), (void**)getter_AddRefs(mPrintProgress));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = prtProgress->QueryInterface(NS_GET_IID(nsIWebProgressListener), (void**)getter_AddRefs(mWebProgressListener));
    NS_ENSURE_SUCCESS(rv, rv);

    nsPrintProgressParams* prtProgressParams = new nsPrintProgressParams();
    rv = prtProgressParams->QueryInterface(NS_GET_IID(nsIPrintProgressParams), (void**)printProgressParams);
    NS_ENSURE_SUCCESS(rv, rv);

    if (printProgressParams) 
    {
        if (mWatcher) 
        {
            nsCOMPtr<nsIDOMWindow> active;
            mWatcher->GetActiveWindow(getter_AddRefs(active));
            nsCOMPtr<nsIDOMWindowInternal> parent(do_QueryInterface(active));
            mPrintProgress->OpenProgressDialog(parent, kPrintProgressDialogURL, *printProgressParams, openDialogObserver, notifyOnOpen);
        }
    }

    *webProgressListener = NS_STATIC_CAST(nsIWebProgressListener*, this);
    NS_ADDREF(*webProgressListener);

    return rv;
}

NS_IMETHODIMP 
nsPrintingPromptService::ShowPageSetup(nsIDOMWindow *parent, nsIPrintSettings *printSettings, nsIObserver *aObs)
{
  nsCOMPtr<nsIPrintSettingsMac> printSettingsMac(do_QueryInterface(printSettings));
  if (!printSettingsMac)
    return NS_ERROR_NO_INTERFACE;

  
  ::PrOpen();
  if(::PrError() != noErr)
    return NS_ERROR_FAILURE;
  
  THPrint printRecH;
  nsresult rv;
  
  rv = printSettingsMac->GetTHPrint(&printRecH);
  if (NS_FAILED(rv))
    return rv;
    
  ::PrValidate(printRecH);
  NS_ASSERTION(::PrError() == noErr, "PrValidate error");

  nsWatchTask::GetTask().Suspend();
  ::InitCursor();
  Boolean   dialogOK = ::PrStlDialog(printRecH);		
  nsWatchTask::GetTask().Resume();
  
  OSErr err = ::PrError();
  ::PrClose();

  if (dialogOK)
    rv = printSettingsMac->SetTHPrint(printRecH);
      
  if (err != noErr)
    return NS_ERROR_FAILURE;
  if (!dialogOK)
    return NS_ERROR_ABORT;
    
  return rv;
}

NS_IMETHODIMP 
nsPrintingPromptService::ShowPrinterProperties(nsIDOMWindow *parent, const PRUnichar *printerName, nsIPrintSettings *printSettings)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP 
nsPrintingPromptService::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
    if ((aStateFlags & STATE_STOP) && mWebProgressListener) 
    {
        mWebProgressListener->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus);
        if (mPrintProgress) 
        {
            mPrintProgress->CloseProgressDialog(PR_TRUE);
        }
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
