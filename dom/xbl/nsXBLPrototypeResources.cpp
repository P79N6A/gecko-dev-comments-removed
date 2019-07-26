




#include "nsIStyleRuleProcessor.h"
#include "nsIDocument.h"
#include "nsIContent.h"
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
#include "mozilla/dom/URL.h"

using mozilla::dom::IsChromeURI;

nsXBLPrototypeResources::nsXBLPrototypeResources(nsXBLPrototypeBinding* aBinding)
{
  MOZ_COUNT_CTOR(nsXBLPrototypeResources);

  mLoader = new nsXBLResourceLoader(aBinding, this);
}

nsXBLPrototypeResources::~nsXBLPrototypeResources()
{
  MOZ_COUNT_DTOR(nsXBLPrototypeResources);
  if (mLoader) {
    mLoader->mResources = nullptr;
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

nsresult
nsXBLPrototypeResources::FlushSkinSheets()
{
  if (mStyleSheetList.Length() == 0)
    return NS_OK;

  nsCOMPtr<nsIDocument> doc =
    mLoader->mBinding->XBLDocumentInfo()->GetDocument();

  
  
  if (!doc) {
    return NS_OK;
  }

  
  
  
  mRuleProcessor = nullptr;

  nsTArray<nsRefPtr<nsCSSStyleSheet>> oldSheets;

  oldSheets.SwapElements(mStyleSheetList);

  mozilla::css::Loader* cssLoader = doc->CSSLoader();

  for (size_t i = 0, count = oldSheets.Length(); i < count; ++i) {
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

  GatherRuleProcessor();

  return NS_OK;
}

nsresult
nsXBLPrototypeResources::Write(nsIObjectOutputStream* aStream)
{
  if (mLoader)
    return mLoader->Write(aStream);
  return NS_OK;
}

void
nsXBLPrototypeResources::Traverse(nsCycleCollectionTraversalCallback &cb) const
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "proto mResources mLoader");
  cb.NoteXPCOMChild(mLoader);
}

void
nsXBLPrototypeResources::ClearLoader()
{
  mLoader = nullptr;
}

void
nsXBLPrototypeResources::GatherRuleProcessor()
{
  mRuleProcessor = new nsCSSRuleProcessor(mStyleSheetList,
                                          nsStyleSet::eDocSheet,
                                          nullptr);
}

void
nsXBLPrototypeResources::AppendStyleSheet(nsCSSStyleSheet* aSheet)
{
  mStyleSheetList.AppendElement(aSheet);
}

void
nsXBLPrototypeResources::RemoveStyleSheet(nsCSSStyleSheet* aSheet)
{
  DebugOnly<bool> found = mStyleSheetList.RemoveElement(aSheet);
  MOZ_ASSERT(found, "Trying to remove a sheet that does not exist.");
}

void
nsXBLPrototypeResources::InsertStyleSheetAt(size_t aIndex, nsCSSStyleSheet* aSheet)
{
  mStyleSheetList.InsertElementAt(aIndex, aSheet);
}

nsCSSStyleSheet*
nsXBLPrototypeResources::StyleSheetAt(size_t aIndex) const
{
  return mStyleSheetList[aIndex];
}

size_t
nsXBLPrototypeResources::SheetCount() const
{
  return mStyleSheetList.Length();
}

bool
nsXBLPrototypeResources::HasStyleSheets() const
{
  return !mStyleSheetList.IsEmpty();
}

void
nsXBLPrototypeResources::AppendStyleSheetsTo(
                                      nsTArray<nsCSSStyleSheet*>& aResult) const
{
  aResult.AppendElements(mStyleSheetList);
}
