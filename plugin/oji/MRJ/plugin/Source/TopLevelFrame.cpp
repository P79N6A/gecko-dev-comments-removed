












































#include <Controls.h>
#include <Events.h>

#include "TopLevelFrame.h"
#include "LocalPort.h"

#include "nsIPluginManager2.h"
#include "nsIEventHandler.h"

#if !defined(MRJPLUGIN_4X)
#define USE_ALT_WINDOW_HANDLING
#endif

#ifdef USE_ALT_WINDOW_HANDLING
#include "AltWindowHandling.h"
#endif

#include "nsIEventHandler.h"
#include "AltWindowHandling.h"

extern nsIPluginManager2* thePluginManager2;

static void UnsetPort(GrafPtr port);
static short getModifiers();

TopLevelFrame::TopLevelFrame(nsIEventHandler* handler, JMFrameRef frameRef, JMFrameKind kind,
							const Rect* initialBounds, Boolean resizeable)
	:	MRJFrame(frameRef),
		mHandler(handler), mWindow(NULL), mBounds(*initialBounds)
{
	Boolean hasGoAway = true;
	SInt16 windowProc = documentProc;
	SInt16 resizeHeight = resizeable ? 15 : 0;
	
	switch (kind) {
	case eBorderlessModelessWindowFrame:
		hasGoAway = false;
		windowProc = plainDBox;
		
		resizeable = false;
		break;
	case eModelessWindowFrame:
	case eModelessDialogFrame:
		hasGoAway = true;
		windowProc = resizeable ? zoomDocProc : documentProc;
		
		break;
	case eModalWindowFrame:
		hasGoAway = true;
		
		windowProc = resizeable ? documentProc : movableDBoxProc;
		break;
	}
	
	mWindow = ::NewCWindow(NULL, &mBounds, "\p", false, windowProc, WindowPtr(-1), hasGoAway, long(this));
	if (mWindow != NULL) {
		Point zeroPt = { 0, 0 };
		::JMSetFrameVisibility(frameRef, mWindow, zeroPt, mWindow->clipRgn);
	}
}

TopLevelFrame::~TopLevelFrame()
{
	
	showHide(false);

	
	::UnsetPort(mWindow);

	if (mWindow != NULL)
		::DisposeWindow(mWindow);
}

void TopLevelFrame::setSize(const Rect* newSize)
{
	mBounds = *newSize;

	if (mWindow != NULL) {
		SInt16 width = newSize->right - newSize->left;
		SInt16 height = newSize->bottom - newSize->top;
		::SizeWindow(mWindow, width, height, true);
		::MoveWindow(mWindow, newSize->left, newSize->top, false);
	}
}

void TopLevelFrame::invalRect(const Rect* invalidRect)
{
	if (mWindow != NULL) {
		::InvalRect(invalidRect);
	}
}

void TopLevelFrame::showHide(Boolean visible)
{
	if (mWindow != NULL && visible != IsWindowVisible(mWindow)) {
		if (visible) {
#if !defined(USE_ALT_WINDOW_HANDLING)		
			
			thePluginManager2->RegisterWindow(mHandler, mWindow);
			
			
			
#else
            AltRegisterWindow(mHandler, mWindow);
#endif
		} else {
#if defined(USE_ALT_WINDOW_HANDLING)		
            AltUnregisterWindow(mHandler, mWindow);
#else
 			
 			
 			
 			thePluginManager2->UnregisterWindow(mHandler, mWindow);
#endif
			activate(false);
		}
		
		
	}
}

void TopLevelFrame::setTitle(const StringPtr title)
{
	if (mWindow != NULL) {
		::SetWTitle(mWindow, title);
	}
}

void TopLevelFrame::checkUpdate()
{
}

void TopLevelFrame::reorder(ReorderRequest request)
{
	switch (request) {
	case eBringToFront:		
		::BringToFront(mWindow);
		break;
	case eSendToBack:		
		::SendBehind(mWindow, NULL);
		break;
	case eSendBehindFront:	
		WindowPtr frontWindow = ::FrontWindow();
		if (mWindow == frontWindow) {
			::SendBehind(mWindow, GetNextWindow(mWindow));
		} else {
			::SendBehind(mWindow, frontWindow);
		}
		break;
	}
}

void TopLevelFrame::setResizeable(Boolean resizeable)
{
	
}

static void computeBounds(WindowRef window, Rect* bounds)
{
	LocalPort port(window);
	port.Enter();
	
		Point position = { 0, 0 };
		::LocalToGlobal(&position);
		
		*bounds = window->portRect;
	
	port.Exit();
	
	::OffsetRect(bounds, position.h, position.v);
}

void TopLevelFrame::activate(Boolean active)
{
	focusEvent(active);
	MRJFrame::activate(active);
}

void TopLevelFrame::click(const EventRecord* event)
{
	Point where = event->where;
	SInt16 modifiers = event->modifiers;
	WindowRef hitWindow;
	short partCode = ::FindWindow(where, &hitWindow);
	switch (partCode) {
	case inContent:
		::SelectWindow(mWindow);
		MRJFrame::click(event);
		break;
	case inDrag:
		{
			Rect bounds = (**GetGrayRgn()).rgnBBox;
			DragWindow(mWindow, where, &bounds);
			computeBounds(mWindow, &bounds);
			::JMSetFrameSize(mFrameRef, &bounds);

			Point zeroPt = { 0, 0 };
			::JMSetFrameVisibility(mFrameRef, mWindow, zeroPt, mWindow->clipRgn);
		}
		break;
	case inGrow:
		Rect limits = { 30, 30, 5000, 5000 };
		long result = GrowWindow(mWindow, where, &limits);
		if (result != 0) {
			short width = (result & 0xFFFF);
			short height = (result >> 16) & 0xFFFF;
			Rect newBounds;
			topLeft(newBounds) = topLeft(mBounds);
			newBounds.right = newBounds.left + width;
			newBounds.bottom = newBounds.top + height;
			::JMSetFrameSize(mFrameRef, &newBounds);

			Point zeroPt = { 0, 0 };
			::JMSetFrameVisibility(mFrameRef, mWindow, zeroPt, mWindow->clipRgn);
		}
		break;
	case inGoAway:
		if (::TrackGoAway(mWindow, where))
			::JMFrameGoAway(mFrameRef);
		break;
	case inZoomIn:
	case inZoomOut:
		if (::TrackBox(mWindow, where, partCode)) {
			ZoomWindow(mWindow, partCode, true);
			computeBounds(mWindow, &mBounds);
			::JMSetFrameSize(mFrameRef, &mBounds);
		}
		break;
	case inCollapseBox:
		break;
	}
}

WindowRef TopLevelFrame::getWindow()
{
	return mWindow;
}

GrafPtr TopLevelFrame::getPort()
{
	return mWindow;
}

static void UnsetPort(GrafPtr port)
{
	GrafPtr curPort;
	::GetPort(&curPort);
	if (curPort == port) {
		::GetWMgrPort(&port);
		::SetPort(port);
	}
}

static short getModifiers()
{
	EventRecord event;
	::OSEventAvail(0, &event);
	return event.modifiers;
}
