





































#include <X11/Xlib.h>
#ifdef HAVE_X11_XKBLIB_H
#   if (defined (__digital__) && defined (__unix__)) || defined(_AIX) || defined(__hpux) || defined (__osf__)
#   define explicit Explicit
#   include <X11/XKBlib.h>
#   undef explicit
#else
#   include <X11/XKBlib.h>
#endif
#endif
#include <X11/keysym.h>
#include <gdk/gdkx.h>
#include <string.h>
#include "nsKeyboardUtils.h"
#include "nspr.h"
#include "nsWindow.h"
#include "nsPrintfCString.h"



























#define MODIFIERMAP_ROW_SIZE 8

PRUint32 nsXKBModeSwitch::gModeSwitchKeycode1 = 0;
PRUint32 nsXKBModeSwitch::gModeSwitchKeycode2 = 0;
PRBool   nsXKBModeSwitch::gGrabDuringPopup = PR_TRUE;
PRBool   nsXKBModeSwitch::gUnGrabDuringModeSwitch = PR_TRUE;
PRBool   nsXKBModeSwitch::gModeSwitchDown = PR_FALSE;
gint     nsXKBModeSwitch::gOwnerEvents;
guint32  nsXKBModeSwitch::gGrabTime;


void 
nsXKBModeSwitch::ControlWorkaround(gboolean grab_during_popup,
                                   gboolean ungrab_during_mode_switch)
{
#ifdef DEBUG_bzbarsky
  NS_WARNING("nsXKBModeSwitch::ControlWorkaround:");
  NS_WARNING(nsPrintfCString("    grab_during_popup = %d", grab_during_popup).get());
  NS_WARNING(nsPrintfCString("    ungrab_during_mode_switch = %d", ungrab_during_mode_switch).get());
#endif
  gGrabDuringPopup = grab_during_popup;
  gUnGrabDuringModeSwitch = ungrab_during_mode_switch;

  
  
  
  
  HandleMappingNotify();
}

gint
nsXKBModeSwitch::GrabKeyboard(GdkWindow *win, gint owner_events, guint32 time)
{
  
  if (!gGrabDuringPopup) {
    return GrabSuccess;
  }

  gint retval;
  retval = gdk_keyboard_grab(win, owner_events, time);
  if (retval == GrabSuccess) {
    gOwnerEvents = owner_events;
    gGrabTime = time;
  }
  else {
    gOwnerEvents = 0;
    gGrabTime = 0;
  }
 
  return retval;
}

void
nsXKBModeSwitch::HandleMappingNotify()
{
  XModifierKeymap *xmodmap;
  int i, j, max_keypermod;

  
  Init();

  Display *lGdkDisplay = GDK_DISPLAY();
  if (!lGdkDisplay)
    return;
  xmodmap = XGetModifierMapping(lGdkDisplay);
  if (!xmodmap)
    return;

  max_keypermod = MIN(xmodmap->max_keypermod,5);
  for (i=0; i<max_keypermod; i++) {
    for (j=0; j<MODIFIERMAP_ROW_SIZE; j++) {
      KeyCode keycode;
      KeySym keysym;
      char *keysymName;
      keycode = *((xmodmap->modifiermap)+(i*MODIFIERMAP_ROW_SIZE)+j);
      if (!keycode)
        continue;
      keysym = XKeycodeToKeysym(GDK_DISPLAY(), keycode, 0);
      if (!keysym)
        continue;
      keysymName = XKeysymToString(keysym);
      if (!keysymName)
        continue;
      if (!strcmp(keysymName,"Mode_switch")) {
        if (!gModeSwitchKeycode1)
          gModeSwitchKeycode1 = keycode;
        else if (!gModeSwitchKeycode2)
          gModeSwitchKeycode2 = keycode;
      }
    }
  }
  XFreeModifiermap(xmodmap);

#ifdef DEBUG_bzbarsky
  if (!gModeSwitchKeycode1) {
    NS_WARNING("\n\nnsXKBModeSwitch::HandleMappingNotify: no Mode_switch\n\n");
  }
  NS_WARNING("\n\nnsXKBModeSwitch::HandleMappingNotify:");
  NS_WARNING(nsPrintfCString("    gModeSwitchKeycode1 = %d", gModeSwitchKeycode1).get());
  NS_WARNING(nsPrintfCString("    gModeSwitchKeycode2 = %d", gModeSwitchKeycode2).get());
#endif

#if defined(HAVE_X11_XKBLIB_H) && \
  defined(XkbMajorVersion) && defined(XbMinorVersion)
  {
    gint xkb_major = XkbMajorVersion;
    gint xkb_minor = XkbMinorVersion;
    if (XkbLibraryVersion (&xkb_major, &xkb_minor)) {
      xkb_major = XkbMajorVersion;
      xkb_minor = XkbMinorVersion;
      if (XkbQueryExtension (gdk_display, NULL, NULL, NULL,
                               &xkb_major, &xkb_minor)) {
      }
    }
  }
#endif

}

void
nsXKBModeSwitch::Init()
{
    gModeSwitchKeycode1 = 0;
    gModeSwitchKeycode2 = 0;
    gModeSwitchDown = FALSE;
}

void
nsXKBModeSwitch::HandleKeyPress(XKeyEvent *xke)
{
  
  if (!gGrabDuringPopup) {
    return;
  }

  
  if ((xke->keycode == gModeSwitchKeycode1) 
      || (xke->keycode == gModeSwitchKeycode2)) {
    gModeSwitchDown = TRUE;
    NS_WARNING("nsXKBModeSwitch::HandleKeyPress: Mode_switch is down");
    nsWindow *win = nsWindow::GetGrabWindow();
    if (!win)
      return;
    if (win->GrabInProgress()) {
      if (gUnGrabDuringModeSwitch) {
        gdk_keyboard_ungrab(GDK_CURRENT_TIME);
        NS_WARNING("\n\n*** ungrab keyboard ***\n\n");
      }
    }
  }
}

void
nsXKBModeSwitch::HandleKeyRelease(XKeyEvent *xke)
{
  
  if (!gGrabDuringPopup) {
    return;
  }

  
  if ((xke->keycode == gModeSwitchKeycode1) 
          || (xke->keycode == gModeSwitchKeycode2)) {
    gModeSwitchDown = FALSE;
    NS_WARNING("nsXKBModeSwitch::HandleKeyPress: Mode_switch is up");
    nsWindow *win = nsWindow::GetGrabWindow();
    if (!win)
      return;
    if (win->GrabInProgress()) {
      if (gUnGrabDuringModeSwitch) {
        if (!win->GetGdkGrabWindow())
          return;
        gdk_keyboard_grab(win->GetGdkGrabWindow(), gOwnerEvents, gGrabTime);
        NS_WARNING("\n\n*** re-grab keyboard ***\n\n");
      }
    }
  }
}

void
nsXKBModeSwitch::UnGrabKeyboard(guint32 time)
{
  
  if (!gGrabDuringPopup) {
    return;
  }

  gdk_keyboard_ungrab(time);
  gOwnerEvents = 0;
  gGrabTime = 0;
}

