












































#include "MRJSession.h"
#include "MRJPlugin.h"
#include "MRJContext.h"
#include "MRJConsole.h"
#include "MRJMonitor.h"
#include "TimedMessage.h"

#include <ControlDefinitions.h>
#include <string.h>
#include <Memory.h>
#include <Files.h>
#include <Dialogs.h>
#include <Appearance.h>
#include <Resources.h>
#include <Gestalt.h>
#include <Folders.h>
#include <Script.h>

#include <JavaControl.h>
#include <CFString.h>

#include <string>

extern MRJConsole* theConsole;
extern short thePluginRefnum;
extern FSSpec thePluginSpec;

static MRJSession* theSession;

#if REDIRECT_VFPRINTF














 



static void* NewMachOFunctionPointer(void* cfmfp)
{
#if TARGET_RT_MAC_CFM
    static UInt32 CFM_glue[6] = {0x3D800000, 0x618C0000, 0x800C0000, 0x804C0004, 0x7C0903A6, 0x4E800420};
    UInt32	*mfp = (UInt32*) NewPtr(sizeof(CFM_glue));		
    if (mfp) {
	    BlockMoveData(CFM_glue, mfp, sizeof(CFM_glue));
	    mfp[0] |= ((UInt32)cfmfp >> 16);
	    mfp[1] |= ((UInt32)cfmfp & 0xFFFF);
	    MakeDataExecutable(mfp, sizeof(CFM_glue));
	}
	return mfp;
#elif TARGET_RT_MAC_MACHO
    return cfmfp;
#endif
}

inline jobject ToGlobalRef(JNIEnv* env, jobject localRef)
{
    jobject globalRef = env->NewGlobalRef(localRef);
    env->DeleteLocalRef(localRef);
    return globalRef;
}

static jobject Get_System_out(JNIEnv* env)
{
    jclass java_lang_System = env->FindClass("java/lang/System");
    if (java_lang_System) {
        jfieldID outID = env->GetStaticFieldID(java_lang_System, "out", "Ljava/io/PrintStream;");
        jobject out = (outID ? env->GetStaticObjectField(java_lang_System, outID) : NULL);
        env->DeleteLocalRef(java_lang_System);
        return ToGlobalRef(env, out);
    }
    return NULL;
}

static jmethodID GetObjectMethodID(JNIEnv* env, jobject object, const char* name, const char* sig)
{
    jclass clazz = env->GetObjectClass(object);
    if (clazz) {
        jmethodID result = env->GetMethodID(clazz, name, sig);
        env->DeleteLocalRef(clazz);
        return result;
    }
    return NULL;
}

static void System_out_print(JNIEnv* env, const jchar* chars, jsize len)
{
    static jobject java_lang_System_out = Get_System_out(env);
    static jmethodID java_io_PrintStream_print = GetObjectMethodID(env, java_lang_System_out, "print", "([C)V");
    jcharArray array = env->NewCharArray(len);
    if (array) {
        env->SetCharArrayRegion(array, 0, len, (jchar*) chars);
        env->CallVoidMethod(java_lang_System_out, java_io_PrintStream_print, array);
        env->DeleteLocalRef(array);
    }
}







class ConsoleMessage : public TimedMessage {
    CFStringRef mMessage;

public:
    ConsoleMessage(CFStringRef message)
        :   mMessage(message)
    {
        ::CFRetain(mMessage);
    }
    
    ~ConsoleMessage()
    {
        ::CFRelease(mMessage);
    }
    
    virtual void execute();
};

void ConsoleMessage::execute()
{
    if (theSession) {
        jsize len = ::CFStringGetLength(mMessage);
        jchar* buffer = new jchar[len];
        CFRange range = { 0, len };
        CFStringGetCharacters(mMessage, range, buffer);
        System_out_print(theSession->getCurrentEnv(), buffer, len);
        delete[] buffer;
    }
}

static jint JNICALL java_vfprintf(FILE *fp, const char *format, va_list args)
{
    jint result = 0;
    CFStringRef formatRef = CFStringCreateWithCString(NULL, format, kCFStringEncodingASCII);
    if (formatRef) {
        CFStringRef text = CFStringCreateWithFormatAndArguments(NULL, NULL, formatRef, args);
        CFRelease(formatRef);
        if (text) {
            ConsoleMessage* message = new ConsoleMessage(text);
            if (message) {
                if (message->send() != noErr)
                    delete message;
            }
            result = ::CFStringGetLength(text);
            ::CFRelease(text);
        }
    }
    return result;
}

