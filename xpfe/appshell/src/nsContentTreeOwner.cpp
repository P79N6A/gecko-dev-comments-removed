







#include "nsContentTreeOwner.h"
#include "nsXULWindow.h"


#include "nsIServiceManager.h"
#include "nsAutoPtr.h"


#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindow.h"
#include "nsIDOMChromeWindow.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDOMXULElement.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIWindowMediator.h"
#include "nsIXULBrowserWindow.h"
#include "nsIPrincipal.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"
#include "nsIWebNavigation.h"
#include "nsIJSContextStack.h"
#include "mozilla/BrowserElementParent.h"

#include "nsIDOMDocument.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"
#if defined(XP_MACOSX)
#include "nsThreadUtils.h"
#endif

#include "mozilla/Preferences.h"

using namespace mozilla;


static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);

static const char *sJSStackContractID="@mozilla.org/js/xpc/ContextStack;1";





class nsSiteWindow : public nsIEmbeddingSiteWindow
{
public:
  nsSiteWindow(nsContentTreeOwner *aAggregator);
  virtual ~nsSiteWindow();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIEMBEDDINGSITEWINDOW

private:
  nsContentTreeOwner *mAggregator;
};





nsContentTreeOwner::nsContentTreeOwner(bool fPrimary) : mXULWindow(nsnull), 
   mPrimary(fPrimary), mContentTitleSetting(false)
{
  
  mSiteWindow = new nsSiteWindow(this);
}

nsContentTreeOwner::~nsContentTreeOwner()
{
  delete mSiteWindow;
}





NS_IMPL_ADDREF(nsContentTreeOwner)
NS_IMPL_RELEASE(nsContentTreeOwner)

