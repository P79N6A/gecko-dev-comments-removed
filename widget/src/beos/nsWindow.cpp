







































#include "nsDebug.h"
#include "nsWindow.h"
#include "nsIAppShell.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsGUIEvent.h"
#include "nsWidgetsCID.h"
#include "nsIDragService.h"
#include "nsIDragSessionBeOS.h"
#include "nsIDeviceContext.h"
#include "nsRect.h"
#include "nsIRegion.h"
#include "nsTransform2D.h"
#include "nsGfxCIID.h"
#include "resource.h"
#include "prtime.h"
#include "nsReadableUtils.h"
#include "nsVoidArray.h"
#include "nsIProxyObjectManager.h"

#include <Application.h>
#include <InterfaceDefs.h>
#include <Region.h>
#include <ScrollBar.h>
#include <app/Message.h>
#include <support/String.h>
#include <Screen.h>

#include <nsBeOSCursors.h>
#if defined(BeIME)
#include <Input.h>
#include <InputServerMethod.h>
#include <String.h>
#endif
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"

#ifdef MOZ_CAIRO_GFX
#include "gfxBeOSSurface.h"
#include "gfxContext.h"
#endif


NS_IMPL_THREADSAFE_ADDREF(nsWindow)
NS_IMPL_THREADSAFE_RELEASE(nsWindow)

static NS_DEFINE_IID(kIWidgetIID,       NS_IWIDGET_IID);
static NS_DEFINE_IID(kRegionCID, NS_REGION_CID);
static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);





static nsIRollupListener * gRollupListener           = nsnull;
static nsIWidget         * gRollupWidget             = nsnull;
static PRBool              gRollupConsumeRollupEvent = PR_FALSE;

static BWindow           * gLastActiveWindow = NULL;





static nsVoidArray		gCursorArray(21);

#define kWindowPositionSlop 20

#define kWindowBorderWidth 5
#define kWindowTitleBarHeight 24


#if defined(BeIME)
#include "nsUTF8Utils.h"
static inline uint32 utf8_str_len(const char* ustring, int32 length) 
{
	CalculateUTF8Length cutf8;
	cutf8.write(ustring, length);
	return cutf8.Length();       
}

nsIMEBeOS::nsIMEBeOS()
	: imeTarget(NULL)
	, imeState(NS_COMPOSITION_END), imeWidth(14)
{
}




								
void nsIMEBeOS::RunIME(uint32 *args, nsWindow *target, BView *fView)
{
	BMessage msg;
	msg.Unflatten((const char*)args);

	switch (msg.FindInt32("be:opcode")) 
	{
	case B_INPUT_METHOD_CHANGED:
		if (msg.HasString("be:string")) 
		{
			const char* src = msg.FindString("be:string");
			CopyUTF8toUTF16(src, imeText);
 
    		if (msg.FindBool("be:confirmed")) 
    		{	
    			if (imeState != NS_COMPOSITION_END)
   					DispatchText(imeText, 0, NULL);
   			}
   			else 
   			{
   				nsTextRange txtRuns[2];
   				PRUint32 txtCount = 2;

	 	    	int32 select[2];
 				select[0] = msg.FindInt32("be:selection", int32(0));
				select[1] = msg.FindInt32("be:selection", 1);

	 			txtRuns[0].mStartOffset = (select[0] == select[1]) ? 0 : utf8_str_len(src, select[1]);
	 			txtRuns[0].mEndOffset	= imeText.Length();
				txtRuns[0].mRangeType	= NS_TEXTRANGE_CONVERTEDTEXT;
				if (select[0] == select[1])
					txtCount = 1;
				else 
				{
	 				txtRuns[1].mStartOffset = utf8_str_len(src, select[0]);
	 				txtRuns[1].mEndOffset	= utf8_str_len(src, select[1]);
	 				txtRuns[1].mRangeType	= NS_TEXTRANGE_SELECTEDCONVERTEDTEXT;
	 			}
	 			imeTarget = target;
				DispatchText(imeText, txtCount, txtRuns);
			}	
		}	
		break;

	case B_INPUT_METHOD_LOCATION_REQUEST:
		if (fView && fView->LockLooper()) 
		{
			BPoint caret(imeCaret);
			DispatchIME(NS_COMPOSITION_QUERY);
			if (caret.x > imeCaret.x) 
				caret.x = imeCaret.x - imeWidth * imeText.Length();	

			BMessage reply(B_INPUT_METHOD_EVENT);
			reply.AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
			for (int32 s= 0; imeText[s]; s++) 
			{ 
				reply.AddPoint("be:location_reply", fView->ConvertToScreen(caret));
				reply.AddFloat("be:height_reply", imeHeight);
				caret.x += imeWidth;
			}
			imeMessenger.SendMessage(&reply);
			fView->UnlockLooper();
		}
		break;

	case B_INPUT_METHOD_STARTED:
		imeTarget = target;
		DispatchIME(NS_COMPOSITION_START);
		DispatchIME(NS_COMPOSITION_QUERY);

		msg.FindMessenger("be:reply_to", &imeMessenger);
		break;
	
	case B_INPUT_METHOD_STOPPED:
		if (imeState != NS_COMPOSITION_END)
			DispatchIME(NS_COMPOSITION_END);
		imeText.Truncate();
		break;
	};
}

void nsIMEBeOS::DispatchText(nsString &text, PRUint32 txtCount, nsTextRange* txtRuns)
{
	nsTextEvent textEvent(PR_TRUE,NS_TEXT_TEXT, imeTarget);

	textEvent.time 		= 0;
	textEvent.isShift   = 
	textEvent.isControl =
	textEvent.isAlt 	= 
	textEvent.isMeta 	= PR_FALSE;
  
	textEvent.refPoint.x	= 
	textEvent.refPoint.y	= 0;

	textEvent.theText 	= text.get();
	textEvent.isChar	= PR_TRUE;
	textEvent.rangeCount= txtCount;
	textEvent.rangeArray= txtRuns;

	DispatchWindowEvent(&textEvent);
}

void nsIMEBeOS::DispatchCancelIME()
{
	if (imeText.Length() && imeState != NS_COMPOSITION_END) 
	{
		BMessage reply(B_INPUT_METHOD_EVENT);
		reply.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
		imeMessenger.SendMessage(&reply);

		DispatchText(imeText, 0, NULL);
		DispatchIME(NS_COMPOSITION_END);

		imeText.Truncate();
	}
}

void nsIMEBeOS::DispatchIME(PRUint32 what)
{
	nsCompositionEvent compEvent(PR_TRUE, what, imeTarget);

	compEvent.refPoint.x =
	compEvent.refPoint.y = 0;
	compEvent.time 	 = 0;

	DispatchWindowEvent(&compEvent);
	imeState = what;

	if (what == NS_COMPOSITION_QUERY) 
	{
		imeCaret.Set(compEvent.theReply.mCursorPosition.x,
		           compEvent.theReply.mCursorPosition.y);
		imeHeight = compEvent.theReply.mCursorPosition.height+4;
	}
}

PRBool nsIMEBeOS::DispatchWindowEvent(nsGUIEvent* event)
{
	nsEventStatus status;
	imeTarget->DispatchEvent(event, status);
	return PR_FALSE;
}

nsIMEBeOS *nsIMEBeOS::GetIME()
{
	if(beosIME == 0)
		beosIME = new nsIMEBeOS();
	return beosIME;
}
nsIMEBeOS *nsIMEBeOS::beosIME = 0;
#endif





nsWindow::nsWindow() : nsBaseWidget()
{
	mView               = 0;
	mPreferredWidth     = 0;
	mPreferredHeight    = 0;
	mFontMetrics        = nsnull;
	mIsVisible          = PR_FALSE;
	mEnabled            = PR_TRUE;
	mIsScrolling        = PR_FALSE;
	mParent             = nsnull;
	mWindowParent       = nsnull;
	mUpdateArea = do_CreateInstance(kRegionCID);
	mForeground = NS_RGBA(0xFF,0xFF,0xFF,0xFF);
	mBackground = mForeground;
	mBWindowFeel        = B_NORMAL_WINDOW_FEEL;
	mBWindowLook        = B_NO_BORDER_WINDOW_LOOK;

	if (mUpdateArea)
	{
		mUpdateArea->Init();
		mUpdateArea->SetTo(0, 0, 0, 0);
	}
}







nsWindow::~nsWindow()
{
	mIsDestroying = PR_TRUE;

	
	
	if (NULL != mView) 
	{
		Destroy();
	}
	NS_IF_RELEASE(mFontMetrics);
}

NS_METHOD nsWindow::BeginResizingChildren(void)
{
	
	NS_NOTYETIMPLEMENTED("BeginResizingChildren not yet implemented"); 
	return NS_OK;
}

NS_METHOD nsWindow::EndResizingChildren(void)
{
	
	NS_NOTYETIMPLEMENTED("EndResizingChildren not yet implemented"); 
	return NS_OK;
}

NS_METHOD nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
	BPoint	point;
	point.x = aOldRect.x;
	point.y = aOldRect.y;
	if (mView && mView->LockLooper())
	{
		mView->ConvertToScreen(&point);
		mView->UnlockLooper();
	}
	aNewRect.x = nscoord(point.x);
	aNewRect.y = nscoord(point.y);
	aNewRect.width = aOldRect.width;
	aNewRect.height = aOldRect.height;
	return NS_OK;
}

NS_METHOD nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
	BPoint	point;
	point.x = aOldRect.x;
	point.y = aOldRect.y;
	if (mView && mView->LockLooper())
	{
		mView->ConvertFromScreen(&point);
		mView->UnlockLooper();
	}
	aNewRect.x = nscoord(point.x);
	aNewRect.y = nscoord(point.y);
	aNewRect.width = aOldRect.width;
	aNewRect.height = aOldRect.height;
	return NS_OK;
}







void nsWindow::InitEvent(nsGUIEvent& event, nsPoint* aPoint)
{
	NS_ADDREF(event.widget);

	if (nsnull == aPoint) 
	{
		
		event.refPoint.x = 0;
		event.refPoint.y = 0;
	}
	else 
	{
		event.refPoint.x = aPoint->x;
		event.refPoint.y = aPoint->y;
	}
	event.time = PR_IntervalNow();
}






NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
	aStatus = nsEventStatus_eIgnore;

	nsCOMPtr <nsIWidget> mWidget = event->widget;

	if (mEventCallback)
		aStatus = (*mEventCallback)(event);

	if ((aStatus != nsEventStatus_eIgnore) && (mEventListener))
		aStatus = mEventListener->ProcessEvent(*event);

	return NS_OK;
}






PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
{
	nsEventStatus status;
	DispatchEvent(event, status);
	return ConvertStatus(status);
}







PRBool nsWindow::DispatchStandardEvent(PRUint32 aMsg)
{
	nsGUIEvent event(PR_TRUE, aMsg, this);
	InitEvent(event);

	PRBool result = DispatchWindowEvent(&event);
	NS_RELEASE(event.widget);
	return result;
}

