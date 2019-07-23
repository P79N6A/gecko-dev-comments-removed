








































#include "nsCommandLineServiceMac.h"


#include "nsDebug.h"
#include "nsILocalFileMac.h"
#include "nsDebug.h"
#include "nsNetUtil.h"
#include "nsIAppStartup.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsIServiceManager.h"
#include "nsNetCID.h"
#include "nsIDOMWindow.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIWindowWatcher.h"
#include "jsapi.h"
#include "nsReadableUtils.h"
#include "nsICloseAllWindows.h"
#include "nsIPrefService.h"

#include "nsAEEventHandling.h"
#include "nsXPFEComponentsCID.h"


#include "prmem.h"
#include "plstr.h"
#include "prenv.h"
#ifdef XP_MAC
#include "pprio.h"  
#endif


nsMacCommandLine nsMacCommandLine::sMacCommandLine;










static PRInt32 ReadLine(FILE* inStream, char* buf, PRInt32 bufSize)
{
  PRInt32 charsRead = 0;
  int c;
  
  if (bufSize < 2)
    return -1;

  while (charsRead < (bufSize-1)) {
    c = getc(inStream);
    if (c == EOF || c == '\n' || c == '\r')
      break;
    buf[charsRead++] = c;
  }
  buf[charsRead] = '\0';
  
  return (c == EOF && !charsRead) ? -1 : charsRead; 
}


nsMacCommandLine::nsMacCommandLine()
: mArgs(NULL)
, mArgsAllocated(0)
, mArgsUsed(0)
, mStartedUp(PR_FALSE)

{
}



nsMacCommandLine::~nsMacCommandLine()

{
  ShutdownAEHandlerClasses();
  if (mArgs) {
    for (PRUint32 i = 0; i < mArgsUsed; i++)
      free(mArgs[i]);
    free(mArgs);
  }
}



nsresult nsMacCommandLine::Initialize(int& argc, char**& argv)

{
  mArgs = static_cast<char **>(malloc(kArgsGrowSize * sizeof(char *)));
  if (!mArgs)
    return NS_ERROR_FAILURE;
  mArgs[0] = nsnull;
  mArgsAllocated = kArgsGrowSize;
  mArgsUsed = 0;
  
#if defined(XP_MACOSX)
  
  
  for (int arg = 0; arg < argc; arg++)
    AddToCommandLine(argv[arg]);
#else
  
  AddToCommandLine("mozilla");
#endif

  
  OSErr err = CreateAEHandlerClasses(false);
  if (err != noErr) return NS_ERROR_FAILURE;

  
  
  
  
  
  
  

  EventRecord anEvent;
  for (short i = 1; i < 5; i++)
    ::WaitNextEvent(0, &anEvent, 0, nsnull);

  while (::EventAvail(highLevelEventMask, &anEvent))
  {
    ::WaitNextEvent(highLevelEventMask, &anEvent, 0, nsnull);
    if (anEvent.what == kHighLevelEvent)
    {
      
      
      err = ::AEProcessAppleEvent(&anEvent);
    }
  }
  
  if (GetCurrentKeyModifiers() & optionKey)
    AddToCommandLine("-p");

  
  mStartedUp = PR_TRUE;
  
  argc = mArgsUsed;
  argv = mArgs;
  
  return NS_OK;
}


nsresult nsMacCommandLine::AddToCommandLine(const char* inArgText)

{
  if (mArgsUsed >= mArgsAllocated) {
    
    char **temp = static_cast<char **>(realloc(mArgs, (mArgsAllocated + kArgsGrowSize) * sizeof(char *)));
    if (!temp)
      return NS_ERROR_OUT_OF_MEMORY;
    mArgs = temp;
    mArgsAllocated += kArgsGrowSize;
  }
  char *temp2 = strdup(inArgText);
  if (!temp2)
    return NS_ERROR_OUT_OF_MEMORY;
  mArgs[mArgsUsed++] = temp2;
  return NS_OK;
}



nsresult nsMacCommandLine::AddToCommandLine(const char* inOptionString, const FSSpec& inFileSpec)

{
  
  FSSpec nonConstSpec = inFileSpec;
  nsCOMPtr<nsILocalFileMac> inFile;
  nsresult rv = NS_NewLocalFileWithFSSpec(&nonConstSpec, PR_TRUE, getter_AddRefs(inFile));
  if (NS_FAILED(rv))
    return rv;
  nsCAutoString specBuf;
  rv = NS_GetURLSpecFromFile(inFile, specBuf);
  if (NS_FAILED(rv))
    return rv;
  AddToCommandLine(inOptionString);  
  AddToCommandLine(specBuf.get());
  return NS_OK;
}


nsresult nsMacCommandLine::AddToEnvironmentVars(const char* inArgText)

{
  (void)PR_SetEnv(inArgText);
  return NS_OK;
}



OSErr nsMacCommandLine::HandleOpenOneDoc(const FSSpec& inFileSpec, OSType inFileType)

