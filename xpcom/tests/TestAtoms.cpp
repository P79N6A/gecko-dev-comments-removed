



































#include "nsIAtom.h"
#include "nsString.h"
#include "UTFStrings.h"
#include "nsIServiceManager.h"
#include "nsStaticAtom.h"

namespace TestAtoms {

bool
test_basic()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(ValidStrings); ++i) {
    nsDependentString str16(ValidStrings[i].m16);
    nsDependentCString str8(ValidStrings[i].m8);

    nsCOMPtr<nsIAtom> atom = do_GetAtom(str16);
    
    if (!atom->Equals(str16) || !atom->EqualsUTF8(str8))
      return PR_FALSE;

    nsString tmp16;
    nsCString tmp8;
    atom->ToString(tmp16);
    atom->ToUTF8String(tmp8);
    if (!str16.Equals(tmp16) || !str8.Equals(tmp8))
      return PR_FALSE;

    if (!nsDependentString(atom->GetUTF16String()).Equals(str16))
      return PR_FALSE;

    if (!nsAtomString(atom).Equals(str16) ||
        !nsDependentAtomString(atom).Equals(str16) ||
        !nsAtomCString(atom).Equals(str8))
      return PR_FALSE;
  }
  
  return PR_TRUE;
}

bool
test_16vs8()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(ValidStrings); ++i) {
    nsCOMPtr<nsIAtom> atom16 = do_GetAtom(ValidStrings[i].m16);
    nsCOMPtr<nsIAtom> atom8 = do_GetAtom(ValidStrings[i].m8);
    if (atom16 != atom8)
      return PR_FALSE;
  }
  
  return PR_TRUE;
}

bool
test_buffersharing()
{
  nsString unique;
  unique.AssignLiteral("this is a unique string !@#$");
  
  nsCOMPtr<nsIAtom> atom = do_GetAtom(unique);
  
  return unique.get() == atom->GetUTF16String();
}

bool
test_null()
{
  nsAutoString str(NS_LITERAL_STRING("string with a \0 char"));
  nsDependentString strCut(str.get());

  if (str.Equals(strCut))
    return PR_FALSE;
  
  nsCOMPtr<nsIAtom> atomCut = do_GetAtom(strCut);
  nsCOMPtr<nsIAtom> atom = do_GetAtom(str);
  
  return atom->GetLength() == str.Length() &&
         atom->Equals(str) &&
         atom->EqualsUTF8(NS_ConvertUTF16toUTF8(str)) &&
         atom != atomCut &&
         atomCut->Equals(strCut);
}

bool
test_invalid()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Invalid16Strings); ++i) {
    nsrefcnt count = NS_GetNumberOfAtoms();

    {
      nsCOMPtr<nsIAtom> atom16 = do_GetAtom(Invalid16Strings[i].m16);
      if (!atom16->Equals(nsDependentString(Invalid16Strings[i].m16)))
        return PR_FALSE;
    }
    
    if (count != NS_GetNumberOfAtoms())
      return PR_FALSE;
  }

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Invalid8Strings); ++i) {
    nsrefcnt count = NS_GetNumberOfAtoms();

    {
      nsCOMPtr<nsIAtom> atom8 = do_GetAtom(Invalid8Strings[i].m8);
      nsCOMPtr<nsIAtom> atom16 = do_GetAtom(Invalid8Strings[i].m16);
      if (atom16 != atom8 ||
          !atom16->Equals(nsDependentString(Invalid8Strings[i].m16)))
        return PR_FALSE;
    }
    
    if (count != NS_GetNumberOfAtoms())
      return PR_FALSE;
  }


#ifndef DEBUG
  nsCOMPtr<nsIAtom> emptyAtom = do_GetAtom("");

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Malformed8Strings); ++i) {
    nsrefcnt count = NS_GetNumberOfAtoms();

    nsCOMPtr<nsIAtom> atom8 = do_GetAtom(Malformed8Strings[i]);
    if (atom8 != emptyAtom ||
        count != NS_GetNumberOfAtoms())
      return PR_FALSE;
  }
#endif

  return PR_TRUE;
}

#define FIRST_ATOM_STR "first static atom. Hello!"
#define SECOND_ATOM_STR "second static atom. @World!"
#define THIRD_ATOM_STR "third static atom?!"

