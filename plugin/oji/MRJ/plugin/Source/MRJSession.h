











































#pragma once

#ifndef CALL_NOT_IN_CARBON
	#define CALL_NOT_IN_CARBON 1
#endif

#include "jni.h"
#include "JManager.h"

class NativeMessage {
public:
	NativeMessage() : mNext(NULL) {}
	
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
	
	JMSessionRef getSessionRef();
	
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
	
	void idle(UInt32 milliseconds = kDefaultJMTime);

	void sendMessage(NativeMessage* message, Boolean async = false);
	
	


	void lock();
	void unlock();

private:
	void postMessage(NativeMessage* message);
	void dispatchMessage();
	
private:
	JMSessionRef mSession;
	OSStatus mStatus;

	JNIEnv* mMainEnv;

	
	NativeMessage* mFirst;
	NativeMessage* mLast;
	Monitor* mMessageMonitor;
	
	UInt32 mLockCount;
};