NS_INTERFACE_MAP_BEGIN(nsContentTreeOwner)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome2)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome3)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWindowProvider)
   
   
   
   
   
   
   
   
   NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIEmbeddingSiteWindow, mSiteWindow)
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsContentTreeOwner::GetInterface(const nsIID& aIID, void** aSink)
{
  NS_ENSURE_ARG_POINTER(aSink);
  *aSink = 0;

  if(aIID.Equals(NS_GET_IID(nsIPrompt))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if(aIID.Equals(NS_GET_IID(nsIAuthPrompt))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if (aIID.Equals(NS_GET_IID(nsIDocShellTreeItem))) {
    NS_ENSURE_STATE(mXULWindow);
    nsCOMPtr<nsIDocShell> shell;
    mXULWindow->GetDocShell(getter_AddRefs(shell));
    if (shell)
      return shell->QueryInterface(aIID, aSink);
    return NS_ERROR_FAILURE;
  }

  if (aIID.Equals(NS_GET_IID(nsIDOMWindow))) {
    NS_ENSURE_STATE(mXULWindow);
    nsCOMPtr<nsIDocShellTreeItem> shell;
    mXULWindow->GetPrimaryContentShell(getter_AddRefs(shell));
    if (shell) {
      nsCOMPtr<nsIInterfaceRequestor> thing(do_QueryInterface(shell));
      if (thing)
        return thing->GetInterface(aIID, aSink);
    }
    return NS_ERROR_FAILURE;
  }

  if (aIID.Equals(NS_GET_IID(nsIXULWindow))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->QueryInterface(aIID, aSink);
  }

  return QueryInterface(aIID, aSink);
}





NS_IMETHODIMP nsContentTreeOwner::FindItemWithName(const PRUnichar* aName,
   nsIDocShellTreeItem* aRequestor, nsIDocShellTreeItem* aOriginalRequestor,
   nsIDocShellTreeItem** aFoundItem)
{
   NS_ENSURE_ARG_POINTER(aFoundItem);

   *aFoundItem = nsnull;

   bool fIs_Content = false;

   
   if (!aName || !*aName)
      return NS_OK;

   nsDependentString name(aName);

   if (name.LowerCaseEqualsLiteral("_blank"))
      return NS_OK;
   
   
   if (name.LowerCaseEqualsLiteral("_content") ||
       name.EqualsLiteral("_main")) {
     
     
     
     
     
     NS_ENSURE_STATE(mXULWindow);
     if (aRequestor) {
       
#ifdef DEBUG
       nsCOMPtr<nsIDocShellTreeItem> debugRoot;
       aRequestor->GetSameTypeRootTreeItem(getter_AddRefs(debugRoot));
       NS_ASSERTION(SameCOMIdentity(debugRoot, aRequestor),
                    "Bogus aRequestor");
#endif

       PRInt32 count = mXULWindow->mTargetableShells.Count();
       for (PRInt32 i = 0; i < count; ++i) {
         nsCOMPtr<nsIDocShellTreeItem> item =
           do_QueryReferent(mXULWindow->mTargetableShells[i]);
         if (SameCOMIdentity(item, aRequestor)) {
           NS_ADDREF(*aFoundItem = aRequestor);
           return NS_OK;
         }
       }
     }
     mXULWindow->GetPrimaryContentShell(aFoundItem);
     if(*aFoundItem)
       return NS_OK;
     
     fIs_Content = true;
   }

   nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
   NS_ENSURE_TRUE(windowMediator, NS_ERROR_FAILURE);

   nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
   NS_ENSURE_SUCCESS(windowMediator->GetXULWindowEnumerator(nsnull, 
      getter_AddRefs(windowEnumerator)), NS_ERROR_FAILURE);
   
   bool more;
   
   windowEnumerator->HasMoreElements(&more);
   while(more) {
     nsCOMPtr<nsISupports> nextWindow = nsnull;
     windowEnumerator->GetNext(getter_AddRefs(nextWindow));
     nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
     NS_ENSURE_TRUE(xulWindow, NS_ERROR_FAILURE);

     if (fIs_Content) {
       xulWindow->GetPrimaryContentShell(aFoundItem);
     } else {
       
       nsRefPtr<nsXULWindow> win;
       xulWindow->QueryInterface(NS_GET_IID(nsXULWindow), getter_AddRefs(win));
       if (win) {
         PRInt32 count = win->mTargetableShells.Count();
         PRInt32 i;
         for (i = 0; i < count && !*aFoundItem; ++i) {
           nsCOMPtr<nsIDocShellTreeItem> shellAsTreeItem =
             do_QueryReferent(win->mTargetableShells[i]);
           if (shellAsTreeItem) {
             
             
             
             
             
             nsCOMPtr<nsIDocShellTreeItem> root;
             shellAsTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
             NS_ASSERTION(root, "Must have root tree item of same type");
             shellAsTreeItem.swap(root);
             if (aRequestor != shellAsTreeItem) {
               
               
               nsCOMPtr<nsIDocShellTreeOwner> shellOwner;
               shellAsTreeItem->GetTreeOwner(getter_AddRefs(shellOwner));
               nsCOMPtr<nsISupports> shellOwnerSupports =
                 do_QueryInterface(shellOwner);

               shellAsTreeItem->FindItemWithName(aName, shellOwnerSupports,
                                                 aOriginalRequestor,
                                                 aFoundItem);
             }
           }
         }
       }
     }
     
     if (*aFoundItem)
       return NS_OK;

     windowEnumerator->HasMoreElements(&more);
   }
   return NS_OK;      
}

NS_IMETHODIMP
nsContentTreeOwner::ContentShellAdded(nsIDocShellTreeItem* aContentShell,
                                      bool aPrimary, bool aTargetable,
                                      const nsAString& aID)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->ContentShellAdded(aContentShell, aPrimary, aTargetable,
                                       aID);
}

NS_IMETHODIMP
nsContentTreeOwner::ContentShellRemoved(nsIDocShellTreeItem* aContentShell)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->ContentShellRemoved(aContentShell);
}

