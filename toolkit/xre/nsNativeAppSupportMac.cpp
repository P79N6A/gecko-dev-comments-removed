




































#include "nsString.h"

#include <Carbon/Carbon.h>

#include "nsCOMPtr.h"
#include "nsNativeAppSupportBase.h"

#include "nsIAppShellService.h"
#include "nsIAppStartup.h"
#include "nsIBaseWindow.h"
#include "nsICommandLineRunner.h"
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

class nsNativeAppSupportMac : public nsNativeAppSupportBase
{
public:
  nsNativeAppSupportMac() :
    mCanShowUI(PR_FALSE) { }

  NS_IMETHOD Start(PRBool* aRetVal);
  NS_IMETHOD ReOpen();
  NS_IMETHOD Enable();

private:
  PRBool mCanShowUI;

};

NS_IMETHODIMP
nsNativeAppSupportMac::Enable()
{
  mCanShowUI = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP nsNativeAppSupportMac::Start(PRBool *_retval)
{
  Str255 str1;
  Str255 str2;
  SInt16 outItemHit;
  long response = 0;
  OSErr err = ::Gestalt (gestaltSystemVersion, &response);

  
  
  
  if ((err != noErr) || response < 0x00001030)
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
    else {
      return PR_FALSE;
    }
  }

  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsNativeAppSupportMac::ReOpen()
{
  if (!mCanShowUI)
    return NS_ERROR_FAILURE;

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
      char* argv[] = { nsnull };
    
      
      nsCOMPtr<nsICommandLineRunner> cmdLine
        (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
      NS_ENSURE_TRUE(cmdLine, NS_ERROR_FAILURE);

      nsresult rv;
      rv = cmdLine->Init(0, argv, nsnull,
                         nsICommandLine::STATE_REMOTE_EXPLICIT);
      NS_ENSURE_SUCCESS(rv, rv);

      return cmdLine->Run();
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
  *aResult = new nsNativeAppSupportMac;
  if (!*aResult) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF( *aResult );
  return NS_OK;
}
