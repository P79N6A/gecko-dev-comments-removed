











































#include "Monitor.h"

#ifndef JNI_H
#include "jni.h"
#endif

class MRJSession;

class MRJMonitor : public Monitor {
public:
	MRJMonitor(MRJSession* session, jobject monitor = NULL);
	~MRJMonitor();
	
	virtual void enter();
	virtual void exit();

	virtual void wait();
	virtual void wait(long long millis);
	virtual void notify();
	virtual void notifyAll();
	
	virtual jobject getObject();

private:
	MRJSession* mSession;
	jobject mMonitor;
	jmethodID mWaitMethod;
	jmethodID mTimedWaitMethod;
	jmethodID mNotifyMethod;
	jmethodID mNotifyAllMethod;
};
