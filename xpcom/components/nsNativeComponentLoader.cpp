
















































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nsNativeComponentLoader.h"

#include "prlog.h"
#include "prinit.h"
#include "prerror.h"

#include "nsComponentManager.h"
#include "nsCRTGlue.h"
#include "nsModule.h"
#include "nsTraceRefcntImpl.h"

#include "nsILocalFile.h"
#include "nsIModule.h"

#ifdef XP_WIN
#include <windows.h>
#endif

#ifdef XP_MACOSX
#include <signal.h>
#endif

#ifdef VMS
#include <lib$routines.h>
#include <ssdef.h>
#endif

#if defined(DEBUG) && !defined(XP_BEOS)
#define IMPLEMENT_BREAK_AFTER_LOAD
#endif

static PRLogModuleInfo *nsNativeModuleLoaderLog =
    PR_NewLogModule("nsNativeModuleLoader");

#define LOG(level, args) PR_LOG(nsNativeModuleLoaderLog, level, args)

NS_IMPL_QUERY_INTERFACE1(nsNativeModuleLoader, 
                         nsIModuleLoader)

NS_IMPL_ADDREF_USING_AGGREGATOR(nsNativeModuleLoader,
                                nsComponentManagerImpl::gComponentManager)
NS_IMPL_RELEASE_USING_AGGREGATOR(nsNativeModuleLoader,
                                 nsComponentManagerImpl::gComponentManager)

nsresult
nsNativeModuleLoader::Init()
{
    LOG(PR_LOG_DEBUG, ("nsNativeModuleLoader::Init()"));

    return mLibraries.Init() ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsNativeModuleLoader::LoadModule(nsILocalFile* aFile, nsIModule* *aResult)
{
    nsresult rv;

    
    nsCAutoString filePath;
    aFile->GetNativePath(filePath);
    if (!StringTail(filePath, sizeof(MOZ_DLL_SUFFIX) - 1).
        LowerCaseEqualsLiteral(MOZ_DLL_SUFFIX))
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIHashable> hashedFile(do_QueryInterface(aFile));
    if (!hashedFile) {
        NS_ERROR("nsIFile is not nsIHashable");
        return NS_NOINTERFACE;
    }

    NativeLoadData data;

    if (mLibraries.Get(hashedFile, &data)) {
        NS_ASSERTION(data.module, "Corrupt mLibraries hash");
        NS_ADDREF(*aResult = data.module);

        LOG(PR_LOG_DEBUG,
            ("nsNativeModuleLoader::LoadModule(\"%s\") - found in cache",
             filePath.get()));
        return NS_OK;
    }

    

    rv = aFile->Load(&data.library);

    if (NS_FAILED(rv)) {
        char errorMsg[1024] = "<unknown; can't get error from NSPR>";

        if (PR_GetErrorTextLength() < (int) sizeof(errorMsg))
            PR_GetErrorText(errorMsg);

        LOG(PR_LOG_ERROR,
            ("nsNativeModuleLoader::LoadModule(\"%s\") - load FAILED, "
             "rv: %lx, error:\n\t%s\n",
             filePath.get(), rv, errorMsg));

#ifdef DEBUG
        fprintf(stderr,
                "nsNativeModuleLoader::LoadModule(\"%s\") - load FAILED, "
                "rv: %lx, error:\n\t%s\n",
                filePath.get(), rv, errorMsg);
#endif

        return rv;
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    
    
    nsTraceRefcntImpl::LoadLibrarySymbols(filePath.get(), data.library);
#endif

#ifdef IMPLEMENT_BREAK_AFTER_LOAD
    nsCAutoString leafName;
    aFile->GetNativeLeafName(leafName);

    char *env = getenv("XPCOM_BREAK_ON_LOAD");
    char *blist;
    if (env && *env && (blist = strdup(env))) {
        char *nextTok = blist;
        while (char *token = NS_strtok(":", &nextTok)) {
            if (leafName.Find(token, PR_TRUE) != kNotFound) {
                NS_BREAK();
            }
        }

        free(blist);
    }
#endif

    nsGetModuleProc proc = (nsGetModuleProc)
        PR_FindFunctionSymbol(data.library, NS_GET_MODULE_SYMBOL);

    if (proc) {
        rv = proc(nsComponentManagerImpl::gComponentManager,
                  aFile,
                  getter_AddRefs(data.module));
        if (NS_SUCCEEDED(rv)) {
            LOG(PR_LOG_DEBUG,
                ("nsNativeModuleLoader::LoadModule(\"%s\") - Success",
                 filePath.get()));

            if (mLibraries.Put(hashedFile, data)) {
                NS_ADDREF(*aResult = data.module);
                return NS_OK;
            }
        }
        else {
            LOG(PR_LOG_WARNING,
                ("nsNativeModuleLoader::LoadModule(\"%s\") - "
                 "Call to NSGetModule failed, rv: %lx", filePath.get(), rv));
        }
    }
    else {
        LOG(PR_LOG_ERROR,
            ("nsNativeModuleLoader::LoadModule(\"%s\") - "
             "Symbol NSGetModule not found", filePath.get()));
    }

    
    data.module = nsnull;
    PR_UnloadLibrary(data.library);

    return NS_ERROR_FAILURE;
}

PLDHashOperator
nsNativeModuleLoader::ReleaserFunc(nsIHashable* aHashedFile,
                                   NativeLoadData& aLoadData, void*)
{
    aLoadData.module = nsnull;
    return PL_DHASH_NEXT;
}

PLDHashOperator
nsNativeModuleLoader::UnloaderFunc(nsIHashable* aHashedFile,
                                   NativeLoadData& aLoadData, void*)
{
    if (PR_LOG_TEST(nsNativeModuleLoaderLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIFile> file(do_QueryInterface(aHashedFile));

        nsCAutoString filePath;
        file->GetNativePath(filePath);

        LOG(PR_LOG_DEBUG,
            ("nsNativeModuleLoader::UnloaderFunc(\"%s\")", filePath.get()));
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(PR_FALSE);
#endif

#if 0
    
    
    PRStatus ret = PR_UnloadLibrary(aLoadData.library);
    NS_ASSERTION(ret == PR_SUCCESS, "Failed to unload library");
#endif

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(PR_TRUE);
#endif

    return PL_DHASH_REMOVE;
}

void
nsNativeModuleLoader::UnloadLibraries()
{
    mLibraries.Enumerate(ReleaserFunc, nsnull);
    mLibraries.Enumerate(UnloaderFunc, nsnull);
}
