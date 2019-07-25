





















#ifndef nsHtml5NamedCharacters_h_
#define nsHtml5NamedCharacters_h_

#include "prtypes.h"
#include "jArray.h"
#include "nscore.h"
#include "nsDebug.h"
#include "prlog.h"
#include "nsMemory.h"

struct nsHtml5CharacterName {
  PRUint16 nameStart;
  PRUint16 nameLen;
  #ifdef DEBUG
  PRInt32 n;
  #endif
  PRInt32 length() const;
  PRUnichar charAt(PRInt32 index) const;
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
