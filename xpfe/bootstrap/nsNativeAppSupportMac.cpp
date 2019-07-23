



































#include "nsNativeAppSupport.h"
#include "nsString.h"

#include <Gestalt.h>
#include <Dialogs.h>
#include <Resources.h>
#include <TextUtils.h>
#include <ControlDefinitions.h>

#include "nsCOMPtr.h"
#include "nsNativeAppSupportBase.h"

#include "nsIAppShellService.h"
#include "nsIAppStartup.h"
#include "nsIBaseWindow.h"
#include "nsICmdLineService.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObserver.h"
#include "nsIServiceManager.h"
#include "nsIWebNavigation.h"
#include "nsIWidget.h"
#include "nsIWindowMediator.h"

#include "nsXPFEComponentsCID.h"

#define rSplashDialog 512

static Boolean VersGreaterThan4(const FSSpec *fSpec);

const OSType kNSCreator = 'MOSS';
const OSType kMozCreator = 'MOZZ';
const SInt16 kNSCanRunStrArrayID = 1000;
const SInt16 kAnotherVersionStrIndex = 1;

nsresult
GetNativeWindowPointerFromDOMWindow(nsIDOMWindowInternal *window, WindowRef *nativeWindow);

const SInt16 kNSOSVersErrsStrArrayID = 1001;

enum {
        eOSXVersTooOldErrIndex = 1,
        eOSXVersTooOldExplanationIndex,
        eContinueButtonTextIndex,
        eQuitButtonTextIndex,
        eCarbonLibVersTooOldIndex,
        eCarbonLibVersTooOldExplanationIndex
     };

class nsNativeAppSupportMac : public nsNativeAppSupportBase,
                              public nsIObserver
{
public:

    
    enum {
      eSplashPictureItem = 1,
      eSplashStatusTextItem    
    };
    
            nsNativeAppSupportMac();    
    virtual ~nsNativeAppSupportMac();

    NS_DECL_ISUPPORTS
    NS_DECL_NSINATIVEAPPSUPPORT
    NS_DECL_NSIOBSERVER

protected:

    DialogPtr mDialog;

}; 


nsNativeAppSupportMac::nsNativeAppSupportMac()
: mDialog(nsnull)
{
}


nsNativeAppSupportMac::~nsNativeAppSupportMac()
{
  HideSplashScreen();
}

NS_IMPL_ISUPPORTS2(nsNativeAppSupportMac, nsINativeAppSupport, nsIObserver)


