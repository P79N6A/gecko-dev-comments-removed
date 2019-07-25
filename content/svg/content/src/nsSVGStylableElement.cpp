





































#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsDOMCSSDeclaration.h"




NS_IMPL_ADDREF_INHERITED(nsSVGStylableElement, nsSVGStylableElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGStylableElement, nsSVGStylableElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGStylableElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGStylable)
NS_INTERFACE_MAP_END_INHERITING(nsSVGStylableElementBase)




nsSVGStylableElement::nsSVGStylableElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGStylableElementBase(aNodeInfo)
{
}




const nsAttrValue*
nsSVGStylableElement::DoGetClasses() const
{
  if (mClassAttribute.IsAnimated()) {
    return mClassAnimAttr;
  }
  return nsSVGStylableElementBase::DoGetClasses();
}

bool
nsSVGStylableElement::ParseAttribute(PRInt32 aNamespaceID,
                                     nsIAtom* aAttribute,
                                     const nsAString& aValue,
                                     nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::_class) {
    mClassAttribute.SetBaseValue(aValue, this, false);
    aResult.ParseAtomArray(aValue);
    return true;
  }
  return nsSVGStylableElementBase::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                                  aResult);
}

nsresult
nsSVGStylableElement::UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                bool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None && aName == nsGkAtoms::_class) {
    mClassAttribute.Init();
  }
  return nsSVGStylableElementBase::UnsetAttr(aNamespaceID, aName, aNotify);
}





NS_IMETHODIMP
nsSVGStylableElement::GetClassName(nsIDOMSVGAnimatedString** aClassName)
{
  return mClassAttribute.ToDOMAnimatedString(aClassName, this);
}


NS_IMETHODIMP
nsSVGStylableElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsresult rv;
  *aStyle = GetStyle(&rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  NS_ADDREF(*aStyle);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGStylableElement::GetPresentationAttribute(const nsAString& aName,
                                               nsIDOMCSSValue** aReturn)
{
  
  
  

  return NS_ERROR_NOT_IMPLEMENTED;
}




void
nsSVGStylableElement::DidAnimateClass()
{
  nsAutoString src;
  mClassAttribute.GetAnimValue(src, this);
  if (!mClassAnimAttr) {
    mClassAnimAttr = new nsAttrValue();
  }
  mClassAnimAttr->ParseAtomArray(src);

  nsIPresShell* shell = OwnerDoc()->GetShell();
  if (shell) {
    shell->RestyleForAnimation(this, eRestyle_Self);
  }
}

nsISMILAttr*
nsSVGStylableElement::GetAnimatedAttr(PRInt32 aNamespaceID, nsIAtom* aName)
{
  if (aNamespaceID == kNameSpaceID_None && 
      aName == nsGkAtoms::_class) {
    return mClassAttribute.ToSMILAttr(this);
  }
  return nsSVGStylableElementBase::GetAnimatedAttr(aNamespaceID, aName);
}