NS_IMETHODIMP nsContentTreeOwner::GetPrimaryContentShell(nsIDocShellTreeItem** aShell)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetPrimaryContentShell(aShell);
}

NS_IMETHODIMP nsContentTreeOwner::SizeShellTo(nsIDocShellTreeItem* aShellItem,
   PRInt32 aCX, PRInt32 aCY)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SizeShellTo(aShellItem, aCX, aCY);
}

NS_IMETHODIMP
nsContentTreeOwner::SetPersistence(bool aPersistPosition,
                                   bool aPersistSize,
                                   bool aPersistSizeMode)
{
  NS_ENSURE_STATE(mXULWindow);
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
  if(!docShellElement)
    return NS_ERROR_FAILURE;

  nsAutoString persistString;
  docShellElement->GetAttribute(NS_LITERAL_STRING("persist"), persistString);

  bool saveString = false;
  PRInt32 index;

  
  index = persistString.Find("screenX");
  if (!aPersistPosition && index >= 0) {
    persistString.Cut(index, 7);
    saveString = true;
  } else if (aPersistPosition && index < 0) {
    persistString.AppendLiteral(" screenX");
    saveString = true;
  }
  
  index = persistString.Find("screenY");
  if (!aPersistPosition && index >= 0) {
    persistString.Cut(index, 7);
    saveString = true;
  } else if (aPersistPosition && index < 0) {
    persistString.AppendLiteral(" screenY");
    saveString = true;
  }
  
  index = persistString.Find("width");
  if (!aPersistSize && index >= 0) {
    persistString.Cut(index, 5);
    saveString = true;
  } else if (aPersistSize && index < 0) {
    persistString.AppendLiteral(" width");
    saveString = true;
  }
  
  index = persistString.Find("height");
  if (!aPersistSize && index >= 0) {
    persistString.Cut(index, 6);
    saveString = true;
  } else if (aPersistSize && index < 0) {
    persistString.AppendLiteral(" height");
    saveString = true;
  }
  
  index = persistString.Find("sizemode");
  if (!aPersistSizeMode && (index >= 0)) {
    persistString.Cut(index, 8);
    saveString = true;
  } else if (aPersistSizeMode && (index < 0)) {
    persistString.AppendLiteral(" sizemode");
    saveString = true;
  }

  if(saveString) 
    docShellElement->SetAttribute(NS_LITERAL_STRING("persist"), persistString);

  return NS_OK;
}

NS_IMETHODIMP
nsContentTreeOwner::GetPersistence(bool* aPersistPosition,
                                   bool* aPersistSize,
                                   bool* aPersistSizeMode)
{
  NS_ENSURE_STATE(mXULWindow);
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
  if(!docShellElement) 
    return NS_ERROR_FAILURE;

  nsAutoString persistString;
  docShellElement->GetAttribute(NS_LITERAL_STRING("persist"), persistString);

  
  
  if (aPersistPosition)
    *aPersistPosition = persistString.Find("screenX") >= 0 || persistString.Find("screenY") >= 0 ? true : false;
  if (aPersistSize)
    *aPersistSize = persistString.Find("width") >= 0 || persistString.Find("height") >= 0 ? true : false;
  if (aPersistSizeMode)
    *aPersistSizeMode = persistString.Find("sizemode") >= 0 ? true : false;

  return NS_OK;
}

NS_IMETHODIMP
nsContentTreeOwner::GetTargetableShellCount(PRUint32* aResult)
{
  NS_ENSURE_STATE(mXULWindow);
  *aResult = mXULWindow->mTargetableShells.Count();
  return NS_OK;
}





