











































#ifndef CALL_NOT_IN_CARBON
	#define CALL_NOT_IN_CARBON 1
#endif

#include "nsIJVMConsole.h"
#include "nsIEventHandler.h"
#include "SupportsMixin.h"

#include "jni.h"
#include <JManager.h>

class MRJPlugin;
class MRJSession;
class TopLevelFrame;

class MRJConsole :	public nsIJVMConsole,
					public nsIEventHandler,
					public SupportsMixin {
public:
	MRJConsole(MRJPlugin* plugin);
	virtual ~MRJConsole();

	
	
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
	NS_IMETHOD_(nsrefcnt) AddRef(void);
	NS_IMETHOD_(nsrefcnt) Release(void);

	

    NS_IMETHOD
    Show(void);

    NS_IMETHOD
    Hide(void);

    NS_IMETHOD
    IsVisible(PRBool* isVisible);

    
    
    
    NS_IMETHOD
    Print(const char* msg, const char* encodingName = NULL);

    NS_IMETHOD
    HandleEvent(nsPluginEvent* event, PRBool* eventHandled);
    
    

	void setFrame(TopLevelFrame* frame) { mFrame = frame; }
	
	void write(const void *message, UInt32 messageLengthInBytes);

private:
	
	OSStatus CallConsoleMethod(jmethodID method);
	OSStatus CallConsoleMethod(jmethodID method, jobject arg);

	void Initialize();
	
private:
	MRJPlugin* mPlugin;
	MRJSession* mSession;
	PRBool mIsInitialized;

	jclass mConsoleClass;
	jmethodID mInitMethod;
	jmethodID mDisposeMethod;
	jmethodID mShowMethod;
	jmethodID mHideMethod;
	jmethodID mVisibleMethod;
	jmethodID mPrintMethod;
	jmethodID mFinishMethod;

	jbooleanArray mResults;

	JMAWTContextRef mContext;
	TopLevelFrame* mFrame;

	
	static const InterfaceInfo sInterfaces[];
	static const UInt32 kInterfaceCount;
};
