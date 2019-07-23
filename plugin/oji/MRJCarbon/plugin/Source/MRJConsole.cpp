












































#include <Appearance.h>
#include <Gestalt.h>

#include <string.h>

#include "MRJConsole.h"
#include "MRJPlugin.h"
#include "MRJSession.h"
#include "TopLevelFrame.h"

#include "nsIPluginManager2.h"

extern nsIPluginManager2* thePluginManager2;

MRJConsole* theConsole = NULL;

const InterfaceInfo MRJConsole::sInterfaces[] = {
	{ NS_IJVMCONSOLE_IID, INTERFACE_OFFSET(MRJConsole, nsIJVMConsole) },
	{ NS_IEVENTHANDLER_IID, INTERFACE_OFFSET(MRJConsole, nsIEventHandler) },
};
const UInt32 MRJConsole::kInterfaceCount = sizeof(sInterfaces) / sizeof(InterfaceInfo);

MRJConsole::MRJConsole(MRJPlugin* plugin)
	:	SupportsMixin(this, sInterfaces, kInterfaceCount, (nsIPlugin*) plugin),
		mPlugin(plugin), mSession(NULL), mIsInitialized(PR_FALSE),
		mConsoleClass(NULL), mInitMethod(NULL), mDisposeMethod(NULL),
		mShowMethod(NULL), mHideMethod(NULL), mVisibleMethod(NULL), mPrintMethod(NULL), mFinishMethod(NULL),
		mResults(NULL), mContext(NULL), mFrame(NULL)
{
	
}

MRJConsole::~MRJConsole()
{
	theConsole = NULL;

	if (mSession != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		
		if (mConsoleClass != NULL) {
			if (mDisposeMethod != NULL)
				env->CallStaticVoidMethod(mConsoleClass, mDisposeMethod);
			env->DeleteGlobalRef(mConsoleClass);
			mConsoleClass = NULL;
		}
		
		if (mResults != NULL) {
			env->DeleteGlobalRef(mResults);
			mResults = NULL;
		}

#if !TARGET_CARBON		
		if (mContext != NULL) {
			JMDisposeAWTContext(mContext);
			mContext = NULL;
		}
#endif
	}
}

NS_METHOD MRJConsole::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
	nsresult result = queryInterface(aIID, aInstancePtr);
	if (result == NS_NOINTERFACE)
		result = mPlugin->queryInterface(aIID, aInstancePtr);
	return result;
}
nsrefcnt MRJConsole::AddRef() { return addRef(); }
nsrefcnt MRJConsole::Release() { return release(); }

#pragma mark ***** MRJConsole *****

NS_METHOD MRJConsole::Show()
{
	Initialize();

	if (mShowMethod != NULL) {
		CallConsoleMethod(mShowMethod);
		return NS_OK;
	}
	
	return NS_ERROR_FAILURE;
}

NS_METHOD MRJConsole::Hide()
{
	Initialize();

	if (mHideMethod != NULL) {
		CallConsoleMethod(mHideMethod);
		return NS_OK;
	}

	return NS_ERROR_FAILURE;
}

NS_METHOD MRJConsole::IsVisible(PRBool* isVisible)
{
	
	*isVisible = PR_FALSE;
	if (mVisibleMethod != NULL) {
		CallConsoleMethod(mVisibleMethod, mResults);
		jboolean isCopy;
		JNIEnv* env = mSession->getCurrentEnv();
		jboolean* elements = env->GetBooleanArrayElements(mResults, &isCopy);
		*isVisible = elements[0];
		env->ReleaseBooleanArrayElements(mResults, elements, JNI_ABORT);
	}
	return NS_OK;
}





NS_METHOD MRJConsole::Print(const char* msg, const char* encodingName)
{
	Initialize();

	if (mPrintMethod != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		jstring jmsg = env->NewStringUTF(msg);
		jvalue args[1]; args[0].l = jmsg;
#if TARGET_CARBON
        
#else
		JMExecJNIStaticMethodInContext(mContext, env, mConsoleClass, mPrintMethod, 1, args);
#endif
		env->DeleteLocalRef(jmsg);
		return NS_OK;
	}
	
	return NS_ERROR_FAILURE;
}



