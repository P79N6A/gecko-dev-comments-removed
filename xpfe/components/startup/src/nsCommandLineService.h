



































#ifndef __nsCommandLineService_h
#define __nsCommandLineService_h

#include "nsISupports.h"
#include "nsICmdLineService.h"
#include "nsVoidArray.h"

class nsCmdLineService : public nsICmdLineService
{
public:
  nsCmdLineService(void);

  NS_DECL_ISUPPORTS
  NS_DECL_NSICMDLINESERVICE

protected:
  virtual ~nsCmdLineService();

  nsVoidArray    mArgList;      
  nsVoidArray    mArgValueList; 
  PRInt32        mArgCount; 
                            
                            
  PRInt32        mArgc;     
  char **        mArgv;     

  PRBool ArgsMatch(const char *lookingFor, const char *userGave);
};


#define NS_COMMANDLINESERVICE_CID \
{  0xe34783f5, 0xac08, 0x11d2, \
  {0x8d, 0x19, 0x00, 0x80, 0x5f, 0xc2, 0x50,0xc} }

#endif
