





































#ifndef __nsGdkKeyUtils_h__
#define __nsGdkKeyUtils_h__

int      GdkKeyCodeToDOMKeyCode     (int aKeysym);
int      DOMKeyCodeToGdkKeyCode     (int aKeysym);
PRUint32 nsConvertCharCodeToUnicode (GdkEventKey* aEvent);

#endif 
