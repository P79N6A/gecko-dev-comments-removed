





































#include "ipcITransactionService.h"
#include "ipcITransactionObserver.h"


#include "ipcIService.h"


#include "nsDebug.h"
#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"
#include "nsString.h"


#include "prmem.h"
#include "plgetopt.h"
#include "nspr.h"
#include "prlog.h"




const int NameSize = 1024;


PRIntn      optDebug = 0;
char        optMode = 's';
char        *profileName = new char[NameSize];
char        *queueName = new char[NameSize];

char        *data = new char[NameSize];
PRUint32    dataLen = 10;         




#define RETURN_IF_FAILED(rv, step) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
        printf("*** %s failed: rv=%x\n", step, rv); \
        return rv;\
    } \
    PR_END_MACRO

static PRBool gKeepRunning = PR_TRUE;

static ipcIService *gIpcServ = nsnull;
static ipcITransactionService *gTransServ = nsnull;



class myTransactionObserver : public ipcITransactionObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IPCITRANSACTIONOBSERVER

    myTransactionObserver() { }
};

NS_IMPL_ISUPPORTS1(myTransactionObserver, ipcITransactionObserver)

NS_IMETHODIMP myTransactionObserver::OnTransactionAvailable(PRUint32 aQueueID, const PRUint8 *aData, PRUint32 aDataLen)
{
    printf("tmModuleTest: myTransactionObserver::OnTransactionAvailable [%s]\n", aData);
    return NS_OK;
}

NS_IMETHODIMP myTransactionObserver::OnAttachReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    printf("tmModuleTest: myTransactionObserver::OnAttachReply [%d]\n", aStatus);
    return NS_OK;
}

NS_IMETHODIMP myTransactionObserver::OnDetachReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    printf("tmModuleTest: myTransactionObserver::OnDetachReply [%d]\n", aStatus);
    return NS_OK;
}

NS_IMETHODIMP myTransactionObserver::OnFlushReply(PRUint32 aQueueID, PRUint32 aStatus)
{
    printf("tmModuleTest: myTransactionObserver::OnFlushReply [%d]\n", aStatus);
    return NS_OK;
}




int main(PRInt32 argc, char *argv[])
{
  nsresult rv;

  
  strcpy(profileName, "defaultProfile");
  strcpy(queueName, "defaultQueue");
  strcpy(data, "test data");

  { 

    
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "bdfhlp:q:");

    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
	    if (PL_OPT_BAD == os) continue;
      switch (opt->option)
      {
        case 'b':  
          printf("tmModuleTest: broadcaster\n");
          optMode = 'b';
          break;
        case 'd':  
          printf("tmModuleTest: debugging baby\n");
          optDebug = 1;
          break;
        case 'f':  
          printf("tmModuleTest: flusher\n");
          optMode = 'f';
          break;
        case 'h':  
          printf("tmModuleTest: hit-n-run\n");
          optMode = 'h';
          break;
        case 'l':  
          printf("tmModuleTest: listener\n");
          optMode = 'l';
          break;
        case 'p':  
          strcpy(profileName, opt->value);
          printf("tmModuleTest: profilename:%s\n",profileName);
          break;
        case 'q':  
          strcpy(queueName, opt->value);
          printf("tmModuleTest: queuename:%s\n",queueName);
          break;
        default:
          printf("tmModuleTest: default\n");
          break;
      }
    }
    PL_DestroyOptState(opt);
  } 

  { 

    printf("tmModuleTest: Starting xpcom\n");
   
    
    nsCOMPtr<nsIServiceManager> servMan;
    NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    if (registrar)
      registrar->AutoRegister(nsnull);

    
    printf("tmModuleTest: getting ipc service\n");
    nsCOMPtr<ipcIService> ipcServ(do_GetService("@mozilla.org/ipc/service;1", &rv));
    RETURN_IF_FAILED(rv, "do_GetService(ipcServ)");
    NS_ADDREF(gIpcServ = ipcServ);

    
    printf("tmModuleTest: getting transaction service\n");
    nsCOMPtr<ipcITransactionService> transServ(do_GetService("@mozilla.org/ipc/transaction-service;1", &rv));
    RETURN_IF_FAILED(rv, "do_GetService(transServ)");
    NS_ADDREF(gTransServ = transServ);

    

    nsCOMPtr<ipcITransactionObserver> observ = new myTransactionObserver();

    
    gTransServ->Init(nsDependentCString(profileName));
    printf("tmModuleTest: using profileName [%s]\n", profileName);

    
    gTransServ->Attach(nsDependentCString(queueName), observ, PR_TRUE);
    printf("tmModuleTest: observing queue [%s]\n", queueName);

    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

    
    int i = 0;       
    switch (optMode)
    {
      case 's':
        printf("tmModuleTest: start standard\n");
        
        for (; i < 5 ; i++) {
          gTransServ->PostTransaction(nsDependentCString(queueName), (PRUint8 *)data, dataLen);
        }
        
        while (gKeepRunning)
          NS_ProcessNextEvent(thread);
        printf("tmModuleTest: end standard\n");
        break;
      case 'b':
        printf("tmModuleTest: start broadcast\n");
        
        for (; i < 50; i++) {
          gTransServ->PostTransaction(nsDependentCString(queueName), (PRUint8 *)data, dataLen);
        }
        
        while (gKeepRunning)
          NS_ProcessNextEvent(thread);
        printf("tmModuleTest: end broadcast\n");
        break;
      case 'f':
        printf("tmModuleTest: start flush\n");
        
        for (; i < 5; i++) {
          gTransServ->PostTransaction(nsDependentCString(queueName), (PRUint8 *)data, dataLen);
        }
        
        gTransServ->Flush(nsDependentCString(queueName), PR_TRUE);
        
        for (i=0; i < 8; i++) {
          gTransServ->PostTransaction(nsDependentCString(queueName), (PRUint8 *)data, dataLen);
        }
        
        while (gKeepRunning)
          NS_ProcessNextEvent(thread);
        
        gTransServ->Detach(nsDependentCString(queueName));
        printf("tmModuleTest: end flush\n");
        break;
      case 'h':
        printf("tmModuleTest: start hit-n-run\n");
        
        for (; i < 5; i++) {
          gTransServ->PostTransaction(nsDependentCString(queueName), (PRUint8 *)data, dataLen);
        }
        
        gTransServ->Detach(nsDependentCString(queueName));
        printf("tmModuleTest: end hit-n-run\n");
        break;
      case 'l':
        printf("tmModuleTest: start listener\n");
        
        while (gKeepRunning)
          NS_ProcessNextEvent(thread);
        printf("tmModuleTest: end listener\n");
        break;
      default :
        printf("tmModuleTest: start & end default\n");
        break;
    }

    

    NS_RELEASE(gTransServ);
    NS_RELEASE(gIpcServ);

    printf("tmModuleTest: processing remaining events\n");

    

    NS_ProcessPendingEvents(thread);

    printf("tmModuleTest: done\n");
  } 

  
  PR_Sleep(PR_SecondsToInterval(4));

  
  rv = NS_ShutdownXPCOM(nsnull);
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

  return 0;
}
