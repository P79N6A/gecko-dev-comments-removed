





















#ifndef nsHtml5NamedCharacters_h_
#define nsHtml5NamedCharacters_h_

#include "prtypes.h"
#include "jArray.h"
#include "nscore.h"
#include "nsDebug.h"
#include "prlog.h"
#include "nsMemory.h"

struct nsHtml5CharacterName {
  uint16_t nameStart;
  uint16_t nameLen;
  #ifdef DEBUG
  int32_t n;
  #endif
  int32_t length() const;
  PRUnichar charAt(int32_t index) const;
};

class nsHtml5NamedCharacters
{
  public:
    static const nsHtml5CharacterName NAMES[];
    static const PRUnichar VALUES[][2];
    static PRUnichar** WINDOWS_1252;
    static void initializeStatics();
    static void releaseStatics();
};

#endif 
