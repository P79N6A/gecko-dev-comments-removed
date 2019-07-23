








































#ifndef nsWindow_h__
#define nsWindow_h__

#include "nsWidget.h"
#include <X11/cursorfont.h>

#include "nsString.h"

class nsListItem {
public:
  nsListItem() {}
  nsListItem(void *aData, nsListItem *aPrev);
  ~nsListItem() {}

  void *getData() { return data; }
  nsListItem *getNext() { return next; }
  void setNext(nsListItem *aNext) { next = aNext; }
  nsListItem *getPrev() { return prev; };
  void setPrev(nsListItem *aPrev) { prev = aPrev; }

private:
  void *data;
  nsListItem *next;
  nsListItem *prev;
};

class nsList {
public:
  nsList();
  ~nsList();
  nsListItem *getHead() { return head; }
  void add(void *aData);
  void remove(void *aData);
  void reset();

private:
  nsListItem *head;
  nsListItem *tail;
};


class nsWindow : public nsWidget
{
 public:
  nsWindow();
  ~nsWindow();

  NS_DECL_ISUPPORTS_INHERITED

  static void      UpdateIdle (void *data);
  NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener, 
                                 PRBool aDoCapture, 
                                 PRBool aConsumeRollupEvent);
  NS_IMETHOD Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
  NS_IMETHOD           InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous);                  
  NS_IMETHOD Update();
  NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
  NS_IMETHOD ScrollWidgets(PRInt32 aDx, PRInt32 aDy);
  NS_IMETHOD ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy);

  NS_IMETHOD SetTitle(const nsAString& aTitle);
  NS_IMETHOD Show(PRBool aShow);

  NS_IMETHOD Resize(PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool   aRepaint);
  NS_IMETHOD Resize(PRInt32 aX,
                    PRInt32 aY,
                    PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool   aRepaint);


  NS_IMETHOD SetFocus(PRBool aRaise);
  virtual  PRBool OnExpose(nsPaintEvent &event);
  NS_IMETHOD GetAttention(PRInt32 aCycleCount);
  
protected:
  virtual long GetEventMask();

  
  void NativeGrab(PRBool aGrab);

  void DoPaint (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight,
                nsIRegion *aClipRegion);
                
  void                 QueueDraw();
  void                 UnqueueDraw();
  PRBool mIsUpdating;
  PRBool mBlockFocusEvents;
  PRBool mIsTooSmall;
  
  GC     mScrollGC; 

  static PRBool    sIsGrabbing;
  static nsWindow  *sGrabWindow;

#if 0
  virtual void CreateNative(Window aParent, nsRect aRect);
#endif
};

class ChildWindow : public nsWindow
{
 public:
  ChildWindow();
  virtual PRInt32 IsChild() { return PR_TRUE; };
};

#endif
