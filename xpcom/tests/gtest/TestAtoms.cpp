




#include "mozilla/ArrayUtils.h"

#include "nsIAtom.h"
#include "nsString.h"
#include "UTFStrings.h"
#include "nsIServiceManager.h"
#include "nsStaticAtom.h"

#include "gtest/gtest.h"

using namespace mozilla;

namespace TestAtoms {

TEST(Atoms, Basic)
{
  for (unsigned int i = 0; i < ArrayLength(ValidStrings); ++i) {
    nsDependentString str16(ValidStrings[i].m16);
    nsDependentCString str8(ValidStrings[i].m8);

    nsCOMPtr<nsIAtom> atom = do_GetAtom(str16);

    EXPECT_TRUE(atom->Equals(str16));
    EXPECT_TRUE(atom->EqualsUTF8(str8));

    nsString tmp16;
    nsCString tmp8;
    atom->ToString(tmp16);
    atom->ToUTF8String(tmp8);
    EXPECT_TRUE(str16.Equals(tmp16));
    EXPECT_TRUE(str8.Equals(tmp8));

    EXPECT_TRUE(nsDependentString(atom->GetUTF16String()).Equals(str16));

    EXPECT_TRUE(nsAtomString(atom).Equals(str16));
    EXPECT_TRUE(nsDependentAtomString(atom).Equals(str16));
    EXPECT_TRUE(nsAtomCString(atom).Equals(str8));
  }
}

TEST(Atoms, 16vs8)
{
  for (unsigned int i = 0; i < ArrayLength(ValidStrings); ++i) {
    nsCOMPtr<nsIAtom> atom16 = do_GetAtom(ValidStrings[i].m16);
    nsCOMPtr<nsIAtom> atom8 = do_GetAtom(ValidStrings[i].m8);
    EXPECT_EQ(atom16, atom8);
  }
}

TEST(Atoms, BufferSharing)
{
  nsString unique;
  unique.AssignLiteral("this is a unique string !@#$");

  nsCOMPtr<nsIAtom> atom = do_GetAtom(unique);

  EXPECT_EQ(unique.get(), atom->GetUTF16String());
}

TEST(Atoms, NUll)
{
  nsAutoString str(NS_LITERAL_STRING("string with a \0 char"));
  nsDependentString strCut(str.get());

  EXPECT_FALSE(str.Equals(strCut));

  nsCOMPtr<nsIAtom> atomCut = do_GetAtom(strCut);
  nsCOMPtr<nsIAtom> atom = do_GetAtom(str);

  EXPECT_EQ(atom->GetLength(), str.Length());
  EXPECT_TRUE(atom->Equals(str));
  EXPECT_TRUE(atom->EqualsUTF8(NS_ConvertUTF16toUTF8(str)));
  EXPECT_NE(atom, atomCut);
  EXPECT_TRUE(atomCut->Equals(strCut));
}

TEST(Atoms, Invalid)
{
  for (unsigned int i = 0; i < ArrayLength(Invalid16Strings); ++i) {
    nsrefcnt count = NS_GetNumberOfAtoms();

    {
      nsCOMPtr<nsIAtom> atom16 = do_GetAtom(Invalid16Strings[i].m16);
      EXPECT_TRUE(atom16->Equals(nsDependentString(Invalid16Strings[i].m16)));
    }

    EXPECT_EQ(count, NS_GetNumberOfAtoms());
  }

  for (unsigned int i = 0; i < ArrayLength(Invalid8Strings); ++i) {
    nsrefcnt count = NS_GetNumberOfAtoms();

    {
      nsCOMPtr<nsIAtom> atom8 = do_GetAtom(Invalid8Strings[i].m8);
      nsCOMPtr<nsIAtom> atom16 = do_GetAtom(Invalid8Strings[i].m16);
      EXPECT_EQ(atom16, atom8);
      EXPECT_TRUE(atom16->Equals(nsDependentString(Invalid8Strings[i].m16)));
    }

    EXPECT_EQ(count, NS_GetNumberOfAtoms());
  }


#ifndef DEBUG
  nsCOMPtr<nsIAtom> emptyAtom = do_GetAtom("");

  for (unsigned int i = 0; i < ArrayLength(Malformed8Strings); ++i) {
    nsrefcnt count = NS_GetNumberOfAtoms();

    nsCOMPtr<nsIAtom> atom8 = do_GetAtom(Malformed8Strings[i]);
    EXPECT_EQ(atom8, emptyAtom);
    EXPECT_EQ(count, NS_GetNumberOfAtoms());
  }
#endif
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

TEST(Atoms, Table)
{
  nsrefcnt count = NS_GetNumberOfAtoms();

  nsCOMPtr<nsIAtom> thirdNonPerm = do_GetAtom(THIRD_ATOM_STR);

  EXPECT_FALSE(isStaticAtom(thirdNonPerm));

  EXPECT_TRUE(thirdNonPerm);
  EXPECT_EQ(NS_GetNumberOfAtoms(), count + 1);

  NS_RegisterStaticAtoms(sAtoms_info);

  EXPECT_TRUE(sAtom1);
  EXPECT_TRUE(sAtom1->Equals(NS_LITERAL_STRING(FIRST_ATOM_STR)));
  EXPECT_TRUE(isStaticAtom(sAtom1));
  EXPECT_TRUE(sAtom2);
  EXPECT_TRUE(sAtom2->Equals(NS_LITERAL_STRING(SECOND_ATOM_STR)));
  EXPECT_TRUE(isStaticAtom(sAtom2));
  EXPECT_TRUE(sAtom3);
  EXPECT_TRUE(sAtom3->Equals(NS_LITERAL_STRING(THIRD_ATOM_STR)));
  EXPECT_TRUE(isStaticAtom(sAtom3));
  EXPECT_EQ(NS_GetNumberOfAtoms(), count + 3);
  EXPECT_EQ(thirdNonPerm, sAtom3);
}

#define FIRST_PERM_ATOM_STR "first permanent atom. Hello!"
#define SECOND_PERM_ATOM_STR "second permanent atom. @World!"

TEST(Atoms, Permanent)
{
  nsrefcnt count = NS_GetNumberOfAtoms();

  {
    nsCOMPtr<nsIAtom> first = do_GetAtom(FIRST_PERM_ATOM_STR);
    EXPECT_TRUE(first->Equals(NS_LITERAL_STRING(FIRST_PERM_ATOM_STR)));
    EXPECT_FALSE(isStaticAtom(first));

    nsCOMPtr<nsIAtom> first_p =
      NS_NewPermanentAtom(NS_LITERAL_STRING(FIRST_PERM_ATOM_STR));
    EXPECT_TRUE(first_p->Equals(NS_LITERAL_STRING(FIRST_PERM_ATOM_STR)));
    EXPECT_TRUE(isStaticAtom(first_p));
    EXPECT_EQ(first, first_p);

    nsCOMPtr<nsIAtom> second_p =
      NS_NewPermanentAtom(NS_LITERAL_STRING(SECOND_PERM_ATOM_STR));
    EXPECT_TRUE(second_p->Equals(NS_LITERAL_STRING(SECOND_PERM_ATOM_STR)));
    EXPECT_TRUE(isStaticAtom(second_p));

    nsCOMPtr<nsIAtom> second = do_GetAtom(SECOND_PERM_ATOM_STR);
    EXPECT_TRUE(second->Equals(NS_LITERAL_STRING(SECOND_PERM_ATOM_STR)));
    EXPECT_TRUE(isStaticAtom(second));
    EXPECT_EQ(second, second_p);
  }

  EXPECT_EQ(NS_GetNumberOfAtoms(), count + 2);
}

}
