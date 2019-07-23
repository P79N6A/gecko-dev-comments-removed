











































#include "Monitor.h"

class MRJSession;
class nsIThreadManager;

class NativeMonitor : public Monitor {
public:
	NativeMonitor(MRJSession* session, nsIThreadManager* manager, void* address = NULL);
	virtual ~NativeMonitor();

	virtual void enter();
	virtual void exit();
	
	virtual void wait();
	virtual void wait(long long millis);
	virtual void notify();
	virtual void notifyAll();

private:
	MRJSession* mSession;
	nsIThreadManager* mManager;
	void* mAddress;
};
