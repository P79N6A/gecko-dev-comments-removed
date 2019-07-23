












































#include "MRJMonitor.h"
#include "MRJSession.h"

MRJMonitor::MRJMonitor(MRJSession* session, jobject monitor)
	:	mSession(session), mMonitor(NULL), mWaitMethod(NULL), mNotifyMethod(NULL), mNotifyAllMethod(NULL)
{
	JNIEnv* env = mSession->getCurrentEnv();
	jclass javaLangObject = env->FindClass("java/lang/Object");
	if (javaLangObject != NULL) {
		
		mWaitMethod = env->GetMethodID(javaLangObject, "wait", "()V");
		mTimedWaitMethod = env->GetMethodID(javaLangObject, "wait", "(J)V");
		mNotifyMethod = env->GetMethodID(javaLangObject, "notify", "()V");
		mNotifyAllMethod = env->GetMethodID(javaLangObject, "notifyAll", "()V");
		
		Boolean allocateMonitor = (monitor == NULL);
		if (allocateMonitor)
			monitor = env->AllocObject(javaLangObject);
		
		if (monitor != NULL)
			mMonitor = env->NewGlobalRef(monitor);

		if (allocateMonitor)
			env->DeleteLocalRef(monitor);

		env->DeleteLocalRef(javaLangObject);
	}
}

MRJMonitor::~MRJMonitor()
{
	if (mMonitor != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		env->DeleteGlobalRef(mMonitor);
		mMonitor = NULL;
	}
}

void MRJMonitor::enter()
{
	JNIEnv* env = mSession->getCurrentEnv();
	env->MonitorEnter(mMonitor);
}

void MRJMonitor::exit()
{
	JNIEnv* env = mSession->getCurrentEnv();
	env->MonitorExit(mMonitor);
}

void MRJMonitor::wait()
{
	if (mMonitor != NULL && mWaitMethod != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		env->MonitorEnter(mMonitor);
		env->CallVoidMethod(mMonitor, mWaitMethod);
		env->MonitorExit(mMonitor);
	}
}

void MRJMonitor::wait(long long millis)
{
	if (mMonitor != NULL && mWaitMethod != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		env->MonitorEnter(mMonitor);
		env->CallVoidMethod(mMonitor, mTimedWaitMethod, jlong(millis));
		env->MonitorExit(mMonitor);
	}
}

void MRJMonitor::notify()
{
	if (mMonitor != NULL && mNotifyMethod != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		env->MonitorEnter(mMonitor);
		env->CallVoidMethod(mMonitor, mNotifyMethod);
		env->MonitorExit(mMonitor);
	}
}

void MRJMonitor::notifyAll()
{
	if (mMonitor != NULL && mNotifyAllMethod != NULL) {
		JNIEnv* env = mSession->getCurrentEnv();
		env->MonitorEnter(mMonitor);
		env->CallVoidMethod(mMonitor, mNotifyAllMethod);
		env->MonitorExit(mMonitor);
	}
}

jobject MRJMonitor::getObject()
{
	return mMonitor;
}