#endif 

MRJSession::MRJSession()
	:	mStatus(noErr), mMainEnv(NULL), mJavaVM(NULL), mSession(NULL),
	    mFirst(NULL), mLast(NULL), mMessageMonitor(NULL), mLockCount(0)
{
}

MRJSession::~MRJSession()
{
    close();
}

OSStatus MRJSession::open(const char* consolePath)
{
    
    string classPath = getClassPath();
    string pluginHome = getPluginHome();
    JavaVMOption theOptions[] = {
    	{ (char*) classPath.c_str() },
    	{ (char*) pluginHome.c_str() },
#if REDIRECT_VFPRINTF
    	{ "vfprintf", NewMachOFunctionPointer(&java_vfprintf) }
#endif
    };

    JavaVMInitArgs theInitArgs = {
    	JNI_VERSION_1_2,
    	sizeof(theOptions) / sizeof(JavaVMOption),
    	theOptions,
    	JNI_TRUE
    };

    mStatus = ::JNI_CreateJavaVM(&mJavaVM, (void**) &mMainEnv, &theInitArgs);
    
    if (mStatus == noErr) {
       	
		mMessageMonitor = new MRJMonitor(this);
    }

    JNIEnv* env = mMainEnv;
    jclass session = env->FindClass("netscape/oji/MRJSession");
    if (session) {
        mSession = (jclass) env->NewGlobalRef(session);
        jmethodID openMethod = env->GetStaticMethodID(session, "open", "(Ljava/lang/String;)V");
        if (openMethod) {
            jstring path = env->NewStringUTF(consolePath);
            if (path) {
                env->CallStaticVoidMethod(session, openMethod, path);
                if (env->ExceptionCheck())
                    env->ExceptionClear();
                env->DeleteLocalRef(path);
            }
        } else {
            env->ExceptionClear();
        }
        env->DeleteLocalRef(session);
    } else {
        env->ExceptionClear();
    }

    if (mStatus == noErr)
        theSession = this;

#if REDIRECT_VFPRINTF
    
    jclass notThere = env->FindClass("class/not/Found");
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
#endif

    return mStatus;
}

OSStatus MRJSession::close()
{
    theSession = NULL;

    if (mJavaVM) {
    	if (mMessageMonitor != NULL) {
    		mMessageMonitor->notifyAll();
    		delete mMessageMonitor;
    		mMessageMonitor = NULL;
    	}
    	
    	if (mSession) {
    	    jclass session = mSession;
    	    JNIEnv* env = mMainEnv;
            jmethodID closeMethod = env->GetStaticMethodID(session, "close", "()V");
            if (closeMethod)
                env->CallStaticVoidMethod(session, closeMethod);
            else
                env->ExceptionClear();
    	    env->DeleteGlobalRef(mSession);
    	    mSession = NULL;
        }

#if !TARGET_RT_MAC_MACHO 
        mJavaVM->DestroyJavaVM();
#endif
        mJavaVM = NULL;
    }
    return noErr;
}

JNIEnv* MRJSession::getCurrentEnv()
{
	JNIEnv* env;
	if (mJavaVM->GetEnv((void**)&env, JNI_VERSION_1_2) == JNI_OK)
		return env;
    return NULL;
}

JNIEnv* MRJSession::getMainEnv()
{
	return mMainEnv;
}

JavaVM* MRJSession::getJavaVM()
{
    return mJavaVM;
}

Boolean MRJSession::onMainThread()
{
	return (getCurrentEnv() == mMainEnv);
}

inline StringPtr c2p(const char* cstr, StringPtr pstr)
{
	pstr[0] = (unsigned char)strlen(cstr);
	::BlockMoveData(cstr, pstr + 1, pstr[0]);
	return pstr;
}

Boolean MRJSession::addToClassPath(const FSSpec& fileSpec)
{
    
    if (mJavaVM)
        return false;

    
    FSRef ref;
    OSStatus status = FSpMakeFSRef(&fileSpec, &ref);
    if (status == noErr)
        mClassPath.push_back(ref);

    return true;
}

Boolean MRJSession::addToClassPath(const char* dirPath)
{
	
	Str255 path;
	FSSpec pathSpec;
	OSStatus status = ::FSMakeFSSpec(0, 0, c2p(dirPath, path), &pathSpec);
	if (status == noErr)
		return addToClassPath(pathSpec);
	return false;
}

