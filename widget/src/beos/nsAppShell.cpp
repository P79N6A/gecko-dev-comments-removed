









































#include "nsAppShell.h"
#include "nsSwitchToUIThread.h"
#include "prprf.h"

#include <Application.h>
#include <stdlib.h>






nsAppShell::nsAppShell()  
	: is_port_error(false), scheduled (false)
{ 
	eventport = -1; 
}








NS_IMETHODIMP nsAppShell::Init()
{
	
	
	
	char portname[B_OS_NAME_LENGTH];
	PR_snprintf(portname, sizeof(portname), "event%lx", (long unsigned) PR_GetCurrentThread());
             
#ifdef DEBUG              
	printf("nsAppShell::Create portname: %s\n", portname);
#endif

	
	if ((eventport = find_port(portname)) >= 0)
	{
		close_port(eventport);
		delete_port(eventport);
	}
		
	eventport = create_port(200, portname);
	return nsBaseAppShell::Init();
}








nsAppShell::~nsAppShell()
{
	close_port(eventport);
	delete_port(eventport);

	if (be_app->Lock())
	{
		be_app->Quit();
	}
}

PRBool nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
	bool gotMessage = false;

	
	if (eventport < 0)
	{
		char  portname[B_OS_NAME_LENGTH];	
		PR_snprintf(portname, sizeof(portname), "event%lx", (long unsigned) PR_GetCurrentThread());
		
		
		if((eventport = find_port(portname)) < 0)
		{
			
#ifdef DEBUG
			printf("nsAppShell::DispatchNativeEvent() was called before init\n");
#endif
			return gotMessage;
		}
	}
	
	
	

	if (port_count(eventport))
		gotMessage = InvokeBeOSMessage(0);

	
	if (port_count(eventport) && !mayWait)
	{
		if (!scheduled)
		{
			
			
			
			
			NativeEventCallback();
		}
		else
		{
			scheduled = false;
			gotMessage = InvokeBeOSMessage(0);
		}
	}
	
	
	
	
	if (mayWait)
		gotMessage = InvokeBeOSMessage(100000);
	return gotMessage;
}


void nsAppShell::ScheduleNativeEventCallback()
{
	if (eventport < 0)
		return;
	port_info portinfo;
	if (get_port_info(eventport, &portinfo) != B_OK)
		return;
	if (port_count(eventport) < portinfo.capacity - 20)
	{
		
		ThreadInterfaceData id;
		id.data = 0;
		id.waitingThread = find_thread(NULL);
		write_port_etc(eventport, 'natv', &id, sizeof(id), B_TIMEOUT, 1000000);
		scheduled = true;
	}
}

bool nsAppShell::InvokeBeOSMessage(bigtime_t timeout)
{
	int32 code;
	ThreadInterfaceData id;
	if (read_port_etc(eventport, &code, &id, sizeof(id), B_TIMEOUT, timeout) < 0)
	{
		is_port_error = true;
		return false;
	}

	id.waitingThread = 0; 
	MethodInfo *mInfo = (MethodInfo *)id.data;
	if (code != 'natv')
		mInfo->Invoke();

	if (id.waitingThread != 0)
		resume_thread(id.waitingThread);
	delete mInfo;
	return true;
}