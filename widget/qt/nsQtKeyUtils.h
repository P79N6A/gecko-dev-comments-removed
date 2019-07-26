






#ifndef __nsQtKeyUtils_h__
#define __nsQtKeyUtils_h__

#include "nsEvent.h"

int      QtKeyCodeToDOMKeyCode     (int aKeysym);
int      DOMKeyCodeToQtKeyCode     (int aKeysym);

mozilla::KeyNameIndex QtKeyCodeToDOMKeyNameIndex(int aKeysym);

#endif 
