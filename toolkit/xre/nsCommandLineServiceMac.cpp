









































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
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsICommandLineRunner.h"
#include "nsDirectoryServiceDefs.h"

#include "prmem.h"
#include "plstr.h"
#include "prenv.h"


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
  
  
  
  for (int arg = 0; arg < argc; arg++) {
    char* flag = argv[arg];
    
    if (strncmp(flag, "-psn_", 5) != 0)
      AddToCommandLine(flag);
  }

  
  mStartedUp = PR_TRUE;
  
  argc = mArgsUsed;
  argv = mArgs;
  
  return NS_OK;
}

void nsMacCommandLine::SetupCommandLine(int& argc, char**& argv)
{
  
  
  
  
  
  
  
  

  
  Initialize(argc, argv);

  Boolean isForeground = PR_FALSE;
  ProcessSerialNumber psnSelf, psnFront;

  
  
  
  
  if (::GetCurrentProcess(&psnSelf) == noErr &&
      ::GetFrontProcess(&psnFront) == noErr &&
      ::SameProcess(&psnSelf, &psnFront, &isForeground) == noErr &&
      isForeground) {
    
    
    AddToCommandLine("-foreground");
  }

  argc = mArgsUsed;
  argv = mArgs;
}

nsresult nsMacCommandLine::AddToCommandLine(const char* inArgText)
{
  if (mArgsUsed >= mArgsAllocated - 1) {
    
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
  mArgs[mArgsUsed] = nsnull;
  return NS_OK;
}

nsresult nsMacCommandLine::AddToCommandLine(const char* inOptionString, const FSRef* inFSRef)
{
  CFURLRef url = ::CFURLCreateFromFSRef(nsnull, inFSRef);
  if (!url)
    return NS_ERROR_FAILURE;

  CFStringRef string = ::CFURLGetString(url);
  if (!string) {
    ::CFRelease(url);
    return NS_ERROR_FAILURE;
  }

  CFIndex length = ::CFStringGetLength(string);
  CFIndex bufLen = 0;
  ::CFStringGetBytes(string, CFRangeMake(0, length), kCFStringEncodingUTF8,
                     0, PR_FALSE, nsnull, 0, &bufLen);

  UInt8 buffer[bufLen + 1];
  if (!buffer) {
    ::CFRelease(url);
    return NS_ERROR_FAILURE;
  }

  ::CFStringGetBytes(string, CFRangeMake(0, length), kCFStringEncodingUTF8,
                     0, PR_FALSE, buffer, bufLen, nsnull);
  buffer[bufLen] = 0;

  ::CFRelease(url);

  AddToCommandLine(inOptionString);  
  AddToCommandLine((char*)buffer);

  return NS_OK;
}

nsresult nsMacCommandLine::AddToEnvironmentVars(const char* inArgText)
{
  (void)PR_SetEnv(inArgText);
  return NS_OK;
}

OSErr nsMacCommandLine::HandleOpenOneDoc(const FSRef* inFSRef, OSType inFileType)
{
  nsCOMPtr<nsILocalFileMac> inFile;
  nsresult rv = NS_NewLocalFileWithFSRef(inFSRef, PR_TRUE, getter_AddRefs(inFile));
  if (NS_FAILED(rv))
    return errAEEventNotHandled;

  if (!mStartedUp) {
    
    if (inFileType == 'TEXT' || inFileType == 'CMDL') {
      
      FILE *fp = 0;
      rv = inFile->OpenANSIFileDesc("r", &fp);
      if (NS_SUCCEEDED(rv)) {
        Boolean foundArgs = false;
        Boolean foundEnv = false;
        char chars[1024];
        static const char kCommandLinePrefix[] = "ARGS:";
        static const char kEnvVarLinePrefix[] = "ENV:";

        while (ReadLine(fp, chars, sizeof(chars)) != -1) {
          
          if (PL_strstr(chars, kCommandLinePrefix) == chars) {
            AddToCommandLine(chars + sizeof(kCommandLinePrefix) - 1);
            foundArgs = true;
          }
          else if (PL_strstr(chars, kEnvVarLinePrefix) == chars) {
            AddToEnvironmentVars(chars + sizeof(kEnvVarLinePrefix) - 1);
            foundEnv = true;
          }
        }

        fclose(fp);
        
        
        if (foundArgs || foundEnv)
          return noErr;
      }
    }
    
    
    
    
    rv = AddToCommandLine("-url", inFSRef);
    return (NS_SUCCEEDED(rv)) ? noErr : errAEEventNotHandled;
  }

  
  nsCOMPtr<nsICommandLineRunner> cmdLine
    (do_CreateInstance("@mozilla.org/toolkit/command-line;1"));
  if (!cmdLine) {
    NS_ERROR("Couldn't create command line!");
    return errAEEventNotHandled;
  }
  nsCString filePath;
  rv = inFile->GetNativePath(filePath);
  if (NS_FAILED(rv))
    return errAEEventNotHandled;

  nsCOMPtr<nsIFile> workingDir;
  rv = NS_GetSpecialDirectory(NS_OS_CURRENT_WORKING_DIR, getter_AddRefs(workingDir));
  if (NS_FAILED(rv))
    return errAEEventNotHandled;

  const char *argv[3] = {nsnull, "-file", filePath.get()};
  rv = cmdLine->Init(3, const_cast<char**>(argv), workingDir, nsICommandLine::STATE_REMOTE_EXPLICIT);
  if (NS_FAILED(rv))
    return errAEEventNotHandled;
  rv = cmdLine->Run();
  return (NS_SUCCEEDED(rv)) ? noErr : errAEEventNotHandled;
}

OSErr nsMacCommandLine::HandlePrintOneDoc(const FSRef* inFSRef, OSType fileType)
{
  
  
  
  
  if (!mStartedUp)
    return AddToCommandLine("-print", inFSRef);
  
  
  NS_NOTYETIMPLEMENTED("Write Me");
  return errAEEventNotHandled;
}

OSErr nsMacCommandLine::DispatchURLToNewBrowser(const char* url)
{
  OSErr err = errAEEventNotHandled;
  err = AddToCommandLine("-url");
  if (err == noErr)
    err = AddToCommandLine(url);
  
  return err;
}

#pragma mark -

void SetupMacCommandLine(int& argc, char**& argv)
{
  nsMacCommandLine& cmdLine = nsMacCommandLine::GetMacCommandLine();
  return cmdLine.SetupCommandLine(argc, argv);
}
