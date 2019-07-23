





































#include "prtypes.h"
#include "prio.h"
#include "prtime.h"
#include "pldhash.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsAutoLock.h"
#include "nsCOMPtr.h"
#include "nsFastLoadFile.h"
#include "nsFastLoadService.h"
#include "nsString.h"

#include "nsIComponentManager.h"
#include "nsIEnumerator.h"
#include "nsIFastLoadFileControl.h"
#include "nsIFile.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsISeekableStream.h"
#include "nsISupports.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsFastLoadService, nsIFastLoadService)

nsFastLoadService::nsFastLoadService()
  : mLock(nsnull),
    mFastLoadPtrMap(nsnull),
    mDirection(0)
{
}

nsFastLoadService::~nsFastLoadService()
{
    if (mInputStream)
        mInputStream->Close();
    if (mOutputStream)
        mOutputStream->Close();

    if (mFastLoadPtrMap)
        PL_DHashTableDestroy(mFastLoadPtrMap);
    if (mLock)
        PR_DestroyLock(mLock);
}

NS_IMETHODIMP
nsFastLoadService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    *aResult = nsnull;
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsFastLoadService* fastLoadService = new nsFastLoadService();
    if (!fastLoadService)
        return NS_ERROR_OUT_OF_MEMORY;

    fastLoadService->mLock = PR_NewLock();
    if (!fastLoadService->mLock) {
        delete fastLoadService;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(fastLoadService);
    nsresult rv = fastLoadService->QueryInterface(aIID, aResult);
    NS_RELEASE(fastLoadService);
    return rv;
}

