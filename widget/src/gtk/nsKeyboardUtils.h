






































#ifndef _NSKEYBOARDUTILS_H_
#define _NSKEYBOARDUTILS_H_

extern "C" {
#include <X11/Xlib.h>
#include <gdk/gdk.h>
}
#include "prtypes.h"

class nsXKBModeSwitch {
  public:
    static void ControlWorkaround(gboolean grab_during_popup,
                                  gboolean ungrab_during_mode_switch);
    static gint GrabKeyboard(GdkWindow *win, gint owner_events, guint32 time);
    static void UnGrabKeyboard(guint32 time);
    static void HandleKeyPress(XKeyEvent *xke);
    static void HandleKeyRelease(XKeyEvent *xke);
    static void HandleMappingNotify();

  private:
    static void Init();

    static PRUint32 gModeSwitchKeycode1;
    static PRUint32 gModeSwitchKeycode2;
    static PRBool   gGrabDuringPopup;
    static PRBool   gUnGrabDuringModeSwitch;
    static PRBool   gModeSwitchDown;
    static gint     gOwnerEvents;
    static guint32  gGrabTime;

};

#endif 