NS_IMETHODIMP nsContentTreeOwner::OnBeforeLinkTraversal(const nsAString &originalTarget,
                                                        nsIURI *linkURI,
                                                        nsIDOMNode *linkNode,
                                                        bool isAppTab,
                                                        nsAString &_retval)
{
  NS_ENSURE_STATE(mXULWindow);

  nsCOMPtr<nsIXULBrowserWindow> xulBrowserWindow;
  mXULWindow->GetXULBrowserWindow(getter_AddRefs(xulBrowserWindow));

  if (xulBrowserWindow)
    return xulBrowserWindow->OnBeforeLinkTraversal(originalTarget, linkURI,
                                                   linkNode, isAppTab, _retval);
  
  _retval = originalTarget;
  return NS_OK;
}





NS_IMETHODIMP nsContentTreeOwner::SetStatusWithContext(PRUint32 aStatusType,
                                                       const nsAString &aStatusText,
                                                       nsISupports *aStatusContext)
{
  
  if (!mPrimary && aStatusType != STATUS_LINK)
    return NS_OK;

  NS_ENSURE_STATE(mXULWindow);
  
  nsCOMPtr<nsIXULBrowserWindow> xulBrowserWindow;
  mXULWindow->GetXULBrowserWindow(getter_AddRefs(xulBrowserWindow));

  if (xulBrowserWindow)
  {
    switch(aStatusType)
    {
    case STATUS_SCRIPT:
      xulBrowserWindow->SetJSStatus(aStatusText);
      break;
    case STATUS_SCRIPT_DEFAULT:
      xulBrowserWindow->SetJSDefaultStatus(aStatusText);
      break;
    case STATUS_LINK:
      {
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aStatusContext);
        xulBrowserWindow->SetOverLink(aStatusText, element);
        break;
      }
    }
  }

  return NS_OK;
}





NS_IMETHODIMP nsContentTreeOwner::SetStatus(PRUint32 aStatusType,
                                            const PRUnichar* aStatus)
{
  return SetStatusWithContext(aStatusType,
      aStatus ? static_cast<const nsString &>(nsDependentString(aStatus))
              : EmptyString(),
      nsnull);
}

NS_IMETHODIMP nsContentTreeOwner::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
  
  
  NS_ENSURE_ARG_POINTER(aWebBrowser);
  *aWebBrowser = 0;
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::SetChromeFlags(PRUint32 aChromeFlags)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetChromeFlags(aChromeFlags);
}

NS_IMETHODIMP nsContentTreeOwner::GetChromeFlags(PRUint32* aChromeFlags)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->GetChromeFlags(aChromeFlags);
}

NS_IMETHODIMP nsContentTreeOwner::DestroyBrowserWindow()
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
   NS_ERROR("Haven't Implemented this yet");
   return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsContentTreeOwner::ShowAsModal()
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->ShowModal();
}

NS_IMETHODIMP nsContentTreeOwner::IsWindowModal(bool *_retval)
{
  NS_ENSURE_STATE(mXULWindow);
  *_retval = mXULWindow->mContinueModalLoop;
  return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::ExitModalEventLoop(nsresult aStatus)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->ExitModalLoop(aStatus);   
}





NS_IMETHODIMP nsContentTreeOwner::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* parentWidget, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)   
{
   
   NS_ENSURE_SUCCESS(SetPositionAndSize(x, y, cx, cy, false), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::Create()
{
   NS_ASSERTION(false, "You can't call this");
   return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsContentTreeOwner::Destroy()
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->Destroy();
}

NS_IMETHODIMP nsContentTreeOwner::SetPosition(PRInt32 aX, PRInt32 aY)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetPosition(aX, aY);
}

NS_IMETHODIMP nsContentTreeOwner::GetPosition(PRInt32* aX, PRInt32* aY)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetPosition(aX, aY);
}

NS_IMETHODIMP nsContentTreeOwner::SetSize(PRInt32 aCX, PRInt32 aCY, bool aRepaint)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetSize(aCX, aCY, aRepaint);
}

NS_IMETHODIMP nsContentTreeOwner::GetSize(PRInt32* aCX, PRInt32* aCY)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetSize(aCX, aCY);
}

