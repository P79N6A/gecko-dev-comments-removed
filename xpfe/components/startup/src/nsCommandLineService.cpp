





































#include "nsICmdLineService.h"
#include "nsCommandLineService.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsString.h"
#include "plstr.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#ifdef XP_MACOSX
#include "nsCommandLineServiceMac.h"
#endif

nsCmdLineService::nsCmdLineService()
	:  mArgCount(0), mArgc(0), mArgv(0)
{
}




NS_IMPL_ISUPPORTS1(nsCmdLineService, nsICmdLineService)

static void* ProcessURLArg(char* str)
{
  
  
  
  
  
  
  if (str && (*str == '\\' || *str == '/'))
  {
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), str);
    if (NS_FAILED(rv))
    {
      nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
      if (file)
      {
        rv = file->InitWithNativePath(nsDependentCString(str));
        if (NS_SUCCEEDED(rv))
        {
          nsCAutoString fileurl;
          rv = NS_GetURLSpecFromFile(file, fileurl);
          if (NS_SUCCEEDED(rv))
            return NS_REINTERPRET_CAST(void*, ToNewCString(fileurl));
        }
      }
    }
  }

  return NS_REINTERPRET_CAST(void*, nsCRT::strdup(str));
}

NS_IMETHODIMP
nsCmdLineService::Initialize(int aArgc, char ** aArgv)
{


  PRInt32   i=0;
  nsresult  rv = nsnull;

#ifdef XP_MACOSX
  rv = InitializeMacCommandLine(aArgc, aArgv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Initializing AppleEvents failed");
#endif

  
  mArgc = aArgc;
  mArgv = new char*[ aArgc ];
  for(i=0; i<aArgc; i++) {
    mArgv[i] = nsCRT::strdup( aArgv[i] ? aArgv[i] : "" );
  }
  
  if (aArgc > 0 && aArgv[0])
  {
    mArgList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("-progname")));
    mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[0])));
    mArgCount++;
    i++;
  }

  for(i=1; i<aArgc; i++) {

    if ((aArgv[i][0] == '-')
#if defined(XP_WIN) || defined(XP_OS2)
        || (aArgv[i][0] == '/')
#endif
      ) {
       


	   mArgList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[i])));
	   
       i++;


     

	   if (i == aArgc) {
	     


	     mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("1")));
	     mArgCount++;
	     break;
	   }
     if ((aArgv[i][0] == '-')
#if defined(XP_WIN) || defined(XP_OS2)
         || (aArgv[i][0] == '/')
#endif
       ) {
        




        mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("1")));
   	    mArgCount++;
        i--;
        continue;
	    }
      else {
        


	      if (i == (aArgc-1)) {
	       


           
	         

 		       
           
		       mArgValueList.AppendElement(ProcessURLArg(aArgv[i]));
	 	       mArgCount++;
           continue;
        }
	      else {
	         


             mArgValueList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup(aArgv[i])));
	         mArgCount++;
	      }
	   }
  }
  else {
       if (i == (aArgc-1)) {
	      


           mArgList.AppendElement(NS_REINTERPRET_CAST(void*, nsCRT::strdup("-url")));
	         mArgValueList.AppendElement(ProcessURLArg(aArgv[i]));
	         mArgCount++;
	     }
	     else {
	       
	       rv = NS_ERROR_INVALID_ARG;
	     }
  }
	
 }  

#if 0
  for (i=0; i<mArgCount; i++)
  {
       printf("Argument: %s, ****** Value: %s\n", (char *)mArgList.ElementAt(i), (char *) mArgValueList.ElementAt(i));      
  }
#endif 

   return rv;
	
}

NS_IMETHODIMP
nsCmdLineService::GetURLToLoad(char ** aResult)
{
  nsresult rv = GetCmdLineValue("-url", aResult);
  if (NS_SUCCEEDED(rv) && *aResult && !strncmp(*aResult, "chrome:", 7)) {
    nsMemory::Free(*aResult);
    *aResult = nsnull;
    return NS_ERROR_INVALID_ARG;
  }
  return rv;
}

NS_IMETHODIMP
nsCmdLineService::GetProgramName(char ** aResult)
{
  *aResult = nsCRT::strdup((char *)mArgValueList.SafeElementAt(0));
  return NS_OK;
}

