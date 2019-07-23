




































#ifndef __nsKeyCode_h
#define __nsKeyCode_h

#include <X11/X.h>
#include <X11/Xlib.h>

#include "prtypes.h"

struct nsKeyCode
{
  static PRInt32  ConvertKeySymToVirtualKey(KeySym aKeySym);

  static PRBool   KeyCodeIsModifier(KeyCode aKeyCode);

  static KeySym   ConvertKeyCodeToKeySym(Display *aDisplay,
                                         KeyCode aKeyCode);
};

#endif 
