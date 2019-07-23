




































#ifndef nsNativeScrollbar_h_
#define nsNativeScrollbar_h_

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#include "mozView.h"
#include "nsChildView.h"

#include "nsINativeScrollbar.h"
#include "nsIContent.h"

class nsIScrollbarMediator;

@class NativeScrollbarView;








class nsNativeScrollbar : public nsChildView, public nsINativeScrollbar
{
private:
  typedef nsChildView Inherited;

public:
                nsNativeScrollbar();
  virtual       ~nsNativeScrollbar();
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSINATIVESCROLLBAR

    
    
  void DoScroll(NSScrollerPart inPart);
  
protected:

  
  virtual PRBool DispatchMouseEvent(nsMouseEvent &aEvent);
  NS_IMETHOD Enable(PRBool aState);
  NS_IMETHOD IsEnabled(PRBool* outState);
  
  NSScroller* GetControl() { return (NSScroller*)mView; }

  void UpdateContentPosition(PRUint32 inNewPos);
  
  void RecreateHorizontalScrollbar();

  virtual NSView*   CreateCocoaView(NSRect inFrame);

  void              UpdateScroller();
  
  NativeScrollbarView*    ScrollbarView() const { return (NativeScrollbarView*)mView; }
  

private:

  nsIContent*       mContent;          
  nsIScrollbarMediator* mMediator;     
  nsISupports*      mScrollbar;        

  PRUint32          mValue;
  PRUint32          mMaxValue;
  PRUint32          mVisibleImageSize;
  PRUint32          mLineIncrement;
  PRBool            mIsEnabled;
};


@interface NativeScrollbarView : NSScroller<mozView>
{
    
  NSWindow* mWindow;
  
    
    
  nsNativeScrollbar* mGeckoChild;

    
  BOOL      mInTracking;

  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;
}

  
- (id)initWithFrame:(NSRect)frameRect geckoChild:(nsNativeScrollbar*)inChild;
  
- (id)initWithFrame:(NSRect)frameRect;

- (IBAction)scroll:(NSScroller*)sender;

@end


#endif 
