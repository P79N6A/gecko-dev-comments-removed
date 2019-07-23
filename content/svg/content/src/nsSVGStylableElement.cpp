





































#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsDOMCSSDeclaration.h"
#include "nsContentUtils.h"




NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGStylableElement::DOMAnimatedClassString, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGStylableElement::DOMAnimatedClassString)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGStylableElement::DOMAnimatedClassString)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGStylableElement::DOMAnimatedClassString)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedString)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(nsSVGStylableElement, nsSVGStylableElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGStylableElement, nsSVGStylableElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGStylableElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGStylable)
NS_INTERFACE_MAP_END_INHERITING(nsSVGStylableElementBase)




nsSVGStylableElement::nsSVGStylableElement(nsINodeInfo *aNodeInfo)
  : nsSVGStylableElementBase(aNodeInfo)
{
}




const nsAttrValue*
nsSVGStylableElement::DoGetClasses() const
{
  return GetClassAnimAttr();
}





NS_IMETHODIMP
nsSVGStylableElement::GetClassName(nsIDOMSVGAnimatedString** aClassName)
{
  *aClassName = new DOMAnimatedClassString(this);
  NS_ENSURE_TRUE(*aClassName, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*aClassName);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGStylableElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  return nsSVGStylableElementBase::GetStyle(aStyle);
}


NS_IMETHODIMP
nsSVGStylableElement::GetPresentationAttribute(const nsAString& aName,
                                               nsIDOMCSSValue** aReturn)
{
  
  
  

  return NS_ERROR_NOT_IMPLEMENTED;
}




PRBool
nsSVGStylableElement::ParseAttribute(PRInt32 aNamespaceID,
                                     nsIAtom* aAttribute,
                                     const nsAString& aValue,
                                     nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::_class) {
    mClassAnimAttr = nsnull;
    
  }

  return nsSVGStylableElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                                   aValue, aResult);
}

nsresult
nsSVGStylableElement::UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                PRBool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None && aName == nsGkAtoms::_class) {
    mClassAnimAttr = nsnull;
  }

  return nsSVGStylableElementBase::UnsetAttr(aNamespaceID, aName, aNotify);
}




const nsAttrValue*
nsSVGStylableElement::GetClassAnimAttr() const
{
  if (mClassAnimAttr)
    return mClassAnimAttr;

  return mAttrsAndChildren.GetAttr(nsGkAtoms::_class, kNameSpaceID_None);
}

void
nsSVGStylableElement::GetClassBaseValString(nsAString& aResult) const
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::_class, aResult);
}

void
nsSVGStylableElement::SetClassBaseValString(const nsAString& aValue)
{
  mClassAnimAttr = nsnull;
  SetAttr(kNameSpaceID_None, nsGkAtoms::_class, aValue, PR_TRUE); 
}

void
nsSVGStylableElement::GetClassAnimValString(nsAString& aResult) const
{
  const nsAttrValue* attr = GetClassAnimAttr();

  if (!attr) {
    aResult.Truncate();
    return;
  }

  attr->ToString(aResult);
}
