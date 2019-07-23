





































#include "nsToolkit.h"
#include "prmon.h"
#include "nsSwitchToUIThread.h"
#include "prprf.h"
#include "nsWidgetAtoms.h"





static PRUintn gToolkitTLSIndex = 0; 






NS_IMPL_THREADSAFE_ISUPPORTS1(nsToolkit, nsIToolkit)

struct ThreadInterfaceData
{
  void	*data;
  thread_id waitingThread;
};




PRBool gThreadState = PR_FALSE;

struct ThreadInitInfo {
  PRMonitor *monitor;
  nsToolkit *toolkit;
};

void nsToolkit::RunPump(void* arg)
{
  int32		code;
  char		portname[64];
  ThreadInterfaceData id;
#ifdef DEBUG
  printf("TK-RunPump\n");
#endif
  ThreadInitInfo *info = (ThreadInitInfo*)arg;
  PR_EnterMonitor(info->monitor);

  gThreadState = PR_TRUE;

  PR_Notify(info->monitor);
  PR_ExitMonitor(info->monitor);

  delete info;

  
  PR_snprintf(portname, sizeof(portname), "event%lx", 
              (long unsigned) PR_GetCurrentThread());

  port_id event = create_port(200, portname);

  while(read_port_etc(event, &code, &id, sizeof(id), B_TIMEOUT, 1000) >= 0)
  {
          MethodInfo *mInfo = (MethodInfo *)id.data;
          mInfo->Invoke();
          if(id.waitingThread != 0)
            resume_thread(id.waitingThread);
          delete mInfo;
  }
}






nsToolkit::nsToolkit()  
{
  localthread = false;
  mGuiThread  = NULL;
}







nsToolkit::~nsToolkit()
{
  Kill();
  PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
}

void nsToolkit::Kill()
{
  if(localthread)
  {
    GetInterface();
    
    close_port(eventport);
  }
}






void nsToolkit::CreateUIThread()
{
#ifdef DEBUG
  printf("TK-CUIT\n");
#endif
  PRMonitor *monitor = ::PR_NewMonitor();
	
  PR_EnterMonitor(monitor);
	
  ThreadInitInfo *ti = new ThreadInitInfo();
  if (ti)
  {
    ti->monitor = monitor;
    ti->toolkit = this;
  
    
    mGuiThread = PR_CreateThread(PR_SYSTEM_THREAD,
                                   RunPump,
                                   (void*)ti,
                                   PR_PRIORITY_HIGH,
                                   PR_LOCAL_THREAD,
                                   PR_UNJOINABLE_THREAD,
                                   0);

    
    while(gThreadState == PR_FALSE)
    {
      PR_Wait(monitor, PR_INTERVAL_NO_TIMEOUT);
    }
  }
    
  
  PR_ExitMonitor(monitor);
  PR_DestroyMonitor(monitor);
}






NS_METHOD nsToolkit::Init(PRThread *aThread)
{
#ifdef DEBUG
  printf("TKInit\n");
#endif
  Kill();
  
  
  if (NULL != aThread) 
  {
    mGuiThread = aThread;
    localthread = false;
  } 
  else 
  {
    localthread = true;
    
    CreateUIThread();
  }

  cached = false;

  nsWidgetAtoms::RegisterAtoms();

  return NS_OK;
}

void nsToolkit::GetInterface()
{
#ifdef DEBUG
  printf("TK-GI\n");
#endif
  if(! cached)
  {
    char portname[64];

    PR_snprintf(portname, sizeof(portname), "event%lx", 
                (long unsigned) mGuiThread);

    eventport = find_port(portname);

    cached = true;
  }
}


bool nsToolkit::CallMethod(MethodInfo *info)
{
#ifdef DEBUG
  printf("TK-CM\n");
#endif
  ThreadInterfaceData id;

  GetInterface();

  id.data = info;
  id.waitingThread = find_thread(NULL);
  
  
  
  port_info portinfo;
  if (get_port_info(eventport, &portinfo) != B_OK)
  {
    return false;
  }
  
  if (port_count(eventport) < portinfo.capacity - 20) 
  {
    
    if(write_port_etc(eventport, WM_CALLMETHOD, &id, sizeof(id), B_TIMEOUT, 5000000) == B_OK)
    {
      
   	  suspend_thread(id.waitingThread);
      return true;
    }
  }
  return false;
}


bool nsToolkit::CallMethodAsync(MethodInfo *info)
{
#ifdef DEBUG
	printf("CMA\n");
#endif
  ThreadInterfaceData id;

  GetInterface();

  id.data = info;
  id.waitingThread = 0;
	
  
  
  
  port_info portinfo;
  if (get_port_info(eventport, &portinfo) != B_OK)
  {
    return false;
  }
  
  if (port_count(eventport) < portinfo.capacity - 20) 
  {
    if(write_port_etc(eventport, WM_CALLMETHOD, &id, sizeof(id), B_TIMEOUT, 0) == B_OK)
    {
      return true;
    }
  }
  return false;
}







NS_METHOD NS_GetCurrentToolkit(nsIToolkit* *aResult) 
{ 
  nsIToolkit* toolkit = nsnull; 
  nsresult rv = NS_OK; 
  PRStatus status; 
#ifdef DEBUG
  printf("TK-GetCTK\n");
#endif
  
  if (0 == gToolkitTLSIndex)  
  { 
    status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, NULL); 
    if (PR_FAILURE == status) 
    { 
      rv = NS_ERROR_FAILURE; 
    } 
  } 

  if (NS_SUCCEEDED(rv)) 
  { 
    toolkit = (nsIToolkit*)PR_GetThreadPrivate(gToolkitTLSIndex); 

    
    
    
    if (!toolkit) 
    { 
      toolkit = new nsToolkit(); 

      if (!toolkit) 
      { 
        rv = NS_ERROR_OUT_OF_MEMORY; 
      } 
      else 
      { 
        NS_ADDREF(toolkit); 
        toolkit->Init(PR_GetCurrentThread()); 
        
        
        
        
        PR_SetThreadPrivate(gToolkitTLSIndex, (void*)toolkit); 
      } 
    } 
    else 
    { 
      NS_ADDREF(toolkit); 
    } 
    *aResult = toolkit; 
  } 

  return rv; 
} 
