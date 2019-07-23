





































#include <stdio.h>


#include "nsQABrowserView.h"
#include "nsIQABrowserView.h"
#include "nsIInterfaceRequestor.h"


#include "nsIComponentManager.h"



#include "nsCWebBrowser.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserChrome.h"
#include "nsWeakPtr.h"
#include "nsWeakReference.h"

nsQABrowserView::nsQABrowserView():mWebBrowser(nsnull)
{
  mWebBrowser = nsnull;
  mWebNav = nsnull;
  mBaseWindow = nsnull;
}

nsQABrowserView::~nsQABrowserView()
{
}





NS_IMPL_ISUPPORTS2(nsQABrowserView, nsIQABrowserView, nsIInterfaceRequestor)





NS_IMETHODIMP
nsQABrowserView::GetInterface(const nsIID& aIID, void ** aSink)
{
  NS_ENSURE_ARG_POINTER(aSink);  
  printf("In nsQABrowserView::GetInterface\n");
  if (NS_SUCCEEDED(QueryInterface(aIID, aSink)))
    return NS_OK;

  if (mWebBrowser) {
    printf("nsQABrowserView::GetInterface, Got mWebBrowser\n");
    nsCOMPtr<nsIInterfaceRequestor>  ifcReq(do_QueryInterface(mWebBrowser));
    if (ifcReq)
      return ifcReq->GetInterface(aIID, aSink);
  }

  return NS_NOINTERFACE;

}


NS_IMETHODIMP
nsQABrowserView::CreateBrowser(nativeWindow aNativeWnd, nsIWebBrowserChrome * aChrome)
{

  nsresult rv;
  mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID, &rv);
  if (NS_FAILED(rv) || !mWebBrowser)
    return NS_ERROR_FAILURE; 

  mWebNav = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;
  
  
  mWebBrowser->SetContainerWindow(aChrome);
  if(NS_FAILED(rv))
		return rv;

  rv = NS_OK;
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;

  
  dsti->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

  
	mBaseWindow = do_QueryInterface(mWebBrowser, &rv);
	if(NS_FAILED(rv))
		return rv;

  
	rv = mBaseWindow->InitWindow(aNativeWnd, nsnull, 0, 0, -1, -1);
	rv = mBaseWindow->Create();

  
  aChrome->SetWebBrowser(mWebBrowser);

  
	
  nsWeakPtr weakling(do_GetWeakReference(aChrome));
  (void)mWebBrowser->AddWebBrowserListener(weakling, NS_GET_IID(nsIWebProgressListener));

	
	mBaseWindow->SetVisibility(PR_TRUE);

	return NS_OK;
}


NS_IMETHODIMP
nsQABrowserView::DestroyBrowser()
{
  
  mBaseWindow->Destroy();
  mWebBrowser = nsnull;
  mWebNav = nsnull;
  return NS_OK;
}




NS_IMETHODIMP
nsQABrowserView::GetWebBrowser(nsIWebBrowser ** aWebBrowser)
{
  if (!mWebBrowser || !(aWebBrowser))
    return NS_ERROR_FAILURE;
  
  *aWebBrowser = mWebBrowser;
  NS_ADDREF(*aWebBrowser);
  return NS_OK;
}
 
