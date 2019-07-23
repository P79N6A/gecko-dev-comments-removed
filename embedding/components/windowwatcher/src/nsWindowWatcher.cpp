








































#include "nsWindowWatcher.h"

#include "nsAutoLock.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsPrompt.h"
#include "nsPromptService.h"
#include "nsWWJSUtils.h"
#include "plstr.h"

#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocumentLoader.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMModalContentWindow.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScreen.h"
#include "nsIScreenManager.h"
#include "nsIScriptContext.h"
#include "nsIGenericFactory.h"
#include "nsIJSContextStack.h"
#include "nsIObserverService.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsXPCOM.h"
#include "nsIURI.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIWebNavigation.h"
#include "nsIWindowCreator.h"
#include "nsIWindowCreator2.h"
#include "nsIXPConnect.h"
#include "nsPIDOMWindow.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIWindowProvider.h"
#include "nsIMutableArray.h"
#include "nsISupportsArray.h"
#include "nsIDeviceContext.h"
#include "nsIDOMStorageObsolete.h"
#include "nsIDOMStorage.h"
#include "nsPIDOMStorage.h"
#include "nsIWidget.h"
#include "nsFocusManager.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#include "jsinterp.h" 

#ifdef USEWEAKREFS
#include "nsIWeakReference.h"
#endif

static const char *sJSStackContractID="@mozilla.org/js/xpc/ContextStack;1";





class nsWindowWatcher;

struct nsWatcherWindowEntry {

  nsWatcherWindowEntry(nsIDOMWindow *inWindow, nsIWebBrowserChrome *inChrome) {
#ifdef USEWEAKREFS
    mWindow = do_GetWeakReference(inWindow);
#else
    mWindow = inWindow;
#endif
    nsCOMPtr<nsISupportsWeakReference> supportsweak(do_QueryInterface(inChrome));
    if (supportsweak) {
      supportsweak->GetWeakReference(getter_AddRefs(mChromeWeak));
    } else {
      mChrome = inChrome;
      mChromeWeak = 0;
    }
    ReferenceSelf();
  }
  ~nsWatcherWindowEntry() {}

  void InsertAfter(nsWatcherWindowEntry *inOlder);
  void Unlink();
  void ReferenceSelf();

#ifdef USEWEAKREFS
  nsCOMPtr<nsIWeakReference> mWindow;
#else 
  nsIDOMWindow              *mWindow;
#endif
  nsIWebBrowserChrome       *mChrome;
  nsWeakPtr                  mChromeWeak;
  
  nsWatcherWindowEntry      *mYounger, 
                            *mOlder;
};

void nsWatcherWindowEntry::InsertAfter(nsWatcherWindowEntry *inOlder)
{
  if (inOlder) {
    mOlder = inOlder;
    mYounger = inOlder->mYounger;
    mOlder->mYounger = this;
    if (mOlder->mOlder == mOlder)
      mOlder->mOlder = this;
    mYounger->mOlder = this;
    if (mYounger->mYounger == mYounger)
      mYounger->mYounger = this;
  }
}

void nsWatcherWindowEntry::Unlink() {

  mOlder->mYounger = mYounger;
  mYounger->mOlder = mOlder;
  ReferenceSelf();
}

void nsWatcherWindowEntry::ReferenceSelf() {

  mYounger = this;
  mOlder = this;
}





class nsWatcherWindowEnumerator : public nsISimpleEnumerator {

public:
  nsWatcherWindowEnumerator(nsWindowWatcher *inWatcher);
  virtual ~nsWatcherWindowEnumerator();
  NS_IMETHOD HasMoreElements(PRBool *retval);
  NS_IMETHOD GetNext(nsISupports **retval);

  NS_DECL_ISUPPORTS

private:
  friend class nsWindowWatcher;

  nsWatcherWindowEntry *FindNext();
  void WindowRemoved(nsWatcherWindowEntry *inInfo);

  nsWindowWatcher      *mWindowWatcher;
  nsWatcherWindowEntry *mCurrentPosition;
};

NS_IMPL_ADDREF(nsWatcherWindowEnumerator)
NS_IMPL_RELEASE(nsWatcherWindowEnumerator)
NS_IMPL_QUERY_INTERFACE1(nsWatcherWindowEnumerator, nsISimpleEnumerator)

nsWatcherWindowEnumerator::nsWatcherWindowEnumerator(nsWindowWatcher *inWatcher)
  : mWindowWatcher(inWatcher),
    mCurrentPosition(inWatcher->mOldestWindow)
{
  mWindowWatcher->AddEnumerator(this);
  mWindowWatcher->AddRef();
}

nsWatcherWindowEnumerator::~nsWatcherWindowEnumerator()
{
  mWindowWatcher->RemoveEnumerator(this);
  mWindowWatcher->Release();
}

NS_IMETHODIMP
nsWatcherWindowEnumerator::HasMoreElements(PRBool *retval)
{
  if (!retval)
    return NS_ERROR_INVALID_ARG;

  *retval = mCurrentPosition? PR_TRUE : PR_FALSE;
  return NS_OK;
}
    
NS_IMETHODIMP
nsWatcherWindowEnumerator::GetNext(nsISupports **retval)
{
  if (!retval)
    return NS_ERROR_INVALID_ARG;

  *retval = NULL;

#ifdef USEWEAKREFS
  while (mCurrentPosition) {
    CallQueryReferent(mCurrentPosition->mWindow, retval);
    if (*retval) {
      mCurrentPosition = FindNext();
      break;
    } else 
      mWindowWatcher->RemoveWindow(mCurrentPosition);
  }
  NS_IF_ADDREF(*retval);
#else
  if (mCurrentPosition) {
    CallQueryInterface(mCurrentPosition->mWindow, retval);
    mCurrentPosition = FindNext();
  }
#endif
  return NS_OK;
}

nsWatcherWindowEntry *
nsWatcherWindowEnumerator::FindNext()
{
  nsWatcherWindowEntry *info;

  if (!mCurrentPosition)
    return 0;

  info = mCurrentPosition->mYounger;
  return info == mWindowWatcher->mOldestWindow ? 0 : info;
}


void nsWatcherWindowEnumerator::WindowRemoved(nsWatcherWindowEntry *inInfo) {

  if (mCurrentPosition == inInfo)
    mCurrentPosition = mCurrentPosition != inInfo->mYounger ?
                       inInfo->mYounger : 0;
}





class NS_STACK_CLASS JSContextAutoPopper {
public:
  JSContextAutoPopper();
  ~JSContextAutoPopper();

  nsresult   Push(JSContext *cx = nsnull);
  JSContext *get() { return mContext; }

protected:
  nsCOMPtr<nsIThreadJSContextStack>  mService;
  JSContext                         *mContext;
  nsCOMPtr<nsIScriptContext>         mContextKungFuDeathGrip;
};

JSContextAutoPopper::JSContextAutoPopper() : mContext(nsnull)
{
}

JSContextAutoPopper::~JSContextAutoPopper()
{
  JSContext *cx;
  nsresult   rv;

  if(mContext) {
    rv = mService->Pop(&cx);
    NS_ASSERTION(NS_SUCCEEDED(rv) && cx == mContext, "JSContext push/pop mismatch");
  }
}

nsresult JSContextAutoPopper::Push(JSContext *cx)
{
  if (mContext) 
    return NS_ERROR_FAILURE;

  mService = do_GetService(sJSStackContractID);
  if(mService) {
    
    if (!cx && NS_FAILED(mService->GetSafeJSContext(&cx))) {
      cx = nsnull;
    }

    
    if (cx && NS_SUCCEEDED(mService->Push(cx))) {
      mContext = cx;
      mContextKungFuDeathGrip = nsWWJSUtils::GetDynamicScriptContext(cx);
    }
  }
  return mContext ? NS_OK : NS_ERROR_FAILURE;
}





