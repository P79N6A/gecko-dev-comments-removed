







































#include "nsContentTreeOwner.h"
#include "nsXULWindow.h"


#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsAutoPtr.h"


#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMChromeWindow.h"
#include "nsIBrowserDOMWindow.h"
#include "nsIDOMXULElement.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIWindowMediator.h"
#include "nsIXULBrowserWindow.h"
#include "nsIPrincipal.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIWebNavigation.h"

#include "nsIDOMDocument.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIURI.h"


static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);





class nsSiteWindow2 : public nsIEmbeddingSiteWindow2
{
public:
  nsSiteWindow2(nsContentTreeOwner *aAggregator);
  virtual ~nsSiteWindow2();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIEMBEDDINGSITEWINDOW
  NS_DECL_NSIEMBEDDINGSITEWINDOW2

private:
  nsContentTreeOwner *mAggregator;
};





nsContentTreeOwner::nsContentTreeOwner(PRBool fPrimary) : mXULWindow(nsnull), 
   mPrimary(fPrimary), mContentTitleSetting(PR_FALSE)
{
  
  mSiteWindow2 = new nsSiteWindow2(this);
}

nsContentTreeOwner::~nsContentTreeOwner()
{
  delete mSiteWindow2;
}





NS_IMPL_ADDREF(nsContentTreeOwner)
NS_IMPL_RELEASE(nsContentTreeOwner)

NS_INTERFACE_MAP_BEGIN(nsContentTreeOwner)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner_MOZILLA_1_8_BRANCH)
   NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
   NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome2)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIWindowProvider)
   
   
   
   
   
   
   
   
   NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIEmbeddingSiteWindow, mSiteWindow2)
   NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIEmbeddingSiteWindow2, mSiteWindow2)
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

   PRBool fIs_Content = PR_FALSE;

   
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
     
     fIs_Content = PR_TRUE;
   }

   nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
   NS_ENSURE_TRUE(windowMediator, NS_ERROR_FAILURE);

   nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
   NS_ENSURE_SUCCESS(windowMediator->GetXULWindowEnumerator(nsnull, 
      getter_AddRefs(windowEnumerator)), NS_ERROR_FAILURE);
   
   PRBool more;
   
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

