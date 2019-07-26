




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
#include "mozilla/DebugOnly.h"

using namespace mozilla;
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

  nsTArray<nsRefPtr<CSSStyleSheet>> oldSheets;

  oldSheets.SwapElements(mStyleSheetList);

  mozilla::css::Loader* cssLoader = doc->CSSLoader();

  for (size_t i = 0, count = oldSheets.Length(); i < count; ++i) {
    CSSStyleSheet* oldSheet = oldSheets[i];

    nsIURI* uri = oldSheet->GetSheetURI();

    nsRefPtr<CSSStyleSheet> newSheet;
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
nsXBLPrototypeResources::Traverse(nsCycleCollectionTraversalCallback &cb)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "proto mResources mLoader");
  cb.NoteXPCOMChild(mLoader);

  CycleCollectionNoteChild(cb, mRuleProcessor.get(), "mRuleProcessor");
  ImplCycleCollectionTraverse(cb, mStyleSheetList, "mStyleSheetList");
}

void
nsXBLPrototypeResources::Unlink()
{
  mStyleSheetList.Clear();
  mRuleProcessor = nullptr;
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
nsXBLPrototypeResources::AppendStyleSheet(CSSStyleSheet* aSheet)
{
  mStyleSheetList.AppendElement(aSheet);
}

void
nsXBLPrototypeResources::RemoveStyleSheet(CSSStyleSheet* aSheet)
{
  mStyleSheetList.RemoveElement(aSheet);
}

void
nsXBLPrototypeResources::InsertStyleSheetAt(size_t aIndex, CSSStyleSheet* aSheet)
{
  mStyleSheetList.InsertElementAt(aIndex, aSheet);
}

CSSStyleSheet*
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
                                      nsTArray<CSSStyleSheet*>& aResult) const
{
  aResult.AppendElements(mStyleSheetList);
}