NS_IMETHODIMP nsContentTreeOwner::SetPositionAndSize(PRInt32 aX, PRInt32 aY,
   PRInt32 aCX, PRInt32 aCY, bool aRepaint)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetPositionAndSize(aX, aY, aCX, aCY, aRepaint);
}

NS_IMETHODIMP nsContentTreeOwner::GetPositionAndSize(PRInt32* aX, PRInt32* aY,
   PRInt32* aCX, PRInt32* aCY)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetPositionAndSize(aX, aY, aCX, aCY); 
}

NS_IMETHODIMP nsContentTreeOwner::Repaint(bool aForce)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->Repaint(aForce);
}

NS_IMETHODIMP nsContentTreeOwner::GetParentWidget(nsIWidget** aParentWidget)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetParentWidget(aParentWidget);
}

NS_IMETHODIMP nsContentTreeOwner::SetParentWidget(nsIWidget* aParentWidget)
{
   NS_ASSERTION(false, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsContentTreeOwner::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetParentNativeWindow(aParentNativeWindow);
}

NS_IMETHODIMP nsContentTreeOwner::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
   NS_ASSERTION(false, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsContentTreeOwner::GetVisibility(bool* aVisibility)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetVisibility(aVisibility);
}

NS_IMETHODIMP nsContentTreeOwner::SetVisibility(bool aVisibility)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetVisibility(aVisibility);
}

NS_IMETHODIMP nsContentTreeOwner::GetEnabled(bool *aEnabled)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetEnabled(aEnabled);
}

NS_IMETHODIMP nsContentTreeOwner::SetEnabled(bool aEnable)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetEnabled(aEnable);
}

NS_IMETHODIMP nsContentTreeOwner::GetMainWidget(nsIWidget** aMainWidget)
{
   NS_ENSURE_ARG_POINTER(aMainWidget);
   NS_ENSURE_STATE(mXULWindow);

   *aMainWidget = mXULWindow->mWindow;
   NS_IF_ADDREF(*aMainWidget);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::SetFocus()
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetFocus();
}

NS_IMETHODIMP nsContentTreeOwner::GetTitle(PRUnichar** aTitle)
{
   NS_ENSURE_ARG_POINTER(aTitle);
   NS_ENSURE_STATE(mXULWindow);

   return mXULWindow->GetTitle(aTitle);
}

NS_IMETHODIMP nsContentTreeOwner::SetTitle(const PRUnichar* aTitle)
{
   
  if(!mPrimary || !mContentTitleSetting)
    return NS_OK;

  NS_ENSURE_STATE(mXULWindow);
  
  nsAutoString   title;
  nsAutoString   docTitle(aTitle);

  if (docTitle.IsEmpty())
    docTitle.Assign(mTitleDefault);
  
  if (!docTitle.IsEmpty()) {
    if (!mTitlePreface.IsEmpty()) {
      
      title.Assign(mTitlePreface);
      title.Append(docTitle);
    }
    else {
      
      title = docTitle;
    }
  
    if (!mWindowTitleModifier.IsEmpty())
      title += mTitleSeparator + mWindowTitleModifier;
  }
  else
    title.Assign(mWindowTitleModifier); 

  
  
  
  
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));

  if (docShellElement) {
    nsAutoString chromeString;
    docShellElement->GetAttribute(NS_LITERAL_STRING("chromehidden"), chromeString);
    if (chromeString.Find(NS_LITERAL_STRING("location")) != kNotFound) {
      
      
      
      
      
      
      nsCOMPtr<nsIDocShellTreeItem> dsitem;
      GetPrimaryContentShell(getter_AddRefs(dsitem));
      nsCOMPtr<nsIDOMDocument> domdoc(do_GetInterface(dsitem));
      nsCOMPtr<nsIScriptObjectPrincipal> doc(do_QueryInterface(domdoc));
      if (doc) {
        nsCOMPtr<nsIURI> uri;
        nsIPrincipal* principal = doc->GetPrincipal();
        if (principal) {
          principal->GetURI(getter_AddRefs(uri));
          if (uri) {
            
            
            
            nsCOMPtr<nsIURIFixup> fixup(do_GetService(NS_URIFIXUP_CONTRACTID));
            if (fixup) {
              nsCOMPtr<nsIURI> tmpuri;
              nsresult rv = fixup->CreateExposableURI(uri,getter_AddRefs(tmpuri));
              if (NS_SUCCEEDED(rv) && tmpuri) {
                
                nsCAutoString host;
                nsCAutoString prepath;
                tmpuri->GetHost(host);
                tmpuri->GetPrePath(prepath);
                if (!host.IsEmpty()) {
                  
                  
                  
                  title.Insert(NS_ConvertUTF8toUTF16(prepath) +
                               mTitleSeparator, 0);
                }
              }
            }
          }
        }
      }
    }
    nsCOMPtr<nsIDOMDocument> document;
    docShellElement->GetOwnerDocument(getter_AddRefs(document));
    if (document) {
      return document->SetTitle(title);
    }
  }

  return mXULWindow->SetTitle(title.get());
}

