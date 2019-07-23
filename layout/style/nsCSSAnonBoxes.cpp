







































#include "nsCSSAnonBoxes.h"
#include "nsAtomListUtils.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"


#define CSS_ANON_BOX(_name, _value) \
  nsICSSAnonBoxPseudo* nsCSSAnonBoxes::_name;
#include "nsCSSAnonBoxList.h"
#undef CSS_ANON_BOX

static const nsStaticAtom CSSAnonBoxes_info[] = {
#define CSS_ANON_BOX(name_, value_) \
  { value_, (nsIAtom**)&nsCSSAnonBoxes::name_ },
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

