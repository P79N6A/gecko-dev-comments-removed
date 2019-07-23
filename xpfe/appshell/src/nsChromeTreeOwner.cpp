







































#include "nsChromeTreeOwner.h"
#include "nsXULWindow.h"


#include "nsString.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIDocShellTreeItem.h"


#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIWebProgress.h"
#include "nsIWindowMediator.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULElement.h"
#include "nsIXULBrowserWindow.h"


static NS_DEFINE_CID(kWindowMediatorCID, NS_WINDOWMEDIATOR_CID);





struct nsChromeTreeOwnerLiterals
{
  const nsLiteralString kPersist;
  const nsLiteralString kScreenX;
  const nsLiteralString kScreenY;
  const nsLiteralString kWidth;
  const nsLiteralString kHeight;
  const nsLiteralString kSizemode;
  const nsLiteralString kSpace;

  nsChromeTreeOwnerLiterals()
    : NS_LITERAL_STRING_INIT(kPersist,"persist")
    , NS_LITERAL_STRING_INIT(kScreenX,"screenX")
    , NS_LITERAL_STRING_INIT(kScreenY,"screenY")
    , NS_LITERAL_STRING_INIT(kWidth,"width")
    , NS_LITERAL_STRING_INIT(kHeight,"height")
    , NS_LITERAL_STRING_INIT(kSizemode,"sizemode")
    , NS_LITERAL_STRING_INIT(kSpace," ")
  {}
};

static nsChromeTreeOwnerLiterals *gLiterals;