nsresult
nsFastLoadService::NewFastLoadFile(const char* aBaseName, nsIFile* *aResult)
{
    nsresult rv;

    
    
    nsCOMPtr<nsIFile> profFile;
    rv = NS_GetSpecialDirectory("ProfDS",
                                getter_AddRefs(profFile));
    if (NS_FAILED(rv)) {
        
        
        
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                    getter_AddRefs(profFile));
        if (NS_FAILED(rv)) {
            return rv;
        }
    }

    
    
    nsCOMPtr<nsIFile> file;
    rv = NS_GetSpecialDirectory("ProfLDS", getter_AddRefs(file));
    if (NS_FAILED(rv)) {
        
        
        
        rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                    getter_AddRefs(file));
    }
    if (NS_FAILED(rv))
        file = profFile;

    PRBool sameDir;
    rv = file->Equals(profFile, &sameDir);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString name(aBaseName);
    name += PLATFORM_FASL_SUFFIX;
    rv = file->AppendNative(name);
    if (NS_FAILED(rv))
        return rv;

    if (!sameDir) {
        
        
        
        rv = profFile->AppendNative(name);
        if (NS_SUCCEEDED(rv))
            profFile->Remove(PR_FALSE);  
    }

    *aResult = file;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::NewInputStream(nsIFile *aFile, nsIObjectInputStream* *aResult)
{
    nsAutoLock lock(mLock);

    nsCOMPtr<nsIObjectInputStream> stream;
    nsresult rv = NS_NewFastLoadFileReader(getter_AddRefs(stream), aFile);
    if (NS_FAILED(rv))
        return rv;

    mFileIO->DisableTruncate();
    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::NewOutputStream(nsIOutputStream* aDestStream,
                                   nsIObjectOutputStream* *aResult)
{
    nsAutoLock lock(mLock);

    return NS_NewFastLoadFileWriter(aResult, aDestStream, mFileIO);
}

NS_IMETHODIMP
nsFastLoadService::GetInputStream(nsIObjectInputStream* *aResult)
{
    NS_IF_ADDREF(*aResult = mInputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::SetInputStream(nsIObjectInputStream* aStream)
{
    nsAutoLock lock(mLock);
    mInputStream = aStream;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::GetOutputStream(nsIObjectOutputStream* *aResult)
{
    NS_IF_ADDREF(*aResult = mOutputStream);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::SetOutputStream(nsIObjectOutputStream* aStream)
{
    nsAutoLock lock(mLock);
    mOutputStream = aStream;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::GetFileIO(nsIFastLoadFileIO* *aResult)
{
    NS_IF_ADDREF(*aResult = mFileIO);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::SetFileIO(nsIFastLoadFileIO* aFileIO)
{
    nsAutoLock lock(mLock);
    mFileIO = aFileIO;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::GetDirection(PRInt32 *aResult)
{
    *aResult = mDirection;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::HasMuxedDocument(const char* aURISpec, PRBool *aResult)
{
    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    nsCOMPtr<nsIFastLoadFileControl> control;

    *aResult = PR_FALSE;
    nsAutoLock lock(mLock);

    if (mInputStream) {
        control = do_QueryInterface(mInputStream);
        if (control)
            rv = control->HasMuxedDocument(aURISpec, aResult);
    }

    if (! *aResult && mOutputStream) {
        control = do_QueryInterface(mOutputStream);
        if (control)
            rv = control->HasMuxedDocument(aURISpec, aResult);
    }

    return rv;
}

NS_IMETHODIMP
nsFastLoadService::StartMuxedDocument(nsISupports* aURI, const char* aURISpec,
                                      PRInt32 aDirectionFlags)
{
    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    nsCOMPtr<nsIFastLoadFileControl> control;
    nsAutoLock lock(mLock);

    
    
    if ((aDirectionFlags & NS_FASTLOAD_READ) && mInputStream) {
        control = do_QueryInterface(mInputStream);
        if (control) {
            
            
            rv = control->StartMuxedDocument(aURI, aURISpec);
            if (NS_SUCCEEDED(rv) || rv != NS_ERROR_NOT_AVAILABLE)
                return rv;

            
            
            if (!mOutputStream && mFileIO) {
                
                rv = NS_NewFastLoadFileUpdater(getter_AddRefs(mOutputStream),
                                               mFileIO, mInputStream);
                if (NS_FAILED(rv))
                    return rv;
            }

            if (aDirectionFlags == NS_FASTLOAD_READ) {
                
                
                return NS_ERROR_NOT_AVAILABLE;
            }
        }
    }

    if ((aDirectionFlags & NS_FASTLOAD_WRITE) && mOutputStream) {
        control = do_QueryInterface(mOutputStream);
        if (control)
            rv = control->StartMuxedDocument(aURI, aURISpec);
    }
    return rv;
}

NS_IMETHODIMP
nsFastLoadService::SelectMuxedDocument(nsISupports* aURI, nsISupports** aResult)
{
    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    nsCOMPtr<nsIFastLoadFileControl> control;
    nsAutoLock lock(mLock);

    
    
    if (mInputStream) {
        control = do_QueryInterface(mInputStream);
        if (control) {
            rv = control->SelectMuxedDocument(aURI, aResult);
            if (NS_SUCCEEDED(rv))
                mDirection = NS_FASTLOAD_READ;
        }
    }

    if (rv == NS_ERROR_NOT_AVAILABLE && mOutputStream) {
        control = do_QueryInterface(mOutputStream);
        if (control) {
            rv = control->SelectMuxedDocument(aURI, aResult);
            if (NS_SUCCEEDED(rv))
                mDirection = NS_FASTLOAD_WRITE;
        }
    }

    return rv;
}

NS_IMETHODIMP
nsFastLoadService::EndMuxedDocument(nsISupports* aURI)
{
    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    nsCOMPtr<nsIFastLoadFileControl> control;
    nsAutoLock lock(mLock);

    
    
    if (mInputStream) {
        control = do_QueryInterface(mInputStream);
        if (control)
            rv = control->EndMuxedDocument(aURI);
    }

    if (rv == NS_ERROR_NOT_AVAILABLE && mOutputStream) {
        control = do_QueryInterface(mOutputStream);
        if (control)
            rv = control->EndMuxedDocument(aURI);
    }

    mDirection = 0;
    return rv;
}

NS_IMETHODIMP
nsFastLoadService::AddDependency(nsIFile* aFile)
{
    nsAutoLock lock(mLock);

    nsCOMPtr<nsIFastLoadWriteControl> control(do_QueryInterface(mOutputStream));
    if (!control)
        return NS_ERROR_NOT_AVAILABLE;

    return control->AddDependency(aFile);
}

NS_IMETHODIMP
nsFastLoadService::ComputeChecksum(nsIFile* aFile,
                                   nsIFastLoadReadControl* aControl,
                                   PRUint32 *aChecksum)
{
    nsCAutoString path;
    nsresult rv = aFile->GetNativePath(path);
    if (NS_FAILED(rv))
        return rv;

    nsCStringKey key(path);
    PRUint32 checksum = NS_PTR_TO_INT32(mChecksumTable.Get(&key));
    if (checksum) {
        *aChecksum = checksum;
        return NS_OK;
    }

    rv = aControl->ComputeChecksum(&checksum);
    if (NS_FAILED(rv))
        return rv;

    mChecksumTable.Put(&key, NS_INT32_TO_PTR(checksum));
    *aChecksum = checksum;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::CacheChecksum(nsIFile* aFile, nsIObjectOutputStream *aStream)
{
    nsCOMPtr<nsIFastLoadFileControl> control(do_QueryInterface(aStream));
    if (!control)
        return NS_ERROR_FAILURE;

    PRUint32 checksum;
    nsresult rv = control->GetChecksum(&checksum);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString path;
    rv = aFile->GetNativePath(path);
    if (NS_FAILED(rv))
        return rv;

    nsCStringKey key(path);
    mChecksumTable.Put(&key, NS_INT32_TO_PTR(checksum));
    return NS_OK;
}

struct nsFastLoadPtrEntry : public PLDHashEntryHdr {
    nsISupports** mPtrAddr;     
    PRUint32      mOffset;
};

NS_IMETHODIMP
nsFastLoadService::GetFastLoadReferent(nsISupports* *aPtrAddr)
{
    NS_ASSERTION(*aPtrAddr == nsnull,
                 "aPtrAddr doesn't point to null nsFastLoadPtr<T>::mRawAddr?");

    nsAutoLock lock(mLock);
    if (!mFastLoadPtrMap || !mInputStream)
        return NS_OK;

    nsFastLoadPtrEntry* entry =
        static_cast<nsFastLoadPtrEntry*>
                   (PL_DHashTableOperate(mFastLoadPtrMap, aPtrAddr,
                                            PL_DHASH_LOOKUP));
    if (PL_DHASH_ENTRY_IS_FREE(entry))
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(mInputStream));

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, entry->mOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = mInputStream->ReadObject(PR_TRUE, aPtrAddr);
    if (NS_FAILED(rv))
        return rv;

    
    PRUint32 size = PL_DHASH_TABLE_SIZE(mFastLoadPtrMap);
    if (mFastLoadPtrMap->removedCount >= (size >> 2))
        PL_DHashTableOperate(mFastLoadPtrMap, entry, PL_DHASH_REMOVE);
    else
        PL_DHashTableRawRemove(mFastLoadPtrMap, entry);

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::ReadFastLoadPtr(nsIObjectInputStream* aInputStream,
                                   nsISupports* *aPtrAddr)
{
    
    
    if (*aPtrAddr)
        return NS_OK;

    nsresult rv;
    PRUint32 nextOffset;
    nsAutoLock lock(mLock);

    rv = aInputStream->Read32(&nextOffset);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(aInputStream));
    if (!seekable)
        return NS_ERROR_FAILURE;

    PRInt64 thisOffset;
    rv = seekable->Tell(&thisOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, nextOffset);
    if (NS_FAILED(rv))
        return rv;

    if (!mFastLoadPtrMap) {
        mFastLoadPtrMap = PL_NewDHashTable(PL_DHashGetStubOps(), this,
                                           sizeof(nsFastLoadPtrEntry),
                                           PL_DHASH_MIN_SIZE);
        if (!mFastLoadPtrMap)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    nsFastLoadPtrEntry* entry =
        static_cast<nsFastLoadPtrEntry*>
                   (PL_DHashTableOperate(mFastLoadPtrMap, aPtrAddr,
                                            PL_DHASH_ADD));
    NS_ASSERTION(entry->mPtrAddr == nsnull, "duplicate nsFastLoadPtr?!");

    entry->mPtrAddr = aPtrAddr;

    LL_L2UI(entry->mOffset, thisOffset);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadService::WriteFastLoadPtr(nsIObjectOutputStream* aOutputStream,
                                    nsISupports* aObject)
{
    NS_ASSERTION(aObject != nsnull, "writing an unread nsFastLoadPtr?!");
    if (!aObject)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;
    nsAutoLock lock(mLock);     

    nsCOMPtr<nsISeekableStream> seekable(do_QueryInterface(aOutputStream));
    if (!seekable)
        return NS_ERROR_FAILURE;

    PRInt64 saveOffset;
    rv = seekable->Tell(&saveOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = aOutputStream->Write32(0);       
    if (NS_FAILED(rv))
        return rv;

    rv = aOutputStream->WriteObject(aObject, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    PRInt64 nextOffset;
    rv = seekable->Tell(&nextOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, saveOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = aOutputStream->Write32(nextOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, nextOffset);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}
