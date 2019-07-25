







































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

PRBool
nsCSSPseudoClasses::HasStringArg(Type aType)
{
  return aType == ePseudoClass_lang ||
         aType == ePseudoClass_mozEmptyExceptChildrenWithLocalname ||
         aType == ePseudoClass_mozSystemMetric ||
         aType == ePseudoClass_mozLocaleDir;
}

PRBool
nsCSSPseudoClasses::HasNthPairArg(Type aType)
{
  return aType == ePseudoClass_nthChild ||
         aType == ePseudoClass_nthLastChild ||
         aType == ePseudoClass_nthOfType ||
         aType == ePseudoClass_nthLastOfType;
}

void
nsCSSPseudoClasses::PseudoTypeToString(Type aType, nsAString& aString)
{
  NS_ABORT_IF_FALSE(aType < ePseudoClass_Count, "Unexpected type");
  NS_ABORT_IF_FALSE(aType >= 0, "Very unexpected type");
  (*CSSPseudoClasses_info[aType].mAtom)->ToString(aString);
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
