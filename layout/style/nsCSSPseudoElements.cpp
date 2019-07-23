







































#include "nsCSSPseudoElements.h"
#include "nsAtomListUtils.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"


#define CSS_PSEUDO_ELEMENT(_name, _value) \
  nsICSSPseudoElement* nsCSSPseudoElements::_name;
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT

static const nsStaticAtom CSSPseudoElements_info[] = {
#define CSS_PSEUDO_ELEMENT(name_, value_) \
    { value_, (nsIAtom**)&nsCSSPseudoElements::name_ },
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT
};

void nsCSSPseudoElements::AddRefAtoms()
{
  NS_RegisterStaticAtoms(CSSPseudoElements_info,
                         NS_ARRAY_LENGTH(CSSPseudoElements_info));
}

PRBool nsCSSPseudoElements::IsPseudoElement(nsIAtom *aAtom)
{
  return nsAtomListUtils::IsMember(aAtom, CSSPseudoElements_info,
                                   NS_ARRAY_LENGTH(CSSPseudoElements_info));
}

PRBool nsCSSPseudoElements::IsCSS2PseudoElement(nsIAtom *aAtom)
{
#define CSS2_PSEUDO_ELEMENT(name_, value_) \
   nsCSSPseudoElements::name_ == aAtom ||
#define CSS_PSEUDO_ELEMENT(name_, value_)
  return
#include "nsCSSPseudoElementList.h"
    PR_FALSE;
#undef CSS_PSEUDO_ELEMENT
#undef CSS2_PSEUDO_ELEMENT
}
