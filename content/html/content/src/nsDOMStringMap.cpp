





#include "nsError.h"
#include "nsDOMStringMap.h"

#include "nsDOMClassInfoID.h"
#include "nsGenericHTMLElement.h"
#include "nsContentUtils.h"

DOMCI_DATA(DOMStringMap, nsDOMStringMap)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStringMap)
  
  if (tmp->mElement) {
    
    tmp->mElement->ClearDataset();
    tmp->mElement = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMStringMap)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMStringMap)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMStringMap)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMStringMap)

nsDOMStringMap::nsDOMStringMap(nsGenericHTMLElement* aElement)
  : mElement(aElement),
    mRemovingProp(false)
{
}

nsDOMStringMap::~nsDOMStringMap()
{
  
  if (mElement) {
    
    mElement->ClearDataset();
  }
}

class nsDOMStringMapRemoveProp : public nsRunnable {
public:
  nsDOMStringMapRemoveProp(nsDOMStringMap* aDataset, nsIAtom* aProperty)
  : mDataset(aDataset),
    mProperty(aProperty)
  {
  }

  NS_IMETHOD Run()
  {
    return mDataset->RemovePropInternal(mProperty);
  }

  virtual ~nsDOMStringMapRemoveProp()
  {
  }

protected:
  nsRefPtr<nsDOMStringMap> mDataset;
  nsCOMPtr<nsIAtom> mProperty;
};


NS_IMETHODIMP_(bool) nsDOMStringMap::HasDataAttr(const nsAString& aProp)
{
  nsAutoString attr;
  if (!DataPropToAttr(aProp, attr)) {
    return false;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  if (!attrAtom) {
    return false;
  }

  return mElement->HasAttr(kNameSpaceID_None, attrAtom);
}


NS_IMETHODIMP nsDOMStringMap::GetDataAttr(const nsAString& aProp,
                                          nsAString& aResult)
{
  nsAutoString attr;

  if (!DataPropToAttr(aProp, attr)) {
    aResult.SetIsVoid(true);
    return NS_OK;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);

  if (!mElement->GetAttr(kNameSpaceID_None, attrAtom, aResult)) {
    aResult.SetIsVoid(true);
    return NS_OK;
  }

  return NS_OK;
}


NS_IMETHODIMP nsDOMStringMap::SetDataAttr(const nsAString& aProp,
                                          const nsAString& aValue)
{
  nsAutoString attr;
  NS_ENSURE_TRUE(DataPropToAttr(aProp, attr), NS_ERROR_DOM_SYNTAX_ERR);

  nsresult rv = nsContentUtils::CheckQName(attr, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);

  return mElement->SetAttr(kNameSpaceID_None, attrAtom, aValue, true);
}


NS_IMETHODIMP_(void) nsDOMStringMap::RemoveDataAttr(const nsAString& aProp)
{
  
  if (mRemovingProp) {
    return;
  }
  
  nsAutoString attr;
  if (!DataPropToAttr(aProp, attr)) {
    return;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  if (!attrAtom) {
    return;
  }

  mElement->UnsetAttr(kNameSpaceID_None, attrAtom, true);
}

nsGenericHTMLElement* nsDOMStringMap::GetElement()
{
  return mElement;
}


NS_IMETHODIMP_(void) nsDOMStringMap::RemoveProp(nsIAtom* aAttr)
{
  nsContentUtils::AddScriptRunner(new nsDOMStringMapRemoveProp(this, aAttr));
}

nsresult nsDOMStringMap::RemovePropInternal(nsIAtom* aAttr)
{
  nsAutoString attr;
  aAttr->ToString(attr);
  nsAutoString prop;
  NS_ENSURE_TRUE(AttrToDataProp(attr, prop), NS_OK);

  jsval val;
  JSContext* cx = nsContentUtils::GetCurrentJSContext();
  nsresult rv = nsContentUtils::WrapNative(cx, JS_GetGlobalForScopeChain(cx),
                                           this, &val);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, JSVAL_TO_OBJECT(val))) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  
  
  mRemovingProp = true;
  jsval dummy;
  JS_DeleteUCProperty2(cx, JSVAL_TO_OBJECT(val), prop.get(), prop.Length(),
                       &dummy);
  mRemovingProp = false;

  return NS_OK;
}





nsresult nsDOMStringMap::GetDataPropList(nsTArray<nsString>& aResult)
{
  uint32_t attrCount = mElement->GetAttrCount();

  
  
  for (uint32_t i = 0; i < attrCount; ++i) {
    nsAutoString attrString;
    const nsAttrName* attrName = mElement->GetAttrNameAt(i);
    attrName->LocalName()->ToString(attrString);

    nsAutoString prop;
    if (!AttrToDataProp(attrString, prop)) {
      continue;
    }

    aResult.AppendElement(prop);
  }

  return NS_OK;
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
