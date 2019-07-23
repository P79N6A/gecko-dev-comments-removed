











































#pragma once

#include "jni.h"
#include "JManager.h"
#include "nsIPluginTagInfo2.h"









class MRJSession;
class MRJPluginInstance;
class nsIPluginInstancePeer;
struct nsPluginWindow;
class MRJFrame;
class MRJPage;
struct MRJPageAttributes;
class MRJSecurityContext;

struct nsPluginPoint {
	PRInt32             x;
    PRInt32             y;
};

class MRJContext {
public:
	MRJContext(MRJSession* session, MRJPluginInstance* instance);
	~MRJContext();

	void processAppletTag();
	Boolean createContext();
	JMAWTContextRef getContextRef();
    JMAppletViewerRef getViewerRef();
	
	void setProxyInfoForURL(char * url, JMProxyType proxyType);
	Boolean appletLoaded();
	Boolean loadApplet();
	Boolean isActive();
	
	void suspendApplet();
	void resumeApplet();
	
	jobject getApplet();
	
	void idle(short modifiers);
	void drawApplet();
	void printApplet(nsPluginWindow* printingWindow);
	
	void activate(Boolean active);
	void resume(Boolean inFront);

	void click(const EventRecord* event, MRJFrame* frame);
	void keyPress(long message, short modifiers);
	void keyRelease(long message, short modifiers);
	
	void setWindow(nsPluginWindow* pluginWindow);
	Boolean inspectWindow();
	
	MRJFrame* findFrame(WindowRef window);
	GrafPtr getPort();
	
	void showFrames();
	void hideFrames();
	void releaseFrames();
	
	void setDocumentBase(const char* documentBase);
	const char* getDocumentBase();
	
	void setAppletHTML(const char* appletHTML, nsPluginTagType tagType);
	const char* getAppletHTML();

    void setSecurityContext(MRJSecurityContext* context);
    MRJSecurityContext* getSecurityContext();

private:
	void localToFrame(Point* pt);
	void ensureValidPort();
	void synchronizeClipping();
	void synchronizeVisibility();

	static OSStatus requestFrame(JMAWTContextRef context, JMFrameRef newFrame, JMFrameKind kind,
								const Rect *initialBounds, Boolean resizeable, JMFrameCallbacks *callbacks);
	static OSStatus releaseFrame(JMAWTContextRef context, JMFrameRef oldFrame);
	static SInt16 getUniqueMenuID(JMAWTContextRef context, Boolean isSubmenu);
	static void exceptionOccurred(JMAWTContextRef context, JMTextRef exceptionName, JMTextRef exceptionMsg, JMTextRef stackTrace);

	static void showDocument(JMAppletViewerRef viewer, JMTextRef urlString, JMTextRef windowName);
	static void setStatusMessage(JMAppletViewerRef viewer, JMTextRef statusMsg);
	
	void showURL(const char* url, const char* target);
	void showStatus(const char* message);
	SInt16 allocateMenuID(Boolean isSubmenu);

	OSStatus createFrame(JMFrameRef frameRef, JMFrameKind kind, const Rect* initialBounds, Boolean resizeable);

	
	MRJPage* findPage(const MRJPageAttributes& attributes);

	static CGrafPtr getEmptyPort();

private:
	MRJPluginInstance*		mPluginInstance;
	MRJSession*				mSession;
	JMSessionRef 			mSessionRef;
	nsIPluginInstancePeer* 	mPeer;
	JMAppletLocatorRef		mLocator;
	JMAWTContextRef			mContext;
	JMAppletViewerRef		mViewer;
	JMFrameRef				mViewerFrame;
	Boolean					mIsActive;
	nsPluginPoint           mCachedOrigin;
	nsPluginRect            mCachedClipRect;
	RgnHandle				mPluginClipping;
	nsPluginWindow*			mPluginWindow;
	CGrafPtr				mPluginPort;
	char*					mDocumentBase;
	char*					mAppletHTML;
	MRJPage*				mPage;
	MRJSecurityContext*     mSecurityContext;
};