Boolean MRJSession::addURLToClassPath(const char* fileURL)
{
    Boolean success = false;
    
    CFURLRef fileURLRef = CFURLCreateWithBytes(NULL, (UInt8*)fileURL, strlen(fileURL),
                                               kCFStringEncodingUTF8, NULL);
    if (fileURLRef) {
        FSRef fsRef;
        if (CFURLGetFSRef(fileURLRef, &fsRef)) {
            mClassPath.push_back(fsRef);
            success = true;
        }
        CFRelease(fileURLRef);
    }
    return success;
}

char* MRJSession::getProperty(const char* propertyName)
{
	char* result = NULL;
#if !TARGET_CARBON
	OSStatus status = noErr;
	JMTextRef nameRef = NULL, valueRef = NULL;
	status = ::JMNewTextRef(mSession, &nameRef, kTextEncodingMacRoman, propertyName, strlen(propertyName));
	if (status == noErr) {
		status = ::JMGetSessionProperty(mSession, nameRef, &valueRef);
		::JMDisposeTextRef(nameRef);
		if (status == noErr && valueRef != NULL) {
			UInt32 valueLength = 0;
			status = ::JMGetTextLengthInBytes(valueRef, kTextEncodingMacRoman, &valueLength);
			if (status == noErr) {
				result = new char[valueLength + 1];
				if (result != NULL) {
					UInt32 actualLength;
					status = ::JMGetTextBytes(valueRef, kTextEncodingMacRoman, result, valueLength, &actualLength);
					result[valueLength] = '\0';
				}
				::JMDisposeTextRef(valueRef);
			}
		}
	}
#endif
	return result;
}

void MRJSession::setStatus(OSStatus status)
{
	mStatus = status;
}

OSStatus MRJSession::getStatus()
{
	return mStatus;
}

void MRJSession::idle(UInt32 milliseconds)
{
	
	dispatchMessage();

#if !TARGET_CARBON
	
	if (mLockCount == 0) {
		lock();
		mStatus = ::JMIdle(mSession, milliseconds);
		unlock();
	}
#endif
}

void MRJSession::sendMessage(NativeMessage* message, Boolean async)
{
	
	if (onMainThread()) {
		message->execute();
	} else {
		postMessage(message);
		if (!async)
			mMessageMonitor->wait();
	}
}




void MRJSession::postMessage(NativeMessage* message)
{
	if (mFirst == NULL) {
		mFirst = mLast = message;
	} else {
		mLast->setNext(message);
		mLast = message;
	}
	message->setNext(NULL);
}

void MRJSession::dispatchMessage()
{
	if (mFirst != NULL) {
		NativeMessage* message = mFirst;
		mFirst = message->getNext();
		if (mFirst == NULL) mLast = NULL;
		
		message->setNext(NULL);
		message->execute();
		mMessageMonitor->notify();
	}
}

void MRJSession::lock()
{
	++mLockCount;
}

void MRJSession::unlock()
{
	--mLockCount;
}

static OSStatus ref2path(const FSRef& ref, char* path, UInt32 maxPathSize)
{
    return FSRefMakePath(&ref, (UInt8*)path, maxPathSize);
}

static OSStatus spec2path(const FSSpec& spec, char* path, UInt32 maxPathSize)
{
    FSRef ref;
    OSStatus status = FSpMakeFSRef(&spec, &ref);
    if (status == noErr)
        status = ref2path(ref, path, maxPathSize);
    return status;
}

string MRJSession::getClassPath()
{
    
    
    string classPath("-Xbootclasspath/p:");
    
    
    MRJClassPath::const_iterator i = mClassPath.begin();
    if (i != mClassPath.end()) {
        char path[1024];
        if (ref2path(*i, path, sizeof(path)) == noErr)
            classPath += path;
        ++i;
        while (i != mClassPath.end()) {
            if (ref2path(*i, path, sizeof(path)) == noErr) {
                classPath += ":";
                classPath += path;
            }    
            ++i;
        }
    }

    return classPath;
}

string MRJSession::getPluginHome()
{
    string pluginHome("-Dnetscape.oji.plugin.home=");

    char path[1024];
    if (spec2path(thePluginSpec, path, sizeof(path)) == noErr) {
        char* lastSlash = strrchr(path, '/');
        if (lastSlash) {
            *lastSlash = '\0';
            pluginHome += path;
        }
    }

    return pluginHome;
}
