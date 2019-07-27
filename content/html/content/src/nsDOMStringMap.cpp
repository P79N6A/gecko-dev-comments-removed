





#include "nsDOMStringMap.h"

#include "jsapi.h"
#include "nsError.h"
#include "nsGenericHTMLElement.h"
#include "nsContentUtils.h"
#include "mozilla/dom/DOMStringMapBinding.h"
#include "nsIDOMMutationEvent.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStringMap)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStringMap)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  
  if (tmp->mElement) {
    
    tmp->mElement->ClearDataset();
    tmp->mElement->RemoveMutationObserver(tmp);
    tmp->mElement = nullptr;
  }
  tmp->mExpandoAndGeneration.Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDOMStringMap)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
  if (tmp->PreservingWrapper()) {
    NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mExpandoAndGeneration.expando);
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStringMap)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMStringMap)

nsDOMStringMap::nsDOMStringMap(nsGenericHTMLElement* aElement)
  : mElement(aElement),
    mRemovingProp(false)
{
  mElement->AddMutationObserver(this);
}

nsDOMStringMap::~nsDOMStringMap()
{
  
  if (mElement) {
    
    mElement->ClearDataset();
    mElement->RemoveMutationObserver(this);
  }
}


JSObject*
nsDOMStringMap::WrapObject(JSContext *cx)
{
  return DOMStringMapBinding::Wrap(cx, this);
}

void
nsDOMStringMap::NamedGetter(const nsAString& aProp, bool& found,
                            DOMString& aResult) const
{
  nsAutoString attr;

  if (!DataPropToAttr(aProp, attr)) {
    found = false;
    return;
  }

  found = mElement->GetAttr(attr, aResult);
}

bool
nsDOMStringMap::NameIsEnumerable(const nsAString& aName)
{
  return true;
}

void
nsDOMStringMap::NamedSetter(const nsAString& aProp,
                            const nsAString& aValue,
                            ErrorResult& rv)
{
  nsAutoString attr;
  if (!DataPropToAttr(aProp, attr)) {
    rv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  nsresult res = nsContentUtils::CheckQName(attr, false);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  MOZ_ASSERT(attrAtom, "Should be infallible");

  res = mElement->SetAttr(kNameSpaceID_None, attrAtom, aValue, true);
  if (NS_FAILED(res)) {
    rv.Throw(res);
  }
}

void
nsDOMStringMap::NamedDeleter(const nsAString& aProp, bool& found)
{
  
  if (mRemovingProp) {
    return;
  }
  
  nsAutoString attr;
  if (!DataPropToAttr(aProp, attr)) {
    return;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  MOZ_ASSERT(attrAtom, "Should be infallible");

  found = mElement->HasAttr(kNameSpaceID_None, attrAtom);

  if (found) {
    mRemovingProp = true;
    mElement->UnsetAttr(kNameSpaceID_None, attrAtom, true);
    mRemovingProp = false;
  }
}

void
nsDOMStringMap::GetSupportedNames(unsigned, nsTArray<nsString>& aNames)
{
  uint32_t attrCount = mElement->GetAttrCount();

  
  
  for (uint32_t i = 0; i < attrCount; ++i) {
    const nsAttrName* attrName = mElement->GetAttrNameAt(i);
    
    if (attrName->NamespaceID() != kNameSpaceID_None) {
      continue;
    }

    nsAutoString prop;
    if (!AttrToDataProp(nsDependentAtomString(attrName->LocalName()),
                        prop)) {
      continue;
    }

    aNames.AppendElement(prop);
  }
}





bool nsDOMStringMap::DataPropToAttr(const nsAString& aProp,
                                    nsAutoString& aResult)
{
  
  
  
  aResult.AppendLiteral("data-");

  
  
  
  
  
  const char16_t* start = aProp.BeginReading();
  const char16_t* end = aProp.EndReading();
  const char16_t* cur = start;
  for (; cur < end; ++cur) {
    const char16_t* next = cur + 1;
    if (char16_t('-') == *cur && next < end &&
        char16_t('a') <= *next && *next <= char16_t('z')) {
      
      return false;
    }

    if (char16_t('A') <= *cur && *cur <= char16_t('Z')) {
      
      aResult.Append(start, cur - start);
      
      aResult.Append(char16_t('-'));
      aResult.Append(*cur - 'A' + 'a');
      start = next; 
    }
  }

  aResult.Append(start, cur - start);

  return true;
}





bool nsDOMStringMap::AttrToDataProp(const nsAString& aAttr,
                                    nsAutoString& aResult)
{
  
  
  if (!StringBeginsWith(aAttr, NS_LITERAL_STRING("data-"))) {
    return false;
  }

  
  const char16_t* cur = aAttr.BeginReading() + 5;
  const char16_t* end = aAttr.EndReading();

  
  

  
  
  
  
  for (; cur < end; ++cur) {
    const char16_t* next = cur + 1;
    if (char16_t('-') == *cur && next < end && 
        char16_t('a') <= *next && *next <= char16_t('z')) {
      
      aResult.Append(*next - 'a' + 'A');
      
      ++cur;
    } else {
      
      aResult.Append(*cur);
    }
  }

  return true;
}

void
nsDOMStringMap::AttributeChanged(nsIDocument *aDocument, Element* aElement,
                                 int32_t aNameSpaceID, nsIAtom* aAttribute,
                                 int32_t aModType)
{
  if ((aModType == nsIDOMMutationEvent::ADDITION ||
       aModType == nsIDOMMutationEvent::REMOVAL) &&
      aNameSpaceID == kNameSpaceID_None &&
      StringBeginsWith(nsDependentAtomString(aAttribute),
                       NS_LITERAL_STRING("data-"))) {
    ++mExpandoAndGeneration.generation;
  }
}