PRBool nsCmdLineService::ArgsMatch(const char *lookingFor, const char *userGave)
{
    if (!lookingFor || !userGave) return PR_FALSE;

    if (!PL_strcasecmp(lookingFor,userGave)) return PR_TRUE;

#if defined(XP_UNIX) || defined(XP_BEOS)
    
    if (lookingFor && userGave && (lookingFor[0] != '\0') && (userGave[0] != '\0') && (userGave[1] != '\0')) {
        if (!PL_strcasecmp(lookingFor+1,userGave+2) && (lookingFor[0] == '-') && (userGave[0] == '-') && (userGave[1] == '-')) return PR_TRUE;
    }
#endif
#if defined(XP_WIN) || defined(XP_OS2)
    
    if (lookingFor && userGave && (lookingFor[0] != '\0') && (userGave[0] != '\0')) {
        if (!PL_strcasecmp(lookingFor+1,userGave+1) && (lookingFor[0] == '-') && (userGave[0] == '/')) return PR_TRUE;
    }
#endif 
    return PR_FALSE;
}

NS_IMETHODIMP
nsCmdLineService::GetCmdLineValue(const char * aArg, char ** aResult)
{
   nsresult  rv = NS_OK;
   
   if (nsnull == aArg || nsnull == aResult ) {
	    return NS_ERROR_NULL_POINTER;
   }

   for (int i = 0; i<mArgCount; i++)
   {
     if (ArgsMatch(aArg,(char *) mArgList.ElementAt(i))) {
       *aResult = nsCRT::strdup((char *)mArgValueList.ElementAt(i));
        return NS_OK;
     }
   }

   *aResult = nsnull;
   return rv;
	
}

NS_IMETHODIMP
nsCmdLineService::GetArgc(PRInt32 * aResult)
{

    if (nsnull == aResult)
        return NS_ERROR_NULL_POINTER;

    
    if (mArgc == 0)
      return NS_ERROR_FAILURE;

    *aResult =  mArgc;
    return NS_OK;
}

NS_IMETHODIMP
nsCmdLineService::GetArgv(char *** aResult)
{
    if (nsnull == aResult)
      return NS_ERROR_NULL_POINTER;

    
    if (!mArgv)
      return NS_ERROR_FAILURE;

    *aResult = mArgv;

    return NS_OK;
}

nsCmdLineService::~nsCmdLineService()
{
  PRInt32 curr = mArgList.Count();
  while ( curr ) {
    char* str = NS_REINTERPRET_CAST(char*, mArgList[curr-1]);
    if ( str )
      nsMemory::Free(str);
    --curr;
  }
  
  curr = mArgValueList.Count();
  while ( curr ) {
    char* str = NS_REINTERPRET_CAST(char*, mArgValueList[curr-1]);
    if ( str )
      nsMemory::Free(str);
    --curr;
  }

  curr = mArgc;
  while ( curr ) {
    char *str = mArgv ? mArgv[curr-1] : 0;
    if ( str )
      nsMemory::Free( mArgv[curr-1] );
    --curr;
  }
  delete [] mArgv;
}

NS_IMETHODIMP
nsCmdLineService::GetHandlerForParam(const char *aParam,
                                     nsICmdLineHandler** aResult)
{
  nsresult rv;

  
  nsAutoVoidArray oneParameter;

  nsVoidArray *paramList;
  
  
  if (!aParam)
    paramList = &mArgList;
  else {
    oneParameter.AppendElement((void *)aParam);
    paramList = &oneParameter;
  }

  PRUint32 i;
  for (i=0; i < (PRUint32)paramList->Count(); i++) {
    const char *param = (const char*)paramList->ElementAt(i);
    
    
    if (*param == '-' || *param == '/') {
      ++param;
      if (*param == *(param-1)) 
        ++param;
    }
    
    nsCAutoString
      contractID("@mozilla.org/commandlinehandler/general-startup;1?type=");
    
    contractID += param;

    nsCOMPtr<nsICmdLineHandler> handler =
      do_GetService(contractID.get(), &rv);
    if (NS_FAILED(rv)) continue;

    *aResult = handler;
    NS_ADDREF(*aResult);
    return NS_OK;
  }

  
  return NS_ERROR_FAILURE;
}

#if 0
NS_IMETHODIMP
nsCmdLineService::PrintCmdArgs()
{

   if (mArgCount == 0) {
     printf("No command line options provided\n");
     return;
   }
   
   for (int i=0; i<mArgCount; i++)
   {
       printf("Argument: %s, ****** Value: %s\n", mArgList.ElementAt(i), mArgValueList.ElementAt(i));      

   }

  return NS_OK;

}
#endif