NS_STACK_CLASS class NullJSContextPusher {
public:
  NullJSContextPusher() {
    mService = do_GetService(sJSStackContractID);
    if (mService) {
#ifdef DEBUG
      nsresult rv =
#endif
        mService->Push(nsnull);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Mismatched push/pop");
    }
  }

  ~NullJSContextPusher() {
    if (mService) {
      JSContext *cx;
#ifdef DEBUG
      nsresult rv =
#endif
        mService->Pop(&cx);
      NS_ASSERTION(NS_SUCCEEDED(rv) && !cx, "Bad pop!");
    }
  }

private:
  nsCOMPtr<nsIThreadJSContextStack> mService;
};




NS_IMETHODIMP
nsContentTreeOwner::ProvideWindow(nsIDOMWindow* aParent,
                                  PRUint32 aChromeFlags,
                                  bool aCalledFromJS,
                                  bool aPositionSpecified,
                                  bool aSizeSpecified,
                                  nsIURI* aURI,
                                  const nsAString& aName,
                                  const nsACString& aFeatures,
                                  bool* aWindowIsNew,
                                  nsIDOMWindow** aReturn)
{
  NS_ENSURE_ARG_POINTER(aParent);
  
  *aReturn = nsnull;

  if (!mXULWindow) {
    
    return NS_OK;
  }

#ifdef DEBUG
  nsCOMPtr<nsIWebNavigation> parentNav = do_GetInterface(aParent);
  nsCOMPtr<nsIDocShellTreeOwner> parentOwner = do_GetInterface(parentNav);
  NS_ASSERTION(SameCOMIdentity(parentOwner,
                               static_cast<nsIDocShellTreeOwner*>(this)),
               "Parent from wrong docshell tree?");
#endif

  
  
  
  nsCOMPtr<nsIDocShell> docshell = do_GetInterface(aParent);
  bool inBrowserFrame = false;
  if (docshell) {
    docshell->GetContainedInBrowserFrame(&inBrowserFrame);
  }

  if (inBrowserFrame &&
      !(aChromeFlags & (nsIWebBrowserChrome::CHROME_MODAL |
                        nsIWebBrowserChrome::CHROME_OPENAS_DIALOG |
                        nsIWebBrowserChrome::CHROME_OPENAS_CHROME))) {
    *aWindowIsNew =
      BrowserElementParent::OpenWindowInProcess(aParent, aURI, aName,
                                                aFeatures, aReturn);

    
    
    
    return *aWindowIsNew ? NS_OK : NS_ERROR_ABORT;
  }

  
  PRInt32 containerPref;
  if (NS_FAILED(Preferences::GetInt("browser.link.open_newwindow",
                                    &containerPref))) {
    return NS_OK;
  }

  if (containerPref != nsIBrowserDOMWindow::OPEN_NEWTAB &&
      containerPref != nsIBrowserDOMWindow::OPEN_CURRENTWINDOW) {
    
    return NS_OK;
  }

  if (aCalledFromJS) {
    





    PRInt32 restrictionPref =
      Preferences::GetInt("browser.link.open_newwindow.restriction", 2);
    if (restrictionPref < 0 || restrictionPref > 2) {
      restrictionPref = 2; 
    }

    if (restrictionPref == 1) {
      return NS_OK;
    }

    if (restrictionPref == 2 &&
        
        
        (aChromeFlags != nsIWebBrowserChrome::CHROME_ALL ||
         aPositionSpecified || aSizeSpecified)) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMWindow> domWin;
  mXULWindow->GetWindowDOMWindow(getter_AddRefs(domWin));
  nsCOMPtr<nsIDOMChromeWindow> chromeWin = do_QueryInterface(domWin);
  if (!chromeWin) {
    
    NS_WARNING("nsXULWindow's DOMWindow is not a chrome window");
    return NS_OK;
  }
  
  nsCOMPtr<nsIBrowserDOMWindow> browserDOMWin;
  chromeWin->GetBrowserDOMWindow(getter_AddRefs(browserDOMWin));
  if (!browserDOMWin) {
    return NS_OK;
  }

  *aWindowIsNew = (containerPref != nsIBrowserDOMWindow::OPEN_CURRENTWINDOW);

  {
    NullJSContextPusher pusher;

    
    
    return browserDOMWin->OpenURI(nsnull, aParent, containerPref,
                                  nsIBrowserDOMWindow::OPEN_NEW, aReturn);
  }
}





#if defined(XP_MACOSX)
class nsContentTitleSettingEvent : public nsRunnable
{
public:
  nsContentTitleSettingEvent(nsIDOMElement *dse, const nsAString& wtm)
    : mElement(dse),
      mTitleDefault(wtm) {}

  NS_IMETHOD Run()
  {
    mElement->SetAttribute(NS_LITERAL_STRING("titledefault"), mTitleDefault);
    mElement->RemoveAttribute(NS_LITERAL_STRING("titlemodifier"));
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMElement> mElement;
  nsString mTitleDefault;
};
#endif

void nsContentTreeOwner::XULWindow(nsXULWindow* aXULWindow)
{
   mXULWindow = aXULWindow;
   if(mXULWindow && mPrimary)
      {
      
      nsCOMPtr<nsIDOMElement> docShellElement;
      mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));

      nsAutoString   contentTitleSetting;

      if(docShellElement)  
         {
         docShellElement->GetAttribute(NS_LITERAL_STRING("contenttitlesetting"), contentTitleSetting);
         if(contentTitleSetting.EqualsLiteral("true"))
            {
            mContentTitleSetting = true;
            docShellElement->GetAttribute(NS_LITERAL_STRING("titledefault"), mTitleDefault);
            docShellElement->GetAttribute(NS_LITERAL_STRING("titlemodifier"), mWindowTitleModifier);
            docShellElement->GetAttribute(NS_LITERAL_STRING("titlepreface"), mTitlePreface);
            
#if defined(XP_MACOSX)
            
            
            if (mTitleDefault.IsEmpty()) {
                NS_DispatchToCurrentThread(
                    new nsContentTitleSettingEvent(docShellElement,
                                                   mWindowTitleModifier));
                mTitleDefault = mWindowTitleModifier;
                mWindowTitleModifier.Truncate();
            }
#endif
            docShellElement->GetAttribute(NS_LITERAL_STRING("titlemenuseparator"), mTitleSeparator);
            }
         }
      else
         {
         NS_ERROR("This condition should never happen.  If it does, "
            "we just won't get a modifier, but it still shouldn't happen.");
         }
      }
}

