








































#ifndef _MOZSTORAGESERVICE_H_
#define _MOZSTORAGESERVICE_H_

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "prlock.h"

#include "mozIStorageService.h"

class mozStorageConnection;

class mozStorageService : public mozIStorageService
{
    friend class mozStorageConnection;

public:
    
    nsresult Init();

    static mozStorageService *GetSingleton();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_MOZISTORAGESERVICE

private:
    virtual ~mozStorageService();

    



    PRLock *mLock;
protected:
    nsCOMPtr<nsIFile> mProfileStorageFile;

    static mozStorageService *gStorageService;
};

#endif 
