





































#ifndef nsNativeScrollbar_h__
#define nsNativeScrollbar_h__

#include "nsMacControl.h"
#include "nsINativeScrollbar.h"
#include <Controls.h>
#include "nsIContent.h"

class nsIScrollbarMediator;










class nsNativeScrollbar : public nsMacControl, public nsINativeScrollbar
{
private:
  typedef nsMacControl Inherited;

public:
                nsNativeScrollbar();
  virtual       ~nsNativeScrollbar();

  NS_IMETHOD    Destroy();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSINATIVESCROLLBAR

protected:

  
  virtual PRBool DispatchMouseEvent(nsMouseEvent &aEvent);
  
  virtual ControlPartCode GetControlHiliteState();

  ControlHandle   GetControl() { return mControl; }
  
  void UpdateContentPosition(PRUint32 inNewPos);

private:

  friend class StNativeControlActionProcOwner;
  
  static pascal void ScrollActionProc(ControlHandle, ControlPartCode);
  void DoScrollAction(ControlPartCode);


private:

  nsIContent*       mContent;          
  nsIScrollbarMediator* mMediator;     
  nsISupports*      mScrollbar;        
  
  PRUint32          mMaxValue;
  PRUint32          mVisibleImageSize;
  PRUint32          mLineIncrement;
  PRBool            mMouseDownInScroll;
  ControlPartCode   mClickedPartCode;
};



#endif 