{
  nsCOMPtr<nsILocalFileMac> inFile;
  nsresult rv = NS_NewLocalFileWithFSSpec(&inFileSpec, PR_TRUE, getter_AddRefs(inFile));
  if (NS_FAILED(rv))
    return errAEEventNotHandled;

  if (!mStartedUp)
  {
    
    if (inFileType == 'TEXT' || inFileType == 'CMDL')
    {
      
      FILE *fp = 0;
      rv = inFile->OpenANSIFileDesc("r", &fp);
      if (NS_SUCCEEDED(rv))
      {
        Boolean foundArgs = false;
        Boolean foundEnv = false;
        char chars[1024];
        static const char kCommandLinePrefix[] = "ARGS:";
        static const char kEnvVarLinePrefix[] = "ENV:";

        while (ReadLine(fp, chars, sizeof(chars)) != -1)
        {       
          if (PL_strstr(chars, kCommandLinePrefix) == chars)
          {
            (void)AddToCommandLine(chars + sizeof(kCommandLinePrefix) - 1);
            foundArgs = true;
          }
          else if (PL_strstr(chars, kEnvVarLinePrefix) == chars)
          {
            (void)AddToEnvironmentVars(chars + sizeof(kEnvVarLinePrefix) - 1);
            foundEnv = true;
          }
        }

        fclose(fp);
#ifndef XP_MACOSX
        
        
        if (foundEnv)
          PR_Init_Log();
#endif
        
        
        if (foundArgs || foundEnv)
          return noErr;
      }
    }
    
    
    
    
    return AddToCommandLine("-url", inFileSpec);
  }

  
  nsCAutoString specBuf;
  rv = NS_GetURLSpecFromFile(inFile, specBuf);
  if (NS_FAILED(rv))
    return errAEEventNotHandled;
  
  return OpenURL(specBuf.get());
}

OSErr nsMacCommandLine::OpenURL(const char* aURL)
{
  nsresult rv;
  
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));

  nsXPIDLCString browserURL;
  if (NS_SUCCEEDED(rv))
    rv = prefBranch->GetCharPref("browser.chromeURL", getter_Copies(browserURL));
  
  if (NS_FAILED(rv)) {
    NS_WARNING("browser.chromeURL not supplied! How is the app supposed to know what the main window is?");
    browserURL.Assign("chrome://navigator/content/navigator.xul");
  }
     
  rv = OpenWindow(browserURL.get(), NS_ConvertASCIItoUTF16(aURL).get());
  if (NS_FAILED(rv))
    return errAEEventNotHandled;
    
  return noErr;
}




OSErr nsMacCommandLine::HandlePrintOneDoc(const FSSpec& inFileSpec, OSType fileType)

{
  
  
  
  
  if (!mStartedUp)
    return AddToCommandLine("-print", inFileSpec);
  
  
  NS_NOTYETIMPLEMENTED("Write Me");
  return errAEEventNotHandled;
}




nsresult nsMacCommandLine::OpenWindow(const char *chrome, const PRUnichar *url)

{
  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  nsCOMPtr<nsISupportsString> urlWrapper(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  if (!wwatch || !urlWrapper)
    return NS_ERROR_FAILURE;

  urlWrapper->SetData(nsDependentString(url));

  nsCOMPtr<nsIDOMWindow> newWindow;
  nsresult rv;
  rv = wwatch->OpenWindow(0, chrome, "_blank",
               "chrome,dialog=no,all", urlWrapper,
               getter_AddRefs(newWindow));

  return rv;
}


OSErr nsMacCommandLine::DispatchURLToNewBrowser(const char* url)

{
  OSErr err = errAEEventNotHandled;
  if (mStartedUp)
    return OpenURL(url);
  else {
    err = AddToCommandLine("-url");
    if (err == noErr)
      err = AddToCommandLine(url);
  }
  
  return err;
}


OSErr nsMacCommandLine::Quit(TAskSave askSave)

{
  nsresult rv;
  
  nsCOMPtr<nsICloseAllWindows> closer =
           do_CreateInstance("@mozilla.org/appshell/closeallwindows;1", &rv);
  if (NS_FAILED(rv))
    return errAEEventNotHandled;

    PRBool doQuit;
    rv = closer->CloseAll(askSave != eSaveNo, &doQuit);
    if (NS_FAILED(rv) || !doQuit)
        return errAEEventNotHandled;
          
  nsCOMPtr<nsIAppStartup> appStartup = 
           (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return errAEEventNotHandled;
  
  (void)appStartup->Quit(nsIAppStartup::eAttemptQuit);
  return noErr;
}







#pragma mark -


nsresult InitializeMacCommandLine(int& argc, char**& argv)

{

  nsMacCommandLine&  cmdLine = nsMacCommandLine::GetMacCommandLine();
  return cmdLine.Initialize(argc, argv);
} 
