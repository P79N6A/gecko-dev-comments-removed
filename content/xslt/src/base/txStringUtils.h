






































#ifndef txStringUtils_h__
#define txStringUtils_h__

#include "nsAString.h"
#include "nsIAtom.h"




inline PRBool
TX_StringEqualsAtom(const nsASingleFragmentString& aString, nsIAtom* aAtom)
{
    const char* ASCIIAtom;
    aAtom->GetUTF8String(&ASCIIAtom);

    return aString.EqualsASCII(ASCIIAtom);
}

#ifndef TX_EXE

#include "nsUnicharUtils.h"
typedef nsCaseInsensitiveStringComparator txCaseInsensitiveStringComparator;

#define TX_ToLowerCase ToLowerCase

#else



class txCaseInsensitiveStringComparator
: public nsStringComparator
{
public:
  virtual int operator()(const char_type*, const char_type*, PRUint32 aLength) const;
  virtual int operator()(char_type, char_type) const;
};

void TX_ToLowerCase(nsAString& aString);
void TX_ToLowerCase(const nsAString& aSource, nsAString& aDest);

#endif

inline already_AddRefed<nsIAtom>
TX_ToLowerCaseAtom(nsIAtom* aAtom)
{
  nsAutoString str;
  aAtom->ToString(str);
  TX_ToLowerCase(str);
  return do_GetAtom(str);
}

#endif 