nsXULWindow* nsContentTreeOwner::XULWindow()
{
   return mXULWindow;
}





nsSiteWindow::nsSiteWindow(nsContentTreeOwner *aAggregator)
{
  mAggregator = aAggregator;
}

nsSiteWindow::~nsSiteWindow()
{
}

NS_IMPL_ADDREF_USING_AGGREGATOR(nsSiteWindow, mAggregator)
NS_IMPL_RELEASE_USING_AGGREGATOR(nsSiteWindow, mAggregator)

NS_INTERFACE_MAP_BEGIN(nsSiteWindow)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
NS_INTERFACE_MAP_END_AGGREGATED(mAggregator)

NS_IMETHODIMP
nsSiteWindow::SetDimensions(PRUint32 aFlags,
                    PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY)
{
  
  return mAggregator->SetPositionAndSize(aX, aY, aCX, aCY, true);
}

NS_IMETHODIMP
nsSiteWindow::GetDimensions(PRUint32 aFlags,
                    PRInt32 *aX, PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY)
{
  
  return mAggregator->GetPositionAndSize(aX, aY, aCX, aCY);
}

NS_IMETHODIMP
nsSiteWindow::SetFocus(void)
{
#if 0
  





  nsXULWindow *window = mAggregator->XULWindow();
  if (window) {
    nsCOMPtr<nsIDocShell> docshell;
    window->GetDocShell(getter_AddRefs(docshell));
    nsCOMPtr<nsIDOMWindow> domWindow(do_GetInterface(docshell));
    if (domWindow)
      domWindow->Focus();
  }
#endif
  return NS_OK;
}