NS_IMETHODIMP nsNativeAppSupportMac::Start(PRBool *_retval)
{
  Str255 str1;
  Str255 str2;
  SInt16 outItemHit;
  long   response = 0;
  OSErr  err = ::Gestalt (gestaltSystemVersion, &response);
  
  if ( err || response < 0x850)
  {
    ::StopAlert (5000, NULL);
    *_retval = PR_FALSE;
    return NS_ERROR_FAILURE;
  }
  
#if TARGET_CARBON
  
  
  
  if (response >= 0x00001000 && response < 0x00001010)
  {
    
    Str255 continueButtonLabel;
    Str255 quitButtonLabel;
    ::GetIndString(str1, kNSOSVersErrsStrArrayID, eOSXVersTooOldErrIndex);
    ::GetIndString(str2, kNSOSVersErrsStrArrayID, eOSXVersTooOldExplanationIndex);
    ::GetIndString(continueButtonLabel, kNSOSVersErrsStrArrayID, eContinueButtonTextIndex);
    ::GetIndString(quitButtonLabel, kNSOSVersErrsStrArrayID, eQuitButtonTextIndex);
    if (StrLength(str1) && StrLength(str1) && StrLength(continueButtonLabel) && StrLength(quitButtonLabel))
    {
      AlertStdAlertParamRec pRec;
      
      pRec.movable      = nil;
      pRec.filterProc 	= nil;
      pRec.defaultText  = continueButtonLabel;
      pRec.cancelText   = quitButtonLabel;
      pRec.otherText    = nil;
      pRec.helpButton   = nil;
      pRec.defaultButton = kAlertStdAlertOKButton;
      pRec.cancelButton  = kAlertStdAlertCancelButton;
      pRec.position      = 0;
      
      ::StandardAlert(kAlertNoteAlert, str1, str2, &pRec, &outItemHit);
      if (outItemHit == kAlertStdAlertCancelButton)
        return PR_FALSE;
    }
    else
      return PR_FALSE;
  }
  
  
  
  if (response < 0x00001000)
  {
    err = ::Gestalt (gestaltCarbonVersion, &response);
    if (err || response < 0x00000140)
    {
      
      ::GetIndString(str1, kNSOSVersErrsStrArrayID, eCarbonLibVersTooOldIndex);
      ::GetIndString(str2, kNSOSVersErrsStrArrayID, eCarbonLibVersTooOldExplanationIndex);
      if (StrLength(str1) && StrLength(str1))
      {
        ::StandardAlert(kAlertStopAlert, str1, str2, nil, &outItemHit);
      }
      return PR_FALSE;
    }
  }
#endif

  *_retval = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::Stop(PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::Quit()
{
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::EnsureProfile(nsICmdLineService *aCmdService)
{
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::ShowSplashScreen()
{
  mDialog = ::GetNewDialog(rSplashDialog, nil, (WindowPtr)-1L);
  if (!mDialog) return NS_ERROR_FAILURE;

#if TARGET_CARBON
  ::ShowWindow(GetDialogWindow(mDialog));
  ::SetPortDialogPort(mDialog);
#else 
  ::ShowWindow(mDialog);
  ::SetPort(mDialog);
#endif

  ::DrawDialog(mDialog);    
                            
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::HideSplashScreen()
{
  if (mDialog)
  {
    ::DisposeDialog( mDialog );
    mDialog = nsnull;
  }
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::GetIsServerMode(PRBool *aIsServerMode)
{
  *aIsServerMode = PR_FALSE;
  return NS_OK;;
}
NS_IMETHODIMP nsNativeAppSupportMac::SetIsServerMode(PRBool aIsServerMode)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsNativeAppSupportMac::GetShouldShowUI(PRBool *aShouldShowUI)
{
  *aShouldShowUI = PR_TRUE;
  return NS_OK;;
}

NS_IMETHODIMP nsNativeAppSupportMac::SetShouldShowUI(PRBool aShouldShowUI)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsNativeAppSupportMac::StartServerMode()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsNativeAppSupportMac::OnLastWindowClosing()
{
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportMac::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  
  
  nsCAutoString statusString;
  statusString.AssignWithConversion(someData);

  ControlHandle   staticTextControl;
  OSErr err = ::GetDialogItemAsControl(mDialog, eSplashStatusTextItem, &staticTextControl);
  if (err != noErr) return NS_OK;
  
  PRInt32   maxLen = statusString.Length();
  if (maxLen > 254)
    maxLen = 254;
  
  ::SetControlData(staticTextControl, 0, kControlStaticTextTextTag, maxLen, statusString.get());
  ::DrawOneControl(staticTextControl);
      
  
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportMac::ReOpen()
{

  PRBool haveUncollapsed = PR_FALSE;
  PRBool haveOpenWindows = PR_FALSE;
  PRBool done = PR_FALSE;
  
  nsCOMPtr<nsIWindowMediator> 
    wm(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (!wm)
  {
    return NS_ERROR_FAILURE;
  } 
  else
  {
    nsCOMPtr<nsISimpleEnumerator> windowList;
    wm->GetXULWindowEnumerator(nsnull, getter_AddRefs(windowList));
    PRBool more;
    windowList->HasMoreElements(&more);
    while (more)
    {
      nsCOMPtr<nsISupports> nextWindow = nsnull;
      windowList->GetNext(getter_AddRefs(nextWindow));
      nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(nextWindow));
		  if (!baseWindow)
		  {
        windowList->HasMoreElements(&more);
        continue;
      }
      else
      {
        haveOpenWindows = PR_TRUE;
      }

      nsCOMPtr<nsIWidget> widget = nsnull;
      baseWindow->GetMainWidget(getter_AddRefs(widget));
      if (!widget)
      {
        windowList->HasMoreElements(&more);
        continue;
      }
      WindowRef windowRef = (WindowRef)widget->GetNativeData(NS_NATIVE_DISPLAY);
      if (!::IsWindowCollapsed(windowRef))
      {
        haveUncollapsed = PR_TRUE;
        break;  
      } 
      windowList->HasMoreElements(&more);
    } 
        
    if (!haveUncollapsed)
    {
      
      nsCOMPtr<nsIDOMWindowInternal> mru = nsnull;
      wm->GetMostRecentWindow(nsnull, getter_AddRefs(mru));
            
      if (mru) 
      {        
        WindowRef mruRef = nil;
        GetNativeWindowPointerFromDOMWindow(mru, &mruRef);
        if (mruRef)
        {
          ::CollapseWindow(mruRef, FALSE);
          ::SelectWindow(mruRef);
          done = PR_TRUE;
        }
      }
      
    } 
    
    if (!haveOpenWindows && !done)
    {    
    
      NS_WARNING("trying to open new window");
      
      nsresult rv = PR_FALSE;
      nsCOMPtr<nsIAppStartup> appStartup(do_GetService(NS_APPSTARTUP_CONTRACTID));
      if (appStartup)
      {
        PRBool openedAWindow = PR_FALSE;
        appStartup->CreateStartupState(nsIAppShellService::SIZE_TO_CONTENT,
                                       nsIAppShellService::SIZE_TO_CONTENT,
                                       &openedAWindow);
      }
    }
    
  } 
  return NS_OK;
}

nsresult
GetNativeWindowPointerFromDOMWindow(nsIDOMWindowInternal *a_window, WindowRef *a_nativeWindow)
{
    *a_nativeWindow = nil;
    if (!a_window) return NS_ERROR_INVALID_ARG;
    
    nsCOMPtr<nsIWebNavigation> mruWebNav(do_GetInterface(a_window));
    if (mruWebNav)
    {
      nsCOMPtr<nsIDocShellTreeItem> mruTreeItem(do_QueryInterface(mruWebNav));
      nsCOMPtr<nsIDocShellTreeOwner> mruTreeOwner = nsnull;
      mruTreeItem->GetTreeOwner(getter_AddRefs(mruTreeOwner));
      if(mruTreeOwner)
      {
        nsCOMPtr<nsIBaseWindow> mruBaseWindow(do_QueryInterface(mruTreeOwner));
        if (mruBaseWindow)
        {
          nsCOMPtr<nsIWidget> mruWidget = nsnull;
          mruBaseWindow->GetMainWidget(getter_AddRefs(mruWidget));
          if (mruWidget)
          {
            *a_nativeWindow = (WindowRef)mruWidget->GetNativeData(NS_NATIVE_DISPLAY);
          }
        }
      }
    }
    return NS_OK;
}

#pragma mark -


nsresult NS_CreateNativeAppSupport(nsINativeAppSupport**aResult)
{
  if ( aResult )
  {  
      *aResult = new nsNativeAppSupportMac;
      if ( *aResult )
      {
          NS_ADDREF( *aResult );
          return NS_OK;
      } 
      else
      {
          return NS_ERROR_OUT_OF_MEMORY;
      }
  } 
  else
  {
      return NS_ERROR_NULL_POINTER;
  }
}



static Boolean VersGreaterThan4(const FSSpec *fSpec)
{
  Boolean result = false;
  short fRefNum = 0;
  
  ::SetResLoad(false);
  fRefNum = ::FSpOpenResFile(fSpec, fsRdPerm);
  ::SetResLoad(true);
  if (fRefNum != -1)
  {
    Handle  h;
    h = ::Get1Resource('vers', 2);
    if (h && **(unsigned short**)h >= 0x0500)
      result = true;
    ::CloseResFile(fRefNum);
  }
    
  return result;
}
