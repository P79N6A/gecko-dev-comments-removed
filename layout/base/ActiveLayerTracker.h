



#ifndef ACTIVELAYERTRACKER_H_
#define ACTIVELAYERTRACKER_H_

#include "nsCSSProperty.h"

class nsIFrame;
class nsIContent;

namespace mozilla {







class ActiveLayerTracker {
public:
  static void Shutdown();

  







  





  static void NotifyRestyle(nsIFrame* aFrame, nsCSSProperty aProperty);
  




  static void NotifyOffsetRestyle(nsIFrame* aFrame);
  



  static void NotifyAnimated(nsIFrame* aFrame, nsCSSProperty aProperty);
  





  static void NotifyInlineStyleRuleModified(nsIFrame* aFrame, nsCSSProperty aProperty);
  



  static bool IsStyleAnimated(nsIFrame* aFrame, nsCSSProperty aProperty);
  



  static bool IsOffsetOrMarginStyleAnimated(nsIFrame* aFrame);
  





  static void TransferActivityToContent(nsIFrame* aFrame, nsIContent* aContent);
  



  static void TransferActivityToFrame(nsIContent* aContent, nsIFrame* aFrame);

  




  



  static void NotifyContentChange(nsIFrame* aFrame);
  


  static bool IsContentActive(nsIFrame* aFrame);
};

}

#endif 
