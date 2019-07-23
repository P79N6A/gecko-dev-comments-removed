






































#ifndef nsLayoutDebugCLH_h_
#define nsLayoutDebugCLH_h_

#ifdef MOZ_XUL_APP
#include "nsICommandLineHandler.h"
#define ICOMMANDLINEHANDLER nsICommandLineHandler
#else
#include "nsICmdLineHandler.h"
#define ICOMMANDLINEHANDLER nsICmdLineHandler
#endif

#define NS_LAYOUTDEBUGCLH_CID \
 { 0xa8f52633, 0x5ecf, 0x424a, \
   { 0xa1, 0x47, 0x47, 0xc3, 0x22, 0xf7, 0xbc, 0xe2 }}

class nsLayoutDebugCLH : public ICOMMANDLINEHANDLER
{
public:
    nsLayoutDebugCLH();
    virtual ~nsLayoutDebugCLH();

    NS_DECL_ISUPPORTS
#ifdef MOZ_XUL_APP
    NS_DECL_NSICOMMANDLINEHANDLER
#else
    NS_DECL_NSICMDLINEHANDLER
    CMDLINEHANDLER_REGISTERPROC_DECLS
#endif
};

#endif 
