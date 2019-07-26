




#ifndef nsIScrollbarOwner_h___
#define nsIScrollbarOwner_h___

#include "nsQueryFrame.h"

class nsIDOMEventTarget;
class nsIFrame;




class nsIScrollbarOwner : public nsQueryFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsIScrollbarOwner)

  



  virtual nsIFrame* GetScrollbarBox(bool aVertical) = 0;

  



  virtual void ScrollbarActivityStarted() const = 0;
  virtual void ScrollbarActivityStopped() const = 0;
};

#endif
