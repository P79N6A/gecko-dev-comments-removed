





































#ifndef nsIJVMThreadManager_h___
#define nsIJVMThreadManager_h___

#include "nsISupports.h"
#include "nsIRunnable.h"
#include "nspr.h"







#define NS_IJVMTHREADMANAGER_IID                        \
{ /* 97bb54c0-6846-11d2-801f-00805f71101c */            \
	0x97bb54c0,											\
	0x6846,												\
	0x11d2,												\
	{0x80, 0x1f, 0x00, 0x80, 0x5f, 0x71, 0x10, 0x1c}	\
}

class nsIRunnable;

class nsIJVMThreadManager : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJVMTHREADMANAGER_IID)
	
	


	NS_IMETHOD
	GetCurrentThread(PRThread* *threadID) = 0;

	




	NS_IMETHOD
	Sleep(PRUint32 milli = 0) = 0;
	
	



	NS_IMETHOD
	EnterMonitor(void* address) = 0;
	
	


	NS_IMETHOD
	ExitMonitor(void* address) = 0;
	
	



	NS_IMETHOD
	Wait(void* address, PRUint32 milli = 0) = 0;

	


	NS_IMETHOD
	Notify(void* address) = 0;

	


	NS_IMETHOD
	NotifyAll(void* address) = 0;

	


	NS_IMETHOD
	CreateThread(PRThread **thread, nsIRunnable* runnable) = 0;
	
	





	NS_IMETHOD
	PostEvent(PRThread* thread, nsIRunnable* runnable, PRBool async) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJVMThreadManager, NS_IJVMTHREADMANAGER_IID)

#ifndef NS_OJI_IMPL

typedef nsIJVMThreadManager nsIThreadManager;
#define NS_ITHREADMANAGER_IID NS_IJVMTHREADMANAGER_IID
#endif

#endif 