NS_IMETHODIMP nsContentTreeOwner::ContentShellAdded(nsIDocShellTreeItem* aContentShell,
   PRBool aPrimary, const PRUnichar* aID)
{
   NS_ENSURE_STATE(mXULWindow);
   if (aID) {
     return mXULWindow->ContentShellAdded(aContentShell, aPrimary, PR_FALSE,
                                          nsDependentString(aID));
   }

   return mXULWindow->ContentShellAdded(aContentShell, aPrimary, PR_FALSE,
                                        EmptyString());
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
nsContentTreeOwner::SetPersistence(PRBool aPersistPosition,
                                   PRBool aPersistSize,
                                   PRBool aPersistSizeMode)
{
  NS_ENSURE_STATE(mXULWindow);
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
  if(!docShellElement)
    return NS_ERROR_FAILURE;

  nsAutoString persistString;
  docShellElement->GetAttribute(NS_LITERAL_STRING("persist"), persistString);

  PRBool saveString = PR_FALSE;
  PRInt32 index;

  
  index = persistString.Find("screenX");
  if (!aPersistPosition && index >= 0) {
    persistString.Cut(index, 7);
    saveString = PR_TRUE;
  } else if (aPersistPosition && index < 0) {
    persistString.AppendLiteral(" screenX");
    saveString = PR_TRUE;
  }
  
  index = persistString.Find("screenY");
  if (!aPersistPosition && index >= 0) {
    persistString.Cut(index, 7);
    saveString = PR_TRUE;
  } else if (aPersistPosition && index < 0) {
    persistString.AppendLiteral(" screenY");
    saveString = PR_TRUE;
  }
  
  index = persistString.Find("width");
  if (!aPersistSize && index >= 0) {
    persistString.Cut(index, 5);
    saveString = PR_TRUE;
  } else if (aPersistSize && index < 0) {
    persistString.AppendLiteral(" width");
    saveString = PR_TRUE;
  }
  
  index = persistString.Find("height");
  if (!aPersistSize && index >= 0) {
    persistString.Cut(index, 6);
    saveString = PR_TRUE;
  } else if (aPersistSize && index < 0) {
    persistString.AppendLiteral(" height");
    saveString = PR_TRUE;
  }
  
  index = persistString.Find("sizemode");
  if (!aPersistSizeMode && (index >= 0)) {
    persistString.Cut(index, 8);
    saveString = PR_TRUE;
  } else if (aPersistSizeMode && (index < 0)) {
    persistString.AppendLiteral(" sizemode");
    saveString = PR_TRUE;
  }

  if(saveString) 
    docShellElement->SetAttribute(NS_LITERAL_STRING("persist"), persistString);

  return NS_OK;
}

NS_IMETHODIMP
nsContentTreeOwner::GetPersistence(PRBool* aPersistPosition,
                                   PRBool* aPersistSize,
                                   PRBool* aPersistSizeMode)
{
  NS_ENSURE_STATE(mXULWindow);
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
  if(!docShellElement) 
    return NS_ERROR_FAILURE;

  nsAutoString persistString;
  docShellElement->GetAttribute(NS_LITERAL_STRING("persist"), persistString);

  
  
  if (aPersistPosition)
    *aPersistPosition = persistString.Find("screenX") >= 0 || persistString.Find("screenY") >= 0 ? PR_TRUE : PR_FALSE;
  if (aPersistSize)
    *aPersistSize = persistString.Find("width") >= 0 || persistString.Find("height") >= 0 ? PR_TRUE : PR_FALSE;
  if (aPersistSizeMode)
    *aPersistSizeMode = persistString.Find("sizemode") >= 0 ? PR_TRUE : PR_FALSE;

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
      aStatus ? NS_STATIC_CAST(const nsString &, nsDependentString(aStatus))
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

NS_IMETHODIMP nsContentTreeOwner::IsWindowModal(PRBool *_retval)
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
   
   NS_ENSURE_SUCCESS(SetPositionAndSize(x, y, cx, cy, PR_FALSE), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsContentTreeOwner::Create()
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
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

NS_IMETHODIMP nsContentTreeOwner::SetSize(PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
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
   PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
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

NS_IMETHODIMP nsContentTreeOwner::Repaint(PRBool aForce)
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
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsContentTreeOwner::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetParentNativeWindow(aParentNativeWindow);
}

NS_IMETHODIMP nsContentTreeOwner::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsContentTreeOwner::GetVisibility(PRBool* aVisibility)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetVisibility(aVisibility);
}

NS_IMETHODIMP nsContentTreeOwner::SetVisibility(PRBool aVisibility)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetVisibility(aVisibility);
}

NS_IMETHODIMP nsContentTreeOwner::GetEnabled(PRBool *aEnabled)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetEnabled(aEnabled);
}

NS_IMETHODIMP nsContentTreeOwner::SetEnabled(PRBool aEnable)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetEnabled(aEnable);
}

NS_IMETHODIMP nsContentTreeOwner::GetBlurSuppression(PRBool *aBlurSuppression)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->GetBlurSuppression(aBlurSuppression);
}

NS_IMETHODIMP nsContentTreeOwner::SetBlurSuppression(PRBool aBlurSuppression)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->SetBlurSuppression(aBlurSuppression);
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
  }

  return mXULWindow->SetTitle(title.get());
}




NS_IMETHODIMP
nsContentTreeOwner::ProvideWindow(nsIDOMWindow* aParent,
                                  PRUint32 aChromeFlags,
                                  PRBool aPositionSpecified,
                                  PRBool aSizeSpecified,
                                  nsIURI* aURI,
                                  const nsAString& aName,
                                  const nsACString& aFeatures,
                                  PRBool* aWindowIsNew,
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
                               NS_STATIC_CAST(nsIDocShellTreeOwner*, this)),
               "Parent from wrong docshell tree?");
#endif

  
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefs) {
    return NS_OK;
  }

  nsCOMPtr<nsIPrefBranch> branch;
  prefs->GetBranch("browser.link.", getter_AddRefs(branch));
  if (!branch) {
    return NS_OK;
  }

  
  PRInt32 containerPref;
  if (NS_FAILED(branch->GetIntPref("open_newwindow", &containerPref))) {
    return NS_OK;
  }

  if (containerPref != nsIBrowserDOMWindow::OPEN_NEWTAB &&
      containerPref != nsIBrowserDOMWindow::OPEN_CURRENTWINDOW) {
    
    return NS_OK;
  }

  





  PRInt32 restrictionPref;
  if (NS_FAILED(branch->GetIntPref("open_newwindow.restriction",
                                   &restrictionPref)) ||
      restrictionPref < 0 ||
      restrictionPref > 2) {
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

  nsCOMPtr<nsIDOMWindowInternal> domWin;
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
  
  
  
  
  return browserDOMWin->OpenURI(nsnull, aParent, containerPref,
                                nsIBrowserDOMWindow::OPEN_NEW, aReturn);
}




