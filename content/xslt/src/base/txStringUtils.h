




#ifndef txStringUtils_h__
#define txStringUtils_h__

#include "nsAString.h"
#include "nsIAtom.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"

typedef nsCaseInsensitiveStringComparator txCaseInsensitiveStringComparator;




inline bool
TX_StringEqualsAtom(const nsASingleFragmentString& aString, nsIAtom* aAtom)
{
  return aAtom->Equals(aString);
}

inline already_AddRefed<nsIAtom>
TX_ToLowerCaseAtom(nsIAtom* aAtom)
{
  nsAutoString str;
  aAtom->ToString(str);
  nsContentUtils::ASCIIToLower(str);
  return do_GetAtom(str);
}

#endif 