NS_IMPL_ADDREF(nsWindowWatcher)
NS_IMPL_RELEASE(nsWindowWatcher)
NS_IMPL_QUERY_INTERFACE4(nsWindowWatcher,
                         nsIWindowWatcher,
                         nsIPromptFactory,
                         nsIAuthPromptAdapterFactory,
                         nsPIWindowWatcher)

nsWindowWatcher::nsWindowWatcher() :
        mEnumeratorList(),
        mOldestWindow(0),
        mListLock(0)
{
}

nsWindowWatcher::~nsWindowWatcher()
{
  
  while (mOldestWindow)
    RemoveWindow(mOldestWindow);

  if (mListLock)
    PR_DestroyLock(mListLock);
}

nsresult
nsWindowWatcher::Init()
{
  mListLock = PR_NewLock();
  if (!mListLock)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::OpenWindow(nsIDOMWindow *aParent,
                            const char *aUrl,
                            const char *aName,
                            const char *aFeatures,
                            nsISupports *aArguments,
                            nsIDOMWindow **_retval)
{
  nsCOMPtr<nsIArray> argsArray;
  PRUint32 argc = 0;
  if (aArguments) {
    
    
    
    nsresult rv;

    nsCOMPtr<nsISupportsArray> supArray(do_QueryInterface(aArguments));
    if (!supArray) {
      nsCOMPtr<nsIArray> array(do_QueryInterface(aArguments));
      if (!array) {
        nsCOMPtr<nsIMutableArray> muteArray;
        argsArray = muteArray = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
        if (NS_FAILED(rv))
          return rv;
        rv = muteArray->AppendElement(aArguments, PR_FALSE);
        if (NS_FAILED(rv))
          return rv;
        argc = 1;
      } else {
        rv = array->GetLength(&argc);
        if (NS_FAILED(rv))
          return rv;
        if (argc > 0)
          argsArray = array;
      }
    } else {
      
      rv = supArray->Count(&argc);
      if (NS_FAILED(rv))
        return rv;
      
      
      if (argc > 0) {
        nsCOMPtr<nsIMutableArray> muteArray;
        argsArray = muteArray = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
        if (NS_FAILED(rv))
          return rv;
        for (PRUint32 i = 0; i < argc; i++) {
          nsCOMPtr<nsISupports> elt(dont_AddRef(supArray->ElementAt(i)));
          rv = muteArray->AppendElement(elt, PR_FALSE);
          if (NS_FAILED(rv))
            return rv;
        }
      }
    }
  }

  PRBool dialog = (argc != 0);
  return OpenWindowJSInternal(aParent, aUrl, aName, aFeatures, dialog, 
                              argsArray, PR_FALSE, _retval);
}

struct SizeSpec {
  SizeSpec() :
    mLeftSpecified(PR_FALSE),
    mTopSpecified(PR_FALSE),
    mOuterWidthSpecified(PR_FALSE),
    mOuterHeightSpecified(PR_FALSE),
    mInnerWidthSpecified(PR_FALSE),
    mInnerHeightSpecified(PR_FALSE),
    mUseDefaultWidth(PR_FALSE),
    mUseDefaultHeight(PR_FALSE)
  {}
  
  PRInt32 mLeft;
  PRInt32 mTop;
  PRInt32 mOuterWidth;  
  PRInt32 mOuterHeight; 
  PRInt32 mInnerWidth;  
  PRInt32 mInnerHeight; 

  PRPackedBool mLeftSpecified;
  PRPackedBool mTopSpecified;
  PRPackedBool mOuterWidthSpecified;
  PRPackedBool mOuterHeightSpecified;
  PRPackedBool mInnerWidthSpecified;
  PRPackedBool mInnerHeightSpecified;

  
  
  PRPackedBool mUseDefaultWidth;
  PRPackedBool mUseDefaultHeight;

  PRBool PositionSpecified() const {
    return mLeftSpecified || mTopSpecified;
  }
  
  PRBool SizeSpecified() const {
    return mOuterWidthSpecified || mOuterHeightSpecified ||
      mInnerWidthSpecified || mInnerHeightSpecified;
  }
};

NS_IMETHODIMP
nsWindowWatcher::OpenWindowJS(nsIDOMWindow *aParent,
                              const char *aUrl,
                              const char *aName,
                              const char *aFeatures,
                              PRBool aDialog,
                              nsIArray *argv,
                              nsIDOMWindow **_retval)
{
  if (argv) {
    PRUint32 argc;
    nsresult rv = argv->GetLength(&argc);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (argc == 0)
      argv = nsnull;
  }

  return OpenWindowJSInternal(aParent, aUrl, aName, aFeatures, aDialog,
                              argv, PR_TRUE, _retval);
}

nsresult
nsWindowWatcher::OpenWindowJSInternal(nsIDOMWindow *aParent,
                                      const char *aUrl,
                                      const char *aName,
                                      const char *aFeatures,
                                      PRBool aDialog,
                                      nsIArray *argv,
                                      PRBool aCalledFromJS,
                                      nsIDOMWindow **_retval)
{
  nsresult                        rv = NS_OK;
  PRBool                          nameSpecified,
                                  featuresSpecified,
                                  isNewToplevelWindow = PR_FALSE,
                                  windowIsNew = PR_FALSE,
                                  windowNeedsName = PR_FALSE,
                                  windowIsModal = PR_FALSE,
                                  uriToLoadIsChrome = PR_FALSE,
                                  windowIsModalContentDialog = PR_FALSE;
  PRUint32                        chromeFlags;
  nsAutoString                    name;             
  nsCAutoString                   features;         
  nsCOMPtr<nsIURI>                uriToLoad;        
  nsCOMPtr<nsIDocShellTreeOwner>  parentTreeOwner;  
  nsCOMPtr<nsIDocShellTreeItem>   newDocShellItem;  
  JSContextAutoPopper             callerContextGuard;

  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;

  GetWindowTreeOwner(aParent, getter_AddRefs(parentTreeOwner));

  if (aUrl) {
    rv = URIfromURL(aUrl, aParent, getter_AddRefs(uriToLoad));
    if (NS_FAILED(rv))
      return rv;
    uriToLoad->SchemeIs("chrome", &uriToLoadIsChrome);
  }

  nameSpecified = PR_FALSE;
  if (aName) {
    CopyUTF8toUTF16(aName, name);
#ifdef DEBUG
    CheckWindowName(name);
#endif
    nameSpecified = PR_TRUE;
  }

  featuresSpecified = PR_FALSE;
  if (aFeatures) {
    features.Assign(aFeatures);
    featuresSpecified = PR_TRUE;
    features.StripWhitespace();
  }

  
  nsCOMPtr<nsIDOMWindow> foundWindow;
  SafeGetWindowByName(name, aParent, getter_AddRefs(foundWindow));
  GetWindowTreeItem(foundWindow, getter_AddRefs(newDocShellItem));

  

  nsCOMPtr<nsIDOMChromeWindow> chromeParent(do_QueryInterface(aParent));

  
  
  
  
  chromeFlags = CalculateChromeFlags(features.get(), featuresSpecified,
                                     aDialog, uriToLoadIsChrome,
                                     !aParent || chromeParent);

  
  
  
  if (!aCalledFromJS && argv &&
      WinHasOption(features.get(), "-moz-internal-modal", 0, nsnull)) {
    windowIsModalContentDialog = PR_TRUE;

    chromeFlags |= nsIWebBrowserChrome::CHROME_MODAL;
  }

  SizeSpec sizeSpec;
  CalcSizeSpec(features.get(), sizeSpec);

  nsCOMPtr<nsIScriptSecurityManager>
    sm(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));

  
  
  
  nsCOMPtr<nsIPrincipal> callerPrincipal;
  rv = sm->GetSubjectPrincipal(getter_AddRefs(callerPrincipal));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isCallerChrome = PR_TRUE;
  if (callerPrincipal) {
    rv = sm->IsSystemPrincipal(callerPrincipal, &isCallerChrome);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  JSContext *cx = GetJSContextFromWindow(aParent);

  if (isCallerChrome && !chromeParent && cx) {
    
    
    
    

    callerContextGuard.Push(cx);
  }

  if (!newDocShellItem) {
    
    
    
    windowNeedsName = PR_TRUE;

    
    
    
    nsCOMPtr<nsIDOMChromeWindow> chromeWin = do_QueryInterface(aParent);
    if (!aDialog && !chromeWin &&
        !(chromeFlags & (nsIWebBrowserChrome::CHROME_MODAL         |
                         nsIWebBrowserChrome::CHROME_OPENAS_DIALOG | 
                         nsIWebBrowserChrome::CHROME_OPENAS_CHROME))) {
      nsCOMPtr<nsIWindowProvider> provider = do_GetInterface(parentTreeOwner);
      if (provider) {
        NS_ASSERTION(aParent, "We've _got_ to have a parent here!");

        nsCOMPtr<nsIDOMWindow> newWindow;
        rv = provider->ProvideWindow(aParent, chromeFlags,
                                     sizeSpec.PositionSpecified(),
                                     sizeSpec.SizeSpecified(),
                                     uriToLoad, name, features, &windowIsNew,
                                     getter_AddRefs(newWindow));
        if (NS_SUCCEEDED(rv)) {
          GetWindowTreeItem(newWindow, getter_AddRefs(newDocShellItem));
          if (windowIsNew && newDocShellItem) {
            
            
            
            
            nsCOMPtr<nsIWebNavigation> webNav =
              do_QueryInterface(newDocShellItem);
            webNav->Stop(nsIWebNavigation::STOP_NETWORK);
          }
        }
      }
    }
  }
  
  if (!newDocShellItem) {
    windowIsNew = PR_TRUE;
    isNewToplevelWindow = PR_TRUE;

    nsCOMPtr<nsIWebBrowserChrome> parentChrome(do_GetInterface(parentTreeOwner));

    
    PRBool weAreModal = (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL) != 0;
    if (!weAreModal && parentChrome)
      parentChrome->IsWindowModal(&weAreModal);

    if (weAreModal) {
      windowIsModal = PR_TRUE;
      
      chromeFlags |= nsIWebBrowserChrome::CHROME_MODAL |
        nsIWebBrowserChrome::CHROME_DEPENDENT;
    }

    
    
    
    
    if (!chromeParent && (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL)) {
      PRBool parentVisible = PR_TRUE;
      nsCOMPtr<nsIBaseWindow> parentWindow(do_GetInterface(parentTreeOwner));
      nsCOMPtr<nsIWidget> parentWidget;
      if (parentWindow)
        parentWindow->GetMainWidget(getter_AddRefs(parentWidget));
      if (parentWidget)
        parentWidget->IsVisible(parentVisible);
      if (!parentVisible) {
        return NS_ERROR_NOT_AVAILABLE;
      }
    }

    NS_ASSERTION(mWindowCreator,
                 "attempted to open a new window with no WindowCreator");
    rv = NS_ERROR_FAILURE;
    if (mWindowCreator) {
      nsCOMPtr<nsIWebBrowserChrome> newChrome;

      






      nsCOMPtr<nsIWindowCreator2> windowCreator2(do_QueryInterface(mWindowCreator));
      if (windowCreator2) {
        PRUint32 contextFlags = 0;
        PRBool popupConditions = PR_FALSE;

        
        nsCOMPtr<nsPIDOMWindow> piWindow(do_QueryInterface(aParent));
        if (piWindow)
          popupConditions = piWindow->IsLoadingOrRunningTimeout();

        
        if (popupConditions) {
          popupConditions = !isCallerChrome;
        }

        if (popupConditions)
          contextFlags |= nsIWindowCreator2::PARENT_IS_LOADING_OR_RUNNING_TIMEOUT;

        PRBool cancel = PR_FALSE;
        rv = windowCreator2->CreateChromeWindow2(parentChrome, chromeFlags,
                                                 contextFlags, uriToLoad,
                                                 &cancel,
                                                 getter_AddRefs(newChrome));
        if (NS_SUCCEEDED(rv) && cancel) {
          newChrome = 0; 
          rv = NS_ERROR_ABORT;
        }
      }
      else
        rv = mWindowCreator->CreateChromeWindow(parentChrome, chromeFlags,
                                                getter_AddRefs(newChrome));
      if (newChrome) {
        


        nsCOMPtr<nsIDOMWindow> newWindow(do_GetInterface(newChrome));
        if (newWindow)
          GetWindowTreeItem(newWindow, getter_AddRefs(newDocShellItem));
        if (!newDocShellItem)
          newDocShellItem = do_GetInterface(newChrome);
        if (!newDocShellItem)
          rv = NS_ERROR_FAILURE;
      }
    }
  }

  
  if (!newDocShellItem)
    return rv;

  nsCOMPtr<nsIDocShell> newDocShell(do_QueryInterface(newDocShellItem));
  NS_ENSURE_TRUE(newDocShell, NS_ERROR_UNEXPECTED);
  
  rv = ReadyOpenedDocShellItem(newDocShellItem, aParent, windowIsNew, _retval);
  if (NS_FAILED(rv))
    return rv;

  





  if (isNewToplevelWindow) {
    


    if (PL_strcasestr(features.get(), "width=") || PL_strcasestr(features.get(), "height=")) {

      nsCOMPtr<nsIDocShellTreeOwner> newTreeOwner;
      newDocShellItem->GetTreeOwner(getter_AddRefs(newTreeOwner));
      if (newTreeOwner)
        newTreeOwner->SetPersistence(PR_FALSE, PR_FALSE, PR_FALSE);
    }
  }

  if ((aDialog || windowIsModalContentDialog) && argv) {
    
    nsCOMPtr<nsPIDOMWindow> piwin(do_QueryInterface(*_retval));
    NS_ENSURE_TRUE(piwin, NS_ERROR_UNEXPECTED);

    rv = piwin->SetArguments(argv, callerPrincipal);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  


  if (windowNeedsName)
    newDocShellItem->SetName(nameSpecified &&
                             !name.LowerCaseEqualsLiteral("_blank") ?
                             name.get() : nsnull);


  
  
  
  
  
  
  nsCOMPtr<nsIContentViewer> newCV;
  newDocShell->GetContentViewer(getter_AddRefs(newCV));
  nsCOMPtr<nsIMarkupDocumentViewer> newMuCV = do_QueryInterface(newCV);
  if (newMuCV) {
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    GetWindowTreeItem(aParent, getter_AddRefs(parentItem));

    if (aCalledFromJS) {
      nsCOMPtr<nsIDocShellTreeItem> callerItem = GetCallerTreeItem(parentItem);
      nsCOMPtr<nsPIDOMWindow> callerWin = do_GetInterface(callerItem);
      if (callerWin) {
        nsCOMPtr<nsIDocument> doc =
          do_QueryInterface(callerWin->GetExtantDocument());
        if (doc) {
          newMuCV->SetDefaultCharacterSet(doc->GetDocumentCharacterSet());
        }
      }
    }
    else {
      nsCOMPtr<nsIDocShell> parentDocshell = do_QueryInterface(parentItem);
      
      if (parentDocshell) {
        nsCOMPtr<nsIContentViewer> parentCV;
        parentDocshell->GetContentViewer(getter_AddRefs(parentCV));
        nsCOMPtr<nsIMarkupDocumentViewer> parentMuCV =
          do_QueryInterface(parentCV);
        if (parentMuCV) {
          nsCAutoString charset;
          nsresult res = parentMuCV->GetDefaultCharacterSet(charset);
          if (NS_SUCCEEDED(res)) {
            newMuCV->SetDefaultCharacterSet(charset);
          }
          res = parentMuCV->GetPrevDocCharacterSet(charset);
          if (NS_SUCCEEDED(res)) {
            newMuCV->SetPrevDocCharacterSet(charset);
          }
        }
      }
    }
  }

  if (isNewToplevelWindow) {
    
    
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService("@mozilla.org/observer-service;1");
    if (obsSvc) {
      obsSvc->NotifyObservers(*_retval, "toplevel-window-ready", nsnull);
    }
  }

  
  
  
  
  
  
  
  nsCOMPtr<nsIPrincipal> subjectPrincipal;
  if (NS_FAILED(sm->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal)))) {
    subjectPrincipal = nsnull;
  }

  if (windowIsNew) {
    
    
    
    
    
    nsIPrincipal* newWindowPrincipal = subjectPrincipal;
    if (!newWindowPrincipal && aParent) {
      nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(aParent));
      if (sop) {
        newWindowPrincipal = sop->GetPrincipal();
      }
    }

    PRBool isSystem;
    rv = sm->IsSystemPrincipal(newWindowPrincipal, &isSystem);
    if (NS_FAILED(rv) || isSystem) {
      
      PRInt32 itemType;
      rv = newDocShellItem->GetItemType(&itemType);
      if (NS_FAILED(rv) || itemType != nsIDocShellTreeItem::typeChrome) {
        newWindowPrincipal = nsnull;        
      }
    }

    nsCOMPtr<nsPIDOMWindow> newWindow = do_QueryInterface(*_retval);
#ifdef DEBUG
    nsCOMPtr<nsPIDOMWindow> newDebugWindow = do_GetInterface(newDocShell);
    NS_ASSERTION(newWindow == newDebugWindow, "Different windows??");
#endif
    if (newWindow) {
      newWindow->SetOpenerScriptPrincipal(newWindowPrincipal);
    }
  }

  if (uriToLoad) { 
    JSContextAutoPopper contextGuard;

    cx = GetJSContextFromCallStack();

    
    if (!cx)
      cx = GetJSContextFromWindow(aParent);
    if (!cx) {
      rv = contextGuard.Push();
      if (NS_FAILED(rv))
        return rv;
      cx = contextGuard.get();
    }

    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    newDocShell->CreateLoadInfo(getter_AddRefs(loadInfo));
    NS_ENSURE_TRUE(loadInfo, NS_ERROR_FAILURE);

    if (subjectPrincipal) {
      loadInfo->SetOwner(subjectPrincipal);
    }

    

    
    nsCOMPtr<nsIJSContextStack> stack = do_GetService(sJSStackContractID);

    JSContext* ccx = nsnull;

    
    if (stack && NS_SUCCEEDED(stack->Peek(&ccx)) && ccx) {
      nsIScriptGlobalObject *sgo = nsWWJSUtils::GetDynamicScriptGlobal(ccx);

      nsCOMPtr<nsPIDOMWindow> w(do_QueryInterface(sgo));
      if (w) {
        





        nsCOMPtr<nsIDocument> doc(do_QueryInterface(w->GetExtantDocument()));
        if (doc) { 
          
          loadInfo->SetReferrer(doc->GetDocumentURI());
        }
      }
    }

    newDocShell->LoadURI(uriToLoad, loadInfo,
      windowIsNew ? nsIWebNavigation::LOAD_FLAGS_FIRST_LOAD :
                    nsIWebNavigation::LOAD_FLAGS_NONE, PR_TRUE);
  }

  
  nsCOMPtr<nsPIDOMWindow> piWindow = do_QueryInterface(aParent);
  nsIDocShell* parentDocShell = nsnull;
  if (piWindow)
    parentDocShell = piWindow->GetDocShell();

  if (subjectPrincipal && parentDocShell) {
    nsCOMPtr<nsIDOMStorage> storage;
    parentDocShell->GetSessionStorageForPrincipal(subjectPrincipal, PR_FALSE,
                                                  getter_AddRefs(storage));
    nsCOMPtr<nsPIDOMStorage> piStorage =
      do_QueryInterface(storage);
    if (piStorage){
      storage = piStorage->Clone();
      newDocShell->AddSessionStorage(
        piStorage->Principal(),
        storage);
    }
  }

  if (isNewToplevelWindow)
    SizeOpenedDocShellItem(newDocShellItem, aParent, sizeSpec);

  
  if (windowIsModal || windowIsModalContentDialog) {
    nsCOMPtr<nsIDocShellTreeOwner> newTreeOwner;
    newDocShellItem->GetTreeOwner(getter_AddRefs(newTreeOwner));
    nsCOMPtr<nsIWebBrowserChrome> newChrome(do_GetInterface(newTreeOwner));

    
    
    NS_ENSURE_TRUE(newChrome, NS_ERROR_NOT_AVAILABLE);

    nsCOMPtr<nsPIDOMWindow> modalContentWindow;

    
    
    
    

    if (windowIsModalContentDialog) {
      modalContentWindow = do_QueryInterface(*_retval);
    }

    nsAutoWindowStateHelper windowStateHelper(aParent);

    if (!windowStateHelper.DefaultEnabled()) {
      
      NS_RELEASE(*_retval);

      return NS_OK;
    }

    
    
    
    nsAutoPopupStatePusher popupStatePusher(modalContentWindow, openAbused);

    newChrome->ShowAsModal();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::RegisterNotification(nsIObserver *aObserver)
{
  
  nsresult rv;

  if (!aObserver)
    return NS_ERROR_INVALID_ARG;
  
  nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1", &rv));
  if (os) {
    rv = os->AddObserver(aObserver, "domwindowopened", PR_FALSE);
    if (NS_SUCCEEDED(rv))
      rv = os->AddObserver(aObserver, "domwindowclosed", PR_FALSE);
  }
  return rv;
}

NS_IMETHODIMP
nsWindowWatcher::UnregisterNotification(nsIObserver *aObserver)
{
  
  nsresult rv;

  if (!aObserver)
    return NS_ERROR_INVALID_ARG;
  
  nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1", &rv));
  if (os) {
    os->RemoveObserver(aObserver, "domwindowopened");
    os->RemoveObserver(aObserver, "domwindowclosed");
  }
  return rv;
}

NS_IMETHODIMP
nsWindowWatcher::GetWindowEnumerator(nsISimpleEnumerator** _retval)
{
  if (!_retval)
    return NS_ERROR_INVALID_ARG;

  nsAutoLock lock(mListLock);
  nsWatcherWindowEnumerator *enumerator = new nsWatcherWindowEnumerator(this);
  if (enumerator)
    return CallQueryInterface(enumerator, _retval);

  return NS_ERROR_OUT_OF_MEMORY;
}
    
NS_IMETHODIMP
nsWindowWatcher::GetNewPrompter(nsIDOMWindow *aParent, nsIPrompt **_retval)
{
  return NS_NewPrompter(_retval, aParent);
}

NS_IMETHODIMP
nsWindowWatcher::GetNewAuthPrompter(nsIDOMWindow *aParent, nsIAuthPrompt **_retval)
{
  return NS_NewAuthPrompter(_retval, aParent);
}

NS_IMETHODIMP
nsWindowWatcher::GetPrompt(nsIDOMWindow *aParent, const nsIID& aIID,
                           void **_retval)
{
  if (aIID.Equals(NS_GET_IID(nsIPrompt)))
    return NS_NewPrompter(reinterpret_cast<nsIPrompt**>(_retval), aParent);
  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)))
    return NS_NewAuthPrompter(reinterpret_cast<nsIAuthPrompt**>(_retval),
                              aParent);
  if (aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    nsresult rv = NS_NewAuthPrompter2(reinterpret_cast<nsIAuthPrompt2**>
                                                      (_retval),
                                      aParent);
    if (rv == NS_NOINTERFACE) {
      
      nsCOMPtr<nsIAuthPrompt> prompt;
      rv = NS_NewAuthPrompter(getter_AddRefs(prompt), aParent);
      if (NS_SUCCEEDED(rv)) {
        NS_WrapAuthPrompt(prompt,
                          reinterpret_cast<nsIAuthPrompt2**>(_retval));
        if (!*_retval)
          rv = NS_ERROR_NOT_AVAILABLE;
      }
    }

    return rv;
  }

  return NS_NOINTERFACE;
}

