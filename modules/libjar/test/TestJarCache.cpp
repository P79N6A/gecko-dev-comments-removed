














































#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsCRT.h"
#include "nsMemory.h"
#include "nsAutoLock.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIZipReader.h"
#include "nsILocalFile.h"

#include <stdio.h>
#include <stdlib.h>

static char** filenames; 

#define ZIP_COUNT    8
#define CACHE_SIZE   4
#define THREAD_COUNT 6
#define THREAD_LOOP_COUNT 1000

static nsCOMPtr<nsILocalFile> files[ZIP_COUNT];

static const char gCacheContractID[] = "@mozilla.org/libjar/zip-reader-cache;1";
static const PRUint32 gCacheSize = 4;

nsCOMPtr<nsIZipReaderCache> gCache = nsnull;

static nsIZipReader* GetZipReader(nsILocalFile* file)
{
    if(!gCache)
    {
        gCache = do_CreateInstance(gCacheContractID);
        if(!gCache || NS_FAILED(gCache->Init(CACHE_SIZE)))
            return nsnull;
    }

    nsIZipReader* reader = nsnull;

    if(!file || NS_FAILED(gCache->GetZip(file, &reader)))
        return nsnull;

    return reader;
}



class TestThread : public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIRUNNABLE

    TestThread();
    virtual ~TestThread();

private:
    PRUint32 mID;
    static PRUint32 gCounter;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(TestThread, nsIRunnable)

PRUint32 TestThread::gCounter = 0;

TestThread::TestThread()
    : mID(++gCounter)
{
}

TestThread::~TestThread()
{
}

NS_IMETHODIMP
TestThread::Run()
{
    printf("thread %d started\n", mID);
    
    nsCOMPtr<nsIZipReader> reader;
    int failure = 0;
    
    for(int i = 0; i < THREAD_LOOP_COUNT; i++)
    {
        int k = rand()%ZIP_COUNT;
        reader = dont_AddRef(GetZipReader(files[k]));
        if(!reader)
        {
            printf("thread %d failed to get reader for %s\n", mID, filenames[k]);
            failure = 1;
            break;         
        }

        

        PR_Sleep(rand()%10);    
    }
    
    reader = nsnull;

    printf("thread %d finished\n", mID);

    if ( failure ) return NS_ERROR_FAILURE;
    return NS_OK;
}



int main(int argc, char **argv)
{
    nsresult rv;
    int i;

    if (ZIP_COUNT != (argc - 1)) 
    {
        printf("usage: TestJarCache ");
        for ( i = 0; i < ZIP_COUNT; i++)
            printf("file%1d ",i + 1);
        printf("\n");
        return 1;
    }
    filenames = argv + 1;

    rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    if(NS_FAILED(rv)) 
    {
        printf("NS_InitXPCOM failed!\n");
        return 1;
    }

    
    nsIZipReader* bogus = GetZipReader(nsnull);


    for(i = 0; i < ZIP_COUNT; i++)
    {
        PRBool exists;
        rv = NS_NewLocalFile(filenames[i], PR_FALSE, getter_AddRefs(files[i]));
        if(NS_FAILED(rv) || NS_FAILED(files[i]->Exists(&exists)) || !exists) 
        {   
            printf("Couldn't find %s\n", filenames[i]);
            return 1;
        }
    }

    nsCOMPtr<nsIThread> threads[THREAD_COUNT];

    for(i = 0; i < THREAD_COUNT; i++)
    {
        rv = NS_NewThread(getter_AddRefs(threads[i]),
                          new TestThread(), 
                          0, PR_JOINABLE_THREAD);
        if(NS_FAILED(rv)) 
        {
            printf("NS_NewThread failed!\n");
            return 1;
        }
        PR_Sleep(10);
    }

    printf("all threads created\n");

    for(i = 0; i < THREAD_COUNT; i++)
    {
        if(threads[i])
        {
            threads[i]->Join();
            threads[i] = nsnull;
        }
    }

    for(i = 0; i < ZIP_COUNT; i++)
        files[i] = nsnull;

    
    gCache = nsnull;

    rv = NS_ShutdownXPCOM(nsnull);
    if(NS_FAILED(rv)) 
    {
        printf("NS_ShutdownXPCOM failed!\n");
        return 1;
    }

    printf("done\n");

    return 0;
}    


