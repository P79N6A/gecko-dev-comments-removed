





































#ifndef nsAppShell_h__
#define nsAppShell_h__

#include "nsBaseAppShell.h"
#include <OS.h>

struct ThreadInterfaceData
{
	void*     data;
	thread_id waitingThread;
};

struct EventItem
{
	int32               code;
	ThreadInterfaceData ifdata;
};

struct MethodInfo;





class nsAppShell : public nsBaseAppShell
{
  public:
	nsAppShell();
	nsresult        Init();
protected:
	virtual void    ScheduleNativeEventCallback();
	virtual PRBool  ProcessNextNativeEvent(PRBool mayWait);
	virtual        ~nsAppShell();

private:
	port_id	       eventport;
	volatile bool  scheduled;
	bool           is_port_error;
	bool           InvokeBeOSMessage(bigtime_t timeout);    
};

#endif 
