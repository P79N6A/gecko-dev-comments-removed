






































#ifndef nsWindowUtils_h_
#define nsWindowUtils_h_

#include <Quickdraw.h>
#include <MacWindows.h>

#include "nsAEDefs.h"

class nsIXULWindow;

namespace nsWindowUtils {

long CountWindowsOfKind(TWindowKind windowKind);

WindowPtr GetNamedOrFrontmostWindow(TWindowKind windowKind, const char* windowName);
WindowPtr GetIndexedWindowOfKind(TWindowKind windowKind, TAEListIndex index);
TAEListIndex GetWindowIndex(TWindowKind windowKind, WindowPtr theWindow); 

void GetCleanedWindowName(WindowPtr wind, char* outName, long maxLen);
void GetWindowUrlString(WindowPtr wind, char** outUrlStringPtr);
void GetWindowGlobalBounds(WindowPtr wind, Rect* outRect);

void LoadURLInWindow(WindowPtr wind, const char* urlString);
void LoadURLInXULWindow(nsIXULWindow* inWindow, const char* urlString);

Boolean WindowIsResizeable(WindowPtr wind);
Boolean WindowIsZoomable(WindowPtr wind);
Boolean WindowIsZoomed(WindowPtr wind);
Boolean WindowHasTitleBar(WindowPtr wind);
Boolean WindowIsModal(WindowPtr wind);
Boolean WindowIsCloseable(WindowPtr wind);
Boolean WindowIsFloating(WindowPtr wind);
Boolean WindowIsModified(WindowPtr wind);

}


#endif 
