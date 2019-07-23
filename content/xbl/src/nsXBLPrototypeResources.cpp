





































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
#include "nsICSSLoader.h"
#include "nsIURI.h"
#include "nsLayoutCID.h"
#include "nsCSSRuleProcessor.h"

static NS_DEFINE_CID(kCSSLoaderCID, NS_CSS_LOADER_CID);

nsXBLPrototypeResources::nsXBLPrototypeResources(nsXBLPrototypeBinding* aBinding)
{
  MOZ_COUNT_CTOR(nsXBLPrototypeResources);

  mLoader = new nsXBLResourceLoader(aBinding, this);
  NS_IF_ADDREF(mLoader);
}

nsXBLPrototypeResources::~nsXBLPrototypeResources()
{
  MOZ_COUNT_DTOR(nsXBLPrototypeResources);
  if (mLoader) {
    mLoader->mResources = nsnull;
    NS_RELEASE(mLoader);
  }
}

void 
nsXBLPrototypeResources::AddResource(nsIAtom* aResourceType, const nsAString& aSrc)
{
  if (mLoader)
    mLoader->AddResource(aResourceType, aSrc);
}
 
void
nsXBLPrototypeResources::LoadResources(PRBool* aResult)
{
  if (mLoader)
    mLoader->LoadResources(aResult);
  else
    *aResult = PR_TRUE; 
}

void
nsXBLPrototypeResources::AddResourceListener(nsIContent* aBoundElement) 
{
  if (mLoader)
    mLoader->AddResourceListener(aBoundElement);
}

static PRBool IsChromeURI(nsIURI* aURI)
{
  PRBool isChrome=PR_FALSE;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome)
    return PR_TRUE;
  return PR_FALSE;
}

nsresult
nsXBLPrototypeResources::FlushSkinSheets()
{
  if (mStyleSheetList.Count() == 0)
    return NS_OK;

  nsresult rv;
  
  nsCOMPtr<nsICSSLoader> loader = do_CreateInstance(kCSSLoaderCID, &rv);
  if (NS_FAILED(rv)) return rv;
  
  
  
  
  mRuleProcessor = nsnull;

  nsCOMArray<nsICSSStyleSheet> oldSheets(mStyleSheetList);
  mStyleSheetList.Clear();
  
  PRInt32 i;
  PRInt32 count = oldSheets.Count();
  for (i = 0; i < count; i++) {
    nsICSSStyleSheet* oldSheet = oldSheets[i];
    
    nsCOMPtr<nsIURI> uri;
    oldSheet->GetSheetURI(getter_AddRefs(uri));

    nsCOMPtr<nsICSSStyleSheet> newSheet;
    if (IsChromeURI(uri)) {
      if (NS_FAILED(loader->LoadSheetSync(uri, getter_AddRefs(newSheet))))
        continue;
    }
    else {
      newSheet = oldSheet;
    }
    
    mStyleSheetList.AppendObject(newSheet);
  }
  mRuleProcessor = new nsCSSRuleProcessor(mStyleSheetList);
  
  return NS_OK;
}