void MRJConsole::write(const void *message, UInt32 messageLengthInBytes)
{
	if (mPrintMethod != NULL) {
		char* buffer = new char[messageLengthInBytes + 1];
		if (buffer != NULL) {
			memcpy(buffer, message, messageLengthInBytes);
			buffer[messageLengthInBytes] = '\0';

			JNIEnv* env = mSession->getCurrentEnv();
			jstring jmsg = env->NewStringUTF(buffer);
			if (jmsg != NULL) {
				env->CallStaticVoidMethod(mConsoleClass, mPrintMethod, jmsg);
				env->DeleteLocalRef(jmsg);
			}

			delete buffer;
		}
	}
}

NS_METHOD MRJConsole::HandleEvent(nsPluginEvent* pluginEvent, PRBool* eventHandled)
{
	*eventHandled = PR_TRUE;

	if (pluginEvent != NULL) {
		EventRecord* event = pluginEvent->event;

		if (event->what == nullEvent) {
			
			MRJSession* session = mPlugin->getSession();
			session->idle(kDefaultJMTime);
		} else {
			TopLevelFrame* frame = mFrame;
			if (frame != NULL) {
				switch (event->what) {
				case nsPluginEventType_GetFocusEvent:
					frame->focusEvent(true);
					break;
				
				case nsPluginEventType_LoseFocusEvent:
					frame->focusEvent(false);
					break;

				case nsPluginEventType_AdjustCursorEvent:
					frame->idle(event->modifiers);
					break;
				
				case nsPluginEventType_MenuCommandEvent:
					frame->menuSelected(event->message, event->modifiers);
					break;
				
				default:
					*eventHandled = frame->handleEvent(event);
					break;
				}
			}
		}
	}
	
	return NS_OK;
}

OSStatus MRJConsole::CallConsoleMethod(jmethodID method)
{
	JNIEnv* env = mSession->getCurrentEnv();
#if TARGET_CARBON
    
    env->CallStaticVoidMethod(mConsoleClass, method);
    return noErr;
#else
	OSStatus status = JMExecJNIStaticMethodInContext(mContext, env, mConsoleClass, method, 0, NULL);
	env->CallStaticVoidMethod(mConsoleClass, mFinishMethod);
	return status;
#endif
}

OSStatus MRJConsole::CallConsoleMethod(jmethodID method, jobject arg)
{
	JNIEnv* env = mSession->getCurrentEnv();
#if TARGET_CARBON
    
    env->CallStaticVoidMethod(mConsoleClass, method, arg);
    return noErr;
#else
	jvalue args[1];
	args[0].l = arg;
	OSStatus status = JMExecJNIStaticMethodInContext(mContext, env, mConsoleClass, method, 1, args);
	env->CallStaticVoidMethod(mConsoleClass, mFinishMethod);
	return status;
#endif
}

#if !TARGET_CARBON

static OSStatus requestFrame(JMAWTContextRef contextRef, JMFrameRef frameRef, JMFrameKind kind,
									const Rect* initialBounds, Boolean resizeable, JMFrameCallbacks* cb)
{
	extern JMFrameCallbacks theFrameCallbacks;
	
	BlockMoveData(&theFrameCallbacks, cb, sizeof(theFrameCallbacks));

	MRJConsole* thisConsole = NULL;
	OSStatus status = ::JMGetAWTContextData(contextRef, (JMClientData*)&thisConsole);
	if (status == noErr && thePluginManager2 != NULL) {
		
		TopLevelFrame* frame = new TopLevelFrame(thisConsole, frameRef, kind, initialBounds, resizeable);
		status = ::JMSetFrameData(frameRef, frame);
		thisConsole->setFrame(frame);
	}
	return status;
}

static OSStatus releaseFrame(JMAWTContextRef contextRef, JMFrameRef frameRef)
{
	MRJConsole* thisConsole = NULL;
	OSStatus status = ::JMGetAWTContextData(contextRef, (JMClientData*)&thisConsole);
	MRJFrame* thisFrame = NULL;
	status = ::JMGetFrameData(frameRef, (JMClientData*)&thisFrame);
	if (thisFrame != NULL) {
		status = ::JMSetFrameData(frameRef, NULL);
		thisConsole->setFrame(NULL);
		delete thisFrame;
	}
	return status;
}

