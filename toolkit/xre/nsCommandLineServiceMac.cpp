







































#include "nsCommandLineServiceMac.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

namespace CommandLineServiceMac {

static const int kArgsGrowSize = 20;

static char** sArgs = NULL;
static int sArgsAllocated = 0;
static int sArgsUsed = 0;

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

  
  for (int arg = 0; arg < argc; arg++) {
    char* flag = argv[arg];
    
    if (strncmp(flag, "-psn_", 5) != 0)
      AddToCommandLine(flag);
  }

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

  argc = sArgsUsed;
  argv = sArgs;
}

} 