nsresult
nsChromeTreeOwner::InitGlobals()
{
  NS_ASSERTION(gLiterals == nsnull, "already initialized");
  gLiterals = new nsChromeTreeOwnerLiterals();
  if (!gLiterals)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

void
nsChromeTreeOwner::FreeGlobals()
{
  delete gLiterals;
  gLiterals = nsnull;
}





nsChromeTreeOwner::nsChromeTreeOwner() : mXULWindow(nsnull)
{
}

nsChromeTreeOwner::~nsChromeTreeOwner()
{
}





NS_IMPL_ADDREF(nsChromeTreeOwner)
NS_IMPL_RELEASE(nsChromeTreeOwner)

NS_INTERFACE_MAP_BEGIN(nsChromeTreeOwner)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner)
   NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeOwner_MOZILLA_1_8_BRANCH)
   NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsChromeTreeOwner::GetInterface(const nsIID& aIID, void** aSink)
{
  NS_ENSURE_ARG_POINTER(aSink);

  if(aIID.Equals(NS_GET_IID(nsIPrompt))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if(aIID.Equals(NS_GET_IID(nsIAuthPrompt))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if(aIID.Equals(NS_GET_IID(nsIWebBrowserChrome))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if (aIID.Equals(NS_GET_IID(nsIEmbeddingSiteWindow))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if (aIID.Equals(NS_GET_IID(nsIEmbeddingSiteWindow2))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->GetInterface(aIID, aSink);
  }
  if (aIID.Equals(NS_GET_IID(nsIXULWindow))) {
    NS_ENSURE_STATE(mXULWindow);
    return mXULWindow->QueryInterface(aIID, aSink);
  }

  return QueryInterface(aIID, aSink);
}





NS_IMETHODIMP nsChromeTreeOwner::FindItemWithName(const PRUnichar* aName,
   nsIDocShellTreeItem* aRequestor, nsIDocShellTreeItem* aOriginalRequestor,
   nsIDocShellTreeItem** aFoundItem)
{
   NS_ENSURE_ARG_POINTER(aFoundItem);

   *aFoundItem = nsnull;

   PRBool fIs_Content = PR_FALSE;

   
   if(!aName || !*aName)
      return NS_OK;

   nsDependentString name(aName);

   if(name.LowerCaseEqualsLiteral("_blank"))
      return NS_OK;
   
   
   if(name.LowerCaseEqualsLiteral("_content") || name.EqualsLiteral("_main"))
      {
      NS_ENSURE_STATE(mXULWindow);
      fIs_Content = PR_TRUE;
      mXULWindow->GetPrimaryContentShell(aFoundItem);
      if(*aFoundItem)
         return NS_OK;
      
      }

   nsCOMPtr<nsIWindowMediator> windowMediator(do_GetService(kWindowMediatorCID));
   NS_ENSURE_TRUE(windowMediator, NS_ERROR_FAILURE);

   nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
   NS_ENSURE_SUCCESS(windowMediator->GetXULWindowEnumerator(nsnull, 
      getter_AddRefs(windowEnumerator)), NS_ERROR_FAILURE);
   
   PRBool more;
   
   windowEnumerator->HasMoreElements(&more);
   while(more)
      {
      nsCOMPtr<nsISupports> nextWindow = nsnull;
      windowEnumerator->GetNext(getter_AddRefs(nextWindow));
      nsCOMPtr<nsIXULWindow> xulWindow(do_QueryInterface(nextWindow));
      NS_ENSURE_TRUE(xulWindow, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDocShellTreeItem> shellAsTreeItem;
     
      if(fIs_Content)
         {
         xulWindow->GetPrimaryContentShell(aFoundItem);
         }
      else
         {
         
         
         nsCOMPtr<nsIDocShell> shell;
         xulWindow->GetDocShell(getter_AddRefs(shell));
         shellAsTreeItem = do_QueryInterface(shell);
         if (shellAsTreeItem) {
           
           
           nsCOMPtr<nsIDocShellTreeItem> root;
           shellAsTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
           shellAsTreeItem = root;
         }
         if(shellAsTreeItem && aRequestor != shellAsTreeItem)
            {
            
            
            nsCOMPtr<nsIDocShellTreeOwner> shellOwner;
            shellAsTreeItem->GetTreeOwner(getter_AddRefs(shellOwner));
            nsCOMPtr<nsISupports> shellOwnerSupports(do_QueryInterface(shellOwner));

            shellAsTreeItem->FindItemWithName(aName, shellOwnerSupports,
                                              aOriginalRequestor, aFoundItem);
            }
         }
      if(*aFoundItem)
         return NS_OK;   
      windowEnumerator->HasMoreElements(&more);
      }
   return NS_OK;      
}

NS_IMETHODIMP nsChromeTreeOwner::ContentShellAdded(nsIDocShellTreeItem* aContentShell,
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

NS_IMETHODIMP nsChromeTreeOwner::GetPrimaryContentShell(nsIDocShellTreeItem** aShell)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetPrimaryContentShell(aShell);
}

NS_IMETHODIMP nsChromeTreeOwner::SizeShellTo(nsIDocShellTreeItem* aShellItem,
   PRInt32 aCX, PRInt32 aCY)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SizeShellTo(aShellItem, aCX, aCY);
}

NS_IMETHODIMP
nsChromeTreeOwner::SetPersistence(PRBool aPersistPosition,
                                  PRBool aPersistSize,
                                  PRBool aPersistSizeMode)
{
  NS_ENSURE_STATE(mXULWindow);
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
  if (!docShellElement)
    return NS_ERROR_FAILURE;

  nsAutoString persistString;
  docShellElement->GetAttribute(gLiterals->kPersist, persistString);

  PRBool saveString = PR_FALSE;
  PRInt32 index;

#define FIND_PERSIST_STRING(aString, aCond)            \
  index = persistString.Find(aString);                 \
  if (!aCond && index > kNotFound) {                   \
    persistString.Cut(index, aString.Length());        \
    saveString = PR_TRUE;                              \
  } else if (aCond && index == kNotFound) {            \
    persistString.Append(gLiterals->kSpace + aString); \
    saveString = PR_TRUE;                              \
  }
  FIND_PERSIST_STRING(gLiterals->kScreenX,  aPersistPosition);
  FIND_PERSIST_STRING(gLiterals->kScreenY,  aPersistPosition);
  FIND_PERSIST_STRING(gLiterals->kWidth,    aPersistSize);
  FIND_PERSIST_STRING(gLiterals->kHeight,   aPersistSize);
  FIND_PERSIST_STRING(gLiterals->kSizemode, aPersistSizeMode);

  if (saveString) 
    docShellElement->SetAttribute(gLiterals->kPersist, persistString);

  return NS_OK;
}

NS_IMETHODIMP
nsChromeTreeOwner::GetPersistence(PRBool* aPersistPosition,
                                  PRBool* aPersistSize,
                                  PRBool* aPersistSizeMode)
{
  NS_ENSURE_STATE(mXULWindow);
  nsCOMPtr<nsIDOMElement> docShellElement;
  mXULWindow->GetWindowDOMElement(getter_AddRefs(docShellElement));
  if (!docShellElement) 
    return NS_ERROR_FAILURE;

  nsAutoString persistString;
  docShellElement->GetAttribute(gLiterals->kPersist, persistString);

  
  
  if (aPersistPosition)
    *aPersistPosition = persistString.Find(gLiterals->kScreenX) > kNotFound ||
                        persistString.Find(gLiterals->kScreenY) > kNotFound;
  if (aPersistSize)
    *aPersistSize = persistString.Find(gLiterals->kWidth) > kNotFound ||
                    persistString.Find(gLiterals->kHeight) > kNotFound;
  if (aPersistSizeMode)
    *aPersistSizeMode = persistString.Find(gLiterals->kSizemode) > kNotFound;

  return NS_OK;
}





NS_IMETHODIMP nsChromeTreeOwner::InitWindow(nativeWindow aParentNativeWindow,
   nsIWidget* parentWidget, PRInt32 x, PRInt32 y, PRInt32 cx, PRInt32 cy)   
{
   
   NS_ENSURE_SUCCESS(SetPositionAndSize(x, y, cx, cy, PR_FALSE), NS_ERROR_FAILURE);

   return NS_OK;
}

NS_IMETHODIMP nsChromeTreeOwner::Create()
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP nsChromeTreeOwner::Destroy()
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->Destroy();
}

NS_IMETHODIMP nsChromeTreeOwner::SetPosition(PRInt32 x, PRInt32 y)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetPosition(x, y);
}

NS_IMETHODIMP nsChromeTreeOwner::GetPosition(PRInt32* x, PRInt32* y)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetPosition(x, y);
}

NS_IMETHODIMP nsChromeTreeOwner::SetSize(PRInt32 cx, PRInt32 cy, PRBool fRepaint)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetSize(cx, cy, fRepaint);
}