NS_IMETHODIMP
nsSiteWindow::Blur(void)
{
  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  nsCOMPtr<nsIXULWindow>        xulWindow;
  bool                          more, foundUs;
  nsXULWindow                  *ourWindow = mAggregator->XULWindow();

  {
    nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
    if (windowMediator)
      windowMediator->GetZOrderXULWindowEnumerator(0, true,
                        getter_AddRefs(windowEnumerator));
  }

  if (!windowEnumerator)
    return NS_ERROR_FAILURE;

  
  foundUs = false;
  windowEnumerator->HasMoreElements(&more);
  while (more) {

    nsCOMPtr<nsISupports>  nextWindow;
    nsCOMPtr<nsIXULWindow> nextXULWindow;

    windowEnumerator->GetNext(getter_AddRefs(nextWindow));
    nextXULWindow = do_QueryInterface(nextWindow);

    
    if (foundUs) {
      xulWindow = nextXULWindow;
      break;
    }

    
    if (!xulWindow)
      xulWindow = nextXULWindow;

    
    if (nextXULWindow == ourWindow)
      foundUs = true;

    windowEnumerator->HasMoreElements(&more);
  }

  
  if (xulWindow) {
    nsCOMPtr<nsIDocShell> docshell;
    xulWindow->GetDocShell(getter_AddRefs(docshell));
    nsCOMPtr<nsIDOMWindow> domWindow(do_GetInterface(docshell));
    if (domWindow)
      domWindow->Focus();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSiteWindow::GetVisibility(bool *aVisibility)
{
  return mAggregator->GetVisibility(aVisibility);
}

NS_IMETHODIMP
nsSiteWindow::SetVisibility(bool aVisibility)
{
  return mAggregator->SetVisibility(aVisibility);
}

NS_IMETHODIMP
nsSiteWindow::GetTitle(PRUnichar * *aTitle)
{
  return mAggregator->GetTitle(aTitle);
}

NS_IMETHODIMP
nsSiteWindow::SetTitle(const PRUnichar * aTitle)
{
  return mAggregator->SetTitle(aTitle);
}

NS_IMETHODIMP
nsSiteWindow::GetSiteWindow(void **aSiteWindow)
{
  return mAggregator->GetParentNativeWindow(aSiteWindow);
}

