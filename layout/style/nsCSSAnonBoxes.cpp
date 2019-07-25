







































#include "nsCSSAnonBoxes.h"
#include "nsAtomListUtils.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"
#include "nsCRT.h"


#define CSS_ANON_BOX(_name, _value) \
  nsICSSAnonBoxPseudo* nsCSSAnonBoxes::_name;
#include "nsCSSAnonBoxList.h"
#undef CSS_ANON_BOX

#define CSS_ANON_BOX(name_, value_) \
  NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsCSSAnonBoxList.h"
#undef CSS_ANON_BOX

static const nsStaticAtom CSSAnonBoxes_info[] = {
#define CSS_ANON_BOX(name_, value_) \
  NS_STATIC_ATOM(name_##_buffer, (nsIAtom**)&nsCSSAnonBoxes::name_),
#include "nsCSSAnonBoxList.h"
#undef CSS_ANON_BOX
};

void nsCSSAnonBoxes::AddRefAtoms()
{
  NS_RegisterStaticAtoms(CSSAnonBoxes_info,
                         NS_ARRAY_LENGTH(CSSAnonBoxes_info));
}

PRBool nsCSSAnonBoxes::IsAnonBox(nsIAtom *aAtom)
{
  return nsAtomListUtils::IsMember(aAtom, CSSAnonBoxes_info,
                                   NS_ARRAY_LENGTH(CSSAnonBoxes_info));
}

#ifdef MOZ_XUL
 PRBool
nsCSSAnonBoxes::IsTreePseudoElement(nsIAtom* aPseudo)
{
  return StringBeginsWith(nsDependentAtomString(aPseudo),
                          NS_LITERAL_STRING(":-moz-tree-"));
}
#endif
