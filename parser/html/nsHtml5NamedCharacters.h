





















#ifndef nsHtml5NamedCharacters_h
#define nsHtml5NamedCharacters_h

#include "jArray.h"
#include "nscore.h"
#include "nsDebug.h"
#include "mozilla/Logging.h"
#include "nsMemory.h"

struct nsHtml5CharacterName {
  uint16_t nameStart;
  uint16_t nameLen;
  #ifdef DEBUG
  int32_t n;
  #endif
  int32_t length() const;
  char16_t charAt(int32_t index) const;
};

class nsHtml5NamedCharacters
{
  public:
    static const nsHtml5CharacterName NAMES[];
    static const char16_t VALUES[][2];
    static char16_t** WINDOWS_1252;
    static void initializeStatics();
    static void releaseStatics();
};

#endif 
