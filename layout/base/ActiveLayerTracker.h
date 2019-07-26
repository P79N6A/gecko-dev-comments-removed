



#ifndef ACTIVELAYERTRACKER_H_
#define ACTIVELAYERTRACKER_H_

#include "nsCSSProperty.h"

class nsIFrame;

namespace mozilla {







class ActiveLayerTracker {
public:
  




  





  static void NotifyRestyle(nsIFrame* aFrame, nsCSSProperty aProperty);
  



  static void NotifyAnimated(nsIFrame* aFrame, nsCSSProperty aProperty);
  



  static bool IsStyleAnimated(nsIFrame* aFrame, nsCSSProperty aProperty);

  



  static void NotifyContentChange(nsIFrame* aFrame);
  


  static bool IsContentActive(nsIFrame* aFrame);

  static void Shutdown();
};

}

#endif 
