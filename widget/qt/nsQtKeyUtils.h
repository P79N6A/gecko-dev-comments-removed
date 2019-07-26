






#ifndef __nsQtKeyUtils_h__
#define __nsQtKeyUtils_h__

#include "mozilla/EventForwards.h"

int      QtKeyCodeToDOMKeyCode     (int aKeysym);
int      DOMKeyCodeToQtKeyCode     (int aKeysym);

mozilla::KeyNameIndex QtKeyCodeToDOMKeyNameIndex(int aKeysym);
mozilla::CodeNameIndex ScanCodeToDOMCodeNameIndex(int32_t aScanCode);

#endif 