static nsIAtom* sAtom1 = 0;
static nsIAtom* sAtom2 = 0;
static nsIAtom* sAtom3 = 0;
NS_STATIC_ATOM_BUFFER(sAtom1_buffer, FIRST_ATOM_STR)
NS_STATIC_ATOM_BUFFER(sAtom2_buffer, SECOND_ATOM_STR)
NS_STATIC_ATOM_BUFFER(sAtom3_buffer, THIRD_ATOM_STR)
static const nsStaticAtom sAtoms_info[] = {
  NS_STATIC_ATOM(sAtom1_buffer, &sAtom1),
  NS_STATIC_ATOM(sAtom2_buffer, &sAtom2),
  NS_STATIC_ATOM(sAtom3_buffer, &sAtom3),
};

bool
isStaticAtom(nsIAtom* atom)
{
  
  
  
  return (atom->AddRef() == 2) &
         (atom->AddRef() == 2) &
         (atom->AddRef() == 2) &
         (atom->Release() == 1) &
         (atom->Release() == 1) &
         (atom->Release() == 1);
}

bool
test_atomtable()
{
  nsrefcnt count = NS_GetNumberOfAtoms();
  
  nsCOMPtr<nsIAtom> thirdNonPerm = do_GetAtom(THIRD_ATOM_STR);
  
  if (isStaticAtom(thirdNonPerm))
    return PR_FALSE;

  if (!thirdNonPerm || NS_GetNumberOfAtoms() != count + 1)
    return PR_FALSE;

  NS_RegisterStaticAtoms(sAtoms_info, NS_ARRAY_LENGTH(sAtoms_info));

  return sAtom1 &&
         sAtom1->Equals(NS_LITERAL_STRING(FIRST_ATOM_STR)) &&
         isStaticAtom(sAtom1) &&
         sAtom2 &&
         sAtom2->Equals(NS_LITERAL_STRING(SECOND_ATOM_STR)) &&
         isStaticAtom(sAtom2) &&
         sAtom3 &&
         sAtom3->Equals(NS_LITERAL_STRING(THIRD_ATOM_STR)) &&
         isStaticAtom(sAtom3) &&
         NS_GetNumberOfAtoms() == count + 3 &&
         thirdNonPerm == sAtom3;
}

#define FIRST_PERM_ATOM_STR "first permanent atom. Hello!"
#define SECOND_PERM_ATOM_STR "second permanent atom. @World!"

bool
test_permanent()
{
  nsrefcnt count = NS_GetNumberOfAtoms();

  {
    nsCOMPtr<nsIAtom> first = do_GetAtom(FIRST_PERM_ATOM_STR);
    if (!first->Equals(NS_LITERAL_STRING(FIRST_PERM_ATOM_STR)) ||
        isStaticAtom(first))
      return PR_FALSE;
  
    nsCOMPtr<nsIAtom> first_p =
      NS_NewPermanentAtom(NS_LITERAL_STRING(FIRST_PERM_ATOM_STR));
    if (!first_p->Equals(NS_LITERAL_STRING(FIRST_PERM_ATOM_STR)) ||
        !isStaticAtom(first_p) ||
        first != first_p)
      return PR_FALSE;
  
    nsCOMPtr<nsIAtom> second_p =
      NS_NewPermanentAtom(NS_LITERAL_STRING(SECOND_PERM_ATOM_STR));
    if (!second_p->Equals(NS_LITERAL_STRING(SECOND_PERM_ATOM_STR)) ||
        !isStaticAtom(second_p))
      return PR_FALSE;
  
    nsCOMPtr<nsIAtom> second = do_GetAtom(SECOND_PERM_ATOM_STR);
    if (!second->Equals(NS_LITERAL_STRING(SECOND_PERM_ATOM_STR)) ||
        !isStaticAtom(second) ||
        second != second_p)
      return PR_FALSE;
  }

  return NS_GetNumberOfAtoms() == count + 2;
}

typedef bool (*TestFunc)();

static const struct Test
  {
    const char* name;
    TestFunc    func;
  }
tests[] =
  {
    { "test_basic", test_basic },
    { "test_16vs8", test_16vs8 },
    { "test_buffersharing", test_buffersharing },
    { "test_null", test_null },
    { "test_invalid", test_invalid },



#if 0
    { "test_atomtable", test_atomtable },
    { "test_permanent", test_permanent },
#endif
    { nsnull, nsnull }
  };

}

using namespace TestAtoms;

int main()
  {
    {
      nsCOMPtr<nsIServiceManager> servMan;
      NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
  
      for (const Test* t = tests; t->name != nsnull; ++t)
        {
          printf("%25s : %s\n", t->name, t->func() ? "SUCCESS" : "FAILURE <--");
        }
    }

    NS_ShutdownXPCOM(nsnull);

    return 0;
  }
