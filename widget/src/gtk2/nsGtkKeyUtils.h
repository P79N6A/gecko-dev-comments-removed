





































#ifndef __nsGdkKeyUtils_h__
#define __nsGdkKeyUtils_h__

int      GdkKeyCodeToDOMKeyCode     (int aKeysym);
int      DOMKeyCodeToGdkKeyCode     (PRUint32 aKeysym);
PRUint32 nsConvertCharCodeToUnicode (GdkEventKey* aEvent);

#endif 
