






































#ifndef nsLayoutDebugCLH_h_
#define nsLayoutDebugCLH_h_

#include "nsICommandLineHandler.h"
#define ICOMMANDLINEHANDLER nsICommandLineHandler

#define NS_LAYOUTDEBUGCLH_CID \
 { 0xa8f52633, 0x5ecf, 0x424a, \
   { 0xa1, 0x47, 0x47, 0xc3, 0x22, 0xf7, 0xbc, 0xe2 }}

class nsLayoutDebugCLH : public ICOMMANDLINEHANDLER
{
public:
    nsLayoutDebugCLH();
    virtual ~nsLayoutDebugCLH();

    NS_DECL_ISUPPORTS
    NS_DECL_NSICOMMANDLINEHANDLER
};

#endif
