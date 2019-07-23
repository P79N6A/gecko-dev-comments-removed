









































#include "nsCRT.h"
#include "prmem.h"
#include "VerReg.h"
#include "nsInstallExecute.h"
#include "nsInstallResources.h"
#include "ScheduledTasks.h"

#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"
#include "nsProcess.h"
#include "nsReadableUtils.h"














PRInt32 xpi_PrepareProcessArguments(const char *aArgsString, char **aArgs, PRInt32 aArgsAvailable)
{
   int   argc;
   char *c;
   char *p; 
   PRBool quoted = PR_FALSE;

   aArgs[0] = (char *)aArgsString;
   if (!aArgs[0])
      return -1;

   
   argc = 0;
   c = aArgs[argc];
   while (*c == ' ') ++c;
   aArgs[argc++] = c;

   for (; *c && argc < aArgsAvailable; ++c) 
   {
      switch(*c) {

      
      case '\\':
         
         if ( *(c+1) == '\\' || *(c+1) == '\"' )
         {
            
            
            for (p=c; *p != 0; ++p)
               *p = *(p+1);
         }
         break;

      case '\"':
         *c = 0; 
         if (quoted) 
         {
            p = c+1; 
            while (*p == ' ')
               ++p; 
            if (*p)
               aArgs[argc++] = p; 
            c = p-1;

            quoted = PR_FALSE;
         }
         else 
         {
            quoted = PR_TRUE;

            if (aArgs[argc-1] == c)
              
              
              aArgs[argc-1] = c+1;
            else
              
              
              aArgs[argc++] = c+1;
         }
         break;

      case ' ':
         if (!quoted) 
         {
            *c = 0; 
            p = c+1; 
            while (*p == ' ')
               ++p; 
            if (*p)
               aArgs[argc++] = p; 
            c = p-1;
         }
         break;

      default:
         break;  
      }
   }
   return argc;
}

nsInstallExecute:: nsInstallExecute(  nsInstall* inInstall,
                                      const nsString& inJarLocation,
                                      const nsString& inArgs,
                                      const PRBool inBlocking,
                                      PRInt32 *error)

: nsInstallObject(inInstall)
{
    MOZ_COUNT_CTOR(nsInstallExecute);

    if ((inInstall == nsnull) || (inJarLocation.IsEmpty()) )
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    mJarLocation        = inJarLocation;
    mArgs               = inArgs;
    mExecutableFile     = nsnull;
    mBlocking           = inBlocking;
    mPid                = nsnull;
}


nsInstallExecute::~nsInstallExecute()
{

    MOZ_COUNT_DTOR(nsInstallExecute);
}



PRInt32 nsInstallExecute::Prepare()
{
    if (mInstall == NULL || mJarLocation.IsEmpty()) 
        return nsInstall::INVALID_ARGUMENTS;

    return mInstall->ExtractFileFromJar(mJarLocation, nsnull, getter_AddRefs(mExecutableFile));
}

PRInt32 nsInstallExecute::Complete()
{
   #define ARG_SLOTS       256

   PRInt32 result = NS_OK;
   PRInt32 rv = nsInstall::SUCCESS;
   char *cArgs[ARG_SLOTS];
   int   argcount = 0;

   if (mExecutableFile == nsnull)
      return nsInstall::INVALID_ARGUMENTS;

   nsCOMPtr<nsIProcess> process = do_CreateInstance(NS_PROCESS_CONTRACTID);

   char *arguments = nsnull;
   if (!mArgs.IsEmpty())
   {
      arguments = ToNewCString(mArgs);
      argcount = xpi_PrepareProcessArguments(arguments, cArgs, ARG_SLOTS);
   }
   if (argcount >= 0)
   {
      result = process->Init(mExecutableFile);
      if (NS_SUCCEEDED(result))
      {
         result = process->Run(mBlocking, (const char**)&cArgs, argcount, mPid);
         if (NS_SUCCEEDED(result))
         {
            if (mBlocking)
            {
               process->GetExitValue(&result);
               if (result != 0)
                  rv = nsInstall::EXECUTION_ERROR;

               
               DeleteFileNowOrSchedule( mExecutableFile );
            }
            else
            {
               
               ScheduleFileForDeletion( mExecutableFile );
            }
         }
         else
            rv = nsInstall::EXECUTION_ERROR;
      }
      else
         rv = nsInstall::EXECUTION_ERROR;
   }
   else
      rv = nsInstall::UNEXPECTED_ERROR;

   if(arguments)
      Recycle(arguments);

   return rv;
}

void nsInstallExecute::Abort()
{
    
    if (mExecutableFile == nsnull) 
        return;

    DeleteFileNowOrSchedule(mExecutableFile);
}

char* nsInstallExecute::toString()
{
    char* buffer = new char[1024];
    char* rsrcVal = nsnull;

    if (buffer == nsnull || !mInstall)
        return nsnull;

    

    if (mExecutableFile == nsnull)
    {
        char *tempString = ToNewCString(mJarLocation);
        rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("Execute"));

        if (rsrcVal)
        {
            sprintf( buffer, rsrcVal, tempString);
            nsCRT::free(rsrcVal);
        }
        
        if (tempString)
            Recycle(tempString);
    }
    else
    {
        rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("Execute"));

        if (rsrcVal)
        {
            nsCAutoString temp;
            mExecutableFile->GetNativePath(temp);
            sprintf( buffer, rsrcVal, temp.get());
            nsCRT::free(rsrcVal);
        }
    }
    return buffer;
}


PRBool
nsInstallExecute::CanUninstall()
{
    return PR_FALSE;
}

PRBool
nsInstallExecute::RegisterPackageNode()
{
    return PR_FALSE;
}

