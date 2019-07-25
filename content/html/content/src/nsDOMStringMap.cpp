






































#include "nsDOMStringMap.h"

#include "nsDOMClassInfo.h"
#include "nsGenericHTMLElement.h"
#include "nsContentUtils.h"

DOMCI_DATA(DOMStringMap, nsDOMStringMap)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMStringMap)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMStringMap)
  
  tmp->mElement->ClearDataset();
  tmp->mElement = nsnull;
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
    mRemovingProp(PR_FALSE)
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


NS_IMETHODIMP_(PRBool) nsDOMStringMap::HasDataAttr(const nsAString& aProp)
{
  nsAutoString attr;
  if (!DataPropToAttr(aProp, attr)) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  if (!attrAtom) {
    return PR_FALSE;
  }

  return mElement->HasAttr(kNameSpaceID_None, attrAtom);
}


NS_IMETHODIMP nsDOMStringMap::GetDataAttr(const nsAString& aProp,
                                          nsAString& aResult NS_OUTPARAM)
{
  nsAutoString attr;

  if (!DataPropToAttr(aProp, attr)) {
    aResult.SetIsVoid(PR_TRUE);
    return NS_OK;
  }

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);

  if (!mElement->GetAttr(kNameSpaceID_None, attrAtom, aResult)) {
    aResult.SetIsVoid(PR_TRUE);
    return NS_OK;
  }

  return NS_OK;
}


NS_IMETHODIMP nsDOMStringMap::SetDataAttr(const nsAString& aProp,
                                          const nsAString& aValue)
{
  nsAutoString attr;
  NS_ENSURE_TRUE(DataPropToAttr(aProp, attr), NS_ERROR_DOM_SYNTAX_ERR);

  nsresult rv = nsContentUtils::CheckQName(attr, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> attrAtom = do_GetAtom(attr);
  NS_ENSURE_TRUE(attrAtom, NS_ERROR_OUT_OF_MEMORY);

  return mElement->SetAttr(kNameSpaceID_None, attrAtom, aValue, PR_TRUE);
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

  mElement->UnsetAttr(kNameSpaceID_None, attrAtom, PR_TRUE);
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
  nsresult rv = nsContentUtils::WrapNative(cx, JS_GetScopeChain(cx),
                                           this, &val);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, JSVAL_TO_OBJECT(val))) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  
  
  mRemovingProp = PR_TRUE;
  jsval dummy;
  JS_DeleteUCProperty2(cx, JSVAL_TO_OBJECT(val), prop.get(), prop.Length(),
                       &dummy);
  mRemovingProp = PR_FALSE;

  return NS_OK;
}





nsresult nsDOMStringMap::GetDataPropList(nsTArray<nsString>& aResult)
{
  PRUint32 attrCount = mElement->GetAttrCount();

  
  
  for (PRUint32 i = 0; i < attrCount; ++i) {
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





PRBool nsDOMStringMap::DataPropToAttr(const nsAString& aProp,
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
      
      return PR_FALSE;
    }

    if (PRUnichar('A') <= *cur && *cur <= PRUnichar('Z')) {
      
      attr.Append(PRUnichar('-'));
      attr.Append(*cur - 'A' + 'a');
    } else {
      attr.Append(*cur);
    }
  }

  aResult.Assign(attr);
  return PR_TRUE;
}





PRBool nsDOMStringMap::AttrToDataProp(const nsAString& aAttr,
                                      nsAString& aResult)
{
  
  
  if (!StringBeginsWith(aAttr, NS_LITERAL_STRING("data-"))) {
    return PR_FALSE;
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
  return PR_TRUE;
}
