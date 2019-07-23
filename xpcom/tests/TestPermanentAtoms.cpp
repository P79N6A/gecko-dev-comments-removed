






































#include "nsIAtom.h"
#include "nsAtomTable.h"
#include "nsCOMPtr.h"
#include <stdio.h>
#include "nsString.h"
#include "nsReadableUtils.h"

static void Assert(PRBool aCondition, const char* aStatement)
{
    printf("%s: %s\n", aCondition?"PASS":"FAIL", aStatement);
}

static void AssertString(nsIAtom *aAtom, const nsACString& aString)
{
    const char *str;
    static_cast<AtomImpl*>(aAtom)->GetUTF8String(&str);
    Assert(nsDependentCString(str) == aString, "string is correct");
}

static void AssertPermanence(nsIAtom *aAtom, PRBool aPermanence)
{
    Assert(static_cast<AtomImpl*>(aAtom)->IsPermanent() == aPermanence,
           aPermanence ? "atom is permanent" : "atom is not permanent");
}

int main()
{
    nsCOMPtr<nsIAtom> foo = do_GetAtom("foo");
    AssertString(foo, NS_LITERAL_CSTRING("foo"));
    AssertPermanence(foo, PR_FALSE);

    nsCOMPtr<nsIAtom> foop = do_GetPermanentAtom("foo");
    AssertString(foop, NS_LITERAL_CSTRING("foo"));
    AssertPermanence(foop, PR_TRUE);
    
    Assert(foo == foop, "atoms are equal");
    
    nsCOMPtr<nsIAtom> barp = do_GetPermanentAtom("bar");
    AssertString(barp, NS_LITERAL_CSTRING("bar"));
    AssertPermanence(barp, PR_TRUE);
    
    nsCOMPtr<nsIAtom> bar = do_GetAtom("bar");
    AssertString(bar, NS_LITERAL_CSTRING("bar"));
    AssertPermanence(bar, PR_TRUE);

    Assert(bar == barp, "atoms are equal");
    
    return 0;
}
