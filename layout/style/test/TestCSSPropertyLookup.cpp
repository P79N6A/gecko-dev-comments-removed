



#include <stdio.h>
#include "plstr.h"
#include "nsCSSProps.h"
#include "nsCSSKeywords.h"
#include "nsString.h"
#include "nsXPCOM.h"

static const char* const kJunkNames[] = {
  nullptr,
  "",
  "123",
  "backgroundz",
  "zzzzzz",
  "#@$&@#*@*$@$#"
};

static bool
TestProps()
{
  bool success = true;
  nsCSSProperty id;
  nsCSSProperty index;

  
  nsCSSProps::AddRefTable();

  
  
  
  extern const char* const kCSSRawProperties[];
  const char*const* et = &kCSSRawProperties[0];
  const char*const* end = &kCSSRawProperties[eCSSProperty_COUNT];
  index = eCSSProperty_UNKNOWN;
  while (et < end) {
    char tagName[100];
    PL_strcpy(tagName, *et);
    index = nsCSSProperty(int32_t(index) + 1);

    id = nsCSSProps::LookupProperty(nsCString(tagName), nsCSSProps::eAny);
    if (id == eCSSProperty_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName);
      success = false;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      success = false;
    }

    
    if (('a' <= tagName[0]) && (tagName[0] <= 'z')) {
      tagName[0] = tagName[0] - 32;
    }
    id = nsCSSProps::LookupProperty(NS_ConvertASCIItoUTF16(tagName),
                                    nsCSSProps::eAny);
    if (id < 0) {
      printf("bug: can't find '%s'\n", tagName);
      success = false;
    }
    if (index != id) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      success = false;
    }
    et++;
  }

  
  for (int i = 0; i < (int) (sizeof(kJunkNames) / sizeof(const char*)); i++) {
    const char* const tag = kJunkNames[i];
    id = nsCSSProps::LookupProperty(nsCAutoString(tag), nsCSSProps::eAny);
    if (id >= 0) {
      printf("bug: found '%s'\n", tag ? tag : "(null)");
      success = false;
    }
  }

  nsCSSProps::ReleaseTable();
  return success;
}

bool
TestKeywords()
{
  nsCSSKeywords::AddRefTable();

  bool success = true;
  nsCSSKeyword id;
  nsCSSKeyword index;

  extern const char* const kCSSRawKeywords[];

  
  
  
  const char*const*  et = &kCSSRawKeywords[0];
  const char*const*  end = &kCSSRawKeywords[eCSSKeyword_COUNT - 1];
  index = eCSSKeyword_UNKNOWN;
  while (et < end) {
    char tagName[512];
    char* underscore = &(tagName[0]);

    PL_strcpy(tagName, *et);
    while (*underscore) {
      if (*underscore == '_') {
        *underscore = '-';
      }
      underscore++;
    }
    index = nsCSSKeyword(int32_t(index) + 1);

    id = nsCSSKeywords::LookupKeyword(nsCString(tagName));
    if (id <= eCSSKeyword_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName);
      success = false;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      success = false;
    }

    
    if (('a' <= tagName[0]) && (tagName[0] <= 'z')) {
      tagName[0] = tagName[0] - 32;
    }
    id = nsCSSKeywords::LookupKeyword(nsCString(tagName));
    if (id <= eCSSKeyword_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName);
      success = false;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      success = false;
    }
    et++;
  }

  
  for (int i = 0; i < (int) (sizeof(kJunkNames) / sizeof(const char*)); i++) {
    const char* const tag = kJunkNames[i];
    id = nsCSSKeywords::LookupKeyword(nsCAutoString(tag));
    if (eCSSKeyword_UNKNOWN < id) {
      printf("bug: found '%s'\n", tag ? tag : "(null)");
      success = false;
    }
  }

  nsCSSKeywords::ReleaseTable();
  return success;
}

int
main(void)
{
  nsresult rv = NS_InitXPCOM2(nullptr, nullptr, nullptr);
  NS_ENSURE_SUCCESS(rv, 2);

  bool testOK = true;
  testOK &= TestProps();
  testOK &= TestKeywords();

  rv = NS_ShutdownXPCOM(nullptr);
  NS_ENSURE_SUCCESS(rv, 2);

  return testOK ? 0 : 1;
}
