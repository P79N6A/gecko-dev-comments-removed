





































#include "nsIStyleRuleProcessor.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsIXBLService.h"
#include "nsIServiceManager.h"
#include "nsXBLResourceLoader.h"
#include "nsXBLPrototypeResources.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDocumentObserver.h"
#include "mozilla/css/Loader.h"
#include "nsIURI.h"
#include "nsLayoutCID.h"
#include "nsCSSRuleProcessor.h"
#include "nsStyleSet.h"

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
nsXBLPrototypeResources::LoadResources(bool* aResult)
{
  if (mLoader)
    mLoader->LoadResources(aResult);
  else
    *aResult = true; 
}

void
nsXBLPrototypeResources::AddResourceListener(nsIContent* aBoundElement) 
{
  if (mLoader)
    mLoader->AddResourceListener(aBoundElement);
}

static bool IsChromeURI(nsIURI* aURI)
{
  bool isChrome=false;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome)
    return true;
  return false;
}

nsresult
nsXBLPrototypeResources::FlushSkinSheets()
{
  if (mStyleSheetList.Length() == 0)
    return NS_OK;

  nsCOMPtr<nsIDocument> doc =
    mLoader->mBinding->XBLDocumentInfo()->GetDocument();
  mozilla::css::Loader* cssLoader = doc->CSSLoader();

  
  
  
  mRuleProcessor = nsnull;

  sheet_array_type oldSheets(mStyleSheetList);
  mStyleSheetList.Clear();

  for (sheet_array_type::size_type i = 0, count = oldSheets.Length();
       i < count; ++i) {
    nsCSSStyleSheet* oldSheet = oldSheets[i];

    nsIURI* uri = oldSheet->GetSheetURI();

    nsRefPtr<nsCSSStyleSheet> newSheet;
    if (IsChromeURI(uri)) {
      if (NS_FAILED(cssLoader->LoadSheetSync(uri, getter_AddRefs(newSheet))))
        continue;
    }
    else {
      newSheet = oldSheet;
    }

    mStyleSheetList.AppendElement(newSheet);
  }
  mRuleProcessor = new nsCSSRuleProcessor(mStyleSheetList, 
                                          nsStyleSet::eDocSheet);

  return NS_OK;
}

nsresult
nsXBLPrototypeResources::Write(nsIObjectOutputStream* aStream)
{
  if (mLoader)
    return mLoader->Write(aStream);
  return NS_OK;
}