NS_IMETHODIMP nsChromeTreeOwner::GetSize(PRInt32* cx, PRInt32* cy)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetSize(cx, cy);
}

NS_IMETHODIMP nsChromeTreeOwner::SetPositionAndSize(PRInt32 x, PRInt32 y, PRInt32 cx,
   PRInt32 cy, PRBool fRepaint)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetPositionAndSize(x, y, cx, cy, fRepaint);
}

NS_IMETHODIMP nsChromeTreeOwner::GetPositionAndSize(PRInt32* x, PRInt32* y, PRInt32* cx,
   PRInt32* cy)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetPositionAndSize(x, y, cx, cy);
}

NS_IMETHODIMP nsChromeTreeOwner::Repaint(PRBool aForce)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->Repaint(aForce);
}

NS_IMETHODIMP nsChromeTreeOwner::GetParentWidget(nsIWidget** aParentWidget)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetParentWidget(aParentWidget);
}

NS_IMETHODIMP nsChromeTreeOwner::SetParentWidget(nsIWidget* aParentWidget)
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsChromeTreeOwner::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetParentNativeWindow(aParentNativeWindow);
}

NS_IMETHODIMP nsChromeTreeOwner::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
   NS_ASSERTION(PR_FALSE, "You can't call this");
   return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsChromeTreeOwner::GetVisibility(PRBool* aVisibility)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetVisibility(aVisibility);
}

NS_IMETHODIMP nsChromeTreeOwner::SetVisibility(PRBool aVisibility)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetVisibility(aVisibility);
}

