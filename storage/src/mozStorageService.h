







































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
    mozStorageService();

    
    nsresult Init();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_MOZISTORAGESERVICE

    NS_DECL_NSIOBSERVER

private:
    ~mozStorageService();
protected:
    nsCOMPtr<nsIFile> mProfileStorageFile;

    nsresult InitStorageAsyncIO();
    nsresult FlushAsyncIO();
    nsresult FinishAsyncIO();
    void FreeLocks();
};

#endif 
