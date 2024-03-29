






#include "mozilla/ArrayUtils.h"

#include "nsCSSPseudoClasses.h"
#include "nsStaticAtom.h"
#include "mozilla/Preferences.h"
#include "nsString.h"

using namespace mozilla;


#define CSS_PSEUDO_CLASS(_name, _value, _flags, _pref) \
  static nsIAtom* sPseudoClass_##_name;
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS

#define CSS_PSEUDO_CLASS(name_, value_, flags_, pref_) \
  NS_STATIC_ATOM_BUFFER(name_##_pseudo_class_buffer, value_)
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS


static const nsStaticAtom CSSPseudoClasses_info[] = {
#define CSS_PSEUDO_CLASS(name_, value_, flags_, pref_) \
  NS_STATIC_ATOM(name_##_pseudo_class_buffer, &sPseudoClass_##name_),
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
};




static const uint32_t CSSPseudoClasses_flags[] = {
#define CSS_PSEUDO_CLASS(name_, value_, flags_, pref_) \
  flags_,
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
};

static bool sPseudoClassEnabled[] = {
#define CSS_PSEUDO_CLASS(name_, value_, flags_, pref_) \
  true,
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
};  

void nsCSSPseudoClasses::AddRefAtoms()
{
  NS_RegisterStaticAtoms(CSSPseudoClasses_info);
  
#define CSS_PSEUDO_CLASS(name_, value_, flags_, pref_)                       \
  if (pref_[0]) {                                                            \
    Preferences::AddBoolVarCache(&sPseudoClassEnabled[ePseudoClass_##name_], \
                                 pref_);                                     \
  }
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
}

bool
nsCSSPseudoClasses::HasStringArg(Type aType)
{
  return aType == ePseudoClass_lang ||
         aType == ePseudoClass_mozEmptyExceptChildrenWithLocalname ||
         aType == ePseudoClass_mozSystemMetric ||
         aType == ePseudoClass_mozLocaleDir ||
         aType == ePseudoClass_dir;
}

bool
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
  MOZ_ASSERT(aType < ePseudoClass_Count, "Unexpected type");
  MOZ_ASSERT(aType >= 0, "Very unexpected type");
  (*CSSPseudoClasses_info[aType].mAtom)->ToString(aString);
}

nsCSSPseudoClasses::Type
nsCSSPseudoClasses::GetPseudoType(nsIAtom* aAtom)
{
  for (uint32_t i = 0; i < ArrayLength(CSSPseudoClasses_info); ++i) {
    if (*CSSPseudoClasses_info[i].mAtom == aAtom) {
      return sPseudoClassEnabled[i] ? Type(i) : ePseudoClass_NotPseudoClass;
    }
  }

  return nsCSSPseudoClasses::ePseudoClass_NotPseudoClass;
}

 bool
nsCSSPseudoClasses::IsUserActionPseudoClass(Type aType)
{
  
  return aType == ePseudoClass_hover ||
         aType == ePseudoClass_active ||
         aType == ePseudoClass_focus;
}

 uint32_t
nsCSSPseudoClasses::FlagsForPseudoClass(const Type aType)
{
  size_t index = static_cast<size_t>(aType);
  NS_ASSERTION(index < ArrayLength(CSSPseudoClasses_flags),
               "argument must be a pseudo-class");
  return CSSPseudoClasses_flags[index];
}

