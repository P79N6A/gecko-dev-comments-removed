





































#ifndef __nsQtKeyUtils_h__
#define __nsQtKeyUtils_h__

int      QtKeyCodeToDOMKeyCode     (int aKeysym);
int      DOMKeyCodeToQtKeyCode     (int aKeysym);
PRUint32 nsConvertCharCodeToUnicode (QKeyEvent* aEvent);

#endif 
