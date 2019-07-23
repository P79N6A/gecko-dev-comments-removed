






































#ifndef nsCommandLineServiceMac_h_
#define nsCommandLineServiceMac_h_

#include <Files.h>

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"

#include "nsAEDefs.h"

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
  nsresult        AddToCommandLine(const char* inOptionString, const FSSpec& inFileSpec);
  nsresult        AddToEnvironmentVars(const char* inArgText);

  OSErr           HandleOpenOneDoc(const FSSpec& inFileSpec, OSType inFileType);
  OSErr           HandlePrintOneDoc(const FSSpec& inFileSpec, OSType fileType);

	OSErr						DispatchURLToNewBrowser(const char* url);
	  
  OSErr						Quit(TAskSave askSave);
  
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
