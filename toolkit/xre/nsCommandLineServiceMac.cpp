







































#include "nsCommandLineServiceMac.h"
#include "MacApplicationDelegate.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

namespace CommandLineServiceMac {

static const int kArgsGrowSize = 20;

static char** sArgs = NULL;
static int sArgsAllocated = 0;
static int sArgsUsed = 0;

static PRBool sBuildingCommandLine = PR_FALSE;

void AddToCommandLine(const char* inArgText)
{
  if (sArgsUsed >= sArgsAllocated - 1) {
    
    char **temp = static_cast<char**>(realloc(sArgs, (sArgsAllocated + kArgsGrowSize) * sizeof(char*)));
    if (!temp)
      return;
    sArgs = temp;
    sArgsAllocated += kArgsGrowSize;
  }

  char *temp2 = strdup(inArgText);
  if (!temp2)
    return;

  sArgs[sArgsUsed++] = temp2;
  sArgs[sArgsUsed] = NULL;

  return;
}

void SetupMacCommandLine(int& argc, char**& argv, PRBool forRestart)
{
  sArgs = static_cast<char **>(malloc(kArgsGrowSize * sizeof(char*)));
  if (!sArgs)
    return;
  sArgsAllocated = kArgsGrowSize;
  sArgs[0] = NULL;
  sArgsUsed = 0;

  sBuildingCommandLine = PR_TRUE;

  
  for (int arg = 0; arg < argc; arg++) {
    char* flag = argv[arg];
    
    if (strncmp(flag, "-psn_", 5) != 0)
      AddToCommandLine(flag);
  }

  
  
  
  
  ProcessPendingGetURLAppleEvents();

  
  
  
  if (forRestart) {
    Boolean isForeground = false;
    ProcessSerialNumber psnSelf, psnFront;
    if (::GetCurrentProcess(&psnSelf) == noErr &&
        ::GetFrontProcess(&psnFront) == noErr &&
        ::SameProcess(&psnSelf, &psnFront, &isForeground) == noErr &&
        isForeground) {
      AddToCommandLine("-foreground");
    }
  }

  sBuildingCommandLine = PR_FALSE;

  argc = sArgsUsed;
  argv = sArgs;
}

PRBool AddURLToCurrentCommandLine(const char* aURL)
{
  if (!sBuildingCommandLine) {
    return PR_FALSE;
  }

  AddToCommandLine("-url");
  AddToCommandLine(aURL);

  return PR_TRUE;
}

} 
