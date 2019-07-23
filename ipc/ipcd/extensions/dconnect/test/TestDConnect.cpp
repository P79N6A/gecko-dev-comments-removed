




































#include "ipcIService.h"
#include "ipcIDConnectService.h"
#include "ipcCID.h"

#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"

#include "nsXPCOMCID.h"
#include "nsILocalFile.h"

#include "nsString.h"
#include "prmem.h"

#if defined( XP_WIN ) || defined( XP_OS2 )
#define TEST_PATH "c:"
#else
#define TEST_PATH "/tmp"
#endif

#define RETURN_IF_FAILED(rv, step) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
        printf("*** %s failed: rv=%x\n", step, rv); \
        return rv;\
    } \
    PR_END_MACRO

static PRBool gKeepRunning = PR_TRUE;

static ipcIService *gIpcServ = nsnull;

static nsresult DoTest()
{
  nsresult rv;

  nsCOMPtr<ipcIDConnectService> dcon = do_GetService("@mozilla.org/ipc/dconnect-service;1", &rv);
  RETURN_IF_FAILED(rv, "getting dconnect service");

  PRUint32 remoteClientID = 1;

  nsCOMPtr<nsIFile> file;
  rv = dcon->CreateInstanceByContractID(remoteClientID,
                                        NS_LOCAL_FILE_CONTRACTID,
                                        NS_GET_IID(nsIFile),
                                        getter_AddRefs(file)); 
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> sup = do_QueryInterface(file, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  printf("*** calling QueryInterface\n");
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(file, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString path;
  path.AssignLiteral(TEST_PATH);

  printf("*** calling InitWithNativePath\n");
  rv = localFile->InitWithPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString buf;
  rv = file->GetPath(buf);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!buf.Equals(path))
  {
    NS_ConvertUTF16toUTF8 temp(buf);
    printf("*** GetPath erroneously returned [%s]\n", temp.get());
    return NS_ERROR_FAILURE;
  }

  PRBool exists;
  rv = file->Exists(&exists);
  if (NS_FAILED(rv))
  {
    printf("*** Exists test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }
  printf("File exists? [%d]\n", exists);

  nsCOMPtr<nsIFile> clone;
  rv = file->Clone(getter_AddRefs(clone));
  if (NS_FAILED(rv))
  {
    printf("*** Clone test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }

  nsAutoString node;
  node.AssignLiteral("hello.txt");

  rv = clone->Append(node);
  if (NS_FAILED(rv))
  {
    printf("*** Append test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }

  PRBool match;
  rv = file->Equals(clone, &match);
  if (NS_FAILED(rv))
  {
    printf("*** Equals test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }
  printf("Files are equals? [%d]\n", match);

  

  rv = clone->Exists(&exists);
  if (NS_FAILED(rv))
  {
    printf("*** Exists test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }
  if (!exists)
  {
    rv = clone->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv))
    {
      printf("*** Create test failed [rv=%x]\n", rv);
      return NS_ERROR_FAILURE;
    }
  }

  rv = clone->MoveTo(nsnull, NS_LITERAL_STRING("helloworld.txt"));
  if (NS_FAILED(rv))
  {
    printf("*** MoveTo test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }

  

  nsCOMPtr<nsILocalFile> myLocalFile;
  rv = NS_NewLocalFile(path, PR_TRUE, getter_AddRefs(myLocalFile));
  if (NS_FAILED(rv))
  {
    printf("*** NS_NewLocalFile failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }

  rv = file->Equals(myLocalFile, &match);
  if (NS_FAILED(rv))
  {
    printf("*** second Equals test failed [rv=%x]\n", rv);
    return NS_ERROR_FAILURE;
  }
  printf("Files are equals? [%d]\n", match);

  printf("*** DoTest completed successfully :-)\n");
  return NS_OK;
}

int main(int argc, char **argv)
{
  nsresult rv;

  PRBool serverMode = PR_FALSE;
  if (argc > 1)
  {
    if (strcmp(argv[1], "-server") == 0)
    {
      serverMode = PR_TRUE;
    }
    else
    {
      printf("usage: %s [-server]\n", argv[0]);
      return -1;
    }
  }

  {
    nsCOMPtr<nsIServiceManager> servMan;
    NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    if (registrar)
        registrar->AutoRegister(nsnull);

    nsCOMPtr<ipcIService> ipcServ(do_GetService(IPC_SERVICE_CONTRACTID, &rv));
    RETURN_IF_FAILED(rv, "do_GetService(ipcServ)");
    NS_ADDREF(gIpcServ = ipcServ);

    if (!serverMode)
    {
      rv = DoTest();
      RETURN_IF_FAILED(rv, "DoTest()");
    }
    else
    {
      gIpcServ->AddName("DConnectServer");
    }

    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
    while (gKeepRunning)
      NS_ProcessNextEvent(thread);

    NS_RELEASE(gIpcServ);

    printf("*** processing remaining events\n");

    
    NS_ProcessPendingEvents(thread);

    printf("*** done\n");
  } 

  
  rv = NS_ShutdownXPCOM(nsnull);
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
}
