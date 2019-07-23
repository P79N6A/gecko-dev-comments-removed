







































#include "nsCSSPseudoClasses.h"
#include "nsAtomListUtils.h"
#include "nsStaticAtom.h"
#include "nsMemory.h"


#define CSS_PSEUDO_CLASS(_name, _value) \
  nsICSSPseudoClass* nsCSSPseudoClasses::_name;
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS

#define CSS_PSEUDO_CLASS(name_, value_) \
  NS_STATIC_ATOM_BUFFER(name_##_buffer, value_)
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS

static const nsStaticAtom CSSPseudoClasses_info[] = {
#define CSS_PSEUDO_CLASS(name_, value_) \
  NS_STATIC_ATOM(name_##_buffer, (nsIAtom**)&nsCSSPseudoClasses::name_),
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
};

void nsCSSPseudoClasses::AddRefAtoms()
{
  NS_RegisterStaticAtoms(CSSPseudoClasses_info,
                         NS_ARRAY_LENGTH(CSSPseudoClasses_info));
}

PRBool nsCSSPseudoClasses::IsPseudoClass(nsIAtom *aAtom)
{
  return nsAtomListUtils::IsMember(aAtom,CSSPseudoClasses_info,
                                   NS_ARRAY_LENGTH(CSSPseudoClasses_info));
}

PRBool
nsCSSPseudoClasses::HasStringArg(nsIAtom* aAtom)
{
  return aAtom == nsCSSPseudoClasses::lang ||
         aAtom == nsCSSPseudoClasses::mozEmptyExceptChildrenWithLocalname ||
         aAtom == nsCSSPseudoClasses::mozSystemMetric ||
         aAtom == nsCSSPseudoClasses::mozLocaleDir;
}

PRBool
nsCSSPseudoClasses::HasNthPairArg(nsIAtom* aAtom)
{
  return aAtom == nsCSSPseudoClasses::nthChild ||
         aAtom == nsCSSPseudoClasses::nthLastChild ||
         aAtom == nsCSSPseudoClasses::nthOfType ||
         aAtom == nsCSSPseudoClasses::nthLastOfType;
}

nsCSSPseudoClasses::Type
nsCSSPseudoClasses::GetPseudoType(nsIAtom* aAtom)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(CSSPseudoClasses_info); ++i) {
    if (*CSSPseudoClasses_info[i].mAtom == aAtom) {
      return Type(i);
    }
  }

  return nsCSSPseudoClasses::ePseudoClass_NotPseudoClass;
}