NS_IMETHODIMP nsWindow::PreCreateWidget(nsWidgetInitData *aInitData)
{
	if ( nsnull == aInitData)
		return NS_ERROR_FAILURE;
	
	SetWindowType(aInitData->mWindowType);
	SetBorderStyle(aInitData->mBorderStyle);
	return NS_OK;
}






nsresult nsWindow::StandardWindowCreate(nsIWidget *aParent,
                                        const nsRect &aRect,
                                        EVENT_CALLBACK aHandleEventFunction,
                                        nsIDeviceContext *aContext,
                                        nsIAppShell *aAppShell,
                                        nsIToolkit *aToolkit,
                                        nsWidgetInitData *aInitData,
                                        nsNativeWidget aNativeParent)
{

	
	if (aInitData->mWindowType == eWindowType_invisible)
		return NS_ERROR_FAILURE;
		
	NS_ASSERTION(aInitData->mWindowType == eWindowType_dialog
		|| aInitData->mWindowType == eWindowType_toplevel,
		"The windowtype is not handled by this class.");

	mIsTopWidgetWindow = PR_TRUE;
	
	BaseCreate(nsnull, aRect, aHandleEventFunction, aContext,
	           aAppShell, aToolkit, aInitData);

	mListenForResizes = aNativeParent ? PR_TRUE : aInitData->mListenForResizes;
		
	mParent = aParent;
	
	
	mWindowParent = (nsWindow *)aParent;
	SetBounds(aRect);

	
	uint32 flags = B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
		| B_NOT_CLOSABLE | B_ASYNCHRONOUS_CONTROLS;

	
	
	if (eBorderStyle_default == mBorderStyle || eBorderStyle_all & mBorderStyle)
	{
		
		

		
		if (eWindowType_toplevel==mWindowType)
		{
			mBWindowLook = B_TITLED_WINDOW_LOOK;
			flags = B_ASYNCHRONOUS_CONTROLS;
		}
	}
	else
	{
		if (eBorderStyle_border & mBorderStyle)
			mBWindowLook = B_MODAL_WINDOW_LOOK;

		if (eBorderStyle_resizeh & mBorderStyle)
		{
			
			mBWindowLook = B_MODAL_WINDOW_LOOK;
			flags &= !B_NOT_RESIZABLE;
		}

		
		if (eBorderStyle_title & mBorderStyle || eBorderStyle_menu & mBorderStyle)
			mBWindowLook = B_TITLED_WINDOW_LOOK;

		if (eBorderStyle_minimize & mBorderStyle)
			flags &= !B_NOT_MINIMIZABLE;

		if (eBorderStyle_maximize & mBorderStyle)
			flags &= !B_NOT_ZOOMABLE;

		if (eBorderStyle_close & mBorderStyle)
			flags &= !B_NOT_CLOSABLE;
	}

	nsWindowBeOS * w = new nsWindowBeOS(this, 
		BRect(aRect.x, aRect.y, aRect.x + aRect.width - 1, aRect.y + aRect.height - 1),
		"", mBWindowLook, mBWindowFeel, flags);
	if (!w)
		return NS_ERROR_OUT_OF_MEMORY;

	mView = new nsViewBeOS(this, w->Bounds(), "Toplevel view", B_FOLLOW_ALL, 0);

	if (!mView)
		return NS_ERROR_OUT_OF_MEMORY;

	w->AddChild(mView);
	
	if (eWindowType_dialog == mWindowType && mWindowParent) 
	{
		nsWindow *topparent = mWindowParent;
		while (topparent->mWindowParent)
			topparent = topparent->mWindowParent;
		
		BWindow* subsetparent = (BWindow *)
			topparent->GetNativeData(NS_NATIVE_WINDOW);
		if (subsetparent)
		{
			mBWindowFeel = B_FLOATING_SUBSET_WINDOW_FEEL;
			w->SetFeel(mBWindowFeel);
			w->AddToSubset(subsetparent);
		}
	} 
	
	w->Run();
	DispatchStandardEvent(NS_CREATE);
	return NS_OK;
}






NS_METHOD nsWindow::Create(nsIWidget *aParent,
                           const nsRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
	
	

	nsToolkit* toolkit = (nsToolkit *)mToolkit;
	if (toolkit && !toolkit->IsGuiThread())
	{
		nsCOMPtr<nsIWidget> widgetProxy;
		nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
										NS_GET_IID(nsIWidget),
										this, 
										NS_PROXY_SYNC | NS_PROXY_ALWAYS, 
										getter_AddRefs(widgetProxy));
	
		if (NS_FAILED(rv))
			return rv;
		return widgetProxy->Create(aParent, aRect, aHandleEventFunction, aContext,
                           			aAppShell, aToolkit, aInitData);
	}
	return(StandardWindowCreate(aParent, aRect, aHandleEventFunction,
	                            aContext, aAppShell, aToolkit, aInitData,
	                            nsnull));
}








NS_METHOD nsWindow::Create(nsNativeWidget aParent,
                           const nsRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
	
	

	nsToolkit* toolkit = (nsToolkit *)mToolkit;
	if (toolkit && !toolkit->IsGuiThread())
	{
		nsCOMPtr<nsIWidget> widgetProxy;
		nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
										NS_GET_IID(nsIWidget),
										this, 
										NS_PROXY_SYNC | NS_PROXY_ALWAYS, 
										getter_AddRefs(widgetProxy));
	
		if (NS_FAILED(rv))
			return rv;
		return widgetProxy->Create(aParent, aRect, aHandleEventFunction, aContext,
                           			aAppShell, aToolkit, aInitData);
	}
	return(StandardWindowCreate(nsnull, aRect, aHandleEventFunction,
	                            aContext, aAppShell, aToolkit, aInitData,
	                            aParent));
}

#ifdef MOZ_CAIRO_GFX
gfxASurface*
nsWindow::GetThebesSurface()
{
	mThebesSurface = nsnull;
	if (!mThebesSurface) {
		mThebesSurface = new gfxBeOSSurface(mView);
	}
	return mThebesSurface;
}
#endif






NS_METHOD nsWindow::Destroy()
{
	
	
	nsToolkit* toolkit = (nsToolkit *)mToolkit;
	if (toolkit != nsnull && !toolkit->IsGuiThread())
	{
		nsCOMPtr<nsIWidget> widgetProxy;
		nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
										NS_GET_IID(nsIWidget),
										this, 
										NS_PROXY_SYNC | NS_PROXY_ALWAYS, 
										getter_AddRefs(widgetProxy));
	
		if (NS_FAILED(rv))
			return rv;
		return widgetProxy->Destroy();
	}
	
	if (!mIsDestroying)
	{
		nsBaseWidget::Destroy();
	}	
	
	
	
	
	
	if (PR_FALSE == mOnDestroyCalled)
		OnDestroy();
	
	
	
	if (mView)
	{
		
		mEventCallback = nsnull;
	
		if (mView->LockLooper())
		{
			while(mView->ChildAt(0))
				mView->RemoveChild(mView->ChildAt(0));
			
			BWindow	*w = mView->Window();
			
			
			if (w)
			{
				w->Sync();
				if (mView->Parent())
				{
					mView->Parent()->RemoveChild(mView);
					if (eWindowType_child != mWindowType)
						w->Quit();
					else
					w->Unlock();
				}
				else
				{
					w->RemoveChild(mView);
					w->Quit();
				}
			}
			else
				mView->RemoveSelf();

			delete mView;
		}

		
		mView = NULL;
	}
	mParent = nsnull;
	mWindowParent = nsnull;
	return NS_OK;
}







nsIWidget* nsWindow::GetParent(void)
{
	
	nsIWidget	*widget = 0;
	if (mIsDestroying || mOnDestroyCalled)
		return nsnull;
	widget = (nsIWidget *)mParent;
	return  widget;
}







NS_METHOD nsWindow::Show(PRBool bState)
{
	if (!mEnabled)
		return NS_OK;
		

	if (!mView || !mView->LockLooper())
		return NS_OK;
		
	
	
	
	
	
	if (bState == PR_FALSE)
	{
		if (mView->Window() && !mView->Window()->IsHidden())
			mView->Window()->Hide();
	}
	else
	{
		if (mView->Window() && mView->Window()->IsHidden())
			mView->Window()->Show();
	}

	mView->UnlockLooper();
	mIsVisible = bState;	
	
	return NS_OK;
}



NS_METHOD nsWindow::CaptureMouse(PRBool aCapture)
{
	if (mView && mView->LockLooper())
	{
		if (PR_TRUE == aCapture)
			mView->SetEventMask(B_POINTER_EVENTS);
		else
			mView->SetEventMask(0);
		mView->UnlockLooper();
	}
	return NS_OK;
}



NS_METHOD nsWindow::CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent)
{
	if (!mEnabled)
		return NS_OK;
		
	if (aDoCapture) 
	{
		
		
		
		NS_ASSERTION(!gRollupWidget, "rollup widget reassigned before release");
		gRollupConsumeRollupEvent = aConsumeRollupEvent;
		NS_IF_RELEASE(gRollupListener);
		NS_IF_RELEASE(gRollupWidget);
		gRollupListener = aListener;
		NS_ADDREF(aListener);
		gRollupWidget = this;
		NS_ADDREF(this);
	} 
	else 
	{
		NS_IF_RELEASE(gRollupListener);
		NS_IF_RELEASE(gRollupWidget);
	}

	return NS_OK;
}




PRBool nsWindow::EventIsInsideWindow(nsWindow* aWindow, nsPoint pos)
{
	BRect r;
	BWindow *window = (BWindow *)aWindow->GetNativeData(NS_NATIVE_WINDOW);
	if (window)
	{
		r = window->Frame();
	}
	else
	{
		
		return PR_FALSE;
	}

	if (pos.x < r.left || pos.x > r.right ||
	    pos.y < r.top || pos.y > r.bottom)
	{
		return PR_FALSE;
	}

	return PR_TRUE;
}






PRBool
nsWindow::DealWithPopups(uint32 methodID, nsPoint pos)
{
	if (gRollupListener && gRollupWidget) 
	{
		
		PRBool rollup = !nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget, pos);

		
		
		if (rollup) 
		{
			nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(gRollupListener) );
			if ( menuRollup ) 
			{
				nsCOMPtr<nsISupportsArray> widgetChain;
				menuRollup->GetSubmenuWidgetChain ( getter_AddRefs(widgetChain) );
				if ( widgetChain ) 
				{
					PRUint32 count = 0;
					widgetChain->Count(&count);
					for ( PRUint32 i = 0; i < count; ++i ) 
					{
						nsCOMPtr<nsISupports> genericWidget;
						widgetChain->GetElementAt ( i, getter_AddRefs(genericWidget) );
						nsCOMPtr<nsIWidget> widget ( do_QueryInterface(genericWidget) );
						if ( widget ) 
						{
							nsIWidget* temp = widget.get();
							if ( nsWindow::EventIsInsideWindow((nsWindow*)temp, pos) ) 
							{
								rollup = PR_FALSE;
								break;
							}
						}
					} 
				} 
			} 
		} 

		if (rollup) 
		{
			gRollupListener->Rollup();

			if (gRollupConsumeRollupEvent) 
			{
				return PR_TRUE;
			}
		}
	} 

	return PR_FALSE;
}








NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
	bState = mIsVisible && mView && mView->Visible();
	return NS_OK;
}






NS_METHOD nsWindow::HideWindowChrome(PRBool aShouldHide)
{
	if(mWindowType == eWindowType_child || mView == 0 || mView->Window() == 0)
		return NS_ERROR_FAILURE;
	
	if (aShouldHide)
		mView->Window()->SetLook(B_NO_BORDER_WINDOW_LOOK);
	else
		mView->Window()->SetLook(mBWindowLook);
	return NS_OK;
}






NS_METHOD nsWindow::ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY)
{
	if (mIsTopWidgetWindow && mView->Window()) 
	{
		BScreen screen;
		
		if (! screen.IsValid()) return NS_OK;
		
		BRect screen_rect = screen.Frame();
		BRect win_bounds = mView->Window()->Frame();

#ifdef DEBUG_CONSTRAIN_POSITION
		printf("ConstrainPosition: allowSlop=%s, x=%d, y=%d\n\tScreen :", (aAllowSlop?"T":"F"),*aX,*aY);
		screen_rect.PrintToStream();
		printf("\tWindow: ");
		win_bounds.PrintToStream();
#endif
		
		if (aAllowSlop) 
		{
			if (*aX < kWindowPositionSlop - win_bounds.IntegerWidth() + kWindowBorderWidth)
				*aX = kWindowPositionSlop - win_bounds.IntegerWidth() + kWindowBorderWidth;
			else if (*aX > screen_rect.IntegerWidth() - kWindowPositionSlop - kWindowBorderWidth)
				*aX = screen_rect.IntegerWidth() - kWindowPositionSlop - kWindowBorderWidth;
				
			if (*aY < kWindowPositionSlop - win_bounds.IntegerHeight() + kWindowTitleBarHeight)
				*aY = kWindowPositionSlop - win_bounds.IntegerHeight() + kWindowTitleBarHeight;
			else if (*aY > screen_rect.IntegerHeight() - kWindowPositionSlop - kWindowBorderWidth)
				*aY = screen_rect.IntegerHeight() - kWindowPositionSlop - kWindowBorderWidth;
				
		} 
		else 
		{
			
			if (*aX < kWindowBorderWidth)
				*aX = kWindowBorderWidth;
			else if (*aX > screen_rect.IntegerWidth() - win_bounds.IntegerWidth() - kWindowBorderWidth)
				*aX = screen_rect.IntegerWidth() - win_bounds.IntegerWidth() - kWindowBorderWidth;
				
			if (*aY < kWindowTitleBarHeight)
				*aY = kWindowTitleBarHeight;
			else if (*aY > screen_rect.IntegerHeight() - win_bounds.IntegerHeight() - kWindowBorderWidth)
				*aY = screen_rect.IntegerHeight() - win_bounds.IntegerHeight() - kWindowBorderWidth;
		}
	}
	return NS_OK;
}

void nsWindow::HideKids(PRBool state)	
{
	for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) 
	{
		nsWindow *childWidget = static_cast<nsWindow*>(kid);
		nsRect kidrect = ((nsWindow *)kid)->mBounds;
		
		if (mBounds.Intersects(kidrect))
		{	
			childWidget->Show(!state);
		}
	}
}






nsresult nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
	
	
	
	if (mWindowType != eWindowType_popup && (mBounds.x == aX) && (mBounds.y == aY))
	{
		
		return NS_OK;    
	}


	
	mBounds.x = aX;
	mBounds.y = aY;

	
	

	
	if (mView && mView->LockLooper())
	{
		if (mView->Parent() || !mView->Window())
			mView->MoveTo(aX, aY);
		else
			((nsWindowBeOS *)mView->Window())->MoveTo(aX, aY);
			
		mView->UnlockLooper();
	}

	OnMove(aX,aY);

	return NS_OK;
}








NS_METHOD nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{

	if (aWidth < 0 || aHeight < 0)
		return NS_OK;

	mBounds.width  = aWidth;
	mBounds.height = aHeight;
	
	
	if (mView && mView->LockLooper())
	{
		if (mView->Parent() || !mView->Window())
			mView->ResizeTo(aWidth - 1, aHeight - 1);
		else
			((nsWindowBeOS *)mView->Window())->ResizeTo(aWidth - 1, aHeight - 1);

		mView->UnlockLooper();
	}


	OnResize(mBounds);
	if (aRepaint)
		Update();
	return NS_OK;
}






NS_METHOD nsWindow::Resize(PRInt32 aX,
                           PRInt32 aY,
                           PRInt32 aWidth,
                           PRInt32 aHeight,
                           PRBool   aRepaint)
{
	Move(aX,aY);
	Resize(aWidth,aHeight,aRepaint);
	return NS_OK;
}

NS_METHOD nsWindow::SetModal(PRBool aModal)
{
	if(!(mView && mView->Window()))
		return NS_ERROR_FAILURE;
	if(aModal)
	{
		window_feel newfeel;
		switch(mBWindowFeel)
		{
			case B_FLOATING_SUBSET_WINDOW_FEEL:
				newfeel = B_MODAL_SUBSET_WINDOW_FEEL;
				break;
 			case B_FLOATING_APP_WINDOW_FEEL:
				newfeel = B_MODAL_APP_WINDOW_FEEL;
				break;
 			case B_FLOATING_ALL_WINDOW_FEEL:
				newfeel = B_MODAL_ALL_WINDOW_FEEL;
				break;				
			default:
				return NS_OK;
		}
		mView->Window()->SetFeel(newfeel);
	}
	else
	{
		mView->Window()->SetFeel(mBWindowFeel);
	}
	return NS_OK;
}





NS_METHOD nsWindow::Enable(PRBool aState)
{
	
	mEnabled = aState;
	return NS_OK;
}


NS_METHOD nsWindow::IsEnabled(PRBool *aState)
{
	NS_ENSURE_ARG_POINTER(aState);
	
	*aState = mEnabled;
	return NS_OK;
}






NS_METHOD nsWindow::SetFocus(PRBool aRaise)
{
	
	
	
	
	nsToolkit* toolkit = (nsToolkit *)mToolkit;
	if (toolkit && !toolkit->IsGuiThread()) 
	{
		nsCOMPtr<nsIWidget> widgetProxy;
		nsresult rv = NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
										NS_GET_IID(nsIWidget),
										this, 
										NS_PROXY_SYNC | NS_PROXY_ALWAYS, 
										getter_AddRefs(widgetProxy));
	
		if (NS_FAILED(rv))
			return rv;
		return widgetProxy->SetFocus(aRaise);
	}
	
	
	if (!mEnabled || eWindowType_popup == mWindowType)
		return NS_OK;
		
	if (mView && mView->LockLooper())
	{
		if (mView->Window() && 
		    aRaise == PR_TRUE &&
		    eWindowType_popup != mWindowType && 
			  !mView->Window()->IsActive() && 
			  gLastActiveWindow != mView->Window())
			mView->Window()->Activate(true);
			
		mView->MakeFocus(true);
		mView->UnlockLooper();
		DispatchFocus(NS_GOTFOCUS);
	}

	return NS_OK;
}






NS_IMETHODIMP nsWindow::GetScreenBounds(nsRect &aRect)
{
	
	if (mView && mView->Window()) 
	{
		BRect r = mView->Window()->Frame();
		aRect.x = nscoord(r.left);
		aRect.y = nscoord(r.top);
		aRect.width  = r.IntegerWidth()+1;
		aRect.height = r.IntegerHeight()+1;
	} 
	else 
	{
		aRect = mBounds;
	}
	return NS_OK;
}  






NS_METHOD nsWindow::SetBackgroundColor(const nscolor &aColor)
{
	nsBaseWidget::SetBackgroundColor(aColor);

	
	
	if (!mIsTopWidgetWindow)
		return NS_OK;

	if (mView && mView->LockLooper())
	{
		mView->SetViewColor(NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor), NS_GET_A(aColor));
		mView->UnlockLooper();
	}
	return NS_OK;
}






nsIFontMetrics* nsWindow::GetFont(void)
{
	return mFontMetrics;
}







NS_METHOD nsWindow::SetFont(const nsFont &aFont)
{
  
	NS_IF_RELEASE(mFontMetrics);
	if (mContext)
		mContext->GetMetricsFor(aFont, mFontMetrics);
	return NS_OK;
}








NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
	if (!mView)
		return NS_ERROR_FAILURE;

	
	if (aCursor != mCursor) 
	{
		BCursor const *newCursor = B_CURSOR_SYSTEM_DEFAULT;
		
		
		if (gCursorArray.Count() == 0) 
		{
			gCursorArray.InsertElementAt((void*) new BCursor(cursorHyperlink),0);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorHorizontalDrag),1);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorVerticalDrag),2);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorUpperLeft),3);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorLowerRight),4);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorUpperRight),5);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorLowerLeft),6);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorCrosshair),7);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorHelp),8);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorGrab),9);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorGrabbing),10);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorCopy),11);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorAlias),12);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorWatch2),13);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorCell),14);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorZoomIn),15);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorZoomOut),16);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorLeft),17);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorRight),18);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorTop),19);
			gCursorArray.InsertElementAt((void*) new BCursor(cursorBottom),20);
		}

		switch (aCursor) 
		{
			case eCursor_standard:
			case eCursor_move:
				newCursor = B_CURSOR_SYSTEM_DEFAULT;
				break;
	
			case eCursor_select:
				newCursor = B_CURSOR_I_BEAM;
				break;
	
			case eCursor_hyperlink:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(0);
				break;
	
			case eCursor_n_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(19);
				break;

			case eCursor_s_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(20);
				break;
	
			case eCursor_w_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(17);
				break;

			case eCursor_e_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(18);
				break;
	
			case eCursor_nw_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(3);
				break;
	
			case eCursor_se_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(4);
				break;
	
			case eCursor_ne_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(5);
				break;
	
			case eCursor_sw_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(6);
				break;
	
			case eCursor_crosshair:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(7);
				break;
	
			case eCursor_help:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(8);
				break;
	
			case eCursor_copy:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(11);
				break;
	
			case eCursor_alias:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(12);
				break;

			case eCursor_context_menu:
				
				break;
				
			case eCursor_cell:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(14);
				break;

			case eCursor_grab:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(9);
				break;
	
			case eCursor_grabbing:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(10);
				break;
	
			case eCursor_wait:
			case eCursor_spinning:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(13);
				break;
	
			case eCursor_zoom_in:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(15);
				break;

			case eCursor_zoom_out:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(16);
				break;

			case eCursor_not_allowed:
			case eCursor_no_drop:
				
				break;

			case eCursor_col_resize:
				
				newCursor = (BCursor *)gCursorArray.SafeElementAt(1);
				break;

			case eCursor_row_resize:
				
				newCursor = (BCursor *)gCursorArray.SafeElementAt(2);
				break;

			case eCursor_vertical_text:
				
				newCursor = B_CURSOR_I_BEAM;
				break;

			case eCursor_all_scroll:
				
				break;

			case eCursor_nesw_resize:
				
				newCursor = (BCursor *)gCursorArray.SafeElementAt(1);
				break;

			case eCursor_nwse_resize:
				
				newCursor = (BCursor *)gCursorArray.SafeElementAt(1);
				break;

			case eCursor_ns_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(2);
				break;

			case eCursor_ew_resize:
				newCursor = (BCursor *)gCursorArray.SafeElementAt(1);
				break;

			default:
				NS_ASSERTION(0, "Invalid cursor type");
				break;
		}
		NS_ASSERTION(newCursor != nsnull, "Cursor not stored in array properly!");
		mCursor = aCursor;
		be_app->SetCursor(newCursor, true);
	}
	return NS_OK;
}






