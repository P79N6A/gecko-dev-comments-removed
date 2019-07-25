



































#include <stdio.h>
#include <stdlib.h>
#include "nsString.h"
#include "nsStringBuffer.h"
#include "nsReadableUtils.h"
#include "nsCRTGlue.h"
#include "UTFStrings.h"
#include "nsCRT.h"

namespace TestUTF {

bool
test_valid()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(ValidStrings); ++i) {
    nsDependentCString str8(ValidStrings[i].m8);
    nsDependentString str16(ValidStrings[i].m16);

    if (!NS_ConvertUTF16toUTF8(str16).Equals(str8))
      return PR_FALSE;

    if (!NS_ConvertUTF8toUTF16(str8).Equals(str16))
      return PR_FALSE;

    nsCString tmp8("string ");
    AppendUTF16toUTF8(str16, tmp8);
    if (!tmp8.Equals(NS_LITERAL_CSTRING("string ") + str8))
      return PR_FALSE;

    nsString tmp16(NS_LITERAL_STRING("string "));
    AppendUTF8toUTF16(str8, tmp16);
    if (!tmp16.Equals(NS_LITERAL_STRING("string ") + str16))
      return PR_FALSE;

    if (CompareUTF8toUTF16(str8, str16) != 0)
      return PR_FALSE;
  }
  
  return PR_TRUE;
}

bool
test_invalid16()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Invalid16Strings); ++i) {
    nsDependentString str16(Invalid16Strings[i].m16);
    nsDependentCString str8(Invalid16Strings[i].m8);

    if (!NS_ConvertUTF16toUTF8(str16).Equals(str8))
      return PR_FALSE;

    nsCString tmp8("string ");
    AppendUTF16toUTF8(str16, tmp8);
    if (!tmp8.Equals(NS_LITERAL_CSTRING("string ") + str8))
      return PR_FALSE;

    if (CompareUTF8toUTF16(str8, str16) != 0)
      return PR_FALSE;
  }
  
  return PR_TRUE;
}

bool
test_invalid8()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Invalid8Strings); ++i) {
    nsDependentString str16(Invalid8Strings[i].m16);
    nsDependentCString str8(Invalid8Strings[i].m8);

    if (!NS_ConvertUTF8toUTF16(str8).Equals(str16))
      return PR_FALSE;

    nsString tmp16(NS_LITERAL_STRING("string "));
    AppendUTF8toUTF16(str8, tmp16);
    if (!tmp16.Equals(NS_LITERAL_STRING("string ") + str16))
      return PR_FALSE;

    if (CompareUTF8toUTF16(str8, str16) != 0)
      return PR_FALSE;
  }
  
  return PR_TRUE;
}

bool
test_malformed8()
{

#ifndef DEBUG
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Malformed8Strings); ++i) {
    nsDependentCString str8(Malformed8Strings[i]);

    if (!NS_ConvertUTF8toUTF16(str8).IsEmpty())
      return PR_FALSE;

    nsString tmp16(NS_LITERAL_STRING("string"));
    AppendUTF8toUTF16(str8, tmp16);
    if (!tmp16.Equals(NS_LITERAL_STRING("string")))
      return PR_FALSE;

    if (CompareUTF8toUTF16(str8, EmptyString()) == 0)
      return PR_FALSE;
  }
#endif
  
  return PR_TRUE;
}

bool
test_hashas16()
{
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(ValidStrings); ++i) {
    nsDependentCString str8(ValidStrings[i].m8);
    bool err;
    if (nsCRT::HashCode(ValidStrings[i].m16) !=
        nsCRT::HashCodeAsUTF16(str8.get(), str8.Length(), &err) ||
        err)
      return PR_FALSE;
  }

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Invalid8Strings); ++i) {
    nsDependentCString str8(Invalid8Strings[i].m8);
    bool err;
    if (nsCRT::HashCode(Invalid8Strings[i].m16) !=
        nsCRT::HashCodeAsUTF16(str8.get(), str8.Length(), &err) ||
        err)
      return PR_FALSE;
  }


#ifndef DEBUG
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(Malformed8Strings); ++i) {
    nsDependentCString str8(Malformed8Strings[i]);
    bool err;
    if (nsCRT::HashCodeAsUTF16(str8.get(), str8.Length(), &err) != 0 ||
        !err)
      return PR_FALSE;
  }
#endif

  return PR_TRUE;
}

typedef bool (*TestFunc)();

static const struct Test
  {
    const char* name;
    TestFunc    func;
  }
tests[] =
  {
    { "test_valid", test_valid },
    { "test_invalid16", test_invalid16 },
    { "test_invalid8", test_invalid8 },
    { "test_malformed8", test_malformed8 },
    { "test_hashas16", test_hashas16 },
    { nsnull, nsnull }
  };

}

using namespace TestUTF;

int main(int argc, char **argv)
  {
    int count = 1;
    if (argc > 1)
      count = atoi(argv[1]);

    while (count--)
      {
        for (const Test* t = tests; t->name != nsnull; ++t)
          {
            printf("%25s : %s\n", t->name, t->func() ? "SUCCESS" : "FAILURE <--");
          }
      }
    
    return 0;
  }
