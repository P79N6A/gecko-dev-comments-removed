












































#pragma once

#include <MacTypes.h>

#include <jni.h>

#include <vector>
#include <string>

using std::vector;
using std::string;

class NativeMessage {
public:
	NativeMessage() : mNext(NULL) {}
	virtual ~NativeMessage() {}
	
	virtual void execute() = 0;

	void setNext(NativeMessage* next) { mNext = next; }
	NativeMessage* getNext() { return mNext; }

private:
	NativeMessage* mNext;
};



class MRJContext;
class Monitor;

class MRJSession {
public:
	MRJSession();
	virtual ~MRJSession();
	
	OSStatus open(const char* consolePath);
	OSStatus close();
	
	JNIEnv* getCurrentEnv();
	JNIEnv* getMainEnv();
	JavaVM* getJavaVM();
	
	Boolean onMainThread();
	
	Boolean addToClassPath(const FSSpec& fileSpec);
	Boolean addToClassPath(const char* dirPath);
	Boolean addURLToClassPath(const char* fileURL);

	char* getProperty(const char* propertyName);
	
	void setStatus(OSStatus status);
	OSStatus getStatus();
	
	void idle(UInt32 milliseconds = 0x00000400);

	void sendMessage(NativeMessage* message, Boolean async = false);
	
	


	void lock();
	void unlock();

private:
	void postMessage(NativeMessage* message);
	void dispatchMessage();
	
	string getClassPath();
	string getPluginHome();
	
private:
	OSStatus mStatus;
	
	typedef vector<FSRef> MRJClassPath;
	MRJClassPath mClassPath;
	
	JNIEnv* mMainEnv;
	JavaVM* mJavaVM;
	jclass mSession;

	
	NativeMessage* mFirst;
	NativeMessage* mLast;
	Monitor* mMessageMonitor;
	
	UInt32 mLockCount;
};