NS_METHOD nsWindow::Invalidate(PRBool aIsSynchronous)
{
	nsresult rv = NS_ERROR_FAILURE;
	
	
	
	
	BRegion reg;
	reg.MakeEmpty();
	if (mView && mView->LockLooper())
	{
		if (PR_TRUE == aIsSynchronous)
		{
			mView->paintregion.Include(mView->Bounds());
			reg.Include(mView->Bounds());
		}
		else
		{
			mView->Draw(mView->Bounds());
			rv = NS_OK;
		}
		mView->UnlockLooper();
	}
	
	if (PR_TRUE == aIsSynchronous)
		rv = OnPaint(&reg);
	return rv;
}






NS_METHOD nsWindow::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
	nsresult rv = NS_ERROR_FAILURE;
	
	BRegion reg;
	reg.MakeEmpty();
	if (mView && mView->LockLooper()) 
	{
		BRect	r(aRect.x, 
				aRect.y, 
				aRect.x + aRect.width - 1, 
				aRect.y + aRect.height - 1);
		if (PR_TRUE == aIsSynchronous)
		{
			mView->paintregion.Include(r);
			reg.Include(r);
		}
		else
		{
			
			
			mView->Draw(r);
			rv = NS_OK;
		}
		mView->UnlockLooper();
	}
	
	
	
	if (PR_TRUE == aIsSynchronous)
		rv = OnPaint(&reg);
	return rv;
}






NS_IMETHODIMP nsWindow::InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous)
{
	
	nsRegionRectSet *rectSet = nsnull;
	if (!aRegion)
		return NS_ERROR_FAILURE;
	nsresult rv = ((nsIRegion *)aRegion)->GetRects(&rectSet);
	if (NS_FAILED(rv))
		return rv;
	BRegion reg;
	reg.MakeEmpty();
	if (mView && mView->LockLooper())
	{
		for (PRUint32 i=0; i< rectSet->mRectsLen; ++i)
		{
			BRect br(rectSet->mRects[i].x, rectSet->mRects[i].y,
					rectSet->mRects[i].x + rectSet->mRects[i].width-1,
					rectSet->mRects[i].y + rectSet->mRects[i].height -1);
			if (PR_TRUE == aIsSynchronous)
			{
				mView->paintregion.Include(br);
				reg.Include(br);
			}
			else
			{
				mView->Draw(br);
				rv = NS_OK;
			}
		}
		mView->UnlockLooper();
	}
	
	
	if (PR_TRUE == aIsSynchronous)
		rv = OnPaint(&reg);

	return rv;
}






NS_IMETHODIMP nsWindow::Update()
{
	nsresult rv = NS_ERROR_FAILURE;
	
	mIsScrolling = PR_FALSE;
	if (mWindowType == eWindowType_child)
		return NS_OK;
	BRegion reg;
	reg.MakeEmpty();
	if(mView && mView->LockLooper())
	{
		
		if (mView->Window())
			mView->Window()->UpdateIfNeeded();
		
		mView->Invalidate();
		bool nonempty = mView->GetPaintRegion(&reg);
		mView->UnlockLooper();
		
		if (nonempty)
			rv = OnPaint(&reg);
	}
	return rv;
}






void* nsWindow::GetNativeData(PRUint32 aDataType)
{
	if (!mView)
		return NULL;	
	switch(aDataType) 
	{
		case NS_NATIVE_WINDOW:
			return (void *)(mView->Window());
		case NS_NATIVE_WIDGET:
		case NS_NATIVE_PLUGIN_PORT:
			return (void *)((nsViewBeOS *)mView);
		case NS_NATIVE_GRAPHIC:
			return (void *)((BView *)mView);
		case NS_NATIVE_COLORMAP:
		default:
			break;
	}
	return NULL;
}






NS_METHOD nsWindow::SetColorMap(nsColorMap *aColorMap)
{
	NS_WARNING("nsWindow::SetColorMap - not implemented");
	return NS_OK;
}







NS_METHOD nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
	
	mIsScrolling = PR_TRUE;
	
	
	
	
	
	HideKids(PR_TRUE);
	if (mView && mView->LockLooper())
	{
		
		mView->SetVisible(false);
		
		BRect src;
		BRect b = mView->Bounds();

		if (aClipRect)
		{
			src.left = aClipRect->x;
			src.top = aClipRect->y;
			src.right = aClipRect->XMost() - 1;
			src.bottom = aClipRect->YMost() - 1;
		}
		else
		{
			src = b;
		}
		
		if (mView->Window())
		{
			BRect screenframe = mView->ConvertFromScreen(BScreen(mView->Window()).Frame());
			src = src & screenframe;
			if (mView->Parent())
			{
				BRect parentframe = mView->ConvertFromParent(mView->Parent()->Frame());
				src = src & parentframe;
			}
		}

		BRegion	invalid;
		invalid.Include(src);
		
		if ( BView *v = mView->Parent() )
		{
			for (BView *child = v->ChildAt(0); child; child = child->NextSibling() )
			{
				BRect siblingframe = mView->ConvertFromParent(child->Frame());
				if (child != mView && child->Parent() != mView)
				{
					invalid.Exclude(siblingframe);
					mView->paintregion.Exclude(siblingframe);
				}
			}
			src = invalid.Frame();
		}

		
		

		if (src.left + aDx < 0)
			src.left = -aDx;
		if (src.right + aDx > b.right)
			src.right = b.right - aDx;
		if (src.top + aDy < 0)
			src.top = -aDy;
		if (src.bottom + aDy > b.bottom)
			src.bottom = b.bottom - aDy;
		
		BRect dest = src.OffsetByCopy(aDx, aDy);
		mView->ConstrainClippingRegion(&invalid);
		
		if (src.IsValid() && dest.IsValid())
			mView->CopyBits(src, dest);

		invalid.Exclude(dest);	
		
		
		
		mView->paintregion.OffsetBy(aDx, aDy);
		mView->ConstrainClippingRegion(&invalid);
		
		for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) 
		{
			nsWindow *childWidget = static_cast<nsWindow*>(kid);
			
			
			nsRect bounds = childWidget->mBounds;
			bounds.x += aDx;
			bounds.y += aDy; 
			childWidget->Move(bounds.x, bounds.y);
			BView *child = ((BView *)kid->GetNativeData(NS_NATIVE_WIDGET));
			if (child)
				mView->paintregion.Exclude(child->Frame());
		}
		
		
		
		OnPaint(&invalid);
		HideKids(PR_FALSE);
		
		mView->SetVisible(true);
		mView->UnlockLooper();
	}
	return NS_OK;
}








