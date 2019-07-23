





































#include <string.h>
#include "prtypes.h"
#include "nscore.h"
#include "nsDebug.h"
#include "nsEnumeratorUtils.h"
#include "nsMemory.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsReadableUtils.h"

#include "nsIComponentManager.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsISeekableStream.h"
#include "nsISerializable.h"
#include "nsIStreamBufferAccess.h"
#include "nsIClassInfo.h"

#include "nsBinaryStream.h"
#include "nsFastLoadFile.h"
#include "nsInt64.h"
#ifdef XP_UNIX
#include <sys/mman.h>
#endif

#ifdef DEBUG_brendan
# define METERING
# define DEBUG_MUX
#endif

#ifdef METERING
# define METER(x)       x
#else
# define METER(x)
#endif

#ifdef DEBUG_MUX
# include <stdio.h>
# include <stdarg.h>

static void trace_mux(char mode, const char *format, ...)
{
    va_list ap;
    static FILE *tfp;
    if (!tfp) {
        char tfn[16];
        sprintf(tfn, "/tmp/mux.%ctrace", mode);
        tfp = fopen(tfn, "w");
        if (!tfp)
            return;
        setvbuf(tfp, NULL, _IOLBF, 0);
    }
    va_start(ap, format);
    vfprintf(tfp, format, ap);
    va_end(ap);
}

# define TRACE_MUX(args) trace_mux args
#else
# define TRACE_MUX(args)
#endif




#define FOLD_ONES_COMPLEMENT_CARRY(X)   ((X) = ((X) & 0xffff) + ((X) >> 16))
#define ONES_COMPLEMENT_ACCUMULATE(X,Y) (X) += (Y); if ((X) & 0x80000000)     \
                                        FOLD_ONES_COMPLEMENT_CARRY(X)
#define FLETCHER_ACCUMULATE(A,B,U)      ONES_COMPLEMENT_ACCUMULATE(A, U);     \
                                        ONES_COMPLEMENT_ACCUMULATE(B, A)

PRUint32
NS_AccumulateFastLoadChecksum(PRUint32 *aChecksum,
                              const PRUint8* aBuffer,
                              PRUint32 aLength,
                              PRBool aLastBuffer)
{
    PRUint32 C = *aChecksum;
    PRUint32 A = C & 0xffff;
    PRUint32 B = C >> 16;

    PRUint16 U = 0;
    if (aLength >= 4) {
        PRBool odd = PRWord(aBuffer) & 1;
        switch (PRWord(aBuffer) & 3) {
          case 3:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = aBuffer[2];
            aBuffer += 3;
            aLength -= 3;
            break;

          case 2:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = 0;
            aBuffer += 2;
            aLength -= 2;
            break;

          case 1:
            U = *aBuffer++;
            aLength--;
            break;
        }

        PRUint32 W;
        if (odd) {
            while (aLength > 3) {
                W = *reinterpret_cast<const PRUint32*>(aBuffer);
                U <<= 8;
#ifdef IS_BIG_ENDIAN
                U |= W >> 24;
                FLETCHER_ACCUMULATE(A, B, U);
                U = PRUint16(W >> 8);
                FLETCHER_ACCUMULATE(A, B, U);
                U = W & 0xff;
#else
                U |= W & 0xff;
                FLETCHER_ACCUMULATE(A, B, U);
                U = PRUint16(W >> 8);
                U = NS_SWAP16(U);
                FLETCHER_ACCUMULATE(A, B, U);
                U = W >> 24;
#endif
                aBuffer += 4;
                aLength -= 4;
            }
            aBuffer--;      
            aLength++;
        } else {
            while (aLength > 3) {
                W = *reinterpret_cast<const PRUint32*>(aBuffer);
#ifdef IS_BIG_ENDIAN
                U = W >> 16;
                FLETCHER_ACCUMULATE(A, B, U);
                U = PRUint16(W);
                FLETCHER_ACCUMULATE(A, B, U);
#else
                U = NS_SWAP16(W);
                FLETCHER_ACCUMULATE(A, B, U);
                U = W >> 16;
                U = NS_SWAP16(W);
                FLETCHER_ACCUMULATE(A, B, U);
#endif
                aBuffer += 4;
                aLength -= 4;
            }
        }
    }

    if (aLastBuffer) {
        NS_ASSERTION(aLength <= 4, "aLength botch");
        switch (aLength) {
          case 4:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = (aBuffer[2] << 8) | aBuffer[3];
            FLETCHER_ACCUMULATE(A, B, U);
            break;

          case 3:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            U = aBuffer[2];
            FLETCHER_ACCUMULATE(A, B, U);
            break;

          case 2:
            U = (aBuffer[0] << 8) | aBuffer[1];
            FLETCHER_ACCUMULATE(A, B, U);
            break;

          case 1:
            U = aBuffer[0];
            FLETCHER_ACCUMULATE(A, B, U);
            break;
        }

        aLength = 0;
    }

    while (A >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(A);
    while (B >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(B);

    *aChecksum = (B << 16) | A;
    return aLength;
}

PRUint32
NS_AddFastLoadChecksums(PRUint32 sum1, PRUint32 sum2, PRUint32 sum2ByteCount)
{
    PRUint32 A1 = sum1 & 0xffff;
    PRUint32 B1 = sum1 >> 16;

    PRUint32 A2 = sum2 & 0xffff;
    PRUint32 B2 = sum2 >> 16;

    PRUint32 A = A1 + A2;
    while (A >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(A);

    PRUint32 B = B2;
    for (PRUint32 n = (sum2ByteCount + 1) / 2; n != 0; n--)
        ONES_COMPLEMENT_ACCUMULATE(B, B1);
    while (B >> 16)
        FOLD_ONES_COMPLEMENT_CARRY(B);

    return (B << 16) | A;
}

#undef FOLD_ONES_COMPLEMENT_CARRY
#undef ONES_COMPLEMENT_ACCUMULATE
#undef FLETCHER_ACCUMULATE

static const char magic[] = MFL_FILE_MAGIC;



nsID nsFastLoadFileReader::nsFastLoadFooter::gDummyID;
nsFastLoadFileReader::nsObjectMapEntry
    nsFastLoadFileReader::nsFastLoadFooter::gDummySharpObjectEntry;

NS_IMPL_ISUPPORTS_INHERITED5(nsFastLoadFileReader,
                             nsBinaryInputStream,
                             nsIObjectInputStream,
                             nsIFastLoadFileControl,
                             nsIFastLoadReadControl,
                             nsISeekableStream,
                             nsIFastLoadFileReader)

nsresult
nsFastLoadFileReader::ReadHeader(nsFastLoadHeader *aHeader)
{
    nsresult rv;
    PRUint32 bytesRead;

    rv = Read(reinterpret_cast<char*>(aHeader), sizeof *aHeader, &bytesRead);
    if (NS_FAILED(rv))
        return rv;

    if (bytesRead != sizeof *aHeader ||
        memcmp(aHeader->mMagic, magic, MFL_FILE_MAGIC_SIZE)) {
        return NS_ERROR_UNEXPECTED;
    }

    aHeader->mChecksum     = NS_SWAP32(aHeader->mChecksum);
    aHeader->mVersion      = NS_SWAP32(aHeader->mVersion);
    aHeader->mFooterOffset = NS_SWAP32(aHeader->mFooterOffset);
    aHeader->mFileSize     = NS_SWAP32(aHeader->mFileSize);

    return NS_OK;
}



NS_IMETHODIMP
nsFastLoadFileReader::GetChecksum(PRUint32 *aChecksum)
{
    *aChecksum = mHeader.mChecksum;
    return NS_OK;
}

struct nsStringMapEntry : public PLDHashEntryHdr {
    const char*     mString;            
    nsISupports*    mURI;               
};

struct nsDocumentMapEntry : public nsStringMapEntry {
    PRUint32    mInitialSegmentOffset;  
};

struct nsDocumentMapReadEntry : public nsDocumentMapEntry {
    PRUint32    mNextSegmentOffset;     
    PRUint32    mBytesLeft : 31,        
                mNeedToSeek : 1;        
                                        
                                        
                                        
    PRInt64     mSaveOffset;            
                                        
};

static void
strmap_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsStringMapEntry* entry = static_cast<nsStringMapEntry*>(aHdr);

    if (entry->mString)
        nsMemory::Free((void*) entry->mString);
    NS_IF_RELEASE(entry->mURI);
    PL_DHashClearEntryStub(aTable, aHdr);
}

static const PLDHashTableOps strmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashStringKey,
    PL_DHashMatchStringKey,
    PL_DHashMoveEntryStub,
    strmap_ClearEntry,
    PL_DHashFinalizeStub,
    NULL
};




struct nsObjectMapEntry : public PLDHashEntryHdr {
    nsISupports*            mObject;        
};


struct nsURIMapReadEntry : public nsObjectMapEntry {
    nsDocumentMapReadEntry* mDocMapEntry;
};

static void
objmap_ClearEntry(PLDHashTable *aTable, PLDHashEntryHdr *aHdr)
{
    nsObjectMapEntry* entry = static_cast<nsObjectMapEntry*>(aHdr);

    
    
    if ((NS_PTR_TO_INT32(entry->mObject) & MFL_OBJECT_DEF_TAG) == 0)
        NS_IF_RELEASE(entry->mObject);
    PL_DHashClearEntryStub(aTable, aHdr);
}

static const PLDHashTableOps objmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    objmap_ClearEntry,
    PL_DHashFinalizeStub,
    NULL
};

