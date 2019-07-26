





#include "nsError.h"
#include "nsDOMStringMap.h"

#include "nsGenericHTMLElement.h"
#include "nsContentUtils.h"
#include "mozilla/dom/DOMStringMapBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStringMap)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
  
  if (tmp->mElement) {
    
    tmp->mElement->ClearDataset();
    tmp->mElement = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDOMStringMap)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStringMap)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMStringMap)

nsDOMStringMap::nsDOMStringMap(nsGenericHTMLElement* aElement)
  : mElement(aElement),
    mRemovingProp(false)
{
  SetIsDOMBinding();
}

nsDOMStringMap::~nsDOMStringMap()
{
  
  if (mElement) {
    
    mElement->ClearDataset();
  }
}


JSObject*
nsDOMStringMap::WrapObject(JSContext *cx, JSObject *scope)
{
  return DOMStringMapBinding::Wrap(cx, scope, this);
}

void
nsDOMStringMap::NamedGetter(const nsAString& aProp, bool& found,
                            nsString& aResult) const
{
  nsAutoString attr;

  if (!DataPropToAttr(aProp, attr)) {
    found = false;
    return;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  MOZ_ASSERT(attrAtom, "Should be infallible");

  found = mElement->GetAttr(kNameSpaceID_None, attrAtom, aResult);
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
nsDOMStringMap::GetSupportedNames(nsTArray<nsString>& aNames)
{
  uint32_t attrCount = mElement->GetAttrCount();

  
  
  for (uint32_t i = 0; i < attrCount; ++i) {
    nsAutoString attrString;
    const nsAttrName* attrName = mElement->GetAttrNameAt(i);
    
    if (attrName->NamespaceID() != kNameSpaceID_None) {
      continue;
    }
    attrName->LocalName()->ToString(attrString);

    nsAutoString prop;
    if (!AttrToDataProp(attrString, prop)) {
      continue;
    }

    aNames.AppendElement(prop);
  }
}





bool nsDOMStringMap::DataPropToAttr(const nsAString& aProp,
                                      nsAString& aResult)
{
  const PRUnichar* cur = aProp.BeginReading();
  const PRUnichar* end = aProp.EndReading();

  
  nsAutoString attr;
  
  attr.SetCapacity(aProp.Length() + 5);

  attr.Append(NS_LITERAL_STRING("data-"));

  
  
  
  
  
  for (; cur < end; ++cur) {
    const PRUnichar* next = cur + 1;
    if (PRUnichar('-') == *cur && next < end &&
        PRUnichar('a') <= *next && *next <= PRUnichar('z')) {
      
      return false;
    }

    if (PRUnichar('A') <= *cur && *cur <= PRUnichar('Z')) {
      
      attr.Append(PRUnichar('-'));
      attr.Append(*cur - 'A' + 'a');
    } else {
      attr.Append(*cur);
    }
  }

  aResult.Assign(attr);
  return true;
}





bool nsDOMStringMap::AttrToDataProp(const nsAString& aAttr,
                                      nsAString& aResult)
{
  
  
  if (!StringBeginsWith(aAttr, NS_LITERAL_STRING("data-"))) {
    return false;
  }

  
  const PRUnichar* cur = aAttr.BeginReading() + 5;
  const PRUnichar* end = aAttr.EndReading();

  
  
  nsAutoString prop;

  
  
  
  
  for (; cur < end; ++cur) {
    const PRUnichar* next = cur + 1;
    if (PRUnichar('-') == *cur && next < end && 
        PRUnichar('a') <= *next && *next <= PRUnichar('z')) {
      
      prop.Append(*next - 'a' + 'A');
      
      ++cur;
    } else {
      
      prop.Append(*cur);
    }
  }

  aResult.Assign(prop);
  return true;
}