bool nsWindow::CallMethod(MethodInfo *info)
{
	bool bRet = TRUE;

	switch (info->methodId)
	{
	case nsSwitchToUIThread::CLOSEWINDOW :
		{
			NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
			if (eWindowType_popup != mWindowType && eWindowType_child != mWindowType)
				DealWithPopups(nsSwitchToUIThread::CLOSEWINDOW,nsPoint(0,0));

			
			

			for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) 
			{
				nsWindow *childWidget = static_cast<nsWindow*>(kid);
				BWindow* kidwindow = (BWindow *)kid->GetNativeData(NS_NATIVE_WINDOW);
				if (kidwindow)
				{
					
					BMessenger bm(kidwindow);
					bm.SendMessage(B_QUIT_REQUESTED);
				}
			}
			DispatchStandardEvent(NS_DESTROY);
		}
		break;

#ifdef DEBUG_FOCUS
	case nsSwitchToUIThread::GOT_FOCUS:
		NS_ASSERTION(info->nArgs == 1, "Wrong number of arguments to CallMethod");
		if (!mEnabled)
			return false;
		if ((uint32)info->args[0] != (uint32)mView)
			printf("Wrong view to get focus\n");*/
		break;
#endif
	case nsSwitchToUIThread::KILL_FOCUS:
		NS_ASSERTION(info->nArgs == 1, "Wrong number of arguments to CallMethod");
		if ((uint32)info->args[0] == (uint32)mView)
			DispatchFocus(NS_LOSTFOCUS);
#ifdef DEBUG_FOCUS
		else
			printf("Wrong view to de-focus\n");
#endif
#if defined BeIME
		nsIMEBeOS::GetIME()->DispatchCancelIME();
		if (mView && mView->LockLooper())
 		{
 			mView->SetFlags(mView->Flags() & ~B_NAVIGABLE);
 			mView->UnlockLooper();
 		}
#endif
		break;

	case nsSwitchToUIThread::BTNCLICK :
		{
			NS_ASSERTION(info->nArgs == 6, "Wrong number of arguments to CallMethod");
			if (!mEnabled)
				return false;
			
			uint32 eventID = ((int32 *)info->args)[0];
			PRBool rollup = PR_FALSE;

			if (eventID == NS_MOUSE_BUTTON_DOWN &&
			        mView && mView->LockLooper())
			{
				BPoint p(((int32 *)info->args)[1], ((int32 *)info->args)[2]);
				mView->ConvertToScreen(&p);
				rollup = DealWithPopups(nsSwitchToUIThread::ONMOUSE, nsPoint(p.x, p.y));
				mView->UnlockLooper();
			}
			
			if (rollup)
				return false;
			DispatchMouseEvent(((int32 *)info->args)[0],
			                   nsPoint(((int32 *)info->args)[1], ((int32 *)info->args)[2]),
			                   ((int32 *)info->args)[3],
			                   ((int32 *)info->args)[4],
			                   ((int32 *)info->args)[5]);

			if (((int32 *)info->args)[0] == NS_MOUSE_BUTTON_DOWN &&
			    ((int32 *)info->args)[5] == nsMouseEvent::eRightButton)
			{
				DispatchMouseEvent (NS_CONTEXTMENU,
				                    nsPoint(((int32 *)info->args)[1], ((int32 *)info->args)[2]),
				                    ((int32 *)info->args)[3],
				                    ((int32 *)info->args)[4],
				                    ((int32 *)info->args)[5]);
			}
		}
		break;

	case nsSwitchToUIThread::ONWHEEL :
		{
			NS_ASSERTION(info->nArgs == 1, "Wrong number of arguments to CallMethod");
			
			if ((uint32)info->args[0] != (uint32)mView)
				return false;
			BPoint cursor(0,0);
			uint32 buttons;
			BPoint delta;
			if (mView && mView->LockLooper())
			{
				mView->GetMouse(&cursor, &buttons, false);
				delta = mView->GetWheel();
				mView->UnlockLooper();
			}
			else
				return false;
			
			
			
			if (nscoord(delta.y) != 0)
			{
				OnWheel(nsMouseScrollEvent::kIsVertical, buttons, cursor, nscoord(delta.y)*3);
			}
			else if(nscoord(delta.x) != 0)
				OnWheel(nsMouseScrollEvent::kIsHorizontal, buttons, cursor, nscoord(delta.x)*3);
		}
		break;

	case nsSwitchToUIThread::ONKEY :
		NS_ASSERTION(info->nArgs == 6, "Wrong number of arguments to CallMethod");
		if (((int32 *)info->args)[0] == NS_KEY_DOWN)
		{
			OnKeyDown(((int32 *)info->args)[0],
			          (const char *)(&((uint32 *)info->args)[1]), ((int32 *)info->args)[2],
			          ((uint32 *)info->args)[3], ((uint32 *)info->args)[4], ((int32 *)info->args)[5]);
		}
		else
		{
			if (((int32 *)info->args)[0] == NS_KEY_UP)
			{
				OnKeyUp(((int32 *)info->args)[0],
				        (const char *)(&((uint32 *)info->args)[1]), ((int32 *)info->args)[2],
				        ((uint32 *)info->args)[3], ((uint32 *)info->args)[4], ((int32 *)info->args)[5]);
			}
		}
		break;

	case nsSwitchToUIThread::ONPAINT :
		NS_ASSERTION(info->nArgs == 1, "Wrong number of arguments to CallMethod");
		{
			if ((uint32)mView != ((uint32 *)info->args)[0])
				return false;
			BRegion reg;
			reg.MakeEmpty();
			if(mView && mView->LockLooper())
			{
				bool nonempty = mView->GetPaintRegion(&reg);
				mView->UnlockLooper();
				if (nonempty)
					OnPaint(&reg);
			}
		}
		break;

	case nsSwitchToUIThread::ONRESIZE :
		{
			NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
			if (eWindowType_popup != mWindowType && eWindowType_child != mWindowType)
				DealWithPopups(nsSwitchToUIThread::ONRESIZE,nsPoint(0,0));
			
			if (!mIsTopWidgetWindow  || !mView  || !mView->Window())
				return false;
			
			nsRect r(mBounds);
			if (mView->LockLooper())
			{
				BRect br = mView->Frame();
				r.x = nscoord(br.left);
				r.y = nscoord(br.top);
				r.width  = br.IntegerWidth() + 1;
				r.height = br.IntegerHeight() + 1;
				((nsWindowBeOS *)mView->Window())->fJustGotBounds = true;
				mView->UnlockLooper();
			}
			OnResize(r);
		}
		break;

	case nsSwitchToUIThread::ONMOUSE :
		{
			NS_ASSERTION(info->nArgs == 4, "Wrong number of arguments to CallMethod");
			if (!mEnabled)
				return false;
			DispatchMouseEvent(((int32 *)info->args)[0],
			                   nsPoint(((int32 *)info->args)[1], ((int32 *)info->args)[2]),
			                   0,
		    	               ((int32 *)info->args)[3]);
		}
		break;

	case nsSwitchToUIThread::ONDROP :
		{
			NS_ASSERTION(info->nArgs == 4, "Wrong number of arguments to CallMethod");

			nsMouseEvent event(PR_TRUE, (int32)  info->args[0], this, nsMouseEvent::eReal);
			nsPoint point(((int32 *)info->args)[1], ((int32 *)info->args)[2]);
			InitEvent (event, &point);
			uint32 mod = (uint32) info->args[3];
			event.isShift   = mod & B_SHIFT_KEY;
			event.isControl = mod & B_CONTROL_KEY;
			event.isAlt     = mod & B_COMMAND_KEY;
			event.isMeta     = mod & B_OPTION_KEY;

			
			nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
			if (dragService)
			{
				nsCOMPtr<nsIDragSession> dragSession;
				dragService->GetCurrentSession(getter_AddRefs(dragSession));
				if (dragSession)
				{
					
					
	
					PRUint32 action_mask = 0;
					dragSession->GetDragAction(&action_mask);
					PRUint32 action = nsIDragService::DRAGDROP_ACTION_MOVE;
					if (mod & B_OPTION_KEY)
					{
						if (mod & B_COMMAND_KEY)
							action = nsIDragService::DRAGDROP_ACTION_LINK & action_mask;
						else
							action = nsIDragService::DRAGDROP_ACTION_COPY & action_mask;
					}
					dragSession->SetDragAction(action);
				}
			}
			DispatchWindowEvent(&event);
			NS_RELEASE(event.widget);

			if (dragService)
				dragService->EndDragSession(PR_TRUE);
		}
		break;

	case nsSwitchToUIThread::ONACTIVATE:
		NS_ASSERTION(info->nArgs == 2, "Wrong number of arguments to CallMethod");
		if (!mEnabled || eWindowType_popup == mWindowType || 0 == mView->Window())
			return false;
		if ((BWindow *)info->args[1] != mView->Window())
			return false;
		if (mEventCallback || eWindowType_child == mWindowType )
		{
			bool active = (bool)info->args[0];
			if (!active) 
			{
				if (eWindowType_dialog == mWindowType || 
				    eWindowType_toplevel == mWindowType)
					DealWithPopups(nsSwitchToUIThread::ONACTIVATE,nsPoint(0,0));
				
				if (!mView->Window()->IsActive())
				{
					
					
					
					if (mWindowParent &&  mView->Window()->IsFloating())
						mWindowParent->DispatchFocus(NS_ACTIVATE);

					DispatchFocus(NS_DEACTIVATE);
#if defined(BeIME)
					nsIMEBeOS::GetIME()->DispatchCancelIME();
#endif
				}
			} 
			else 
			{

				if (mView->Window()->IsActive())
				{
					
					if (mWindowParent &&  mView->Window()->IsFloating())
						mWindowParent->DispatchFocus(NS_DEACTIVATE);
					
					DispatchFocus(NS_ACTIVATE);
					if (mView && mView->Window())
						gLastActiveWindow = mView->Window();
				}
			}
		}
		break;

	case nsSwitchToUIThread::ONMOVE:
		{
			NS_ASSERTION(info->nArgs == 0, "Wrong number of arguments to CallMethod");
			nsRect r;
			
			GetScreenBounds(r);		
			if (eWindowType_popup != mWindowType && eWindowType_child != mWindowType)
				DealWithPopups(nsSwitchToUIThread::ONMOVE,nsPoint(0,0));
			OnMove(r.x, r.y);
		}
		break;
		
	case nsSwitchToUIThread::ONWORKSPACE:
		{
			NS_ASSERTION(info->nArgs == 2, "Wrong number of arguments to CallMethod");
			if (eWindowType_popup != mWindowType && eWindowType_child != mWindowType)
				DealWithPopups(nsSwitchToUIThread::ONWORKSPACE,nsPoint(0,0));
		}
		break;

#if defined(BeIME)
 	case nsSwitchToUIThread::ONIME:
 		
 		if (mView && mView->LockLooper())
 		{
 			mView->SetFlags(mView->Flags() | B_NAVIGABLE);
 			mView->UnlockLooper();
 		}
 		nsIMEBeOS::GetIME()->RunIME(info->args, this, mView);
 		break;
#endif
		default:
			bRet = FALSE;
			break;
		
	}

	return bRet;
}







struct nsKeyConverter {
	int vkCode; 
	char bekeycode; 
};







struct nsKeyConverter nsKeycodesBeOS[] = {
	        
	        { NS_VK_BACK,       0x1e },
	        { NS_VK_TAB,        0x26 },
	        
	        
	        { NS_VK_RETURN,     0x47 },
	        { NS_VK_SHIFT,      0x4b },
	        { NS_VK_SHIFT,      0x56 },
	        { NS_VK_CONTROL,    0x5c },
	        { NS_VK_CONTROL,    0x60 },
	        { NS_VK_ALT,        0x5d },
	        { NS_VK_ALT,        0x5f },
	        { NS_VK_PAUSE,      0x22 },
	        { NS_VK_CAPS_LOCK,  0x3b },
	        { NS_VK_ESCAPE,     0x1 },
	        { NS_VK_SPACE,      0x5e },
	        { NS_VK_PAGE_UP,    0x21 },
	        { NS_VK_PAGE_DOWN,  0x36 },
	        { NS_VK_END,        0x35 },
	        { NS_VK_HOME,       0x20 },
	        { NS_VK_LEFT,       0x61 },
	        { NS_VK_UP,         0x57 },
	        { NS_VK_RIGHT,      0x63 },
	        { NS_VK_DOWN,       0x62 },
	        { NS_VK_PRINTSCREEN, 0xe },
	        { NS_VK_INSERT,     0x1f },
	        { NS_VK_DELETE,     0x34 },

	        
	        { NS_VK_META,       0x66 },
	        { NS_VK_META,       0x67 },

	        
	        { NS_VK_MULTIPLY,   0x24 },
	        { NS_VK_ADD,        0x3a },
	        
	        { NS_VK_SUBTRACT,   0x25 },
	        { NS_VK_DIVIDE,     0x23 },
	        { NS_VK_RETURN,     0x5b },

	        { NS_VK_COMMA,      0x53 },
	        { NS_VK_PERIOD,     0x54 },
	        { NS_VK_SLASH,      0x55 },
	        { NS_VK_BACK_SLASH, 0x33 },
	        { NS_VK_BACK_SLASH, 0x6a }, 
	        { NS_VK_BACK_SLASH, 0x6b }, 
	        { NS_VK_BACK_QUOTE, 0x11 },
	        { NS_VK_OPEN_BRACKET, 0x31 },
	        { NS_VK_CLOSE_BRACKET, 0x32 },
	        { NS_VK_SEMICOLON, 0x45 },
	        { NS_VK_QUOTE, 0x46 },

	        
	        
	        { NS_VK_SUBTRACT, 0x1c },
	        { NS_VK_EQUALS, 0x1d },

	        { NS_VK_F1, B_F1_KEY },
	        { NS_VK_F2, B_F2_KEY },
	        { NS_VK_F3, B_F3_KEY },
	        { NS_VK_F4, B_F4_KEY },
	        { NS_VK_F5, B_F5_KEY },
	        { NS_VK_F6, B_F6_KEY },
	        { NS_VK_F7, B_F7_KEY },
	        { NS_VK_F8, B_F8_KEY },
	        { NS_VK_F9, B_F9_KEY },
	        { NS_VK_F10, B_F10_KEY },
	        { NS_VK_F11, B_F11_KEY },
	        { NS_VK_F12, B_F12_KEY },

