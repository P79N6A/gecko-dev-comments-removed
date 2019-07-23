




































#ifndef nsAuthSambaNTLM_h__
#define nsAuthSambaNTLM_h__

#include "nsIAuthModule.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "prio.h"
#include "prproces.h"











class nsAuthSambaNTLM : public nsIAuthModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHMODULE

    nsAuthSambaNTLM();

    
    
    
    
    nsresult SpawnNTLMAuthHelper();

private:
    ~nsAuthSambaNTLM();

    void Shutdown();

    PRUint8*    mInitialMessage; 
    PRUint32    mInitialMessageLen;
    PRProcess*  mChildPID;
    PRFileDesc* mFromChildFD;
    PRFileDesc* mToChildFD;
};

#endif 
