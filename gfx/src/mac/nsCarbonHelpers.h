




































#ifndef CarbonHelpers_h__
#define CarbonHelpers_h__

#include <ConditionalMacros.h>
#include <ControlDefinitions.h>
#include <Menus.h>
#include <MacWindows.h>
#include <LowMem.h>





inline void GetWindowUpdateRegion ( WindowPtr window, RgnHandle outUpdateRgn )
{
		::GetWindowRegion(window, kWindowUpdateRgn, outUpdateRgn);
}

inline void SetControlPopupMenuStuff ( ControlHandle control, MenuHandle menu, short aID )
{
		::SetControlPopupMenuHandle ( control, menu );
		::SetControlPopupMenuID ( control, aID );
}


inline WindowRef GetTheWindowList(void)
{
  return GetWindowList();
}

#endif 