NS_IMETHODIMP
nsWindowWatcher::CreateAdapter(nsIAuthPrompt* aPrompt, nsIAuthPrompt2** _retval)
{
  *_retval = new AuthPromptWrapper(aPrompt);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::SetWindowCreator(nsIWindowCreator *creator)
{
  mWindowCreator = creator; 
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::GetActiveWindow(nsIDOMWindow **aActiveWindow)
{
  *aActiveWindow = nsnull;
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm)
    return fm->GetActiveWindow(aActiveWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::SetActiveWindow(nsIDOMWindow *aActiveWindow)
{
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm)
    return fm->SetActiveWindow(aActiveWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::AddWindow(nsIDOMWindow *aWindow, nsIWebBrowserChrome *aChrome)
{
  nsresult rv;

  if (!aWindow)
    return NS_ERROR_INVALID_ARG;

#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aWindow));

    NS_ASSERTION(win->IsOuterWindow(),
                 "Uh, the active window must be an outer window!");
  }
#endif

  {
    nsWatcherWindowEntry *info;
    nsAutoLock lock(mListLock);

    
    
    info = FindWindowEntry(aWindow);
    if (info) {
      nsCOMPtr<nsISupportsWeakReference> supportsweak(do_QueryInterface(aChrome));
      if (supportsweak) {
        supportsweak->GetWeakReference(getter_AddRefs(info->mChromeWeak));
      } else {
        info->mChrome = aChrome;
        info->mChromeWeak = 0;
      }
      return NS_OK;
    }
  
    
    info = new nsWatcherWindowEntry(aWindow, aChrome);
    if (!info)
      return NS_ERROR_OUT_OF_MEMORY;

    if (mOldestWindow)
      info->InsertAfter(mOldestWindow->mOlder);
    else
      mOldestWindow = info;
  } 

  
  
  nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1", &rv));
  if (os) {
    nsCOMPtr<nsISupports> domwin(do_QueryInterface(aWindow));
    rv = os->NotifyObservers(domwin, "domwindowopened", 0);
  }

  return rv;
}

NS_IMETHODIMP
nsWindowWatcher::RemoveWindow(nsIDOMWindow *aWindow)
{
  

  if (!aWindow)
    return NS_ERROR_INVALID_ARG;

  nsWatcherWindowEntry *info = FindWindowEntry(aWindow);
  if (info) {
    RemoveWindow(info);
    return NS_OK;
  }
  NS_WARNING("requested removal of nonexistent window");
  return NS_ERROR_INVALID_ARG;
}

nsWatcherWindowEntry *
nsWindowWatcher::FindWindowEntry(nsIDOMWindow *aWindow)
{
  
  nsWatcherWindowEntry *info,
                       *listEnd;
#ifdef USEWEAKREFS
  nsresult    rv;
  PRBool      found;
#endif

  info = mOldestWindow;
  listEnd = 0;
#ifdef USEWEAKREFS
  rv = NS_OK;
  found = PR_FALSE;
  while (info != listEnd && NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDOMWindow> infoWindow(do_QueryReferent(info->mWindow));
    if (!infoWindow) { 
      rv = RemoveWindow(info);
    }
    else if (infoWindow.get() == aWindow)
      return info;

    info = info->mYounger;
    listEnd = mOldestWindow;
  }
  return 0;
#else
  while (info != listEnd) {
    if (info->mWindow == aWindow)
      return info;
    info = info->mYounger;
    listEnd = mOldestWindow;
  }
  return 0;
#endif
}

nsresult nsWindowWatcher::RemoveWindow(nsWatcherWindowEntry *inInfo)
{
  PRUint32  ctr,
            count = mEnumeratorList.Length();
  nsresult rv;

  {
    
    nsAutoLock lock(mListLock);
    for (ctr = 0; ctr < count; ++ctr) 
      mEnumeratorList[ctr]->WindowRemoved(inInfo);

    
    if (inInfo == mOldestWindow)
      mOldestWindow = inInfo->mYounger == mOldestWindow ? 0 : inInfo->mYounger;
    inInfo->Unlink();
  }

  
  
  nsCOMPtr<nsIObserverService> os(do_GetService("@mozilla.org/observer-service;1", &rv));
  if (os) {
#ifdef USEWEAKREFS
    nsCOMPtr<nsISupports> domwin(do_QueryReferent(inInfo->mWindow));
    if (domwin)
      rv = os->NotifyObservers(domwin, "domwindowclosed", 0);
    
#else
    nsCOMPtr<nsISupports> domwin(do_QueryInterface(inInfo->mWindow));
    rv = os->NotifyObservers(domwin, "domwindowclosed", 0);
#endif
  }

  delete inInfo;
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::GetChromeForWindow(nsIDOMWindow *aWindow, nsIWebBrowserChrome **_retval)
{
  if (!aWindow || !_retval)
    return NS_ERROR_INVALID_ARG;
  *_retval = 0;

  nsAutoLock lock(mListLock);
  nsWatcherWindowEntry *info = FindWindowEntry(aWindow);
  if (info) {
    if (info->mChromeWeak != nsnull) {
      return info->mChromeWeak->
                            QueryReferent(NS_GET_IID(nsIWebBrowserChrome),
                                          reinterpret_cast<void**>(_retval));
    }
    *_retval = info->mChrome;
    NS_IF_ADDREF(*_retval);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWindowWatcher::GetWindowByName(const PRUnichar *aTargetName, 
                                 nsIDOMWindow *aCurrentWindow,
                                 nsIDOMWindow **aResult)
{
  if (!aResult) {
    return NS_ERROR_INVALID_ARG;
  }

  *aResult = nsnull;

  nsCOMPtr<nsIDocShellTreeItem> treeItem;

  nsCOMPtr<nsIDocShellTreeItem> startItem;
  GetWindowTreeItem(aCurrentWindow, getter_AddRefs(startItem));
  if (startItem) {
    
    startItem->FindItemWithName(aTargetName, nsnull, nsnull,
                                getter_AddRefs(treeItem));
  }
  else {
    
    FindItemWithName(aTargetName, nsnull, nsnull, getter_AddRefs(treeItem));
  }

  nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(treeItem);
  domWindow.swap(*aResult);

  return NS_OK;
}

PRBool
nsWindowWatcher::AddEnumerator(nsWatcherWindowEnumerator* inEnumerator)
{
  
  return mEnumeratorList.AppendElement(inEnumerator) != nsnull;
}

PRBool
nsWindowWatcher::RemoveEnumerator(nsWatcherWindowEnumerator* inEnumerator)
{
  
  return mEnumeratorList.RemoveElement(inEnumerator);
}

nsresult
nsWindowWatcher::URIfromURL(const char *aURL,
                            nsIDOMWindow *aParent,
                            nsIURI **aURI)
{
  nsCOMPtr<nsIDOMWindow> baseWindow;

  


  JSContext *cx = GetJSContextFromCallStack();
  if (cx) {
    nsIScriptContext *scriptcx = nsWWJSUtils::GetDynamicScriptContext(cx);
    if (scriptcx) {
      baseWindow = do_QueryInterface(scriptcx->GetGlobalObject());
    }
  }

  
  if (!baseWindow)
    baseWindow = aParent;

  

  nsIURI *baseURI = nsnull;

  
  if (baseWindow) {
    nsCOMPtr<nsIDOMDocument> domDoc;
    baseWindow->GetDocument(getter_AddRefs(domDoc));
    if (domDoc) {
      nsCOMPtr<nsIDocument> doc;
      doc = do_QueryInterface(domDoc);
      if (doc) {
        baseURI = doc->GetBaseURI();
      }
    }
  }

  
  return NS_NewURI(aURI, aURL, baseURI);
}

#ifdef DEBUG



void nsWindowWatcher::CheckWindowName(nsString& aName)
{
  nsReadingIterator<PRUnichar> scan;
  nsReadingIterator<PRUnichar> endScan;

  aName.EndReading(endScan);
  for (aName.BeginReading(scan); scan != endScan; ++scan)
    if (!nsCRT::IsAsciiAlpha(*scan) && !nsCRT::IsAsciiDigit(*scan) &&
        *scan != '_') {

      
      
      nsCAutoString warn;
      warn.AssignLiteral("Illegal character in window name ");
      AppendUTF16toUTF8(aName, warn);
      NS_WARNING(warn.get());
      break;
    }
}
#endif 

#define NS_CALCULATE_CHROME_FLAG_FOR(feature, flag)               \
    prefBranch->GetBoolPref(feature, &forceEnable);               \
    if (forceEnable && !(aDialog && isChrome) &&                  \
        !(isChrome && aHasChromeParent) && !aChromeURL) {         \
      chromeFlags |= flag;                                        \
    } else {                                                      \
      chromeFlags |= WinHasOption(aFeatures, feature,             \
                                  0, &presenceFlag)               \
                     ? flag : 0;                                  \
    }










PRUint32 nsWindowWatcher::CalculateChromeFlags(const char *aFeatures,
                                               PRBool aFeaturesSpecified,
                                               PRBool aDialog,
                                               PRBool aChromeURL,
                                               PRBool aHasChromeParent)
{
   if(!aFeaturesSpecified || !aFeatures) {
      if(aDialog)
         return nsIWebBrowserChrome::CHROME_ALL | 
                nsIWebBrowserChrome::CHROME_OPENAS_DIALOG | 
                nsIWebBrowserChrome::CHROME_OPENAS_CHROME;
      else
         return nsIWebBrowserChrome::CHROME_ALL;
   }

  







  PRUint32 chromeFlags = 0;
  PRBool presenceFlag = PR_FALSE;

  chromeFlags = nsIWebBrowserChrome::CHROME_WINDOW_BORDERS;
  if (aDialog && WinHasOption(aFeatures, "all", 0, &presenceFlag))
    chromeFlags = nsIWebBrowserChrome::CHROME_ALL;

  

  nsCOMPtr<nsIScriptSecurityManager>
    securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(securityManager, NS_ERROR_FAILURE);

  PRBool isChrome = PR_FALSE;
  securityManager->SubjectPrincipalIsSystem(&isChrome);

  nsCOMPtr<nsIPrefBranch> prefBranch;
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  rv = prefs->GetBranch("dom.disable_window_open_feature.", getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, PR_TRUE);

  PRBool forceEnable = PR_FALSE;

  NS_CALCULATE_CHROME_FLAG_FOR("titlebar",
                               nsIWebBrowserChrome::CHROME_TITLEBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("close",
                               nsIWebBrowserChrome::CHROME_WINDOW_CLOSE);
  NS_CALCULATE_CHROME_FLAG_FOR("toolbar",
                               nsIWebBrowserChrome::CHROME_TOOLBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("location",
                               nsIWebBrowserChrome::CHROME_LOCATIONBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("directories",
                               nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("personalbar",
                               nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("status",
                               nsIWebBrowserChrome::CHROME_STATUSBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("menubar",
                               nsIWebBrowserChrome::CHROME_MENUBAR);
  NS_CALCULATE_CHROME_FLAG_FOR("scrollbars",
                               nsIWebBrowserChrome::CHROME_SCROLLBARS);
  NS_CALCULATE_CHROME_FLAG_FOR("resizable",
                               nsIWebBrowserChrome::CHROME_WINDOW_RESIZE);
  NS_CALCULATE_CHROME_FLAG_FOR("minimizable",
                               nsIWebBrowserChrome::CHROME_WINDOW_MIN);

  chromeFlags |= WinHasOption(aFeatures, "popup", 0, &presenceFlag)
                 ? nsIWebBrowserChrome::CHROME_WINDOW_POPUP : 0; 

  






  
  if (!PL_strcasestr(aFeatures, "titlebar"))
    chromeFlags |= nsIWebBrowserChrome::CHROME_TITLEBAR;
  if (!PL_strcasestr(aFeatures, "close"))
    chromeFlags |= nsIWebBrowserChrome::CHROME_WINDOW_CLOSE;

  if (aDialog && !presenceFlag)
    chromeFlags = nsIWebBrowserChrome::CHROME_DEFAULT;

  



  if (WinHasOption(aFeatures, "alwaysLowered", 0, nsnull) ||
      WinHasOption(aFeatures, "z-lock", 0, nsnull))
    chromeFlags |= nsIWebBrowserChrome::CHROME_WINDOW_LOWERED;
  else if (WinHasOption(aFeatures, "alwaysRaised", 0, nsnull))
    chromeFlags |= nsIWebBrowserChrome::CHROME_WINDOW_RAISED;

  chromeFlags |= WinHasOption(aFeatures, "chrome", 0, nsnull) ?
    nsIWebBrowserChrome::CHROME_OPENAS_CHROME : 0;
  chromeFlags |= WinHasOption(aFeatures, "extrachrome", 0, nsnull) ?
    nsIWebBrowserChrome::CHROME_EXTRA : 0;
  chromeFlags |= WinHasOption(aFeatures, "centerscreen", 0, nsnull) ?
    nsIWebBrowserChrome::CHROME_CENTER_SCREEN : 0;
  chromeFlags |= WinHasOption(aFeatures, "dependent", 0, nsnull) ?
    nsIWebBrowserChrome::CHROME_DEPENDENT : 0;
  chromeFlags |= WinHasOption(aFeatures, "modal", 0, nsnull) ?
    (nsIWebBrowserChrome::CHROME_MODAL | nsIWebBrowserChrome::CHROME_DEPENDENT) : 0;
  chromeFlags |= WinHasOption(aFeatures, "dialog", 0, nsnull) ?
    nsIWebBrowserChrome::CHROME_OPENAS_DIALOG : 0;

  

  if (aDialog) {
    if (!PL_strcasestr(aFeatures, "dialog"))
      chromeFlags |= nsIWebBrowserChrome::CHROME_OPENAS_DIALOG;
    if (!PL_strcasestr(aFeatures, "chrome"))
      chromeFlags |= nsIWebBrowserChrome::CHROME_OPENAS_CHROME;
  }

  



  
  PRBool enabled;
  nsresult res =
    securityManager->IsCapabilityEnabled("UniversalBrowserWrite", &enabled);

  if (NS_FAILED(res) || !enabled || (isChrome && !aHasChromeParent)) {
    
    
    
    chromeFlags |= nsIWebBrowserChrome::CHROME_TITLEBAR;
    chromeFlags |= nsIWebBrowserChrome::CHROME_WINDOW_CLOSE;
    chromeFlags &= ~nsIWebBrowserChrome::CHROME_WINDOW_LOWERED;
    chromeFlags &= ~nsIWebBrowserChrome::CHROME_WINDOW_RAISED;
    chromeFlags &= ~nsIWebBrowserChrome::CHROME_WINDOW_POPUP;
    



    if (!aChromeURL)
      chromeFlags &= ~(nsIWebBrowserChrome::CHROME_MODAL |
                       nsIWebBrowserChrome::CHROME_OPENAS_CHROME);
  }

  if (!(chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)) {
    
    chromeFlags &= ~nsIWebBrowserChrome::CHROME_DEPENDENT;
  }

  return chromeFlags;
}


PRInt32
nsWindowWatcher::WinHasOption(const char *aOptions, const char *aName,
                              PRInt32 aDefault, PRBool *aPresenceFlag)
{
  if (!aOptions)
    return 0;

  char *comma, *equal;
  PRInt32 found = 0;

#ifdef DEBUG
    nsCAutoString options(aOptions);
    NS_ASSERTION(options.FindCharInSet(" \n\r\t") == kNotFound, 
                  "There should be no whitespace in this string!");
#endif

  while (PR_TRUE) {
    comma = PL_strchr(aOptions, ',');
    if (comma)
      *comma = '\0';
    equal = PL_strchr(aOptions, '=');
    if (equal)
      *equal = '\0';
    if (nsCRT::strcasecmp(aOptions, aName) == 0) {
      if (aPresenceFlag)
        *aPresenceFlag = PR_TRUE;
      if (equal)
        if (*(equal + 1) == '*')
          found = aDefault;
        else if (nsCRT::strcasecmp(equal + 1, "yes") == 0)
          found = 1;
        else
          found = atoi(equal + 1);
      else
        found = 1;
    }
    if (equal)
      *equal = '=';
    if (comma)
      *comma = ',';
    if (found || !comma)
      break;
    aOptions = comma + 1;
  }
  return found;
}





NS_IMETHODIMP
nsWindowWatcher::FindItemWithName(const PRUnichar* aName,
                                  nsIDocShellTreeItem* aRequestor,
                                  nsIDocShellTreeItem* aOriginalRequestor,
                                  nsIDocShellTreeItem** aFoundItem)
{
  *aFoundItem = 0;

  
  if(!aName || !*aName)
    return NS_OK;

  nsDependentString name(aName);
  
  nsCOMPtr<nsISimpleEnumerator> windows;
  GetWindowEnumerator(getter_AddRefs(windows));
  if (!windows)
    return NS_ERROR_FAILURE;

  PRBool   more;
  nsresult rv = NS_OK;

  do {
    windows->HasMoreElements(&more);
    if (!more)
      break;
    nsCOMPtr<nsISupports> nextSupWindow;
    windows->GetNext(getter_AddRefs(nextSupWindow));
    nsCOMPtr<nsIDOMWindow> nextWindow(do_QueryInterface(nextSupWindow));
    if (nextWindow) {
      nsCOMPtr<nsIDocShellTreeItem> treeItem;
      GetWindowTreeItem(nextWindow, getter_AddRefs(treeItem));
      if (treeItem) {
        
        
        nsCOMPtr<nsIDocShellTreeItem> root;
        treeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
        NS_ASSERTION(root, "Must have root tree item of same type");
        
        if (root != aRequestor) {
          
          
          
          nsCOMPtr<nsIDocShellTreeOwner> rootOwner;
          
          
          
          
          
          if (aRequestor) {
            root->GetTreeOwner(getter_AddRefs(rootOwner));
          }
          rv = root->FindItemWithName(aName, rootOwner, aOriginalRequestor,
                                      aFoundItem);
          if (NS_FAILED(rv) || *aFoundItem || !aRequestor)
            break;
        }
      }
    }
  } while(1);

  return rv;
}

already_AddRefed<nsIDocShellTreeItem>
nsWindowWatcher::GetCallerTreeItem(nsIDocShellTreeItem* aParentItem)
{
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService(sJSStackContractID);

  JSContext *cx = nsnull;

  if (stack) {
    stack->Peek(&cx);
  }

  nsIDocShellTreeItem* callerItem = nsnull;

  if (cx) {
    nsCOMPtr<nsIWebNavigation> callerWebNav =
      do_GetInterface(nsWWJSUtils::GetDynamicScriptGlobal(cx));

    if (callerWebNav) {
      CallQueryInterface(callerWebNav, &callerItem);
    }
  }

  if (!callerItem) {
    NS_IF_ADDREF(callerItem = aParentItem);
  }

  return callerItem;
}

nsresult
nsWindowWatcher::SafeGetWindowByName(const nsAString& aName,
                                     nsIDOMWindow* aCurrentWindow,
                                     nsIDOMWindow** aResult)
{
  *aResult = nsnull;
  
  nsCOMPtr<nsIDocShellTreeItem> startItem;
  GetWindowTreeItem(aCurrentWindow, getter_AddRefs(startItem));

  nsCOMPtr<nsIDocShellTreeItem> callerItem = GetCallerTreeItem(startItem);

  const nsAFlatString& flatName = PromiseFlatString(aName);

  nsCOMPtr<nsIDocShellTreeItem> foundItem;
  if (startItem) {
    startItem->FindItemWithName(flatName.get(), nsnull, callerItem,
                                getter_AddRefs(foundItem));
  }
  else {
    FindItemWithName(flatName.get(), nsnull, callerItem,
                     getter_AddRefs(foundItem));
  }

  nsCOMPtr<nsIDOMWindow> foundWin = do_GetInterface(foundItem);
  foundWin.swap(*aResult);
  return NS_OK;
}






nsresult
nsWindowWatcher::ReadyOpenedDocShellItem(nsIDocShellTreeItem *aOpenedItem,
                                         nsIDOMWindow        *aParent,
                                         PRBool              aWindowIsNew,
                                         nsIDOMWindow        **aOpenedWindow)
{
  nsresult rv = NS_ERROR_FAILURE;

  *aOpenedWindow = 0;
  nsCOMPtr<nsPIDOMWindow> piOpenedWindow(do_GetInterface(aOpenedItem));
  if (piOpenedWindow) {
    if (aParent) {
      nsCOMPtr<nsIDOMWindowInternal> internalParent(do_QueryInterface(aParent));
      piOpenedWindow->SetOpenerWindow(internalParent, aWindowIsNew); 

      if (aWindowIsNew) {
#ifdef DEBUG
        
        
        
        nsCOMPtr<nsIDocumentLoader> docloader =
          do_QueryInterface(aOpenedItem);
        NS_ASSERTION(docloader, "How can we not have a docloader here?");

        nsCOMPtr<nsIChannel> chan;
        docloader->GetDocumentChannel(getter_AddRefs(chan));
        NS_ASSERTION(!chan, "Why is there a document channel?");
#endif

        nsCOMPtr<nsIDocument> doc =
          do_QueryInterface(piOpenedWindow->GetExtantDocument());
        if (doc) {
          doc->SetIsInitialDocument(PR_TRUE);
        }
      }
    }
    rv = CallQueryInterface(piOpenedWindow, aOpenedWindow);
  }
  return rv;
}


void
nsWindowWatcher::CalcSizeSpec(const char* aFeatures, SizeSpec& aResult)
{
  
  PRBool  present;
  PRInt32 temp;

  present = PR_FALSE;
  if ((temp = WinHasOption(aFeatures, "left", 0, &present)) || present)
    aResult.mLeft = temp;
  else if ((temp = WinHasOption(aFeatures, "screenX", 0, &present)) || present)
    aResult.mLeft = temp;
  aResult.mLeftSpecified = present;

  present = PR_FALSE;
  if ((temp = WinHasOption(aFeatures, "top", 0, &present)) || present)
    aResult.mTop = temp;
  else if ((temp = WinHasOption(aFeatures, "screenY", 0, &present)) || present)
    aResult.mTop = temp;
  aResult.mTopSpecified = present;

  
  if ((temp = WinHasOption(aFeatures, "outerWidth", PR_INT32_MIN, nsnull))) {
    if (temp == PR_INT32_MIN) {
      aResult.mUseDefaultWidth = PR_TRUE;
    }
    else {
      aResult.mOuterWidth = temp;
    }
    aResult.mOuterWidthSpecified = PR_TRUE;
  } else if ((temp = WinHasOption(aFeatures, "width", PR_INT32_MIN, nsnull)) ||
             (temp = WinHasOption(aFeatures, "innerWidth", PR_INT32_MIN,
                                  nsnull))) {
    if (temp == PR_INT32_MIN) {
      aResult.mUseDefaultWidth = PR_TRUE;
    } else {
      aResult.mInnerWidth = temp;
    }
    aResult.mInnerWidthSpecified = PR_TRUE;
  }

  if ((temp = WinHasOption(aFeatures, "outerHeight", PR_INT32_MIN, nsnull))) {
    if (temp == PR_INT32_MIN) {
      aResult.mUseDefaultHeight = PR_TRUE;
    }
    else {
      aResult.mOuterHeight = temp;
    }
    aResult.mOuterHeightSpecified = PR_TRUE;
  } else if ((temp = WinHasOption(aFeatures, "height", PR_INT32_MIN,
                                  nsnull)) ||
             (temp = WinHasOption(aFeatures, "innerHeight", PR_INT32_MIN,
                                  nsnull))) {
    if (temp == PR_INT32_MIN) {
      aResult.mUseDefaultHeight = PR_TRUE;
    } else {
      aResult.mInnerHeight = temp;
    }
    aResult.mInnerHeightSpecified = PR_TRUE;
  }
}






void
nsWindowWatcher::SizeOpenedDocShellItem(nsIDocShellTreeItem *aDocShellItem,
                                        nsIDOMWindow *aParent,
                                        const SizeSpec & aSizeSpec)
{
  
  PRInt32 left = 0,
          top = 0,
          width = 100,
          height = 100;
  
  PRInt32 chromeWidth = 0,
          chromeHeight = 0;
  
  PRBool  sizeChromeWidth = PR_TRUE,
          sizeChromeHeight = PR_TRUE;

  
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  aDocShellItem->GetTreeOwner(getter_AddRefs(treeOwner));
  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin(do_QueryInterface(treeOwner));
  if (!treeOwnerAsWin) 
    return;
    
  float devPixelsPerCSSPixel = 1.0;
  nsCOMPtr<nsIWidget> mainWidget;
  treeOwnerAsWin->GetMainWidget(getter_AddRefs(mainWidget));
  if (!mainWidget) {
    
    
    nsCOMPtr<nsIBaseWindow> shellWindow(do_QueryInterface(aDocShellItem));
    if (shellWindow)
      shellWindow->GetParentWidget(getter_AddRefs(mainWidget));
  }
  if (mainWidget) {
    nsCOMPtr<nsIDeviceContext> ctx = mainWidget->GetDeviceContext();
    devPixelsPerCSSPixel = float(ctx->AppUnitsPerCSSPixel()) / ctx->AppUnitsPerDevPixel();
  }

  






  treeOwnerAsWin->GetPositionAndSize(&left, &top, &width, &height);
  { 
    nsCOMPtr<nsIBaseWindow> shellWindow(do_QueryInterface(aDocShellItem));
    if (shellWindow) {
      PRInt32 cox, coy;
      shellWindow->GetSize(&cox, &coy);
      chromeWidth = width - cox;
      chromeHeight = height - coy;
    }
  }

  
  if (aSizeSpec.mLeftSpecified) {
    left = NSToIntRound(aSizeSpec.mLeft * devPixelsPerCSSPixel);
  }

  if (aSizeSpec.mTopSpecified) {
    top = NSToIntRound(aSizeSpec.mTop * devPixelsPerCSSPixel);
  }

  
  if (aSizeSpec.mOuterWidthSpecified) {
    if (!aSizeSpec.mUseDefaultWidth) {
      width = NSToIntRound(aSizeSpec.mOuterWidth * devPixelsPerCSSPixel);
    } 
  }
  else if (aSizeSpec.mInnerWidthSpecified) {
    sizeChromeWidth = PR_FALSE;
    if (aSizeSpec.mUseDefaultWidth) {
      width = width - chromeWidth;
    } else {
      width = NSToIntRound(aSizeSpec.mInnerWidth * devPixelsPerCSSPixel);
    }
  }

  
  if (aSizeSpec.mOuterHeightSpecified) {
    if (!aSizeSpec.mUseDefaultHeight) {
      height = NSToIntRound(aSizeSpec.mOuterHeight * devPixelsPerCSSPixel);
    } 
  }
  else if (aSizeSpec.mInnerHeightSpecified) {
    sizeChromeHeight = PR_FALSE;
    if (aSizeSpec.mUseDefaultHeight) {
      height = height - chromeHeight;
    } else {
      height = NSToIntRound(aSizeSpec.mInnerHeight * devPixelsPerCSSPixel);
    }
  }

  PRBool positionSpecified = aSizeSpec.PositionSpecified();
  
  nsresult res;
  PRBool enabled = PR_FALSE;

  
  nsCOMPtr<nsIScriptSecurityManager>
    securityManager(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  if (securityManager) {
    res = securityManager->IsCapabilityEnabled("UniversalBrowserWrite",
                                               &enabled);
    if (NS_FAILED(res))
      enabled = PR_FALSE;
    else if (enabled && aParent) {
      nsCOMPtr<nsIDOMChromeWindow> chromeWin(do_QueryInterface(aParent));

      PRBool isChrome = PR_FALSE;
      securityManager->SubjectPrincipalIsSystem(&isChrome);

      
      
      enabled = !(isChrome && chromeWin == nsnull);
    }
  }

  if (!enabled) {

    

    PRInt32 oldTop = top,
            oldLeft = left;

    
    nsCOMPtr<nsIScreen> screen;
    nsCOMPtr<nsIScreenManager> screenMgr(do_GetService(
                                         "@mozilla.org/gfx/screenmanager;1"));
    if (screenMgr)
      screenMgr->ScreenForRect(left, top, width, height,
                               getter_AddRefs(screen));
    if (screen) {
      PRInt32 screenLeft, screenTop, screenWidth, screenHeight;
      PRInt32 winWidth = width + (sizeChromeWidth ? 0 : chromeWidth),
              winHeight = height + (sizeChromeHeight ? 0 : chromeHeight);

      screen->GetAvailRect(&screenLeft, &screenTop,
                           &screenWidth, &screenHeight);

      if (aSizeSpec.SizeSpecified()) {
        


        if (height < 100)
          height = 100;
        if (winHeight > screenHeight)
          height = screenHeight - (sizeChromeHeight ? 0 : chromeHeight);
        if (width < 100)
          width = 100;
        if (winWidth > screenWidth)
          width = screenWidth - (sizeChromeWidth ? 0 : chromeWidth);
      }

      if (left+winWidth > screenLeft+screenWidth)
        left = screenLeft+screenWidth - winWidth;
      if (left < screenLeft)
        left = screenLeft;
      if (top+winHeight > screenTop+screenHeight)
        top = screenTop+screenHeight - winHeight;
      if (top < screenTop)
        top = screenTop;
      if (top != oldTop || left != oldLeft)
        positionSpecified = PR_TRUE;
    }
  }

  

  if (positionSpecified)
    treeOwnerAsWin->SetPosition(left, top);
  if (aSizeSpec.SizeSpecified()) {
    


    if (!sizeChromeWidth && !sizeChromeHeight)
      treeOwner->SizeShellTo(aDocShellItem, width, height);
    else {
      if (!sizeChromeWidth)
        width += chromeWidth;
      if (!sizeChromeHeight)
        height += chromeHeight;
      treeOwnerAsWin->SetSize(width, height, PR_FALSE);
    }
  }
  treeOwnerAsWin->SetVisibility(PR_TRUE);
}

void
nsWindowWatcher::GetWindowTreeItem(nsIDOMWindow *inWindow,
                                   nsIDocShellTreeItem **outTreeItem)
{
  *outTreeItem = 0;

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(inWindow));
  if (window) {
    nsIDocShell *docshell = window->GetDocShell();
    if (docshell)
      CallQueryInterface(docshell, outTreeItem);
  }
}

void
nsWindowWatcher::GetWindowTreeOwner(nsIDOMWindow *inWindow,
                                    nsIDocShellTreeOwner **outTreeOwner)
{
  *outTreeOwner = 0;

  nsCOMPtr<nsIDocShellTreeItem> treeItem;
  GetWindowTreeItem(inWindow, getter_AddRefs(treeItem));
  if (treeItem)
    treeItem->GetTreeOwner(outTreeOwner);
}

JSContext *
nsWindowWatcher::GetJSContextFromCallStack()
{
  JSContext *cx = 0;

  nsCOMPtr<nsIThreadJSContextStack> cxStack(do_GetService(sJSStackContractID));
  if (cxStack)
    cxStack->Peek(&cx);

  return cx;
}

JSContext *
nsWindowWatcher::GetJSContextFromWindow(nsIDOMWindow *aWindow)
{
  JSContext *cx = 0;

  if (aWindow) {
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(aWindow));
    if (sgo) {
      nsIScriptContext *scx = sgo->GetContext();
      if (scx)
        cx = (JSContext *) scx->GetNativeContext();
    }
    



  }

  return cx;
}