static SInt16 getUniqueMenuID(JMAWTContextRef contextRef, Boolean isSubmenu)
{
	MRJConsole* thisConsole = NULL;
	OSStatus status = ::JMGetAWTContextData(contextRef, (JMClientData*)&thisConsole);
	if (thePluginManager2 != NULL) {
		PRInt16 menuID;
		if (thePluginManager2->AllocateMenuID(thisConsole, isSubmenu, &menuID) == NS_OK)
			return menuID;
	}
	return -1;
}

static Boolean appearanceManagerExists()
{
	long response = 0;
	return (Gestalt(gestaltAppearanceAttr, &response) == noErr && (response & (1 << gestaltAppearanceExists)));
}

static OSStatus JMTextToStr255(JMTextRef textRef, Str255 str)
{
	UInt32 length = 0;
	OSStatus status = JMGetTextBytes(textRef, kTextEncodingMacRoman, &str[1], sizeof(Str255) - 1, &length);
	if (status == noErr)
		str[0] = (unsigned char)(status == noErr ? length : 0);
	return status;
}

inline int min(int x, int y) { return (x <= y ? x : y); }

static void push(StringPtr dest, StringPtr str)
{
	int length = dest[0];
	int newLength = min(255, length + str[0]);
	if (newLength > length)
		::BlockMoveData(&str[1], &dest[1] + length, newLength - length);
}

static void exceptionOccurred(JMAWTContextRef context, JMTextRef exceptionName, JMTextRef exceptionMsg, JMTextRef stackTrace)
{
	
	if (appearanceManagerExists()) {
		OSStatus status;
		Str255 preason = { '\0' }, pmessage = { '\0'}, ptrace = { '\0' };
		if (exceptionName != NULL) {
			status = ::JMTextToStr255(exceptionName, preason);
			if (exceptionMsg != NULL) {
				status = ::JMTextToStr255(exceptionMsg, pmessage);
				push(preason, "\p (");
				push(preason, pmessage);
				push(preason, "\p)");
			}
		}
		
		if (stackTrace != NULL)
			status = ::JMTextToStr255(stackTrace, ptrace);
		
		SInt16 itemHit = 0;
		OSErr result = ::StandardAlert(kAlertPlainAlert, preason, ptrace, NULL, &itemHit);
	}
}

#endif

void MRJConsole::Initialize()
{
	if (mIsInitialized)
		return;
	
	mSession = mPlugin->getSession();
	JNIEnv* env = mSession->getCurrentEnv();
	
	jclass consoleClass = env->FindClass("netscape/oji/MRJConsole");
	if (consoleClass == NULL) return;
	mConsoleClass = (jclass) env->NewGlobalRef(consoleClass);
	
	mInitMethod = env->GetStaticMethodID(consoleClass, "init", "()V");
	mDisposeMethod = env->GetStaticMethodID(consoleClass, "dispose", "()V");
	mShowMethod = env->GetStaticMethodID(consoleClass, "show", "()V");
	mHideMethod = env->GetStaticMethodID(consoleClass, "hide", "()V");
	mVisibleMethod = env->GetStaticMethodID(consoleClass, "visible", "([Z)V");
	mPrintMethod = env->GetStaticMethodID(consoleClass, "print", "(Ljava/lang/String;)V");
	mFinishMethod = env->GetStaticMethodID(consoleClass, "finish", "()V");
	env->DeleteLocalRef(consoleClass);
	
	jbooleanArray results = env->NewBooleanArray(1);
	mResults = (jbooleanArray) env->NewGlobalRef(results);
	env->DeleteLocalRef(results);

    OSStatus status = noErr;

#if !TARGET_CARBON	
	
	JMAWTContextCallbacks callbacks = {
		kJMVersion,						
		&requestFrame, 					
		&releaseFrame,					
		&getUniqueMenuID, 				
		&exceptionOccurred,				
	};
	status = ::JMNewAWTContext(&mContext, mSession->getSessionRef(), &callbacks, this);
#endif

	

	if (mInitMethod != NULL && status == noErr)
	    status = CallConsoleMethod(mInitMethod);

	mIsInitialized = (status == noErr);

	if (mIsInitialized)
		theConsole = this;
}
