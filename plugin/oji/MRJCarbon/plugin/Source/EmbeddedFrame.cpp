













































#include <Controls.h>
#include <Events.h>

#include "EmbeddedFrame.h"
#include "EmbeddedFramePluginInstance.h"
#include "MRJPlugin.h"
#include "MRJSession.h"

#include "nsIPluginInstancePeer.h"
#include "nsIOutputStream.h"
#include "JSEvaluator.h"
#include "LocalPort.h"
#include "StringUtils.h"

static void UnsetPort(GrafPtr port);

EmbeddedFrame::EmbeddedFrame(MRJPluginInstance* pluginInstance, JMFrameRef frameRef, JMFrameKind kind,
							const Rect* initialBounds, Boolean resizeable)
	:	MRJFrame(frameRef),
		mPluginInstance(NULL), mEvaluator(NULL), mWindow(NULL), mBounds(*initialBounds)
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

#if 0
	
	
	
	

	class NewStreamMessage : public NativeMessage {
		nsIPluginInstancePeer* mPeer;
		const char* mType;
	public:
		NewStreamMessage(nsIPluginInstancePeer* peer, const char* type) : mPeer(peer), mType(type) {}
		
		virtual void execute() {
			nsIOutputStream* output = NULL;
			if (mPeer->NewStream(mType, "_new", &output) == NS_OK) {
				
				output->Close();
				NS_RELEASE(output);
			}
		}
	};

	
	
	nsIPluginInstancePeer* peer = NULL;
	if (pluginInstance->GetPeer(&peer) == NS_OK) {
		NewStreamMessage msg(peer, "application/x-java-frame");
		pluginInstance->getSession()->sendMessage(&msg);
		NS_RELEASE(peer);
	}

#else
	

	static UInt32 embeddedFrameCounter = 0;

	
	const char* kEmbeddedFrameScript = "var w = window.open('','__MRJ_JAVA_FRAME_%d__','resizable=no,status=no,width=%d,height=%d,screenX=%d,screenY=%d');"
	                                   "var d = w.document; d.open();"
	                                   "d.writeln('<BODY BGCOLOR=#FFFFFF MARGINWIDTH=0 MARGINHEIGHT=0>&nbsp;<EMBED TYPE=application/x-java-frame WIDTH=%d HEIGHT=%d JAVAFRAME=%08X>');"
	                                   "d.close();";

	int width = mBounds.right - mBounds.left;
	int height = mBounds.bottom - mBounds.top;
	int screenX = mBounds.left;
	int screenY = mBounds.top;

	char* script = new char[::strlen(kEmbeddedFrameScript) + 100];
	::sprintf(script, kEmbeddedFrameScript, ++embeddedFrameCounter, width, height, screenX, screenY, width, height, this);

	JSEvaluator* evaluator = new JSEvaluator(pluginInstance);
	evaluator->AddRef();
	
	
	const char* result = evaluator->eval(script);
	
	evaluator->Release();
	
	delete[] script;

#endif

	if (mWindow != NULL) {
		Point zeroPt = { 0, 0 };
		::JMSetFrameVisibility(mFrameRef, mWindow, zeroPt, NULL);
	}
}

EmbeddedFrame::~EmbeddedFrame()
{
	if (mPluginInstance != NULL)
		mPluginInstance->setFrame(NULL);

	
	showHide(false);

	
	::UnsetPort(mWindow);

	
	
}

void EmbeddedFrame::setSize(const Rect* newSize)
{
	mBounds = *newSize;

	if (mWindow != NULL) {
		SInt16 width = newSize->right - newSize->left;
		SInt16 height = newSize->bottom - newSize->top;
		::SizeWindow(mWindow, width, height, true);
		::MoveWindow(mWindow, newSize->left, newSize->top, false);
	}
}

void EmbeddedFrame::invalRect(const Rect* invalidRect)
{
	if (mWindow != NULL) {
		::InvalRect(invalidRect);
	}
}

void EmbeddedFrame::showHide(Boolean visible)
{
	if (mWindow != NULL && visible != IsWindowVisible(mWindow)) {
		if (visible) {
			
			::ShowWindow(mWindow);
			::SelectWindow(mWindow);
		} else {
			::HideWindow(mWindow);
		}
		
		
	}
}

void EmbeddedFrame::setTitle(const StringPtr title)
{
	if (mWindow != NULL) {
		::SetWTitle(mWindow, title);
	}
}

void EmbeddedFrame::checkUpdate()
{
}

void EmbeddedFrame::reorder(ReorderRequest request)
{
	switch (request) {
	case eBringToFront:		
		break;
	case eSendToBack:		
		break;
	case eSendBehindFront:	
		break;
	}
}

void EmbeddedFrame::setResizeable(Boolean resizeable)
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

void EmbeddedFrame::activate(Boolean active)
{
	focusEvent(active);
	MRJFrame::activate(active);
}

void EmbeddedFrame::click(const EventRecord* event)
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
		Rect bounds = (**GetGrayRgn()).rgnBBox;
		DragWindow(mWindow, where, &bounds);
		computeBounds(mWindow, &mBounds);
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

void EmbeddedFrame::setPluginInstance(EmbeddedFramePluginInstance* embeddedInstance)
{
	mPluginInstance = embeddedInstance;
}

void EmbeddedFrame::setWindow(WindowRef window)
{
	mWindow = window;
}

WindowRef EmbeddedFrame::getWindow()
{
	return mWindow;
}

GrafPtr EmbeddedFrame::getPort()
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
