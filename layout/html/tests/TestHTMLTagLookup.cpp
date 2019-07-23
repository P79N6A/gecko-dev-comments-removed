



































#include <stdio.h>
#include "plstr.h"
#include "nsHTMLTags.h"

static const char* kJunkNames[] = {
  nsnull,
  "",
  "123",
  "ab",
  "zzzzzz",
  "#@$&@#*@*$@$#"
};

int main(int argc, char** argv)
{
  PRInt32 id;

  
  
  
  const nsHTMLTags::NameTableEntry* et = &nsHTMLTags::kNameTable[0];
  const nsHTMLTags::NameTableEntry* end = &nsHTMLTags::kNameTable[TAG_MAX];
  while (et < end) {
    char tagName[100];
    id = nsHTMLTags::LookupName(et->name);
    if (id < 0) {
      printf("bug: can't find '%s'\n", et->name);
    }
    if (et->id != id) {
      printf("bug: name='%s' et->id=%d id=%d\n", et->name, et->id, id);
    }

    
    PL_strcpy(tagName, et->name);
    tagName[0] = tagName[0] - 32;
    id = nsHTMLTags::LookupName(tagName);
    if (id < 0) {
      printf("bug: can't find '%s'\n", tagName);
    }
    if (et->id != id) {
      printf("bug: name='%s' et->id=%d id=%d\n", et->name, et->id, id);
    }
    et++;
  }

  
  for (int i = 0; i < sizeof(kJunkNames) / sizeof(const char*); i++) {
    const char* tag = kJunkNames[i];
    id = nsHTMLTags::LookupName(tag);
    if (id >= 0) {
      printf("bug: found '%s'\n", tag ? tag : "(null)");
    }
  }

  return 0;
}
