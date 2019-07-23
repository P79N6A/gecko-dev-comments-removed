












































#pragma once

#include "jni.h"
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

#if !TARGET_CARBON
	JMAWTContextRef getContextRef();
    JMAppletViewerRef getViewerRef();
#endif
    
	Boolean appletLoaded();
	Boolean loadApplet();
	Boolean isActive();
	
	void suspendApplet();
	void resumeApplet();
	jobject getApplet();

	nsIPluginInstance* getInstance();
	nsIPluginInstancePeer* getPeer();
	
	Boolean handleEvent(EventRecord* event);
	
	void idle(short modifiers);
	void drawApplet();
	void printApplet(nsPluginWindow* printingWindow);
	
	void activate(Boolean active);
	void resume(Boolean inFront);

	void click(const EventRecord* event, MRJFrame* frame);
	void keyPress(long message, short modifiers);
	void keyRelease(long message, short modifiers);
	
    void scrollingBegins();
    void scrollingEnds();

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

	void showURL(const char* url, const char* target);
	
private:
	void localToFrame(Point* pt);
	void ensureValidPort();
	void synchronizeClipping();
	void synchronizeVisibility();

#if !TARGET_CARBON
    static OSStatus requestFrame(JMAWTContextRef context, JMFrameRef newFrame, JMFrameKind kind,
								const Rect *initialBounds, Boolean resizeable, JMFrameCallbacks *callbacks);
	static OSStatus releaseFrame(JMAWTContextRef context, JMFrameRef oldFrame);
	static SInt16 getUniqueMenuID(JMAWTContextRef context, Boolean isSubmenu);
	static void exceptionOccurred(JMAWTContextRef context, JMTextRef exceptionName, JMTextRef exceptionMsg, JMTextRef stackTrace);
    
	SInt16 allocateMenuID(Boolean isSubmenu);

	OSStatus createFrame(JMFrameRef frameRef, JMFrameKind kind, const Rect* initialBounds, Boolean resizeable);
#endif

	
	MRJPage* findPage(const MRJPageAttributes& attributes);

	static CGrafPtr getEmptyPort();

#if !TARGET_CARBON
	void setProxyInfoForURL(char * url, JMProxyType proxyType);
#endif
    
	OSStatus installEventHandlers(WindowRef window);
	OSStatus removeEventHandlers(WindowRef window);
	
private:
	MRJPluginInstance*		    mPluginInstance;
	MRJSession*				    mSession;
	nsIPluginInstancePeer* 	    mPeer;
#if !TARGET_CARBON
    JMAppletLocatorRef		    mLocator;
	JMAWTContextRef			    mContext;
	JMAppletViewerRef		    mViewer;
	JMFrameRef				    mViewerFrame;
#endif
    Boolean					    mIsActive;
	Boolean                     mIsFocused;
	Boolean                     mIsVisible;
	nsPluginPoint               mCachedOrigin;
	nsPluginRect                mCachedClipRect;
	RgnHandle				    mPluginClipping;
	nsPluginWindow*			    mPluginWindow;
	CGrafPtr				    mPluginPort;
	char*					    mDocumentBase;
	char*					    mAppletHTML;
	MRJPage*				    mPage;
	MRJSecurityContext*         mSecurityContext;
#if TARGET_CARBON
    jobject                     mAppletFrame;
    jobject                     mAppletObject;
    ControlRef                  mAppletControl;
    UInt32                      mScrollCounter;
#endif
};
