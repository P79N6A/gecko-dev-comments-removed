






































#ifndef nsCommandLineServiceMac_h_
#define nsCommandLineServiceMac_h_

#include <Carbon/Carbon.h>

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"

#ifdef __cplusplus

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
  nsresult        AddToCommandLine(const char* inOptionString, const FSRef* inFSRef);
  nsresult        AddToEnvironmentVars(const char* inArgText);

  OSErr           HandleOpenOneDoc(const FSRef* inFSRef, OSType inFileType);
  OSErr           HandlePrintOneDoc(const FSRef* inFSRef, OSType fileType);

	OSErr						DispatchURLToNewBrowser(const char* url);

protected:

  OSErr           OpenURL(const char* aURL);

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

#endif


#ifdef __cplusplus
extern "C" {
#endif

void SetupMacCommandLine(int& argc, char**& argv);

#ifdef __cplusplus
}
#endif


#endif