NS_IMETHODIMP nsChromeTreeOwner::GetEnabled(PRBool *aEnabled)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetEnabled(aEnabled);
}

NS_IMETHODIMP nsChromeTreeOwner::SetEnabled(PRBool aEnable)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetEnabled(aEnable);
}

NS_IMETHODIMP nsChromeTreeOwner::GetBlurSuppression(PRBool *aBlurSuppression)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->GetBlurSuppression(aBlurSuppression);
}

NS_IMETHODIMP nsChromeTreeOwner::SetBlurSuppression(PRBool aBlurSuppression)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->SetBlurSuppression(aBlurSuppression);
}

NS_IMETHODIMP nsChromeTreeOwner::GetMainWidget(nsIWidget** aMainWidget)
{
   NS_ENSURE_ARG_POINTER(aMainWidget);
   NS_ENSURE_STATE(mXULWindow);

   *aMainWidget = mXULWindow->mWindow;
   NS_IF_ADDREF(*aMainWidget);

   return NS_OK;
}

NS_IMETHODIMP nsChromeTreeOwner::SetFocus()
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetFocus();
}

NS_IMETHODIMP nsChromeTreeOwner::GetTitle(PRUnichar** aTitle)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->GetTitle(aTitle);
}

NS_IMETHODIMP nsChromeTreeOwner::SetTitle(const PRUnichar* aTitle)
{
   NS_ENSURE_STATE(mXULWindow);
   return mXULWindow->SetTitle(aTitle);
}





NS_IMETHODIMP
nsChromeTreeOwner::OnProgressChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    PRInt32 aCurSelfProgress,
                                    PRInt32 aMaxSelfProgress, 
                                    PRInt32 aCurTotalProgress,
                                    PRInt32 aMaxTotalProgress)
{
   return NS_OK;
}

NS_IMETHODIMP
nsChromeTreeOwner::OnStateChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 PRUint32 aProgressStateFlags,
                                 nsresult aStatus)
{
   return NS_OK;
}

NS_IMETHODIMP nsChromeTreeOwner::OnLocationChange(nsIWebProgress* aWebProgress,
                                                  nsIRequest* aRequest,
                                                  nsIURI* aLocation)
{
  PRBool itsForYou = PR_TRUE;

  if (aWebProgress) {
    NS_ENSURE_STATE(mXULWindow);
    nsCOMPtr<nsIDOMWindow> progressWin;
    aWebProgress->GetDOMWindow(getter_AddRefs(progressWin));

    nsCOMPtr<nsIDocShell> docshell;
    mXULWindow->GetDocShell(getter_AddRefs(docshell));
    nsCOMPtr<nsIDOMWindow> ourWin(do_QueryInterface(docshell));

    if (ourWin != progressWin)
      itsForYou = PR_FALSE;
  }

   
  if (itsForYou) {
    NS_ENSURE_STATE(mXULWindow);
    mXULWindow->mChromeLoaded = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsChromeTreeOwner::OnStatusChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsresult aStatus,
                                  const PRUnichar* aMessage)
{
    return NS_OK;
}



NS_IMETHODIMP 
nsChromeTreeOwner::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
    return NS_OK;
}




NS_IMETHODIMP
nsChromeTreeOwner::ContentShellAdded2(nsIDocShellTreeItem* aContentShell,
                                      PRBool aPrimary, PRBool aTargetable,
                                      const nsAString& aID)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->ContentShellAdded(aContentShell, aPrimary, aTargetable,
                                       aID);
}

NS_IMETHODIMP
nsChromeTreeOwner::ContentShellRemoved(nsIDocShellTreeItem* aContentShell)
{
  NS_ENSURE_STATE(mXULWindow);
  return mXULWindow->ContentShellRemoved(aContentShell);
}









void nsChromeTreeOwner::XULWindow(nsXULWindow* aXULWindow)
{
   mXULWindow = aXULWindow;
}

nsXULWindow* nsChromeTreeOwner::XULWindow()
{
   return mXULWindow;
}
