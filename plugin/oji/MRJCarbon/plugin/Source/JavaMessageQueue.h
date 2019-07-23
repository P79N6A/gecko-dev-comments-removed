








































#pragma once

#ifndef JNI_H
#include "jni.h"
#endif

class Monitor;

class JavaMessage {
public:
	JavaMessage() : mNext(NULL) {}
	virtual ~JavaMessage() {}

	void setNext(JavaMessage* next) { mNext = next; }
	JavaMessage* getNext() { return mNext; }

	virtual void execute(JNIEnv* env) = 0;

private:
	JavaMessage* mNext;
};

class JavaMessageQueue {
public:
	JavaMessageQueue(Monitor* monitor);

	void putMessage(JavaMessage* msg);
	JavaMessage* getMessage();

	void enter();
	void exit();

	void wait();
	void wait(long long millis);
	void notify();

private:
	
	JavaMessage* mFirst;
	JavaMessage* mLast;
	Monitor* mMonitor;
};
