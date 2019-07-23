




































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsToolkitBase.h"



















#include <MacTypes.h>
#include <MacWindows.h>

class nsIEventSink;
class nsIWidget;

#define MAC_OS_X_VERSION_10_0_HEX 0x00001000
#define MAC_OS_X_VERSION_10_1_HEX 0x00001010
#define MAC_OS_X_VERSION_10_2_HEX 0x00001020
#define MAC_OS_X_VERSION_10_3_HEX 0x00001030
#define MAC_OS_X_VERSION_10_4_HEX 0x00001040

class nsToolkit : public nsToolkitBase
{

public:
  nsToolkit();
  virtual				~nsToolkit();
  
protected:

  virtual nsresult  InitEventQueue(PRThread * aThread);

public:

  
  
  static void GetWindowEventSink ( WindowPtr aWindow, nsIEventSink** outSink ) ;
  static void GetTopWidget ( WindowPtr aWindow, nsIWidget** outWidget ) ;

  
  
  static long OSXVersion ( ) ;
};
#endif  
