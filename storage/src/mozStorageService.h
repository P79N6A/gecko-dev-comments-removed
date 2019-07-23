








































#ifndef _MOZSTORAGESERVICE_H_
#define _MOZSTORAGESERVICE_H_

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"

#include "mozIStorageService.h"

class mozStorageConnection;

class mozStorageService : public mozIStorageService,
                          public nsIObserver
{
    friend class mozStorageConnection;

public:
    
    nsresult Init();

    static mozStorageService *GetSingleton();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_MOZISTORAGESERVICE

    NS_DECL_NSIOBSERVER

private:
    virtual ~mozStorageService();
protected:
    nsCOMPtr<nsIFile> mProfileStorageFile;

    static mozStorageService *gStorageService;

    nsresult InitStorageAsyncIO();
    nsresult FlushAsyncIO();
    nsresult FinishAsyncIO();
    void FreeLocks();
};

#endif 
