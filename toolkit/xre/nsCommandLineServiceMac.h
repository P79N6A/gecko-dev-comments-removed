






































#ifndef nsCommandLineServiceMac_h_
#define nsCommandLineServiceMac_h_

#include <CoreFoundation/CoreFoundation.h>

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"

class nsMacCommandLine
{
public:

  enum
  {
    kArgsGrowSize      = 20  
  };

                  nsMacCommandLine();
                  ~nsMacCommandLine();

  nsresult        Initialize(int& argc, char**& argv);
  void            SetupCommandLine(int& argc, char**& argv);
  
  nsresult        AddToCommandLine(const char* inArgText);
  nsresult        AddToCommandLine(const char* inOptionString, const CFURLRef file);
  nsresult        AddToEnvironmentVars(const char* inArgText);

  nsresult        HandleOpenOneDoc(const CFURLRef file, OSType inFileType);
  nsresult        HandlePrintOneDoc(const CFURLRef file, OSType fileType);

  nsresult        DispatchURLToNewBrowser(const char* url);

protected:

  nsresult        OpenURL(const char* aURL);

  nsresult        OpenWindow(const char *chrome, const PRUnichar *url);
    
  char**          mArgs;              
  PRUint32        mArgsAllocated;     
  PRUint32        mArgsUsed;          

  PRBool          mStartedUp;

public:

  static nsMacCommandLine& GetMacCommandLine() { return sMacCommandLine; }

private:

  static nsMacCommandLine sMacCommandLine;
  
};

void SetupMacCommandLine(int& argc, char**& argv);

#endif
