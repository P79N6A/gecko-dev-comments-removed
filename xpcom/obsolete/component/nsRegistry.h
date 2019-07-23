






































#ifndef nsRegistry_h__
#define nsRegistry_h__

#include "nsIRegistry.h"
#include "NSReg.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"

struct nsRegistry : public nsIRegistry, nsIRegistryGetter {
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIREGISTRY

    
    NS_DECL_NSIREGISTRYGETTER

    int SetBufferSize( int bufsize );  

    
    nsRegistry();

private:
    ~nsRegistry();

protected:
    HREG   mReg; 
#ifdef EXTRA_THREADSAFE
    PRLock *mregLock;    
#endif
    nsCOMPtr<nsIFile> mCurRegFile; 
    nsWellKnownRegistry mCurRegID;

    NS_IMETHOD Close();
}; 

#endif
