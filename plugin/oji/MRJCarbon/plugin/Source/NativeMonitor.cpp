












































#include "NativeMonitor.h"
#include "MRJSession.h"
#include "nsIThreadManager.h"

NativeMonitor::NativeMonitor(MRJSession* session, nsIThreadManager* manager, void* address)
	:	mSession(session), mManager(manager), mAddress(address)
{
	if (address == NULL)
		mAddress = this;
}

NativeMonitor::~NativeMonitor() {}

void NativeMonitor::enter()
{
	mManager->EnterMonitor(mAddress);
}

void NativeMonitor::exit()
{
	mManager->ExitMonitor(mAddress);
}

void NativeMonitor::wait()
{
	
	Boolean inJavaThread = (mSession->getMainEnv() != mSession->getCurrentEnv());
	if (inJavaThread)
		mSession->lock();

	if (mManager->EnterMonitor(mAddress) == NS_OK) {
		mManager->Wait(mAddress);
		mManager->ExitMonitor(mAddress);
	}

	if (inJavaThread)
		mSession->unlock();
}

void NativeMonitor::wait(long long millis)
{
	if (mManager->EnterMonitor(mAddress) == NS_OK) {
		mManager->Wait(mAddress, PRUint32(millis));
		mManager->ExitMonitor(mAddress);
	}
}

void NativeMonitor::notify()
{
	if (mManager->EnterMonitor(mAddress) == NS_OK) {
		mManager->Notify(mAddress);
		mManager->ExitMonitor(mAddress);
	}
}

void NativeMonitor::notifyAll()
{
	if (mManager->EnterMonitor(mAddress) == NS_OK) {
		mManager->NotifyAll(mAddress);
		mManager->ExitMonitor(mAddress);
	}
}
