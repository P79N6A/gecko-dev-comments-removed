




































#include "nsToolkit.h"

#include "nsIEventSink.h"

#include "nsWidgetSupport.h"

#include <Gestalt.h>
#include <Movies.h>






nsToolkit::nsToolkit()
{
  ::EnterMovies();	
}




nsToolkit::~nsToolkit()
{ 
  ::ExitMovies();	
}





nsresult
nsToolkit::InitEventQueue(PRThread * aThread)
{
  return NS_OK;
}





long
nsToolkit :: OSXVersion()
{
  static long gOSXVersion = 0x0;
  if (gOSXVersion == 0x0) {
    OSErr err = ::Gestalt(gestaltSystemVersion, &gOSXVersion);
    if (err != noErr) {
      NS_ERROR("Couldn't determine OS X version, assume 10.0");
      gOSXVersion = MAC_OS_X_VERSION_10_0_HEX;
    }
  }
  return gOSXVersion;
}








void
nsToolkit::GetTopWidget ( WindowPtr aWindow, nsIWidget** outWidget )
{
  nsIWidget* topLevelWidget = nsnull;
  ::GetWindowProperty ( aWindow,
      kTopLevelWidgetPropertyCreator, kTopLevelWidgetRefPropertyTag,
      sizeof(nsIWidget*), nsnull, (void*)&topLevelWidget);
  if ( topLevelWidget ) {
    *outWidget = topLevelWidget;
    NS_ADDREF(*outWidget);
  }
}








void
nsToolkit::GetWindowEventSink ( WindowPtr aWindow, nsIEventSink** outSink )
{
  *outSink = nsnull;
  
  nsCOMPtr<nsIWidget> topWidget;
  GetTopWidget ( aWindow, getter_AddRefs(topWidget) );
  nsCOMPtr<nsIEventSink> sink ( do_QueryInterface(topWidget) );
  if ( sink ) {
    *outSink = sink;
    NS_ADDREF(*outSink);
  }
}




nsToolkitBase* NS_CreateToolkitInstance()
{
  return new nsToolkit();
}
