



































#include <stdio.h>
#include "plstr.h"
#include "nsCSSProps.h"
#include "nsCSSKeywords.h"
#include "nsString.h"

static const char* const kJunkNames[] = {
  nsnull,
  "",
  "123",
  "backgroundz",
  "zzzzzz",
  "#@$&@#*@*$@$#"
};

int TestProps() {
  int rv = 0;
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
    index = nsCSSProperty(PRInt32(index) + 1);

    id = nsCSSProps::LookupProperty(nsCString(tagName));
    if (id == eCSSProperty_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName);
      rv = -1;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      rv = -1;
    }

    
    if (('a' <= tagName[0]) && (tagName[0] <= 'z')) {
      tagName[0] = tagName[0] - 32;
    }
    id = nsCSSProps::LookupProperty(NS_ConvertASCIItoUTF16(tagName));
    if (id < 0) {
      printf("bug: can't find '%s'\n", tagName);
      rv = -1;
    }
    if (index != id) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      rv = -1;
    }
    et++;
  }

  
  for (int i = 0; i < (int) (sizeof(kJunkNames) / sizeof(const char*)); i++) {
    const char* const tag = kJunkNames[i];
    id = nsCSSProps::LookupProperty(nsCAutoString(tag));
    if (id >= 0) {
      printf("bug: found '%s'\n", tag ? tag : "(null)");
      rv = -1;
    }
  }

  nsCSSProps::ReleaseTable();
  return rv;
}

int TestKeywords() {
  nsCSSKeywords::AddRefTable();

  int rv = 0;
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
    index = nsCSSKeyword(PRInt32(index) + 1);

    id = nsCSSKeywords::LookupKeyword(nsCString(tagName));
    if (id <= eCSSKeyword_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName);
      rv = -1;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      rv = -1;
    }

    
    if (('a' <= tagName[0]) && (tagName[0] <= 'z')) {
      tagName[0] = tagName[0] - 32;
    }
    id = nsCSSKeywords::LookupKeyword(nsCString(tagName));
    if (id <= eCSSKeyword_UNKNOWN) {
      printf("bug: can't find '%s'\n", tagName);
      rv = -1;
    }
    if (id != index) {
      printf("bug: name='%s' id=%d index=%d\n", tagName, id, index);
      rv = -1;
    }
    et++;
  }

  
  for (int i = 0; i < (int) (sizeof(kJunkNames) / sizeof(const char*)); i++) {
    const char* const tag = kJunkNames[i];
    id = nsCSSKeywords::LookupKeyword(nsCAutoString(tag));
    if (eCSSKeyword_UNKNOWN < id) {
      printf("bug: found '%s'\n", tag ? tag : "(null)");
      rv = -1;
    }
  }

  nsCSSKeywords::ReleaseTable();
  return rv;
}

int main(int argc, char** argv)
{
  TestProps();
  TestKeywords();
  return 0;
}
