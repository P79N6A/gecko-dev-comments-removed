






































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
    Assert(aAtom->EqualsUTF8(aString), "string is correct");
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

    nsCOMPtr<nsIAtom> foop = NS_NewPermanentAtom(NS_LITERAL_STRING("foo"));
    AssertString(foop, NS_LITERAL_CSTRING("foo"));
    AssertPermanence(foop, PR_TRUE);
    
    Assert(foo == foop, "atoms are equal");
    
    nsCOMPtr<nsIAtom> barp = NS_NewPermanentAtom(NS_LITERAL_STRING("bar"));
    AssertString(barp, NS_LITERAL_CSTRING("bar"));
    AssertPermanence(barp, PR_TRUE);
    
    nsCOMPtr<nsIAtom> bar = do_GetAtom("bar");
    AssertString(bar, NS_LITERAL_CSTRING("bar"));
    AssertPermanence(bar, PR_TRUE);

    Assert(bar == barp, "atoms are equal");
    
    return 0;
}
