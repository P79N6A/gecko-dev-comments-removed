




#include "mozilla/dom/SVGDocument.h"

#include "mozilla/css/Loader.h"
#include "nsICategoryManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIStyleSheetService.h"
#include "nsISupportsPrimitives.h"
#include "nsLayoutStylesheetCache.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsLiteralString.h"
#include "nsIDOMSVGElement.h"
#include "mozilla/dom/Element.h"
#include "nsSVGElement.h"
#include "mozilla/dom/SVGDocumentBinding.h"

using namespace mozilla::css;
using namespace mozilla::dom;

namespace mozilla {
namespace dom {







void
SVGDocument::GetDomain(nsAString& aDomain, ErrorResult& aRv)
{
  SetDOMStringToNull(aDomain);

  if (mDocumentURI) {
    nsAutoCString domain;
    nsresult rv = mDocumentURI->GetHost(domain);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return;
    }
    if (domain.IsEmpty()) {
      return;
    }
    CopyUTF8toUTF16(domain, aDomain);
  }
}

nsSVGElement*
SVGDocument::GetRootElement(ErrorResult& aRv)
{
  Element* root = nsDocument::GetRootElement();
  if (!root) {
    return nullptr;
  }
  if (!root->IsSVGElement()) {
    aRv.Throw(NS_NOINTERFACE);
    return nullptr;
  }
  return static_cast<nsSVGElement*>(root);
}

nsresult
SVGDocument::InsertChildAt(nsIContent* aKid, uint32_t aIndex, bool aNotify)
{
  if (aKid->IsElement() && !aKid->IsSVGElement()) {
    
    
    
    
    
    
    EnsureNonSVGUserAgentStyleSheetsLoaded();
  }

  return XMLDocument::InsertChildAt(aKid, aIndex, aNotify);
}

nsresult
SVGDocument::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  NS_ASSERTION(aNodeInfo->NodeInfoManager() == mNodeInfoManager,
               "Can't import this document into another document!");

  nsRefPtr<SVGDocument> clone = new SVGDocument();
  nsresult rv = CloneDocHelper(clone.get());
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(clone.get(), aResult);
}

void
SVGDocument::EnsureNonSVGUserAgentStyleSheetsLoaded()
{
  if (mHasLoadedNonSVGUserAgentStyleSheets) {
    return;
  }

  mHasLoadedNonSVGUserAgentStyleSheets = true;

  if (IsBeingUsedAsImage()) {
    
    
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
    if (catMan) {
      nsCOMPtr<nsISimpleEnumerator> sheets;
      catMan->EnumerateCategory("agent-style-sheets", getter_AddRefs(sheets));
      if (sheets) {
        bool hasMore;
        while (NS_SUCCEEDED(sheets->HasMoreElements(&hasMore)) && hasMore) {
          nsCOMPtr<nsISupports> sheet;
          if (NS_FAILED(sheets->GetNext(getter_AddRefs(sheet))))
            break;

          nsCOMPtr<nsISupportsCString> icStr = do_QueryInterface(sheet);
          MOZ_ASSERT(icStr,
                     "category manager entries must be nsISupportsCStrings");

          nsAutoCString name;
          icStr->GetData(name);

          nsXPIDLCString spec;
          catMan->GetCategoryEntry("agent-style-sheets", name.get(),
                                   getter_Copies(spec));

          mozilla::css::Loader* cssLoader = CSSLoader();
          if (cssLoader->GetEnabled()) {
            nsCOMPtr<nsIURI> uri;
            NS_NewURI(getter_AddRefs(uri), spec);
            if (uri) {
              nsRefPtr<CSSStyleSheet> cssSheet;
              cssLoader->LoadSheetSync(uri, true, true, getter_AddRefs(cssSheet));
              if (cssSheet) {
                EnsureOnDemandBuiltInUASheet(cssSheet);
              }
            }
          }
        }
      }
    }
  }

  CSSStyleSheet* sheet = nsLayoutStylesheetCache::NumberControlSheet();
  if (sheet) {
    
    EnsureOnDemandBuiltInUASheet(sheet);
  }
  EnsureOnDemandBuiltInUASheet(nsLayoutStylesheetCache::FormsSheet());
  EnsureOnDemandBuiltInUASheet(nsLayoutStylesheetCache::CounterStylesSheet());
  EnsureOnDemandBuiltInUASheet(nsLayoutStylesheetCache::HTMLSheet());
  EnsureOnDemandBuiltInUASheet(nsLayoutStylesheetCache::UASheet());
}

JSObject*
SVGDocument::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return SVGDocumentBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 




nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult)
{
  nsRefPtr<SVGDocument> doc = new SVGDocument();

  nsresult rv = doc->Init();
  if (NS_FAILED(rv)) {
    return rv;
  }

  doc.forget(aInstancePtrResult);
  return rv;
}
