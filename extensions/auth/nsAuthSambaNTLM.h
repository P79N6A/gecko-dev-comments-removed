




#ifndef nsAuthSambaNTLM_h__
#define nsAuthSambaNTLM_h__

#include "nsIAuthModule.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "prio.h"
#include "prproces.h"
#include "mozilla/Attributes.h"











class nsAuthSambaNTLM MOZ_FINAL : public nsIAuthModule
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIAUTHMODULE

    nsAuthSambaNTLM();

    
    
    
    
    nsresult SpawnNTLMAuthHelper();

private:
    ~nsAuthSambaNTLM();

    void Shutdown();

    uint8_t*    mInitialMessage; 
    uint32_t    mInitialMessageLen;
    PRProcess*  mChildPID;
    PRFileDesc* mFromChildFD;
    PRFileDesc* mToChildFD;
};

#endif 