	        { NS_VK_1, 0x12 },
	        { NS_VK_2, 0x13 },
	        { NS_VK_3, 0x14 },
	        { NS_VK_4, 0x15 },
	        { NS_VK_5, 0x16 },
	        { NS_VK_6, 0x17 },
	        { NS_VK_7, 0x18 },
	        { NS_VK_8, 0x19 },
	        { NS_VK_9, 0x1a },
	        { NS_VK_0, 0x1b },

	        { NS_VK_A, 0x3c },
	        { NS_VK_B, 0x50 },
	        { NS_VK_C, 0x4e },
	        { NS_VK_D, 0x3e },
	        { NS_VK_E, 0x29 },
	        { NS_VK_F, 0x3f },
	        { NS_VK_G, 0x40 },
	        { NS_VK_H, 0x41 },
	        { NS_VK_I, 0x2e },
	        { NS_VK_J, 0x42 },
	        { NS_VK_K, 0x43 },
	        { NS_VK_L, 0x44 },
	        { NS_VK_M, 0x52 },
	        { NS_VK_N, 0x51 },
	        { NS_VK_O, 0x2f },
	        { NS_VK_P, 0x30 },
	        { NS_VK_Q, 0x27 },
	        { NS_VK_R, 0x2a },
	        { NS_VK_S, 0x3d },
	        { NS_VK_T, 0x2b },
	        { NS_VK_U, 0x2d },
	        { NS_VK_V, 0x4f },
	        { NS_VK_W, 0x28 },
	        { NS_VK_X, 0x4d },
	        { NS_VK_Y, 0x2c },
	        { NS_VK_Z, 0x4c }
        };


struct nsKeyConverter nsKeycodesBeOSNumLock[] = {
	        { NS_VK_NUMPAD0, 0x64 },
	        { NS_VK_NUMPAD1, 0x58 },
	        { NS_VK_NUMPAD2, 0x59 },
	        { NS_VK_NUMPAD3, 0x5a },
	        { NS_VK_NUMPAD4, 0x48 },
	        { NS_VK_NUMPAD5, 0x49 },
	        { NS_VK_NUMPAD6, 0x4a },
	        { NS_VK_NUMPAD7, 0x37 },
	        { NS_VK_NUMPAD8, 0x38 },
	        { NS_VK_NUMPAD9, 0x39 },
	        { NS_VK_DECIMAL, 0x65 }
        };


struct nsKeyConverter nsKeycodesBeOSNoNumLock[] = {
	        { NS_VK_LEFT,       0x48 },
	        { NS_VK_RIGHT,      0x4a },
	        { NS_VK_UP,         0x38 },
	        { NS_VK_DOWN,       0x59 },
	        { NS_VK_PAGE_UP,    0x39 },
	        { NS_VK_PAGE_DOWN,  0x5a },
	        { NS_VK_HOME,       0x37 },
	        { NS_VK_END,        0x58 },
	        { NS_VK_INSERT,     0x64 },
	        { NS_VK_DELETE,     0x65 }
        };








static int TranslateBeOSKeyCode(int32 bekeycode, bool isnumlock)
{
#ifdef KB_DEBUG
	printf("TranslateBeOSKeyCode: bekeycode = 0x%x\n",bekeycode);
#endif
	int i;
	int length = sizeof(nsKeycodesBeOS) / sizeof(struct nsKeyConverter);
	int length_numlock = sizeof(nsKeycodesBeOSNumLock) / sizeof(struct nsKeyConverter);
	int length_nonumlock = sizeof(nsKeycodesBeOSNoNumLock) / sizeof(struct nsKeyConverter);

	
	for (i = 0; i < length; i++)
	{
		if (nsKeycodesBeOS[i].bekeycode == bekeycode)
			return(nsKeycodesBeOS[i].vkCode);
	}
	
	if (isnumlock)
	{
		for (i = 0; i < length_numlock; i++)
		{
			if (nsKeycodesBeOSNumLock[i].bekeycode == bekeycode)
				return(nsKeycodesBeOSNumLock[i].vkCode);
		}
	}
	else
	{
		for (i = 0; i < length_nonumlock; i++)
		{
			if (nsKeycodesBeOSNoNumLock[i].bekeycode == bekeycode)
				return(nsKeycodesBeOSNoNumLock[i].vkCode);
		}
	}
#ifdef KB_DEBUG
	printf("TranslateBeOSKeyCode: ####### Translation not Found #######\n");
#endif
	return((int)0);
}






PRBool nsWindow::OnKeyDown(PRUint32 aEventType, const char *bytes,
                           int32 numBytes, PRUint32 mod, PRUint32 bekeycode, int32 rawcode)
{
	PRUint32 aTranslatedKeyCode;
	PRBool noDefault = PR_FALSE;

	mIsShiftDown   = (mod & B_SHIFT_KEY) ? PR_TRUE : PR_FALSE;
	mIsControlDown = (mod & B_CONTROL_KEY) ? PR_TRUE : PR_FALSE;
	mIsAltDown     = ((mod & B_COMMAND_KEY) && !(mod & B_RIGHT_OPTION_KEY))? PR_TRUE : PR_FALSE;
	mIsMetaDown    = (mod & B_LEFT_OPTION_KEY) ? PR_TRUE : PR_FALSE;	
	bool IsNumLocked = ((mod & B_NUM_LOCK) != 0);

	aTranslatedKeyCode = TranslateBeOSKeyCode(bekeycode, IsNumLocked);

	if (numBytes <= 1)
	{
		noDefault  = DispatchKeyEvent(NS_KEY_DOWN, 0, aTranslatedKeyCode);
	}
	else
	{
		
	}

	
	PRUint32	uniChar;

	if ((mIsControlDown || mIsAltDown || mIsMetaDown) && rawcode >= 'a' && rawcode <= 'z')
	{
		if (mIsShiftDown)
			uniChar = rawcode + 'A' - 'a';
		else
			uniChar = rawcode;
		aTranslatedKeyCode = 0;
	} 
	else
	{
		if (numBytes == 0) 
			return noDefault;

		switch((unsigned char)bytes[0])
		{
		case 0xc8:
		case 0xca:
			return noDefault;

		case B_INSERT:
		case B_ESCAPE:
		case B_FUNCTION_KEY:
		case B_HOME:
		case B_PAGE_UP:
		case B_END:
		case B_PAGE_DOWN:
		case B_UP_ARROW:
		case B_LEFT_ARROW:
		case B_DOWN_ARROW:
		case B_RIGHT_ARROW:
		case B_TAB:
		case B_DELETE:
		case B_BACKSPACE:
		case B_ENTER:
			uniChar = 0;
			break;

		default:
			
			if (numBytes >= 1 && (bytes[0] & 0x80) == 0)
			{
				
				uniChar = bytes[0];
			} 
			else
			{
				if (numBytes >= 2 && (bytes[0] & 0xe0) == 0xc0)
				{
					
					uniChar = ((uint16)(bytes[0] & 0x1f) << 6) | (uint16)(bytes[1] & 0x3f);
				}
				else
				{
					if (numBytes >= 3 && (bytes[0] & 0xf0) == 0xe0)
					{
						
						uniChar = ((uint16)(bytes[0] & 0x0f) << 12) | ((uint16)(bytes[1] & 0x3f) << 6)
						          | (uint16)(bytes[2] & 0x3f);
					}
					else
					{
						
						uniChar = 0;
						NS_WARNING("nsWindow::OnKeyDown() error: bytes[] has not enough chars.");
					}
				}
			}
			aTranslatedKeyCode = 0;
			break;
		}
	}

	
	PRUint32 extraFlags = (noDefault ? NS_EVENT_FLAG_NO_DEFAULT : 0);
	return DispatchKeyEvent(NS_KEY_PRESS, uniChar, aTranslatedKeyCode, extraFlags) && noDefault;
}






PRBool nsWindow::OnKeyUp(PRUint32 aEventType, const char *bytes,
                         int32 numBytes, PRUint32 mod, PRUint32 bekeycode, int32 rawcode)
{
	PRUint32 aTranslatedKeyCode;
	bool IsNumLocked = ((mod & B_NUM_LOCK) != 0);

	mIsShiftDown   = (mod & B_SHIFT_KEY) ? PR_TRUE : PR_FALSE;
	mIsControlDown = (mod & B_CONTROL_KEY) ? PR_TRUE : PR_FALSE;
	mIsAltDown     = ((mod & B_COMMAND_KEY) && !(mod & B_RIGHT_OPTION_KEY))? PR_TRUE : PR_FALSE;
	mIsMetaDown    = (mod & B_LEFT_OPTION_KEY) ? PR_TRUE : PR_FALSE;	

	aTranslatedKeyCode = TranslateBeOSKeyCode(bekeycode, IsNumLocked);

	PRBool result = DispatchKeyEvent(NS_KEY_UP, 0, aTranslatedKeyCode);
	return result;

}







PRBool nsWindow::DispatchKeyEvent(PRUint32 aEventType, PRUint32 aCharCode,
                                  PRUint32 aKeyCode, PRUint32 aFlags)
{
	nsKeyEvent event(PR_TRUE, aEventType, this);
	nsPoint point;

	point.x = 0;
	point.y = 0;

	InitEvent(event, &point); 

	event.flags |= aFlags;
	event.charCode = aCharCode;
	event.keyCode  = aKeyCode;

#ifdef KB_DEBUG
	static int cnt=0;
	printf("%d DispatchKE Type: %s charCode 0x%x  keyCode 0x%x ", cnt++,
	       (NS_KEY_PRESS == aEventType)?"PRESS":(aEventType == NS_KEY_UP?"Up":"Down"),
	       event.charCode, event.keyCode);
	printf("Shift: %s Control %s Alt: %s Meta: %s\n",  
	       (mIsShiftDown?"D":"U"), 
	       (mIsControlDown?"D":"U"), 
	       (mIsAltDown?"D":"U"),
	       (mIsMetaDown?"D":"U"));
#endif

	event.isShift   = mIsShiftDown;
	event.isControl = mIsControlDown;
	event.isMeta   =  mIsMetaDown;
	event.isAlt     = mIsAltDown;

	PRBool result = DispatchWindowEvent(&event);
	NS_RELEASE(event.widget);

	return result;
}






void nsWindow::OnDestroy()
{
	mOnDestroyCalled = PR_TRUE;

	
	nsBaseWidget::OnDestroy();

	
	if (!mIsDestroying) 
	{
		
		
		
		AddRef();
		DispatchStandardEvent(NS_DESTROY);
		Release();
	}
}






PRBool nsWindow::OnMove(PRInt32 aX, PRInt32 aY)
{
	nsGUIEvent event(PR_TRUE, NS_MOVE, this);
	InitEvent(event);
	event.refPoint.x = aX;
	event.refPoint.y = aY;

	PRBool result = DispatchWindowEvent(&event);
	NS_RELEASE(event.widget);
	return result;
}