NS_IMETHODIMP
nsContentTreeOwner::ContentShellAdded2(nsIDocShellTreeItem* aContentShell,
                                       PRBool aPrimary, PRBool aTargetable,
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
            mContentTitleSetting = PR_TRUE;
            docShellElement->GetAttribute(NS_LITERAL_STRING("titledefault"), mTitleDefault);
            docShellElement->GetAttribute(NS_LITERAL_STRING("titlemodifier"), mWindowTitleModifier);
            docShellElement->GetAttribute(NS_LITERAL_STRING("titlepreface"), mTitlePreface);
            
#if defined(XP_MACOSX) && defined(MOZ_XUL_APP)
            
            
            if (mTitleDefault.IsEmpty()) {
                docShellElement->SetAttribute(NS_LITERAL_STRING("titledefault"),
                                              mWindowTitleModifier);
                docShellElement->RemoveAttribute(NS_LITERAL_STRING("titlemodifier"));
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





nsSiteWindow2::nsSiteWindow2(nsContentTreeOwner *aAggregator)
{
  mAggregator = aAggregator;
}

nsSiteWindow2::~nsSiteWindow2()
{
}

NS_IMPL_ADDREF_USING_AGGREGATOR(nsSiteWindow2, mAggregator)
NS_IMPL_RELEASE_USING_AGGREGATOR(nsSiteWindow2, mAggregator)

NS_INTERFACE_MAP_BEGIN(nsSiteWindow2)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
  NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow2)
NS_INTERFACE_MAP_END_AGGREGATED(mAggregator)

NS_IMETHODIMP
nsSiteWindow2::SetDimensions(PRUint32 aFlags,
                    PRInt32 aX, PRInt32 aY, PRInt32 aCX, PRInt32 aCY)
{
  
  return mAggregator->SetPositionAndSize(aX, aY, aCX, aCY, PR_TRUE);
}

NS_IMETHODIMP
nsSiteWindow2::GetDimensions(PRUint32 aFlags,
                    PRInt32 *aX, PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY)
{
  
  return mAggregator->GetPositionAndSize(aX, aY, aCX, aCY);
}

NS_IMETHODIMP
nsSiteWindow2::SetFocus(void)
{
#if 0
  





  nsXULWindow *window = mAggregator->XULWindow();
  if (window) {
    nsCOMPtr<nsIDocShell> docshell;
    window->GetDocShell(getter_AddRefs(docshell));
    nsCOMPtr<nsIDOMWindowInternal> domWindow(do_GetInterface(docshell));
    if (domWindow)
      domWindow->Focus();
  }
#endif
  return NS_OK;
}



NS_IMETHODIMP
nsSiteWindow2::Blur(void)
{
  nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
  nsCOMPtr<nsIXULWindow>        xulWindow;
  PRBool                        more, foundUs;
  nsXULWindow                  *ourWindow = mAggregator->XULWindow();

  {
    nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
    if (windowMediator)
      windowMediator->GetZOrderXULWindowEnumerator(0, PR_TRUE,
                        getter_AddRefs(windowEnumerator));
  }

  if (!windowEnumerator)
    return NS_ERROR_FAILURE;

  
  foundUs = PR_FALSE;
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
      foundUs = PR_TRUE;

    windowEnumerator->HasMoreElements(&more);
  }

  
  if (xulWindow) {
    nsCOMPtr<nsIDocShell> docshell;
    xulWindow->GetDocShell(getter_AddRefs(docshell));
    nsCOMPtr<nsIDOMWindowInternal> domWindow(do_GetInterface(docshell));
    if (domWindow)
      domWindow->Focus();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSiteWindow2::GetVisibility(PRBool *aVisibility)
{
  return mAggregator->GetVisibility(aVisibility);
}

NS_IMETHODIMP
nsSiteWindow2::SetVisibility(PRBool aVisibility)
{
  return mAggregator->SetVisibility(aVisibility);
}

NS_IMETHODIMP
nsSiteWindow2::GetTitle(PRUnichar * *aTitle)
{
  return mAggregator->GetTitle(aTitle);
}

NS_IMETHODIMP
nsSiteWindow2::SetTitle(const PRUnichar * aTitle)
{
  return mAggregator->SetTitle(aTitle);
}

NS_IMETHODIMP
nsSiteWindow2::GetSiteWindow(void **aSiteWindow)
{
  return mAggregator->GetParentNativeWindow(aSiteWindow);
}

