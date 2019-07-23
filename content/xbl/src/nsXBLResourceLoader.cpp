





































#include "nsICSSStyleSheet.h"
#include "nsIStyleRuleProcessor.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsIXBLService.h"
#include "nsIServiceManager.h"
#include "nsXBLResourceLoader.h"
#include "nsXBLPrototypeResources.h"
#include "nsIDocumentObserver.h"
#include "imgILoader.h"
#include "imgIRequest.h"
#include "nsICSSLoader.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsGkAtoms.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsXBLPrototypeBinding.h"
#include "nsCSSRuleProcessor.h"
#include "nsContentUtils.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXBLResourceLoader)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsXBLResourceLoader)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXBLResourceLoader)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mBoundElements)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsXBLResourceLoader)
  NS_INTERFACE_MAP_ENTRY(nsICSSLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsXBLResourceLoader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsXBLResourceLoader)

nsXBLResourceLoader::nsXBLResourceLoader(nsXBLPrototypeBinding* aBinding,
                                         nsXBLPrototypeResources* aResources)
:mBinding(aBinding),
 mResources(aResources),
 mResourceList(nsnull),
 mLastResource(nsnull),
 mLoadingResources(PR_FALSE),
 mInLoadResourcesFunc(PR_FALSE),
 mPendingSheets(0)
{
}

nsXBLResourceLoader::~nsXBLResourceLoader()
{
  delete mResourceList;
}

void
nsXBLResourceLoader::LoadResources(PRBool* aResult)
{
  mInLoadResourcesFunc = PR_TRUE;

  if (mLoadingResources) {
    *aResult = (mPendingSheets == 0);
    mInLoadResourcesFunc = PR_FALSE;
    return;
  }

  mLoadingResources = PR_TRUE;
  *aResult = PR_TRUE;

  
  nsCOMPtr<nsIDocument> doc;
  mBinding->XBLDocumentInfo()->GetDocument(getter_AddRefs(doc));

  nsICSSLoader* cssLoader = doc->CSSLoader();
  nsIURI *docURL = doc->GetDocumentURI();
  nsIPrincipal* docPrincipal = doc->NodePrincipal();

  nsCOMPtr<nsIURI> url;

  for (nsXBLResource* curr = mResourceList; curr; curr = curr->mNext) {
    if (curr->mSrc.IsEmpty())
      continue;

    if (NS_FAILED(NS_NewURI(getter_AddRefs(url), curr->mSrc,
                            doc->GetDocumentCharacterSet().get(), docURL)))
      continue;

    if (curr->mType == nsGkAtoms::image) {
      if (!nsContentUtils::CanLoadImage(url, doc, doc, docPrincipal)) {
        
        continue;
      }

      
      
      
      nsCOMPtr<imgIRequest> req;
      nsContentUtils::LoadImage(url, doc, docPrincipal, docURL, nsnull,
                                nsIRequest::LOAD_BACKGROUND,
                                getter_AddRefs(req));
    }
    else if (curr->mType == nsGkAtoms::stylesheet) {
      

      
      
      PRBool chrome;
      nsresult rv;
      if (NS_SUCCEEDED(url->SchemeIs("chrome", &chrome)) && chrome)
      {
        nsCOMPtr<nsICSSStyleSheet> sheet;
        rv = cssLoader->LoadSheetSync(url, getter_AddRefs(sheet));
        NS_ASSERTION(NS_SUCCEEDED(rv), "Load failed!!!");
        if (NS_SUCCEEDED(rv))
        {
          rv = StyleSheetLoaded(sheet, PR_FALSE, NS_OK);
          NS_ASSERTION(NS_SUCCEEDED(rv), "Processing the style sheet failed!!!");
        }
      }
      else
      {
        rv = cssLoader->LoadSheet(url, docPrincipal, this);
        if (NS_SUCCEEDED(rv))
          ++mPendingSheets;
      }
    }
  }

  *aResult = (mPendingSheets == 0);
  mInLoadResourcesFunc = PR_FALSE;
  
  
  delete mResourceList;
  mResourceList = nsnull;
}


NS_IMETHODIMP
nsXBLResourceLoader::StyleSheetLoaded(nsICSSStyleSheet* aSheet,
                                      PRBool aWasAlternate,
                                      nsresult aStatus)
{
  if (!mResources) {
    
    return NS_OK;
  }
   
  mResources->mStyleSheetList.AppendObject(aSheet);

  if (!mInLoadResourcesFunc)
    mPendingSheets--;
  
  if (mPendingSheets == 0) {
    
    mResources->mRuleProcessor =
      new nsCSSRuleProcessor(mResources->mStyleSheetList);

    
    if (!mInLoadResourcesFunc)
      NotifyBoundElements();
  }
  return NS_OK;
}

void 
nsXBLResourceLoader::AddResource(nsIAtom* aResourceType, const nsAString& aSrc)
{
  nsXBLResource* res = new nsXBLResource(aResourceType, aSrc);
  if (!res)
    return;

  if (!mResourceList)
    mResourceList = res;
  else
    mLastResource->mNext = res;

  mLastResource = res;
}

void
nsXBLResourceLoader::AddResourceListener(nsIContent* aBoundElement) 
{
  if (aBoundElement) {
    mBoundElements.AppendObject(aBoundElement);
  }
}

void
nsXBLResourceLoader::NotifyBoundElements()
{
  nsCOMPtr<nsIXBLService> xblService(do_GetService("@mozilla.org/xbl;1"));
  nsIURI* bindingURI = mBinding->BindingURI();

  PRUint32 eltCount = mBoundElements.Count();
  for (PRUint32 j = 0; j < eltCount; j++) {
    nsCOMPtr<nsIContent> content = mBoundElements.ObjectAt(j);
    
    PRBool ready = PR_FALSE;
    xblService->BindingReady(content, bindingURI, &ready);

    if (ready) {
      
      
      nsIDocument* doc = content->GetCurrentDoc();
    
      if (doc) {
        
        doc->FlushPendingNotifications(Flush_Frames);

        
        
        
        
        
        
        
        
        
        nsIPresShell *shell = doc->GetPrimaryShell();
        if (shell) {
          nsIFrame* childFrame = shell->GetPrimaryFrameFor(content);
          if (!childFrame) {
            
            nsStyleContext* sc =
              shell->FrameManager()->GetUndisplayedContent(content);

            if (!sc) {
              shell->RecreateFramesFor(content);
            }
          }
        }

        
        
        doc->FlushPendingNotifications(Flush_ContentAndNotify);
      }
    }
  }

  
  mBoundElements.Clear();

  
  NS_RELEASE(mResources->mLoader);
}