void nsWindow::OnWheel(PRInt32 aDirection, uint32 aButtons, BPoint aPoint, nscoord aDelta)
{
		
		
		

		nsMouseScrollEvent scrollEvent(PR_TRUE, NS_MOUSE_SCROLL, this);
		uint32 mod (modifiers());
		scrollEvent.isControl = mod & B_CONTROL_KEY;
		scrollEvent.isShift = mod & B_SHIFT_KEY;
		scrollEvent.isAlt   = mod & B_COMMAND_KEY;
		scrollEvent.isMeta  = mod & B_OPTION_KEY;
						
		scrollEvent.scrollFlags = aDirection;
		scrollEvent.delta = aDelta;
		scrollEvent.time      = PR_IntervalNow();
		scrollEvent.refPoint.x = nscoord(aPoint.x);
		scrollEvent.refPoint.y = nscoord(aPoint.y);

		nsEventStatus rv;
		DispatchEvent (&scrollEvent, rv);
}






nsresult nsWindow::OnPaint(BRegion *breg)
{
	nsresult rv = NS_ERROR_FAILURE;
	if (mView && mView->LockLooper())
	{
		
		mView->Validate(breg);
		
		
		mView->ConstrainClippingRegion(breg);
		mView->UnlockLooper();
	}
	else
		return rv;
	BRect br = breg->Frame();
	if (!br.IsValid() || !mEventCallback || !mView  || (eWindowType_child != mWindowType && eWindowType_popup != mWindowType))
		return rv;
	nsRect nsr(nscoord(br.left), nscoord(br.top), 
			nscoord(br.IntegerWidth() + 1), nscoord(br.IntegerHeight() + 1));
	mUpdateArea->SetTo(0,0,0,0);
	int numrects = breg->CountRects();
	for (int i = 0; i< numrects; i++)
	{
		BRect br = breg->RectAt(i);
		mUpdateArea->Union(int(br.left), int(br.top), 
							br.IntegerWidth() + 1, br.IntegerHeight() + 1);
	}	

	nsIRenderingContext* rc = GetRenderingContext();
	
#ifdef MOZ_CAIRO_GFX
	nsRefPtr<gfxContext> ctx =
		(gfxContext*)rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
	ctx->Save();

	
	ctx->NewPath();
	for (int i = 0; i< numrects; i++)
	{
		BRect br = breg->RectAt(i);
		ctx->Rectangle(gfxRect(int(br.left), int(br.top), 
			       br.IntegerWidth() + 1, br.IntegerHeight() + 1));
	}
	ctx->Clip();

	
	ctx->PushGroup(gfxContext::CONTENT_COLOR);
#endif

	nsPaintEvent event(PR_TRUE, NS_PAINT, this);

	InitEvent(event);
	event.region = mUpdateArea;
	event.rect = &nsr;
	event.renderingContext = rc;
	if (event.renderingContext != nsnull)
	{
		
		
		
		
		
		
		rv = DispatchWindowEvent(&event) ? NS_OK : NS_ERROR_FAILURE;
		NS_RELEASE(event.renderingContext);
	}

	NS_RELEASE(event.widget);

#ifdef MOZ_CAIRO_GFX
	
	if (rv == NS_OK) {
		ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
		ctx->PopGroupToSource();
		ctx->Paint();
	} else {
		
		ctx->PopGroup();
	}

	ctx->Restore();
#endif

	return rv;
}







PRBool nsWindow::OnResize(nsRect &aWindowRect)
{
	
	if (mEventCallback)
	{
		nsSizeEvent event(PR_TRUE, NS_SIZE, this);
		InitEvent(event);
		event.windowSize = &aWindowRect;
		
		event.mWinWidth  = aWindowRect.width;
		event.mWinHeight = aWindowRect.height;
		PRBool result = DispatchWindowEvent(&event);
		NS_RELEASE(event.widget);
		return result;
	}
	return PR_FALSE;
}








PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType, nsPoint aPoint, PRUint32 clicks, PRUint32 mod,
                                    PRUint16 aButton)
{
	PRBool result = PR_FALSE;
	if (nsnull != mEventCallback || nsnull != mMouseListener)
	{
		nsMouseEvent event(PR_TRUE, aEventType, this, nsMouseEvent::eReal);
		InitEvent (event, &aPoint);
		event.isShift   = mod & B_SHIFT_KEY;
		event.isControl = mod & B_CONTROL_KEY;
		event.isAlt     = mod & B_COMMAND_KEY;
		event.isMeta     = mod & B_OPTION_KEY;
		event.clickCount = clicks;
		event.button = aButton;

		
		if (nsnull != mEventCallback)
		{
			result = DispatchWindowEvent(&event);
			NS_RELEASE(event.widget);
			return result;
		}
		else
		{
			switch(aEventType)
			{
			case NS_MOUSE_MOVE :
				result = ConvertStatus(mMouseListener->MouseMoved(event));
				break;

			case NS_MOUSE_BUTTON_DOWN :
				result = ConvertStatus(mMouseListener->MousePressed(event));
				break;

			case NS_MOUSE_BUTTON_UP :
				result = ConvertStatus(mMouseListener->MouseReleased(event)) && ConvertStatus(mMouseListener->MouseClicked(event));
				break;
			}
			NS_RELEASE(event.widget);
			return result;
		}
	}

	return PR_FALSE;
}






PRBool nsWindow::DispatchFocus(PRUint32 aEventType)
{
	
	if (mEventCallback)
		return(DispatchStandardEvent(aEventType));

	return PR_FALSE;
}

NS_METHOD nsWindow::SetTitle(const nsAString& aTitle)
{
	if (mView && mView->LockLooper())
	{
		mView->Window()->SetTitle(NS_ConvertUTF16toUTF8(aTitle).get());
		mView->UnlockLooper();
	}
	return NS_OK;
}






NS_METHOD nsWindow::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
	
	
	aWidth  = mPreferredWidth;
	aHeight = mPreferredHeight;
	return NS_ERROR_FAILURE;
}

NS_METHOD nsWindow::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
	mPreferredWidth  = aWidth;
	mPreferredHeight = aHeight;
	return NS_OK;
}




nsIWidgetStore::nsIWidgetStore( nsIWidget *aWidget )
		: mWidget( aWidget )
{
	
	
	
	
}

nsIWidgetStore::~nsIWidgetStore()
{
}

nsIWidget *nsIWidgetStore::GetMozillaWidget(void)
{
	return mWidget;
}





nsWindowBeOS::nsWindowBeOS( nsIWidget *aWidgetWindow, BRect aFrame, const char *aName, window_look aLook,
                            window_feel aFeel, int32 aFlags, int32 aWorkspace )
		: BWindow( aFrame, aName, aLook, aFeel, aFlags, aWorkspace ),
		nsIWidgetStore( aWidgetWindow )
{
	fJustGotBounds = true;
}

nsWindowBeOS::~nsWindowBeOS()
{
	
}

bool nsWindowBeOS::QuitRequested( void )
{
	if (CountChildren() != 0)
	{
		nsWindow	*w = (nsWindow *)GetMozillaWidget();
		nsToolkit	*t;
		if (w && (t = w->GetToolkit()) != 0)
		{
			MethodInfo *info = nsnull;
			if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::CLOSEWINDOW)))
				t->CallMethodAsync(info);
		}
	}
	return true;
}

void nsWindowBeOS::MessageReceived(BMessage *msg)
{
	
	if (msg->what == B_SIMPLE_DATA)
	{
		printf("BWindow::SIMPLE_DATA\n");
		be_app_messenger.SendMessage(msg);
	}
	BWindow::MessageReceived(msg);
}



void nsWindowBeOS::DispatchMessage(BMessage *msg, BHandler *handler)
{
	if (msg->what == B_KEY_DOWN && modifiers() & B_COMMAND_KEY)
	{
		BString bytes;
		if (B_OK == msg->FindString("bytes", &bytes))
		{
			BView *view = this->CurrentFocus();
			if (view)
				view->KeyDown(bytes.String(), bytes.Length());
		}
		if (strcmp(bytes.String(),"w") && strcmp(bytes.String(),"W"))
			BWindow::DispatchMessage(msg, handler);
	}
	
	
	else if(msg->what == B_QUIT_REQUESTED)
	{
		
		nsWindow	*w = (nsWindow *)GetMozillaWidget();
		nsToolkit	*t;
		if (w && (t = w->GetToolkit()) != 0)
		{
			MethodInfo *info = nsnull;
			if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::CLOSEWINDOW)))
				t->CallMethodAsync(info);
		}		
	}
	else
		BWindow::DispatchMessage(msg, handler);
}



void nsWindowBeOS::FrameMoved(BPoint origin)
{	

	
	if (origin.x == lastWindowPoint.x && origin.x == lastWindowPoint.x) 
	{
		
		return;
	}
	lastWindowPoint = origin;
	nsWindow  *w = (nsWindow *)GetMozillaWidget();
	nsToolkit *t;
	if (w && (t = w->GetToolkit()) != 0) 
	{
		MethodInfo *info = nsnull;
		if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONMOVE)))
			t->CallMethodAsync(info);
	}
}

void nsWindowBeOS::WindowActivated(bool active)
{

	nsWindow        *w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	if (w && (t = w->GetToolkit()) != 0)
	{
		uint32	args[2];
		args[0] = (uint32)active;
		args[1] = (uint32)this;
		MethodInfo *info = nsnull;
		if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONACTIVATE, 2, args)))
			t->CallMethodAsync(info);
	}
}

void  nsWindowBeOS::WorkspacesChanged(uint32 oldworkspace, uint32 newworkspace)
{
	if (oldworkspace == newworkspace)
		return;
	nsWindow        *w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	if (w && (t = w->GetToolkit()) != 0)
	{
		uint32	args[2];
		args[0] = newworkspace;
		args[1] = oldworkspace;
		MethodInfo *info = nsnull;
		if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONWORKSPACE, 2, args)))
			t->CallMethodAsync(info);
	}	
}

void  nsWindowBeOS::FrameResized(float width, float height)
{
	
	
	if (!fJustGotBounds)
		return;
	nsWindow        *w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	if (w && (t = w->GetToolkit()) != 0)
	{
		MethodInfo *info = nsnull;
		if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONRESIZE)))
		{
			
			if (t->CallMethodAsync(info))
				fJustGotBounds = false;
		}
	}	
}





nsViewBeOS::nsViewBeOS(nsIWidget *aWidgetWindow, BRect aFrame, const char *aName, uint32 aResizingMode, uint32 aFlags)
	: BView(aFrame, aName, aResizingMode, aFlags), nsIWidgetStore(aWidgetWindow), wheel(.0,.0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	paintregion.MakeEmpty();	
	buttons = 0;
	fRestoreMouseMask = false;
	fJustValidated = true;
	fWheelDispatched = true;
	fVisible = true;
}

void nsViewBeOS::SetVisible(bool visible)
{
	if (visible)
		SetFlags(Flags() | B_WILL_DRAW);
	else
		SetFlags(Flags() & ~B_WILL_DRAW);
	fVisible = visible;
}

inline bool nsViewBeOS::Visible()
{
	return fVisible;
}
 