NS_IMETHODIMP
nsFastLoadFileReader::HasMuxedDocument(const char* aURISpec, PRBool *aResult)
{
    nsDocumentMapReadEntry* docMapEntry =
        static_cast<nsDocumentMapReadEntry*>
                   (PL_DHashTableOperate(&mFooter.mDocumentMap, aURISpec,
                                         PL_DHASH_LOOKUP));

    *aResult = PL_DHASH_ENTRY_IS_BUSY(docMapEntry);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::StartMuxedDocument(nsISupports* aURI, const char* aURISpec)
{
    nsDocumentMapReadEntry* docMapEntry =
        static_cast<nsDocumentMapReadEntry*>
                   (PL_DHashTableOperate(&mFooter.mDocumentMap, aURISpec,
                                         PL_DHASH_LOOKUP));

    
    
    if (PL_DHASH_ENTRY_IS_FREE(docMapEntry))
        return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapReadEntry* uriMapEntry =
        static_cast<nsURIMapReadEntry*>
                   (PL_DHashTableOperate(&mFooter.mURIMap, key,
                                         PL_DHASH_ADD));
    if (!uriMapEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(uriMapEntry->mDocMapEntry == nsnull,
                 "URI mapped to two different specs?");
    if (uriMapEntry->mDocMapEntry)
        return NS_ERROR_UNEXPECTED;

    docMapEntry->mURI = aURI;
    NS_ADDREF(docMapEntry->mURI);
    uriMapEntry->mObject = key;
    NS_ADDREF(uriMapEntry->mObject);
    uriMapEntry->mDocMapEntry = docMapEntry;
    TRACE_MUX(('r', "start %p (%p) %s\n", aURI, key.get(), aURISpec));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::SelectMuxedDocument(nsISupports* aURI,
                                          nsISupports** aResult)
{
    nsresult rv;

    
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapReadEntry* uriMapEntry =
        static_cast<nsURIMapReadEntry*>
                   (PL_DHashTableOperate(&mFooter.mURIMap, key,
                                         PL_DHASH_LOOKUP));

    
    
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    nsDocumentMapReadEntry* prevDocMapEntry = mCurrentDocumentMapEntry;
    if (prevDocMapEntry &&
        prevDocMapEntry->mBytesLeft &&
        !prevDocMapEntry->mNeedToSeek) {
        rv = Tell(&prevDocMapEntry->mSaveOffset);
        if (NS_FAILED(rv))
            return rv;
    }

    
    
    
    
    nsDocumentMapReadEntry* docMapEntry = uriMapEntry->mDocMapEntry;
    if (docMapEntry == prevDocMapEntry) {
        TRACE_MUX(('r', "select prev %s same as current!\n",
                   docMapEntry->mString));
    }

    
    
    else if (docMapEntry->mBytesLeft) {
        NS_ASSERTION(docMapEntry->mSaveOffset != 0,
                     "reselecting from multiplex at unsaved offset?");

        
        
        
        
        docMapEntry->mNeedToSeek = PR_TRUE;
    }

    *aResult = prevDocMapEntry ? prevDocMapEntry->mURI : nsnull;
    NS_IF_ADDREF(*aResult);

    mCurrentDocumentMapEntry = docMapEntry;
#ifdef DEBUG_MUX
    PRInt64 currentSegmentOffset;
    Tell(&currentSegmentOffset);
    trace_mux('r', "select %p (%p) offset %ld\n",
              aURI, key.get(), (long) currentSegmentOffset);
#endif
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::EndMuxedDocument(nsISupports* aURI)
{
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapReadEntry* uriMapEntry =
        static_cast<nsURIMapReadEntry*>
                   (PL_DHashTableOperate(&mFooter.mURIMap, key,
                                         PL_DHASH_LOOKUP));

    
    
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry))
        return NS_ERROR_NOT_AVAILABLE;

    
    
    if (uriMapEntry->mDocMapEntry)
        NS_RELEASE(uriMapEntry->mDocMapEntry->mURI);

    
    PRUint32 size = PL_DHASH_TABLE_SIZE(&mFooter.mURIMap);
    if (mFooter.mURIMap.removedCount >= (size >> 2))
        PL_DHashTableOperate(&mFooter.mURIMap, key, PL_DHASH_REMOVE);
    else
        PL_DHashTableRawRemove(&mFooter.mURIMap, uriMapEntry);

    TRACE_MUX(('r', "end %p (%p)\n", aURI, key.get()));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aBytesRead)
{
    nsDocumentMapReadEntry* entry = mCurrentDocumentMapEntry;
    if (entry) {
        
        if (entry->mNeedToSeek) {
            SeekTo(entry->mSaveOffset);
            entry->mNeedToSeek = PR_FALSE;
        }

        
        
        
        
        while (entry->mBytesLeft == 0) {
            
            NS_ASSERTION(entry->mNextSegmentOffset != 0,
                         "document demuxed from FastLoad file more than once?");
            if (entry->mNextSegmentOffset == 0)
                return NS_ERROR_UNEXPECTED;

            SeekTo(entry->mNextSegmentOffset);
            
            mCurrentDocumentMapEntry = nsnull;

            nsresult rv = Read32(&entry->mNextSegmentOffset);
            if (NS_SUCCEEDED(rv)) {
                PRUint32 bytesLeft = 0;
                rv = Read32(&bytesLeft);
                entry->mBytesLeft = bytesLeft;
            }

            mCurrentDocumentMapEntry = entry;
            if (NS_FAILED(rv))
                return rv;

            NS_ASSERTION(entry->mBytesLeft >= 8, "demux segment length botch!");
            entry->mBytesLeft -= 8;
        }
    }
    PRUint32 count = PR_MIN(mFileLen - mFilePos, aCount);
    memcpy(aBuffer, mFileData+mFilePos, count);
    *aBytesRead = count;
    mFilePos += count;
    if (entry) {
        NS_ASSERTION(entry->mBytesLeft >= *aBytesRead, "demux Read underflow!");
        entry->mBytesLeft -= *aBytesRead;

#ifdef NS_DEBUG
        
        if (entry->mBytesLeft == 0)
            entry->mSaveOffset = 0;
#endif
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                                   PRUint32 aCount, PRUint32 *aResult)
{
    nsDocumentMapReadEntry* entry = mCurrentDocumentMapEntry;

    NS_ASSERTION(!entry || (!entry->mNeedToSeek && entry->mBytesLeft != 0),
                 "ReadSegments called from above nsFastLoadFileReader layer?!");

    PRUint32 count = PR_MIN(mFileLen - mFilePos, aCount);

    
    aWriter(this, aClosure, (char*)(mFileData + mFilePos), 0,
            count, aResult);
    mFilePos += count;
    if (entry) {
        NS_ASSERTION(entry->mBytesLeft >= *aResult,
                     "demux ReadSegments underflow!");
        entry->mBytesLeft -= *aResult;

#ifdef NS_DEBUG
        
        if (entry->mBytesLeft == 0)
            entry->mSaveOffset = 0;
#endif
    }
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::ComputeChecksum(PRUint32 *aResult)
{
    PRUint32 checksum = 0;
    
    PRUint32 pos = offsetof(nsFastLoadHeader, mVersion);
    NS_AccumulateFastLoadChecksum(&checksum,
                                  mFileData + pos,
                                  mFileLen - pos,
                                  PR_TRUE);
    *aResult = checksum;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::GetDependencies(nsISimpleEnumerator* *aDependencies)
{
    return NS_NewArrayEnumerator(aDependencies, mFooter.mDependencies);
}

nsresult
nsFastLoadFileReader::ReadFooter(nsFastLoadFooter *aFooter)
{
    nsresult rv;

    rv = ReadFooterPrefix(aFooter);
    if (NS_FAILED(rv))
        return rv;

    aFooter->mIDMap = new nsID[aFooter->mNumIDs];
    if (!aFooter->mIDMap)
        return NS_ERROR_OUT_OF_MEMORY;

    PRUint32 i, n;
    for (i = 0, n = aFooter->mNumIDs; i < n; i++) {
        rv = ReadSlowID(&aFooter->mIDMap[i]);
        if (NS_FAILED(rv))
            return rv;
    }

    aFooter->mObjectMap = new nsObjectMapEntry[aFooter->mNumSharpObjects];
    if (!aFooter->mObjectMap)
        return NS_ERROR_OUT_OF_MEMORY;

    for (i = 0, n = aFooter->mNumSharpObjects; i < n; i++) {
        nsObjectMapEntry* entry = &aFooter->mObjectMap[i];

        rv = ReadSharpObjectInfo(entry);
        if (NS_FAILED(rv))
            return rv;

        entry->mReadObject = nsnull;
        entry->mSkipOffset = 0;
        entry->mSaveStrongRefCnt = entry->mStrongRefCnt;
        entry->mSaveWeakRefCnt = entry->mWeakRefCnt;
    }

    if (!PL_DHashTableInit(&aFooter->mDocumentMap, &strmap_DHashTableOps,
                           (void *)this, sizeof(nsDocumentMapReadEntry),
                           aFooter->mNumMuxedDocuments)) {
        aFooter->mDocumentMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&aFooter->mURIMap, &objmap_DHashTableOps,
                           (void *)this, sizeof(nsURIMapReadEntry),
                           aFooter->mNumMuxedDocuments)) {
        aFooter->mURIMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0, n = aFooter->mNumMuxedDocuments; i < n; i++) {
        nsFastLoadMuxedDocumentInfo info;

        rv = ReadMuxedDocumentInfo(&info);
        if (NS_FAILED(rv))
            return rv;

        nsDocumentMapReadEntry* entry =
            static_cast<nsDocumentMapReadEntry*>
                       (PL_DHashTableOperate(&aFooter->mDocumentMap,
                                             info.mURISpec,
                                             PL_DHASH_ADD));
        if (!entry) {
            nsMemory::Free((void*) info.mURISpec);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        NS_ASSERTION(!entry->mString, "duplicate URISpec in MuxedDocumentMap");
        entry->mString = info.mURISpec;
        entry->mURI = nsnull;
        entry->mInitialSegmentOffset = info.mInitialSegmentOffset;
        entry->mNextSegmentOffset = info.mInitialSegmentOffset;
        entry->mBytesLeft = 0;
        entry->mNeedToSeek = PR_FALSE;
        entry->mSaveOffset = 0;
    }

    nsCOMPtr<nsISupportsArray> readDeps;
    rv = NS_NewISupportsArray(getter_AddRefs(readDeps));
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString filename;
    for (i = 0, n = aFooter->mNumDependencies; i < n; i++) {
        rv = ReadCString(filename);
        if (NS_FAILED(rv))
            return rv;

        PRInt64 fastLoadMtime;
        rv = Read64(reinterpret_cast<PRUint64*>(&fastLoadMtime));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsILocalFile> file;
        rv = NS_NewNativeLocalFile(filename, PR_TRUE, getter_AddRefs(file));
        if (NS_FAILED(rv))
            return rv;
#ifdef DEBUG
        PRInt64 currentMtime;
        rv = file->GetLastModifiedTime(&currentMtime);
        if (NS_FAILED(rv))
            return rv;

        if (LL_NE(fastLoadMtime, currentMtime)) {
            nsCAutoString path;
            file->GetNativePath(path);
            printf("%s mtime changed, invalidating FastLoad file\n",
                   path.get());
            return NS_ERROR_FAILURE;
        }
#endif

        rv = readDeps->AppendElement(file);
        if (NS_FAILED(rv))
            return rv;
    }

    aFooter->mDependencies = readDeps;
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadFooterPrefix(nsFastLoadFooterPrefix *aFooterPrefix)
{
    nsresult rv;

    rv = Read32(&aFooterPrefix->mNumIDs);
    if (NS_FAILED(rv))
        return rv;

    rv = Read32(&aFooterPrefix->mNumSharpObjects);
    if (NS_FAILED(rv))
        return rv;

    rv = Read32(&aFooterPrefix->mNumMuxedDocuments);
    if (NS_FAILED(rv))
        return rv;

    rv = Read32(&aFooterPrefix->mNumDependencies);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadSlowID(nsID *aID)
{
    nsresult rv;

    rv = Read32(&aID->m0);
    if (NS_FAILED(rv))
        return rv;

    rv = Read16(&aID->m1);
    if (NS_FAILED(rv))
        return rv;

    rv = Read16(&aID->m2);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 bytesRead;
    rv = Read(reinterpret_cast<char*>(aID->m3), sizeof aID->m3, &bytesRead);
    if (NS_FAILED(rv))
        return rv;

    if (bytesRead != sizeof aID->m3)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadFastID(NSFastLoadID *aID)
{
    nsresult rv = Read32(aID);
    if (NS_SUCCEEDED(rv))
        *aID ^= MFL_ID_XOR_KEY;
    return rv;
}

nsresult
nsFastLoadFileReader::ReadSharpObjectInfo(nsFastLoadSharpObjectInfo *aInfo)
{
    nsresult rv;

    rv = Read32(&aInfo->mCIDOffset);
    if (NS_FAILED(rv))
        return rv;

    NS_ASSERTION(aInfo->mCIDOffset != 0,
                 "fastload reader: mCIDOffset cannot be zero!");

    rv = Read16(&aInfo->mStrongRefCnt);
    if (NS_FAILED(rv))
        return rv;

    rv = Read16(&aInfo->mWeakRefCnt);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadMuxedDocumentInfo(nsFastLoadMuxedDocumentInfo *aInfo)
{
    nsresult rv;

    nsCAutoString spec;
    rv = ReadCString(spec);
    if (NS_FAILED(rv))
        return rv;

    rv = Read32(&aInfo->mInitialSegmentOffset);
    if (NS_FAILED(rv))
        return rv;

    aInfo->mURISpec = ToNewCString(spec);
    return NS_OK;
}

nsresult
nsFastLoadFileReader::Open()
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(mFile, &rv);
    if (NS_FAILED(rv))
        return rv;
    rv = localFile->OpenNSPRFileDesc(PR_RDONLY, 0, &mFd);
    if (NS_FAILED(rv))
        return rv;

    PRInt64 size = PR_Available64(mFd);
    if (size >= PR_INT32_MAX)
        return NS_ERROR_FILE_TOO_BIG;

    mFileLen = (PRUint32) size;

    mFileMap = PR_CreateFileMap(mFd, mFileLen, PR_PROT_READONLY);
    if (!mFileMap)
        return NS_ERROR_FAILURE;

    mFileData = (PRUint8*) PR_MemMap(mFileMap, 0, mFileLen);

    if (mFileLen < sizeof(nsFastLoadHeader))
        return NS_ERROR_FAILURE;
    
#if defined(XP_UNIX) && !defined(SOLARIS)
    madvise(mFileData, mFileLen, MADV_WILLNEED);
#endif

    rv = ReadHeader(&mHeader);
    if (NS_FAILED(rv))
        return rv;

    if (mHeader.mVersion != MFL_FILE_VERSION ||
        mHeader.mFooterOffset == 0 || 
        memcmp(mHeader.mMagic, magic, MFL_FILE_MAGIC_SIZE))
        return NS_ERROR_UNEXPECTED;
    
    SeekTo(mHeader.mFooterOffset);

    rv = ReadFooter(&mFooter);
    if (NS_FAILED(rv))
        return rv;

    SeekTo(sizeof(nsFastLoadHeader));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Close()
{
    
    
    
    
    
    
    
    
    
    if (mFd) {
        if (mFileData)
            PR_MemUnmap(mFileData, mFileLen);
        mFileData = nsnull;
        if (mFileMap)
            PR_CloseFileMap(mFileMap);
        mFileMap = nsnull;
        PR_Close(mFd);
        mFd = nsnull;
    }
    
    if (!mFooter.mObjectMap)
        return NS_OK;

    for (PRUint32 i = 0, n = mFooter.mNumSharpObjects; i < n; i++) {
        nsObjectMapEntry* entry = &mFooter.mObjectMap[i];
        entry->mReadObject = nsnull;
    }
    mFooter.mNumSharpObjects = 0;

    return NS_OK;
}

nsresult
nsFastLoadFileReader::DeserializeObject(nsISupports* *aObject)
{
    nsresult rv;
    NSFastLoadID fastCID;

    rv = ReadFastID(&fastCID);
    if (NS_FAILED(rv))
        return rv;

    const nsID& slowCID = mFooter.GetID(fastCID);
    nsCOMPtr<nsISupports> object(do_CreateInstance(slowCID, &rv));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISerializable> serializable(do_QueryInterface(object));
    if (!serializable)
        return NS_ERROR_FAILURE;

    rv = serializable->Read(this);
    if (NS_FAILED(rv))
        return rv;

    *aObject = object;
    NS_ADDREF(*aObject);
    return NS_OK;
}

nsresult
nsFastLoadFileReader::ReadObject(PRBool aIsStrongRef, nsISupports* *aObject)
{
    nsresult rv;
    NSFastLoadOID oid;

    rv = Read32(&oid);
    if (NS_FAILED(rv))
        return rv;
    oid ^= MFL_OID_XOR_KEY;

    nsCOMPtr<nsISupports> object;

    if (oid == MFL_DULL_OBJECT_OID) {
        
        NS_ASSERTION(aIsStrongRef, "dull object read via weak ref!");

        rv = DeserializeObject(getter_AddRefs(object));
        if (NS_FAILED(rv))
            return rv;
    } else {
        NS_ASSERTION((oid & MFL_WEAK_REF_TAG) ==
                     (aIsStrongRef ? 0 : MFL_WEAK_REF_TAG),
                     "strong vs. weak ref deserialization mismatch!");

        nsObjectMapEntry* entry = &mFooter.GetSharpObjectEntry(oid);

        
        object = entry->mReadObject;
        if (!object) {
            nsDocumentMapReadEntry* saveDocMapEntry = nsnull;

            PRUint32 saveOffset32 = mFilePos;
            if (entry->mCIDOffset != saveOffset32) {
                
                
                
                
                
                
                NS_ASSERTION(entry->mCIDOffset < saveOffset32,
                             "out of order object?!");

                
                
                
                saveDocMapEntry = mCurrentDocumentMapEntry;
                mCurrentDocumentMapEntry = nsnull;
                SeekTo(entry->mCIDOffset);
            }

            rv = DeserializeObject(getter_AddRefs(object));
            if (NS_FAILED(rv))
                return rv;

            if (entry->mCIDOffset != saveOffset32) {
                
                
                entry->mSkipOffset = mFilePos;
                
                
                
                SeekTo(saveOffset32);
                mCurrentDocumentMapEntry = saveDocMapEntry;
            }

            
            entry->mReadObject = object;
        } else {
            
            
            
            
            if (oid & MFL_OBJECT_DEF_TAG) {
                NS_ASSERTION(entry->mSkipOffset != 0, "impossible! see above");

                
                
                
                PRUint32 currentOffset = mFilePos;
                NS_ASSERTION(entry->mSkipOffset > (PRUint32)currentOffset,
                             "skipping backwards from object?!");
                NS_ASSERTION(mCurrentDocumentMapEntry->mBytesLeft >=
                             entry->mSkipOffset - (PRUint32)currentOffset,
                             "skipped object buffer underflow!");

                mCurrentDocumentMapEntry->mBytesLeft -=
                    entry->mSkipOffset - (PRUint32)currentOffset;

                SeekTo(entry->mSkipOffset);
            }
        }

        if (aIsStrongRef) {
            NS_ASSERTION(entry->mStrongRefCnt != 0,
                         "mStrongRefCnt underflow!");
            --entry->mStrongRefCnt;
        } else {
            NS_ASSERTION(MFL_GET_WEAK_REFCNT(entry) != 0,
                         "mWeakRefCnt underflow!");
            MFL_DROP_WEAK_REFCNT(entry);
        }

        if (entry->mStrongRefCnt == 0 && MFL_GET_WEAK_REFCNT(entry) == 0)
            entry->mReadObject = nsnull;
    }

    if (oid & MFL_QUERY_INTERFACE_TAG) {
        NSFastLoadID iid;
        rv = ReadFastID(&iid);
        if (NS_FAILED(rv))
            return rv;

        rv = object->QueryInterface(mFooter.GetID(iid),
                                    reinterpret_cast<void**>(aObject));
        if (NS_FAILED(rv))
            return rv;
    } else {
        *aObject = object;
        NS_ADDREF(*aObject);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::ReadID(nsID *aResult)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = ReadFastID(&fastID);
    if (NS_FAILED(rv))
        return rv;

    *aResult = mFooter.GetID(fastID);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
    NS_ASSERTION(aWhence == nsISeekableStream::NS_SEEK_SET, "Only NS_SEEK_SET seeks are supported");
    mCurrentDocumentMapEntry = nsnull;
    SeekTo(aOffset);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::Tell(PRInt64 *aResult)
{
    *aResult = mFilePos;
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileReader::SetEOF()
{
    NS_ERROR("Refusing to truncate a memory-mapped file");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_COM nsresult
NS_NewFastLoadFileReader(nsIObjectInputStream* *aResult, nsIFile *aFile)
{
    nsFastLoadFileReader* reader = new nsFastLoadFileReader(aFile);
    if (!reader)
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsCOMPtr<nsIObjectInputStream> stream(reader);

    nsresult rv = reader->Open();
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}



NS_IMPL_ISUPPORTS_INHERITED4(nsFastLoadFileWriter,
                             nsBinaryOutputStream,
                             nsIObjectOutputStream,
                             nsIFastLoadFileControl,
                             nsIFastLoadWriteControl,
                             nsISeekableStream)

struct nsIDMapEntry : public PLDHashEntryHdr {
    NSFastLoadID    mFastID;            
    nsID            mSlowID;            
};

static PLDHashNumber
idmap_HashKey(PLDHashTable *aTable, const void *aKey)
{
    const nsID *idp = reinterpret_cast<const nsID*>(aKey);

    return idp->m0;
}

static PRBool
idmap_MatchEntry(PLDHashTable *aTable,
                const PLDHashEntryHdr *aHdr,
                const void *aKey)
{
    const nsIDMapEntry* entry = static_cast<const nsIDMapEntry*>(aHdr);
    const nsID *idp = reinterpret_cast<const nsID*>(aKey);

    return memcmp(&entry->mSlowID, idp, sizeof(nsID)) == 0;
}

static const PLDHashTableOps idmap_DHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    idmap_HashKey,
    idmap_MatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    NULL
};

nsresult
nsFastLoadFileWriter::MapID(const nsID& aSlowID, NSFastLoadID *aResult)
{
    nsIDMapEntry* entry =
        static_cast<nsIDMapEntry*>
                   (PL_DHashTableOperate(&mIDMap, &aSlowID, PL_DHASH_ADD));
    if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

    if (entry->mFastID == 0) {
        entry->mFastID = mIDMap.entryCount;
        entry->mSlowID = aSlowID;
    }

    *aResult = entry->mFastID;
    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteHeader(nsFastLoadHeader *aHeader)
{
    nsresult rv;
    PRUint32 bytesWritten;

    rv = Write(aHeader->mMagic, MFL_FILE_MAGIC_SIZE, &bytesWritten);
    if (NS_FAILED(rv))
        return rv;

    if (bytesWritten != MFL_FILE_MAGIC_SIZE)
        return NS_ERROR_FAILURE;

    rv = Write32(aHeader->mChecksum);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aHeader->mVersion);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aHeader->mFooterOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aHeader->mFileSize);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}



NS_IMETHODIMP
nsFastLoadFileWriter::GetChecksum(PRUint32 *aChecksum)
{
    if (mHeader.mChecksum == 0)
        return NS_ERROR_NOT_AVAILABLE;
    *aChecksum = mHeader.mChecksum;
    return NS_OK;
}

struct nsDocumentMapWriteEntry : public nsDocumentMapEntry {
    PRUint32    mCurrentSegmentOffset;      
};





struct nsURIMapWriteEntry : public nsObjectMapEntry {
    nsDocumentMapWriteEntry* mDocMapEntry;
    PRUint32                 mGeneration;
    const char*              mURISpec;
};

NS_IMETHODIMP
nsFastLoadFileWriter::HasMuxedDocument(const char* aURISpec, PRBool *aResult)
{
    nsDocumentMapWriteEntry* docMapEntry =
        static_cast<nsDocumentMapWriteEntry*>
                   (PL_DHashTableOperate(&mDocumentMap, aURISpec,
                                         PL_DHASH_LOOKUP));

    *aResult = PL_DHASH_ENTRY_IS_BUSY(docMapEntry);
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::StartMuxedDocument(nsISupports* aURI,
                                         const char* aURISpec)
{
    
    
    PRUint32 saveGeneration = mDocumentMap.generation;
    const char* saveURISpec = mCurrentDocumentMapEntry
                              ? mCurrentDocumentMapEntry->mString
                              : nsnull;

    nsDocumentMapWriteEntry* docMapEntry =
        static_cast<nsDocumentMapWriteEntry*>
                   (PL_DHashTableOperate(&mDocumentMap, aURISpec,
                                         PL_DHASH_ADD));
    if (!docMapEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    
    if (mCurrentDocumentMapEntry && mDocumentMap.generation != saveGeneration) {
        mCurrentDocumentMapEntry =
            static_cast<nsDocumentMapWriteEntry*>
                       (PL_DHashTableOperate(&mDocumentMap, saveURISpec,
                                             PL_DHASH_LOOKUP));
        NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(mCurrentDocumentMapEntry),
                     "mCurrentDocumentMapEntry lost during table growth?!");

        
        saveGeneration = mDocumentMap.generation;
    }

    NS_WARN_IF_FALSE(docMapEntry->mString == nsnull,
                     "redundant multiplexed document?");
    if (docMapEntry->mString)
        return NS_ERROR_UNEXPECTED;

    void* spec = nsMemory::Clone(aURISpec, strlen(aURISpec) + 1);
    if (!spec)
        return NS_ERROR_OUT_OF_MEMORY;
    docMapEntry->mString = reinterpret_cast<const char*>(spec);
    docMapEntry->mURI = aURI;
    NS_ADDREF(docMapEntry->mURI);

    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapWriteEntry* uriMapEntry =
        static_cast<nsURIMapWriteEntry*>
                   (PL_DHashTableOperate(&mURIMap, key, PL_DHASH_ADD));
    if (!uriMapEntry)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ASSERTION(uriMapEntry->mDocMapEntry == nsnull,
                 "URI mapped to two different specs?");
    if (uriMapEntry->mDocMapEntry)
        return NS_ERROR_UNEXPECTED;

    uriMapEntry->mObject = key;
    NS_ADDREF(uriMapEntry->mObject);
    uriMapEntry->mDocMapEntry = docMapEntry;
    uriMapEntry->mGeneration = saveGeneration;
    uriMapEntry->mURISpec = reinterpret_cast<const char*>(spec);
    TRACE_MUX(('w', "start %p (%p) %s\n", aURI, key.get(), aURISpec));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::SelectMuxedDocument(nsISupports* aURI,
                                          nsISupports** aResult)
{
    
    nsresult rv;
    PRInt64 currentSegmentOffset;
    rv = mSeekableOutput->Tell(&currentSegmentOffset);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 currentSegmentOffset32 = currentSegmentOffset;
    
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapWriteEntry* uriMapEntry =
        static_cast<nsURIMapWriteEntry*>
                   (PL_DHashTableOperate(&mURIMap, key, PL_DHASH_LOOKUP));
    NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(uriMapEntry),
                 "SelectMuxedDocument without prior StartMuxedDocument?");
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry))
        return NS_ERROR_UNEXPECTED;

    
    
    
    

    nsDocumentMapWriteEntry* docMapEntry = uriMapEntry->mDocMapEntry;
    if (uriMapEntry->mGeneration != mDocumentMap.generation) {
        docMapEntry =
            static_cast<nsDocumentMapWriteEntry*>
                       (PL_DHashTableOperate(&mDocumentMap,
                                             uriMapEntry->mURISpec,
                                             PL_DHASH_LOOKUP));
        NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(docMapEntry), "lost mDocMapEntry!?");
        uriMapEntry->mDocMapEntry = docMapEntry;
        uriMapEntry->mGeneration = mDocumentMap.generation;
    }

    
    
    nsDocumentMapWriteEntry* prevDocMapEntry = mCurrentDocumentMapEntry;
    if (prevDocMapEntry) {
        if (prevDocMapEntry == docMapEntry) {
            TRACE_MUX(('w', "select prev %s same as current!\n",
                       prevDocMapEntry->mString));
            *aResult = docMapEntry->mURI;
            NS_ADDREF(*aResult);
            return NS_OK;
        }

        PRUint32 prevSegmentOffset = prevDocMapEntry->mCurrentSegmentOffset;
        TRACE_MUX(('w', "select prev %s offset %lu\n",
                   prevDocMapEntry->mString, prevSegmentOffset));

        rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                                   prevSegmentOffset + 4);
        if (NS_FAILED(rv))
            return rv;

        
        
        rv = Write32(currentSegmentOffset32 - prevSegmentOffset);
        if (NS_FAILED(rv))
            return rv;

        
        
        
        if (!docMapEntry->mInitialSegmentOffset) {
            rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                                       currentSegmentOffset);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    
    
    
    if (!docMapEntry->mInitialSegmentOffset) {
        docMapEntry->mInitialSegmentOffset = currentSegmentOffset32;
    } else {
        rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                                   docMapEntry->mCurrentSegmentOffset);
        if (NS_FAILED(rv))
            return rv;

        rv = Write32(currentSegmentOffset32);
        if (NS_FAILED(rv))
            return rv;

        rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                                   currentSegmentOffset);
        if (NS_FAILED(rv))
            return rv;
    }

    
    
    
    docMapEntry->mCurrentSegmentOffset = currentSegmentOffset32;

    rv = Write32(0);    
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(0);    
    if (NS_FAILED(rv))
        return rv;

    *aResult = prevDocMapEntry ? prevDocMapEntry->mURI : nsnull;
    NS_IF_ADDREF(*aResult);

    mCurrentDocumentMapEntry = docMapEntry;
    TRACE_MUX(('w', "select %p (%p) offset %lu\n",
               aURI, key.get(), currentSegmentOffset));
    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::EndMuxedDocument(nsISupports* aURI)
{
    nsCOMPtr<nsISupports> key(do_QueryInterface(aURI));
    nsURIMapWriteEntry* uriMapEntry =
        static_cast<nsURIMapWriteEntry*>
                   (PL_DHashTableOperate(&mURIMap, key, PL_DHASH_LOOKUP));

    
    
    
    if (PL_DHASH_ENTRY_IS_FREE(uriMapEntry)) {
        TRACE_MUX(('w', "bad end %p (%p)\n", aURI, key.get()));
        return NS_ERROR_UNEXPECTED;
    }

    
    
    if (uriMapEntry->mDocMapEntry)
        NS_RELEASE(uriMapEntry->mDocMapEntry->mURI);

    
    PRUint32 size = PL_DHASH_TABLE_SIZE(&mURIMap);
    if (mURIMap.removedCount >= (size >> 2))
        PL_DHashTableOperate(&mURIMap, key, PL_DHASH_REMOVE);
    else
        PL_DHashTableRawRemove(&mURIMap, uriMapEntry);

    TRACE_MUX(('w', "end %p (%p)\n", aURI, key.get()));
    return NS_OK;
}

struct nsDependencyMapEntry : public nsStringMapEntry {
    PRInt64 mLastModified;
};

NS_IMETHODIMP
nsFastLoadFileWriter::AddDependency(nsIFile* aFile)
{
    nsCAutoString path;
    nsresult rv = aFile->GetNativePath(path);
    if (NS_FAILED(rv))
        return rv;

    nsDependencyMapEntry* entry =
        static_cast<nsDependencyMapEntry*>
                   (PL_DHashTableOperate(&mDependencyMap, path.get(),
                                         PL_DHASH_ADD));
    if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

    if (!entry->mString) {
        const char *tmp = ToNewCString(path);
        if (!tmp)
            return NS_ERROR_OUT_OF_MEMORY;
        entry->mString = tmp;

        
        
        
        
        
        

        rv = aFile->GetLastModifiedTime(&entry->mLastModified);
        if (NS_FAILED(rv)) {
            PL_DHashTableOperate(&mDependencyMap, path.get(), PL_DHASH_REMOVE);
            rv = NS_OK;
        }
    }
    return rv;
}

nsresult
nsFastLoadFileWriter::WriteFooterPrefix(const nsFastLoadFooterPrefix& aFooterPrefix)
{
    nsresult rv;

    rv = Write32(aFooterPrefix.mNumIDs);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aFooterPrefix.mNumSharpObjects);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aFooterPrefix.mNumMuxedDocuments);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aFooterPrefix.mNumDependencies);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteSlowID(const nsID& aID)
{
    nsresult rv;

    rv = Write32(aID.m0);
    if (NS_FAILED(rv))
        return rv;

    rv = Write16(aID.m1);
    if (NS_FAILED(rv))
        return rv;

    rv = Write16(aID.m2);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 bytesWritten;
    rv = Write(reinterpret_cast<const char*>(aID.m3), sizeof aID.m3,
               &bytesWritten);
    if (NS_FAILED(rv))
        return rv;

    if (bytesWritten != sizeof aID.m3)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteFastID(NSFastLoadID aID)
{
    return Write32(aID ^ MFL_ID_XOR_KEY);
}

nsresult
nsFastLoadFileWriter::WriteSharpObjectInfo(const nsFastLoadSharpObjectInfo& aInfo)
{
    nsresult rv;

    NS_ASSERTION(aInfo.mCIDOffset != 0,
                 "fastload writer: mCIDOffset cannot be zero!");

    rv = Write32(aInfo.mCIDOffset);
    if (NS_FAILED(rv))
        return rv;

    rv = Write16(aInfo.mStrongRefCnt);
    if (NS_FAILED(rv))
        return rv;

    rv = Write16(aInfo.mWeakRefCnt);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::WriteMuxedDocumentInfo(const nsFastLoadMuxedDocumentInfo& aInfo)
{
    nsresult rv;

    rv = WriteStringZ(aInfo.mURISpec);
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(aInfo.mInitialSegmentOffset);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

PLDHashOperator
nsFastLoadFileWriter::IDMapEnumerate(PLDHashTable *aTable,
                                     PLDHashEntryHdr *aHdr,
                                     PRUint32 aNumber,
                                     void *aData)
{
    nsIDMapEntry* entry = static_cast<nsIDMapEntry*>(aHdr);
    PRUint32 index = entry->mFastID - 1;
    nsID* vector = reinterpret_cast<nsID*>(aData);

    NS_ASSERTION(index < aTable->entryCount, "bad nsIDMap index!");
    vector[index] = entry->mSlowID;
    return PL_DHASH_NEXT;
}

struct nsSharpObjectMapEntry : public nsObjectMapEntry {
    NSFastLoadOID               mOID;
    nsFastLoadSharpObjectInfo   mInfo;
};

PLDHashOperator
nsFastLoadFileWriter::ObjectMapEnumerate(PLDHashTable *aTable,
                                         PLDHashEntryHdr *aHdr,
                                         PRUint32 aNumber,
                                         void *aData)
{
    nsSharpObjectMapEntry* entry = static_cast<nsSharpObjectMapEntry*>(aHdr);
    PRUint32 index = MFL_OID_TO_SHARP_INDEX(entry->mOID);
    nsFastLoadSharpObjectInfo* vector =
        reinterpret_cast<nsFastLoadSharpObjectInfo*>(aData);

    NS_ASSERTION(index < aTable->entryCount, "bad nsObjectMap index!");
    vector[index] = entry->mInfo;

    NS_ASSERTION(entry->mInfo.mStrongRefCnt, "no strong ref in serialization!");

    
    
    if ((NS_PTR_TO_INT32(entry->mObject) & MFL_OBJECT_DEF_TAG) == 0)
        NS_RELEASE(entry->mObject);

    return PL_DHASH_NEXT;
}

PLDHashOperator
nsFastLoadFileWriter::DocumentMapEnumerate(PLDHashTable *aTable,
                                           PLDHashEntryHdr *aHdr,
                                           PRUint32 aNumber,
                                           void *aData)
{
    nsFastLoadFileWriter* writer =
        reinterpret_cast<nsFastLoadFileWriter*>(aTable->data);
    nsDocumentMapWriteEntry* entry =
        static_cast<nsDocumentMapWriteEntry*>(aHdr);
    nsresult* rvp = reinterpret_cast<nsresult*>(aData);

    nsFastLoadMuxedDocumentInfo info;
    info.mURISpec = entry->mString;
    info.mInitialSegmentOffset = entry->mInitialSegmentOffset;
    *rvp = writer->WriteMuxedDocumentInfo(info);

    return NS_FAILED(*rvp) ? PL_DHASH_STOP : PL_DHASH_NEXT;
}

PLDHashOperator
nsFastLoadFileWriter::DependencyMapEnumerate(PLDHashTable *aTable,
                                             PLDHashEntryHdr *aHdr,
                                             PRUint32 aNumber,
                                             void *aData)
{
    nsFastLoadFileWriter* writer =
        reinterpret_cast<nsFastLoadFileWriter*>(aTable->data);
    nsDependencyMapEntry* entry = static_cast<nsDependencyMapEntry*>(aHdr);
    nsresult* rvp = reinterpret_cast<nsresult*>(aData);

    *rvp = writer->WriteStringZ(entry->mString);
    if (NS_SUCCEEDED(*rvp))
        *rvp = writer->Write64(entry->mLastModified);

    return NS_FAILED(*rvp) ? PL_DHASH_STOP :PL_DHASH_NEXT;
}

nsresult
nsFastLoadFileWriter::WriteFooter()
{
    nsresult rv;
    PRUint32 i, count;

    nsFastLoadFooterPrefix footerPrefix;
    footerPrefix.mNumIDs = mIDMap.entryCount;
    footerPrefix.mNumSharpObjects = mObjectMap.entryCount;
    footerPrefix.mNumMuxedDocuments = mDocumentMap.entryCount;
    footerPrefix.mNumDependencies = mDependencyMap.entryCount;

    rv = WriteFooterPrefix(footerPrefix);
    if (NS_FAILED(rv))
        return rv;

    
    nsID* idvec = new nsID[footerPrefix.mNumIDs];
    if (!idvec)
        return NS_ERROR_OUT_OF_MEMORY;

    count = PL_DHashTableEnumerate(&mIDMap, IDMapEnumerate, idvec);
    NS_ASSERTION(count == footerPrefix.mNumIDs, "bad mIDMap enumeration!");
    for (i = 0; i < count; i++) {
        rv = WriteSlowID(idvec[i]);
        if (NS_FAILED(rv)) break;
    }

    delete[] idvec;
    if (NS_FAILED(rv))
        return rv;

    
    nsFastLoadSharpObjectInfo* objvec =
        new nsFastLoadSharpObjectInfo[footerPrefix.mNumSharpObjects];
    if (!objvec)
        return NS_ERROR_OUT_OF_MEMORY;
#ifdef NS_DEBUG
    memset(objvec, 0, footerPrefix.mNumSharpObjects *
                      sizeof(nsFastLoadSharpObjectInfo));
#endif

    count = PL_DHashTableEnumerate(&mObjectMap, ObjectMapEnumerate, objvec);
    NS_ASSERTION(count == footerPrefix.mNumSharpObjects,
                 "bad mObjectMap enumeration!");
    for (i = 0; i < count; i++) {
        rv = WriteSharpObjectInfo(objvec[i]);
        if (NS_FAILED(rv)) break;
    }

    delete[] objvec;
    if (NS_FAILED(rv))
        return rv;

    
    count = PL_DHashTableEnumerate(&mDocumentMap, DocumentMapEnumerate, &rv);
    if (NS_FAILED(rv))
        return rv;

    NS_ASSERTION(count == footerPrefix.mNumMuxedDocuments,
                 "bad mDocumentMap enumeration!");

    
    count = PL_DHashTableEnumerate(&mDependencyMap, DependencyMapEnumerate, &rv);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::Init()
{
    if (!PL_DHashTableInit(&mIDMap, &idmap_DHashTableOps, (void *)this,
                           sizeof(nsIDMapEntry), PL_DHASH_MIN_SIZE)) {
        mIDMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mObjectMap, &objmap_DHashTableOps, (void *)this,
                           sizeof(nsSharpObjectMapEntry), PL_DHASH_MIN_SIZE)) {
        mObjectMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mDocumentMap, &strmap_DHashTableOps, (void *)this,
                           sizeof(nsDocumentMapWriteEntry),
                           PL_DHASH_MIN_SIZE)) {
        mDocumentMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mURIMap, &objmap_DHashTableOps, (void *)this,
                           sizeof(nsURIMapWriteEntry), PL_DHASH_MIN_SIZE)) {
        mURIMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (!PL_DHashTableInit(&mDependencyMap, &strmap_DHashTableOps, (void *)this,
                           sizeof(nsDependencyMapEntry), PL_DHASH_MIN_SIZE)) {
        mDependencyMap.ops = nsnull;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

nsresult
nsFastLoadFileWriter::Open()
{
    nsresult rv;

    rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                               sizeof(nsFastLoadHeader));
    if (NS_FAILED(rv))
        return rv;

    return Init();
}

#define MFL_CHECKSUM_BUFSIZE    (6 * 8192)

NS_IMETHODIMP
nsFastLoadFileWriter::Close()
{
    nsresult rv;

    memcpy(mHeader.mMagic, magic, MFL_FILE_MAGIC_SIZE);
    mHeader.mChecksum = 0;
    mHeader.mVersion = MFL_FILE_VERSION;

    PRInt64 footerOffset;
    rv = mSeekableOutput->Tell(&footerOffset);

    LL_L2UI(mHeader.mFooterOffset, footerOffset);
    if (NS_FAILED(rv))
        return rv;

    
    
    if (mCurrentDocumentMapEntry) {
        PRUint32 currentSegmentOffset =
            mCurrentDocumentMapEntry->mCurrentSegmentOffset;
        rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                                   currentSegmentOffset + 4);
        if (NS_FAILED(rv))
            return rv;

        rv = Write32(mHeader.mFooterOffset - currentSegmentOffset);
        if (NS_FAILED(rv))
            return rv;

        
        rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                                   mHeader.mFooterOffset);
        if (NS_FAILED(rv))
            return rv;

        mCurrentDocumentMapEntry = nsnull;
    }

    rv = WriteFooter();
    if (NS_FAILED(rv))
        return rv;
    PRInt64 fileSize;
    rv = mSeekableOutput->Tell(&fileSize);
    LL_L2UI(mHeader.mFileSize, fileSize);
    if (NS_FAILED(rv))
        return rv;

    rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET, 0);
    if (NS_FAILED(rv))
        return rv;

    rv = WriteHeader(&mHeader);
    if (NS_FAILED(rv))
        return rv;

    
    
    if (mFileIO) {
        
        
        
        nsCOMPtr<nsIOutputStream> output;
        rv = mBufferAccess->GetUnbufferedStream(getter_AddRefs(output));
        if (NS_FAILED(rv) || !output)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIInputStream> input;
        rv = mFileIO->GetInputStream(getter_AddRefs(input));
        if (NS_FAILED(rv))
            return rv;
 
        
        nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(input);
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            offsetof(nsFastLoadHeader, mVersion));
        if (NS_FAILED(rv))
            return rv;

        char buf[MFL_CHECKSUM_BUFSIZE];
        PRUint32 len, rem = 0;
        PRUint32 checksum = 0;

        
        while (NS_SUCCEEDED(rv =
                            input->Read(buf + rem, sizeof buf - rem, &len)) &&
               len) {
            len += rem;
            rem = NS_AccumulateFastLoadChecksum(&checksum,
                                                reinterpret_cast<PRUint8*>
                                                                (buf),
                                                len,
                                                PR_FALSE);
            if (rem)
                memcpy(buf, buf + len - rem, rem);
        }
        if (NS_FAILED(rv))
            return rv;

        if (rem) {
            NS_AccumulateFastLoadChecksum(&checksum,
                                          reinterpret_cast<PRUint8*>(buf),
                                          rem,
                                          PR_TRUE);
        }

        
        
        seekable = do_QueryInterface(output);
        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                            offsetof(nsFastLoadHeader, mChecksum));
        if (NS_FAILED(rv))
            return rv;

        mHeader.mChecksum = checksum;
        checksum = NS_SWAP32(checksum);
        PRUint32 bytesWritten;
        rv = output->Write(reinterpret_cast<char*>(&checksum),
                           sizeof checksum,
                           &bytesWritten);
        if (NS_FAILED(rv))
            return rv;
        if (bytesWritten != sizeof checksum)
            return NS_ERROR_FAILURE;
    }

    return mOutputStream->Close();
}


#define MFL_SINGLE_REF_PSEUDO_TAG       PR_BIT(MFL_OBJECT_TAG_BITS)

nsresult
nsFastLoadFileWriter::WriteObjectCommon(nsISupports* aObject,
                                        PRBool aIsStrongRef,
                                        PRUint32 aTags)
{
    nsrefcnt rc;
    nsresult rv;

    NS_ASSERTION((NS_PTR_TO_INT32(aObject) & MFL_OBJECT_DEF_TAG) == 0,
                 "odd nsISupports*, oh no!");

    
    rc = aObject->AddRef();
    NS_ASSERTION(rc != 0, "bad refcnt when writing aObject!");

    NSFastLoadOID oid;
    nsCOMPtr<nsIClassInfo> classInfo;

    if (rc == 2 && (aTags & MFL_SINGLE_REF_PSEUDO_TAG)) {
        
        
        
        
        oid = MFL_DULL_OBJECT_OID;
        aObject->Release();
    } else {
        
        
        nsSharpObjectMapEntry* entry =
            static_cast<nsSharpObjectMapEntry*>
                       (PL_DHashTableOperate(&mObjectMap, aObject,
                                             PL_DHASH_ADD));
        if (!entry) {
            aObject->Release();
            return NS_ERROR_OUT_OF_MEMORY;
        }

        if (!entry->mObject) {
            
            
            PRInt64 thisOffset;
            rv = Tell(&thisOffset);
            if (NS_FAILED(rv)) {
                aObject->Release();
                return rv;
            }

            
            entry->mObject = aObject;

            oid = (mObjectMap.entryCount << MFL_OBJECT_TAG_BITS);
            entry->mOID = oid;

            
            entry->mInfo.mCIDOffset = thisOffset + sizeof(oid);
            entry->mInfo.mStrongRefCnt = aIsStrongRef ? 1 : 0;
            entry->mInfo.mWeakRefCnt   = aIsStrongRef ? 0 : 1;

            
            
            
            
            oid |= MFL_OBJECT_DEF_TAG;
            classInfo = do_QueryInterface(aObject);
            if (!classInfo) {
                NS_NOTREACHED("aObject must implement nsIClassInfo");
                return NS_ERROR_FAILURE;
            }

            PRUint32 flags;
            if (NS_SUCCEEDED(classInfo->GetFlags(&flags)) &&
                (flags & nsIClassInfo::SINGLETON)) {
                MFL_SET_SINGLETON_FLAG(&entry->mInfo);
            }
        } else {
            
            oid = entry->mOID;
            if (aIsStrongRef) {
                ++entry->mInfo.mStrongRefCnt;
                NS_ASSERTION(entry->mInfo.mStrongRefCnt != 0,
                             "mStrongRefCnt overflow");
            } else {
                MFL_BUMP_WEAK_REFCNT(&entry->mInfo);
                NS_ASSERTION(MFL_GET_WEAK_REFCNT(&entry->mInfo) != 0,
                             "mWeakRefCnt overflow");
            }

            aObject->Release();
        }
    }

    if (!aIsStrongRef)
        oid |= MFL_WEAK_REF_TAG;
    oid |= (aTags & MFL_QUERY_INTERFACE_TAG);

    rv = Write32(oid ^ MFL_OID_XOR_KEY);
    if (NS_FAILED(rv))
        return rv;

    if (oid & MFL_OBJECT_DEF_TAG) {
        nsCOMPtr<nsISerializable> serializable(do_QueryInterface(aObject));
        if (!serializable) {
            NS_NOTREACHED("aObject must implement nsISerializable");
            return NS_ERROR_FAILURE;
        }

        nsCID slowCID;
        rv = classInfo->GetClassIDNoAlloc(&slowCID);
        if (NS_FAILED(rv))
            return rv;

        NSFastLoadID fastCID;
        rv = MapID(slowCID, &fastCID);
        if (NS_FAILED(rv))
            return rv;

        rv = WriteFastID(fastCID);
        if (NS_FAILED(rv))
            return rv;

        rv = serializable->Write(this);
        if (NS_FAILED(rv))
            return rv;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteObject(nsISupports* aObject, PRBool aIsStrongRef)
{
#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

    NS_ASSERTION(rootObject.get() == aObject,
                 "bad call to WriteObject -- call WriteCompoundObject!");
#endif

    return WriteObjectCommon(aObject, aIsStrongRef, 0);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteSingleRefObject(nsISupports* aObject)
{
#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));

    NS_ASSERTION(rootObject.get() == aObject,
                 "bad call to WriteSingleRefObject -- call WriteCompoundObject!");
#endif

    return WriteObjectCommon(aObject, PR_TRUE, MFL_SINGLE_REF_PSEUDO_TAG);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteCompoundObject(nsISupports* aObject,
                                          const nsIID& aIID,
                                          PRBool aIsStrongRef)
{
    nsresult rv;
    nsCOMPtr<nsISupports> rootObject(do_QueryInterface(aObject));
    
    
    
    
    

#ifdef NS_DEBUG
    nsCOMPtr<nsISupports> roundtrip;
    rootObject->QueryInterface(aIID, getter_AddRefs(roundtrip));
    NS_ASSERTION(roundtrip.get() == aObject,
                 "bad aggregation or multiple inheritance detected by call to "
                 "WriteCompoundObject!");
#endif

    rv = WriteObjectCommon(rootObject, aIsStrongRef, MFL_QUERY_INTERFACE_TAG);
    if (NS_FAILED(rv))
        return rv;

    NSFastLoadID iid;
    rv = MapID(aIID, &iid);
    if (NS_FAILED(rv))
        return rv;

    return WriteFastID(iid);
}

NS_IMETHODIMP
nsFastLoadFileWriter::WriteID(const nsID& aID)
{
    nsresult rv;
    NSFastLoadID fastID;

    rv = MapID(aID, &fastID);
    if (NS_FAILED(rv))
        return rv;

    return WriteFastID(fastID);
}

NS_IMETHODIMP
nsFastLoadFileWriter::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
    mCurrentDocumentMapEntry = nsnull;
    return mSeekableOutput->Seek(aWhence, aOffset);
}

NS_IMETHODIMP
nsFastLoadFileWriter::Tell(PRInt64 *aResult)
{
    return mSeekableOutput->Tell(aResult);
}

NS_IMETHODIMP
nsFastLoadFileWriter::SetEOF()
{
    return mSeekableOutput->SetEOF();
}

NS_IMETHODIMP
nsFastLoadFileWriter::SetOutputStream(nsIOutputStream *aStream)
{
    nsresult rv = nsBinaryOutputStream::SetOutputStream(aStream);
    mSeekableOutput = do_QueryInterface(mOutputStream);
    return rv;
}

NS_COM nsresult
NS_NewFastLoadFileWriter(nsIObjectOutputStream* *aResult,
                         nsIOutputStream* aDestStream,
                         nsIFastLoadFileIO* aFileIO)
{
    nsFastLoadFileWriter* writer =
        new nsFastLoadFileWriter(aDestStream, aFileIO);
    if (!writer)
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsCOMPtr<nsIObjectOutputStream> stream(writer);

    nsresult rv = writer->Open();
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}



NS_IMPL_ISUPPORTS_INHERITED0(nsFastLoadFileUpdater,
                             nsFastLoadFileWriter)

PLDHashOperator
nsFastLoadFileUpdater::CopyReadDocumentMapEntryToUpdater(PLDHashTable *aTable,
                                                         PLDHashEntryHdr *aHdr,
                                                         PRUint32 aNumber,
                                                         void *aData)
{
    nsDocumentMapReadEntry* readEntry =
        static_cast<nsDocumentMapReadEntry*>(aHdr);
    nsFastLoadFileUpdater* updater =
        reinterpret_cast<nsFastLoadFileUpdater*>(aData);

    void* spec = nsMemory::Clone(readEntry->mString,
                                 strlen(readEntry->mString) + 1);
    if (!spec)
        return PL_DHASH_STOP;

    nsDocumentMapWriteEntry* writeEntry =
        static_cast<nsDocumentMapWriteEntry*>
                   (PL_DHashTableOperate(&updater->mDocumentMap, spec,
                                         PL_DHASH_ADD));
    if (!writeEntry) {
        nsMemory::Free(spec);
        return PL_DHASH_STOP;
    }

    writeEntry->mString = reinterpret_cast<const char*>(spec);
    writeEntry->mURI = nsnull;
    writeEntry->mInitialSegmentOffset = readEntry->mInitialSegmentOffset;
    writeEntry->mCurrentSegmentOffset = 0;
    return PL_DHASH_NEXT;
}

nsresult
nsFastLoadFileUpdater::Open(nsFastLoadFileReader* aReader)
{
    nsresult rv;
    rv = nsFastLoadFileWriter::Init();
    if (NS_FAILED(rv))
        return rv;

    PRUint32 i, n;

    
    
    nsID* readIDMap = aReader->mFooter.mIDMap;
    for (i = 0, n = aReader->mFooter.mNumIDs; i < n; i++) {
        NSFastLoadID fastID;
        rv = MapID(readIDMap[i], &fastID);
        NS_ASSERTION(fastID == i + 1, "huh?");
        if (NS_FAILED(rv))
            return rv;
    }

    
    
    nsFastLoadFileReader::nsObjectMapEntry* readObjectMap =
        aReader->mFooter.mObjectMap;

    
    
    nsDocumentMapReadEntry* saveDocMapEntry = nsnull;
    PRInt64 saveOffset = 0;

    for (i = 0, n = aReader->mFooter.mNumSharpObjects; i < n; i++) {
        nsFastLoadFileReader::nsObjectMapEntry* readEntry = &readObjectMap[i];

        NS_ASSERTION(readEntry->mCIDOffset != 0,
                     "fastload updater: mCIDOffset cannot be zero!");

        
        
        
        
        

        nsISupports* obj = readEntry->mReadObject;
        if (!obj && MFL_GET_SINGLETON_FLAG(readEntry)) {
            if (!saveDocMapEntry) {
                rv = aReader->Tell(&saveOffset);
                if (NS_FAILED(rv))
                    return rv;

                saveDocMapEntry = aReader->mCurrentDocumentMapEntry;
                aReader->mCurrentDocumentMapEntry = nsnull;
            }

            rv = aReader->Seek(nsISeekableStream::NS_SEEK_SET,
                                     readEntry->mCIDOffset);
            if (NS_FAILED(rv))
                return rv;

            rv = aReader
                 ->DeserializeObject(getter_AddRefs(readEntry->mReadObject));
            if (NS_FAILED(rv))
                return rv;
            obj = readEntry->mReadObject;

            
            
            
            
            
            
            
            
            
            
            
            
            
            

            rv = aReader->Tell(&readEntry->mSkipOffset);
            if (NS_FAILED(rv))
                return rv;
        }

        NSFastLoadOID oid = MFL_SHARP_INDEX_TO_OID(i);
        void* key = obj
                    ? reinterpret_cast<void*>(obj)
                    : reinterpret_cast<void*>((oid | MFL_OBJECT_DEF_TAG));

        nsSharpObjectMapEntry* writeEntry =
            static_cast<nsSharpObjectMapEntry*>
                       (PL_DHashTableOperate(&mObjectMap, key,
                                             PL_DHASH_ADD));
        if (!writeEntry)
            return NS_ERROR_OUT_OF_MEMORY;

        
        
        NS_IF_ADDREF(obj);
        writeEntry->mObject = reinterpret_cast<nsISupports*>(key);
        writeEntry->mOID = oid;
        writeEntry->mInfo.mCIDOffset = readEntry->mCIDOffset;
        writeEntry->mInfo.mStrongRefCnt = readEntry->mSaveStrongRefCnt;
        writeEntry->mInfo.mWeakRefCnt = readEntry->mSaveWeakRefCnt;
    }

    
    if (saveDocMapEntry) {
        rv = aReader->Seek(nsISeekableStream::NS_SEEK_SET, saveOffset);
        if (NS_FAILED(rv))
            return rv;

        aReader->mCurrentDocumentMapEntry = saveDocMapEntry;
    }

    
    
    
    n = PL_DHashTableEnumerate(&aReader->mFooter.mDocumentMap,
                               CopyReadDocumentMapEntryToUpdater,
                               this);
    if (n != aReader->mFooter.mDocumentMap.entryCount)
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsISupportsArray* readDeps = aReader->mFooter.mDependencies;
    rv = readDeps->Count(&n);
    if (NS_FAILED(rv))
        return rv;

    for (i = 0; i < n; i++) {
        nsCOMPtr<nsIFile> file;
        rv = readDeps->GetElementAt(i, getter_AddRefs(file));
        if (NS_FAILED(rv))
            return rv;

        rv = AddDependency(file);
        if (NS_FAILED(rv))
            return rv;
    }

    
    
    
    
    rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                               offsetof(nsFastLoadHeader, mFooterOffset));
    if (NS_FAILED(rv))
        return rv;

    rv = Write32(0);
    if (NS_FAILED(rv))
        return rv;

    rv = mSeekableOutput->Seek(nsISeekableStream::NS_SEEK_SET,
                               aReader->mHeader.mFooterOffset);
    if (NS_FAILED(rv))
        return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsFastLoadFileUpdater::Close()
{
    
    nsresult rv = nsFastLoadFileWriter::Close();

    
    mFileIO = nsnull;
    return rv;
}

NS_COM nsresult
NS_NewFastLoadFileUpdater(nsIObjectOutputStream* *aResult,
                          nsIFastLoadFileIO *aFileIO,
                          nsIObjectInputStream* aReaderAsStream)
{
    
    nsCOMPtr<nsIFastLoadFileReader> reader(do_QueryInterface(aReaderAsStream));
    if (!reader)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIOutputStream> output;
    nsresult rv = aFileIO->GetOutputStream(getter_AddRefs(output));
    if (NS_FAILED(rv))
        return rv;

    nsFastLoadFileUpdater* updater = new nsFastLoadFileUpdater(output, aFileIO);
    if (!updater)
        return NS_ERROR_OUT_OF_MEMORY;

    
    nsCOMPtr<nsIObjectOutputStream> stream(updater);

    rv = updater->Open(static_cast<nsFastLoadFileReader*>(aReaderAsStream));
    if (NS_FAILED(rv))
        return rv;

    *aResult = stream;
    NS_ADDREF(*aResult);
    return NS_OK;
}
