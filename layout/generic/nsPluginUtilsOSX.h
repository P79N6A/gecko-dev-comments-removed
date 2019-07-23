








































#ifndef __LP64__
#import <Carbon/Carbon.h>
#endif

#include "nsRect.h"
#include "nsIWidget.h"
#include "npapi.h"



#ifndef __LP64__

void NS_NPAPI_CarbonWindowFrame(WindowRef aWindow, nsRect& outRect);
#endif


void NS_NPAPI_CocoaWindowFrame(void* aWindow, nsRect& outRect);


NPError NS_NPAPI_ShowCocoaContextMenu(void* menu, nsIWidget* widget, NPCocoaEvent* event);

NPBool NS_NPAPI_ConvertPointCocoa(void* inView,
                                  double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                                  double *destX, double *destY, NPCoordinateSpace destSpace);