void nsViewBeOS::Draw(BRect updateRect)
{
	
	if (!fVisible)
		return;

	paintregion.Include(updateRect);

	
	
	
	if (paintregion.CountRects() == 0 || !paintregion.Frame().IsValid() || !fJustValidated)
		return;
	uint32	args[1];
	args[0] = (uint32)this;
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	if (w && (t = w->GetToolkit()) != 0)
	{
		MethodInfo *info = nsnull;
		info = new MethodInfo(w, w, nsSwitchToUIThread::ONPAINT, 1, args);
		if (info)
		{
			
			if (t->CallMethodAsync(info))
				fJustValidated = false;
		}
	}
}


bool nsViewBeOS::GetPaintRegion(BRegion *r)
{

	
	
	fJustValidated = true;
	if (paintregion.CountRects() == 0)
		return false;
	r->Include(&paintregion);
	return true;
}


void nsViewBeOS::Validate(BRegion *reg)
{
	paintregion.Exclude(reg);
}

BPoint nsViewBeOS::GetWheel()
{
	BPoint retvalue = wheel;
	
	fWheelDispatched = true;
	wheel.x = 0;
	wheel.y = 0;
	return retvalue;
}

void nsViewBeOS::MouseDown(BPoint point)
{
	if (!fRestoreMouseMask)
		mouseMask = SetMouseEventMask(B_POINTER_EVENTS);
	fRestoreMouseMask = true;
	
	
	mousePos = point;

	uint32 clicks = 0;
	BMessage *msg = Window()->CurrentMessage();
	msg->FindInt32("buttons", (int32 *) &buttons);
	msg->FindInt32("clicks", (int32 *) &clicks);

	if (0 == buttons)
		return;

	nsWindow	*w = (nsWindow *) GetMozillaWidget();
	if (w == NULL)
		return;
		
	nsToolkit	*t = w->GetToolkit();
	if (t == NULL)
		return;

	PRUint16 eventButton =
	  (buttons & B_PRIMARY_MOUSE_BUTTON) ? nsMouseEvent::eLeftButton :
	    ((buttons & B_SECONDARY_MOUSE_BUTTON) ? nsMouseEvent::eRightButton :
	      nsMouseEvent::eMiddleButton);
	uint32	args[6];
	args[0] = NS_MOUSE_BUTTON_DOWN;
	args[1] = (uint32) point.x;
	args[2] = (uint32) point.y;
	args[3] = clicks;
	args[4] = modifiers();
	args[5] = eventButton;
	MethodInfo *info = nsnull;
	if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::BTNCLICK, 6, args)))
		t->CallMethodAsync(info);
}

void nsViewBeOS::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	
	
	if (mousePos == point && (transit == B_INSIDE_VIEW || transit == B_OUTSIDE_VIEW))
		return;

	mousePos = point;
		
	
	if (NULL == msg && !fRestoreMouseMask && buttons)
		return;
		
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	if (w == NULL)
		return;
	nsToolkit	*t = w->GetToolkit();
	if (t == NULL)
		return;
	uint32	args[4];
	args[1] = (int32) point.x;
	args[2] = (int32) point.y;
	args[3] = modifiers();

	switch (transit)
 	{
 	case B_ENTERED_VIEW:
		{
			args[0] = NULL != msg ? NS_DRAGDROP_ENTER : NS_MOUSE_ENTER;
			if (msg == NULL)
				break;
			nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
			dragService->StartDragSession();
			
			nsCOMPtr<nsIDragSessionBeOS> dragSessionBeOS = do_QueryInterface(dragService);
			dragSessionBeOS->UpdateDragMessageIfNeeded(new BMessage(*msg));
		}
		break;
	case B_EXITED_VIEW:
		{
			args[0] = NULL != msg ? NS_DRAGDROP_EXIT : NS_MOUSE_EXIT;
			if (msg == NULL)
				break;
			nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
			dragService->EndDragSession(PR_FALSE);
		}
		break;
	default:
		args[0]= msg == NULL ? NS_MOUSE_MOVE : NS_DRAGDROP_OVER;
        
        if (msg != NULL) {
			nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
			dragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);
        }
 	}
 	
	MethodInfo *moveInfo = nsnull;
	if (nsnull != (moveInfo = new MethodInfo(w, w, nsSwitchToUIThread::ONMOUSE, 4, args)))
		t->CallMethodAsync(moveInfo);
}

void nsViewBeOS::MouseUp(BPoint point)
{
	if (fRestoreMouseMask) 
	{
		SetMouseEventMask(mouseMask);
		fRestoreMouseMask = false;
	}
	
	
	mousePos = point;

	PRUint16 eventButton =
	  (buttons & B_PRIMARY_MOUSE_BUTTON) ? nsMouseEvent::eLeftButton :
	    ((buttons & B_SECONDARY_MOUSE_BUTTON) ? nsMouseEvent::eRightButton :
	      nsMouseEvent::eMiddleButton);
	
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	if (w == NULL)
		return;
	nsToolkit	*t = w->GetToolkit();
	if (t == NULL)
		return;


	uint32	args[6];
	args[0] = NS_MOUSE_BUTTON_UP;
	args[1] = (uint32) point.x;
	args[2] = (int32) point.y;
	args[3] = 0;
	args[4] = modifiers();
	args[5] = eventButton;
	MethodInfo *info = nsnull;
	if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::BTNCLICK, 6, args)))
		t->CallMethodAsync(info);
}

void nsViewBeOS::MessageReceived(BMessage *msg)
{
	if(msg->WasDropped())
	{
		nsWindow	*w = (nsWindow *)GetMozillaWidget();
		if (w == NULL)
			return;
		nsToolkit	*t = w->GetToolkit();
		if (t == NULL)
			return;

		uint32	args[4];
		args[0] = NS_DRAGDROP_DROP;

		
		BPoint aPoint = ConvertFromScreen(msg->DropPoint()); 
	
		args[1] = (uint32) aPoint.x;
		args[2] = (uint32) aPoint.y;
		args[3] = modifiers();

		MethodInfo *info = new MethodInfo(w, w, nsSwitchToUIThread::ONDROP, 4, args);
		t->CallMethodAsync(info);
		BView::MessageReceived(msg);
		return;
	}

	switch(msg->what)
	{
	
	case B_COPY_TARGET:
	case B_MOVE_TARGET:
	case B_LINK_TARGET:
	case B_TRASH_TARGET:
		{
			nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
			nsCOMPtr<nsIDragSessionBeOS> dragSessionBeOS = do_QueryInterface(dragService);
			dragSessionBeOS->TransmitData(new BMessage(*msg));
		}
		break;
	case B_UNMAPPED_KEY_DOWN:
		
		KeyDown(NULL, 0);
		break;

	case B_UNMAPPED_KEY_UP:
		
		KeyUp(NULL, 0);
		break;

	case B_MOUSE_WHEEL_CHANGED:
		{
			float wheel_y;
			float wheel_x;

			msg->FindFloat ("be:wheel_delta_y", &wheel_y);
			msg->FindFloat ("be:wheel_delta_x", &wheel_x);
			wheel.x += wheel_x;
			wheel.y += wheel_y;

			if(!fWheelDispatched || (nscoord(wheel_x) == 0 && nscoord(wheel_y) == 0))
				return;
			uint32	args[1];
			args[0] = (uint32)this;
			nsWindow    *w = (nsWindow *)GetMozillaWidget();
			nsToolkit   *t;

			if (w && (t = w->GetToolkit()) != 0)
			{
					
				MethodInfo *info = nsnull;
				if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONWHEEL, 1, args)))
				{
					if (t->CallMethodAsync(info))
						fWheelDispatched = false;
					
				}
			}
		}
		break;
		
#if defined(BeIME)
	case B_INPUT_METHOD_EVENT:
		DoIME(msg);
		break;
#endif
	default :
		BView::MessageReceived(msg);
		break;
	}
}

void nsViewBeOS::KeyDown(const char *bytes, int32 numBytes)
{
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	int32 keycode = 0;
	int32 rawcode = 0;

	BMessage *msg = this->Window()->CurrentMessage();
	if (msg)
	{
		msg->FindInt32("key", &keycode);
		msg->FindInt32("raw_char", &rawcode);
	}

	if (w && (t = w->GetToolkit()) != 0)
	{
		uint32 bytebuf = 0;
		uint8 *byteptr = (uint8 *)&bytebuf;
		for(int32 i = 0; i < numBytes; i++)
			byteptr[i] = bytes[i];

		uint32	args[6];
		args[0] = NS_KEY_DOWN;
		args[1] = bytebuf;
		args[2] = numBytes;
		args[3] = modifiers();
		args[4] = keycode;
		args[5] = rawcode;
		MethodInfo *info = nsnull;
		if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONKEY, 6, args)))
			t->CallMethodAsync(info);
	}
}

void nsViewBeOS::KeyUp(const char *bytes, int32 numBytes)
{
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	int32 keycode = 0;
	int32 rawcode = 0;
	BMessage *msg = this->Window()->CurrentMessage();
	if (msg)
	{
		msg->FindInt32("key", &keycode);
		msg->FindInt32("raw_char", &rawcode);
	}

	if (w && (t = w->GetToolkit()) != 0)
	{
		uint32 bytebuf = 0;
		uint8 *byteptr = (uint8 *)&bytebuf;
		for(int32 i = 0; i < numBytes; i++)
			byteptr[i] = bytes[i];

		uint32	args[6];
		args[0] = NS_KEY_UP;
		args[1] = (int32)bytebuf;
		args[2] = numBytes;
		args[3] = modifiers();
		args[4] = keycode;
		args[5] = rawcode;
		MethodInfo *info = nsnull;
		if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::ONKEY, 6, args)))
			t->CallMethodAsync(info);
	}
}

void nsViewBeOS::MakeFocus(bool focused)
{
	if (!IsFocus() && focused)
		BView::MakeFocus(focused);
	uint32	args[1];
	args[0] = (uint32)this;
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;
	if (w && (t = w->GetToolkit()) != 0)
	{
		MethodInfo *info = nsnull;
		if (!focused)
		{
			if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::KILL_FOCUS, 1, args)))
				t->CallMethodAsync(info);
		}
#ifdef DEBUG_FOCUS
		else
		{
			if (nsnull != (info = new MethodInfo(w, w, nsSwitchToUIThread::GOT_FOCUS, 1, args)))
				t->CallMethodAsync(info);
		}
#endif		
	}
}

#if defined(BeIME)

void nsViewBeOS::DoIME(BMessage *msg)
{
	nsWindow	*w = (nsWindow *)GetMozillaWidget();
	nsToolkit	*t;

	if(w && (t = w->GetToolkit()) != 0) 
	{
		ssize_t size = msg->FlattenedSize();
		int32		argc = (size+3)/4;
		uint32 *args = new uint32[argc];
		if (args) 
		{
			msg->Flatten((char*)args, size);
			MethodInfo *info = new MethodInfo(w, w, nsSwitchToUIThread::ONIME, argc, args);
			if (info) 
			{
				t->CallMethodAsync(info);
				NS_RELEASE(t);
			}
			delete[] args;
		}	
	}
}
#endif
