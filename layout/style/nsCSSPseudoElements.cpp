







































#include "nsCSSPseudoElements.h"
#include "nsAtomListUtils.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"
#include "nsCSSAnonBoxes.h"


#define CSS_PSEUDO_ELEMENT(name_, value_, flags_) \
  nsICSSPseudoElement* nsCSSPseudoElements::name_;
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT

static const nsStaticAtom CSSPseudoElements_info[] = {
#define CSS_PSEUDO_ELEMENT(name_, value_, flags_) \
    { value_, (nsIAtom**)&nsCSSPseudoElements::name_ },
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT
};






static const PRUint32 CSSPseudoElements_flags[] = {
#define CSS_PSEUDO_ELEMENT(name_, value_, flags_) \
  flags_,
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

 PRBool
nsCSSPseudoElements::IsCSS2PseudoElement(nsIAtom *aAtom)
{
  
  
  NS_ASSERTION(nsCSSPseudoElements::IsPseudoElement(aAtom) ||
               nsCSSAnonBoxes::IsAnonBox(aAtom),
               "must be pseudo element or anon box");
  PRBool result = aAtom == nsCSSPseudoElements::after ||
                  aAtom == nsCSSPseudoElements::before ||
                  aAtom == nsCSSPseudoElements::firstLetter ||
                  aAtom == nsCSSPseudoElements::firstLine;
  NS_ASSERTION(nsCSSAnonBoxes::IsAnonBox(aAtom) ||
               result ==
                 PseudoElementHasFlags(aAtom, CSS_PSEUDO_ELEMENT_IS_CSS2),
               "result doesn't match flags");
  return result;
}

 nsCSSPseudoElements::Type
nsCSSPseudoElements::GetPseudoType(nsIAtom *aAtom)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(CSSPseudoElements_info); ++i) {
    if (*CSSPseudoElements_info[i].mAtom == aAtom) {
      return Type(i);
    }
  }

  if (nsCSSAnonBoxes::IsAnonBox(aAtom)) {
#ifdef MOZ_XUL
    if (nsCSSAnonBoxes::IsTreePseudoElement(aAtom)) {
      return ePseudo_XULTree;
    }
#endif

    return ePseudo_AnonBox;
  }

  return ePseudo_NotPseudoElement;
}

 PRUint32
nsCSSPseudoElements::FlagsForPseudoElement(nsIAtom *aAtom)
{
  PRUint32 i;
  for (i = 0; i < NS_ARRAY_LENGTH(CSSPseudoElements_info); ++i) {
    if (*CSSPseudoElements_info[i].mAtom == aAtom) {
      break;
    }
  }
  NS_ASSERTION(i < NS_ARRAY_LENGTH(CSSPseudoElements_info),
               "argument must be a pseudo-element");
  return CSSPseudoElements_flags[i];
}
