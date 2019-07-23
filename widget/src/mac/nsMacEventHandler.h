



































#ifndef MacMacEventHandler_h__
#define MacMacEventHandler_h__

#include <ConditionalMacros.h>
#include <Events.h>
#include <MacWindows.h>
#include <TextServices.h>
#include <Controls.h>
#include "prtypes.h"
#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsDeleteObserver.h"
#include "nsString.h"

class nsWindow;
class nsMacWindow;





class nsMacEventDispatchHandler : public nsDeleteObserver
{
public:
	nsMacEventDispatchHandler();
	virtual			~nsMacEventDispatchHandler();

	void			DispatchGuiEvent(nsWindow *aWidget, PRUint32 aEventType);
	void			DispatchSizeModeEvent(nsWindow *aWidget, nsSizeMode aMode);

    void 			SetFocus(nsWindow *aFocusedWidget);

	void 			SetActivated(nsWindow *aActiveWidget);
	nsWindow*		GetActive()	{return(mActiveWidget);}
	void			SetDeactivated(nsWindow *aActiveWidget);

	void 			SetWidgetHit(nsWindow *aWidgetHit);
	void 			SetWidgetPointed(nsWindow *aWidgetPointed);

	nsWindow*		GetWidgetHit()		{return(mWidgetHit);}
	nsWindow*		GetWidgetPointed()	{return(mWidgetPointed);}

	
	virtual void	NotifyDelete(void* aDeletedObject);

#if TRACK_MOUSE_LOC
  void     SetGlobalPoint(Point inPoint);
  Point    GetGlobalPoint() { return mLastGlobalMouseLoc; }
#endif
  
private:

  nsWindow*	mActiveWidget;
  nsWindow*	mWidgetHit;
  nsWindow*	mWidgetPointed;
#if TRACK_MOUSE_LOC
  Point   mLastGlobalMouseLoc;
#endif
};






class nsMacEventHandler
{
public:
		nsMacEventHandler(nsMacWindow* aTopLevelWidget,
                                  nsMacEventDispatchHandler* aEventDispatchHandler);
		virtual ~nsMacEventHandler();

		virtual PRBool	HandleOSEvent(EventRecord& aOSEvent);
#if USE_MENUSELECT
		virtual PRBool	HandleMenuCommand(EventRecord& aOSEvent, long aMenuResult);
#endif
		
		
		virtual PRBool	DragEvent ( unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers ) ;
		

		
		
		
		virtual long 		HandlePositionToOffset(Point aPoint,short* regionClass);
		virtual nsresult 	HandleOffsetToPosition(long offset,Point* position);
		virtual nsresult	UnicodeHandleUpdateInputArea(const PRUnichar* text, long charCount, long fixedLength,TextRangeArray* textRangeArray);
		virtual nsresult	HandleUnicodeGetSelectedText(nsAString& outString);
		virtual nsresult	ResetInputState();
		virtual PRBool		HandleUKeyEvent(const PRUnichar* text, long charCount, EventRecord& aOSEvent);
		virtual PRBool		HandleKeyUpDownEvent(EventHandlerCallRef aHandlerCallRef,
					                     EventRef aEvent);
		virtual PRBool		HandleKeyModifierEvent(EventHandlerCallRef aHandlerCallRef,
					                       EventRef aEvent);
		
		
		
		
		
		virtual PRBool ResizeEvent ( WindowRef inWindow ) ;
		virtual PRBool Scroll (PRInt32 aDeltaY, PRInt32 aDeltaX,
                                       PRBool aIsPixels,
                                       const Point& aMouseLoc,
                                       nsWindow* aWindow, PRUint32 aModifiers);
		 
		virtual void	HandleActivateEvent(EventRef aEvent);
		inline nsMacEventDispatchHandler* GetEventDispatchHandler() { return mEventDispatchHandler; }
protected:
		void InitializeMouseEvent(nsMouseEvent& aMouseEvent,
                                          nsPoint&      aPoint,
                                          PRInt16       aModifiers,
                                          PRUint32      aClickCount);
		void InitializeKeyEvent(nsKeyEvent& aKeyEvent, EventRecord& aOSEvent, 
                              nsWindow* aFocusedWidget, PRUint32 aMessage, 
                              PRBool aConvertChar=PR_TRUE);
		virtual PRBool		IsSpecialRaptorKey(UInt32 macKeyCode);
		virtual PRUint32	ConvertKeyEventToUnicode(EventRecord& aOSEvent);
		virtual PRBool	HandleMouseDownEvent(EventRecord& aOSEvent);
		virtual PRBool	HandleMouseUpEvent(EventRecord& aOSEvent);
		virtual PRBool	HandleMouseMoveEvent(EventRecord& aOSEvent);

		virtual void		ConvertOSEventToMouseEvent(
												EventRecord&	aOSEvent,
												nsMouseEvent&	aMouseEvent);
		virtual nsresult	HandleStartComposition(void);
		virtual nsresult	HandleEndComposition(void);
		virtual nsresult  HandleTextEvent(PRUint32 textRangeCount, nsTextRangeArray textRangeArray);
		virtual PRBool ScrollAxis (nsMouseScrollEvent::nsMouseScrollFlags aAxis,
		                           PRInt32 aDelta, PRBool aIsPixels,
		                           const Point& aMouseLoc,
		                           nsWindow* aWindow,
		                           PRUint32 aModifiers);
		void ClearLastMouseUp();

protected:
	nsMacEventDispatchHandler* mEventDispatchHandler;
	nsMacWindow*	mTopLevelWidget;
	RgnHandle			mUpdateRgn;
	TSMDocumentID	mTSMDocument;
	nsPoint 		mIMEPos;
	nsAutoString		*mIMECompositionStr;
	Point				mLastMouseUpWhere;
	UInt32				mLastMouseUpWhen;
	PRUint32			mClickCount;
	PRUint32			mLastModifierState;
	PRPackedBool			mIMEIsComposing;
	PRPackedBool			mKeyIgnore;
	PRPackedBool			mKeyHandled;
	PRPackedBool			mMouseInWidgetHit;
	PRPackedBool			mOwnEventDispatchHandler;
};

#endif 
