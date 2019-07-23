





































#include "nsSVGStylableElement.h"
#include "nsICSSOMFactory.h"
#include "nsSVGAnimatedString.h"
#include "nsGkAtoms.h"
#include "nsDOMCSSDeclaration.h"
#include "nsIDOMClassInfo.h"

static NS_DEFINE_CID(kCSSOMFactoryCID, NS_CSSOMFACTORY_CID);




NS_IMPL_ADDREF_INHERITED(nsSVGStylableElement, nsSVGStylableElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGStylableElement, nsSVGStylableElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGStylableElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGStylable)
NS_INTERFACE_MAP_END_INHERITING(nsSVGStylableElementBase)




nsSVGStylableElement::nsSVGStylableElement(nsINodeInfo *aNodeInfo)
  : nsSVGStylableElementBase(aNodeInfo)
{

}

nsresult
nsSVGStylableElement::Init()
{
  nsresult rv = nsSVGStylableElementBase::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  

  
  {
    mClassName = new nsSVGClassValue;
    NS_ENSURE_TRUE(mClassName, NS_ERROR_OUT_OF_MEMORY);
    rv = AddMappedSVGValue(nsGkAtoms::_class,
			   NS_STATIC_CAST(nsIDOMSVGAnimatedString*, mClassName),
			   kNameSpaceID_None);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}




const nsAttrValue*
nsSVGStylableElement::GetClasses() const
{
  return mClassName->GetAttrValue();
}





NS_IMETHODIMP
nsSVGStylableElement::GetClassName(nsIDOMSVGAnimatedString** aClassName)
{
  NS_ADDREF(*aClassName = mClassName);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGStylableElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsDOMSlots *slots = GetDOMSlots();

  if (!slots->mStyle) {
    nsICSSOMFactory* cssOMFactory = nsnull;
    
    
    nsresult rv = CallGetService(kCSSOMFactoryCID, &cssOMFactory);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = cssOMFactory->
      CreateDOMCSSAttributeDeclaration(this, getter_AddRefs(slots->mStyle));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aStyle = slots->mStyle);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGStylableElement::GetPresentationAttribute(const nsAString& aName,
						nsIDOMCSSValue** aReturn)
{
  
  
  

  return NS_ERROR_NOT_IMPLEMENTED;
}
