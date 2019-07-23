






































#include "StreamFunctions.h"
#include "nsZipWriter.h"
#include "nsZipDataStream.h"
#include "nsISeekableStream.h"
#include "nsIAsyncStreamCopier.h"
#include "nsIStreamListener.h"
#include "nsIInputStreamPump.h"
#include "nsComponentManagerUtils.h"
#include "nsMemory.h"
#include "nsNetError.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "prio.h"

#define ZIP_EOCDR_HEADER_SIZE 22
#define ZIP_EOCDR_HEADER_SIGNATURE 0x06054b50




















NS_IMPL_ISUPPORTS2(nsZipWriter, nsIZipWriter,
                                nsIRequestObserver)

nsZipWriter::nsZipWriter()
{
    mEntryHash.Init();
    mInQueue = PR_FALSE;
}

nsZipWriter::~nsZipWriter()
{
    if (mStream && !mInQueue)
        Close();
}


NS_IMETHODIMP nsZipWriter::GetComment(nsACString & aComment)
{
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    aComment = mComment;
    return NS_OK;
}

NS_IMETHODIMP nsZipWriter::SetComment(const nsACString & aComment)
{
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    mComment = aComment;
    mCDSDirty = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP nsZipWriter::GetInQueue(PRBool *aInQueue)
{
    *aInQueue = mInQueue;
    return NS_OK;
}


NS_IMETHODIMP nsZipWriter::GetFile(nsIFile **aFile)
{
    if (!mFile)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsIFile> file;
    nsresult rv = mFile->Clone(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ADDREF(*aFile = file);
    return NS_OK;
}




nsresult nsZipWriter::ReadFile(nsIFile *aFile)
{
    PRInt64 size;
    nsresult rv = aFile->GetFileSize(&size);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    NS_ENSURE_TRUE(size > ZIP_EOCDR_HEADER_SIZE, NS_ERROR_FILE_CORRUPTED);

    nsCOMPtr<nsIInputStream> inputStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), aFile);
    NS_ENSURE_SUCCESS(rv, rv);

    char buf[1024];
    PRInt64 seek = size - 1024;
    PRUint32 length = 1024;

    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(inputStream);

    while (true) {
        if (seek < 0) {
            length += (PRInt32)seek;
            seek = 0;
        }

        rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, seek);
        if (NS_FAILED(rv)) {
            inputStream->Close();
            return rv;
        }
        rv = ZW_ReadData(inputStream, buf, length);
        if (NS_FAILED(rv)) {
            inputStream->Close();
            return rv;
        }

        



        
        for (PRUint32 pos = length - ZIP_EOCDR_HEADER_SIZE;
             (PRInt32)pos >= 0; pos--) {
            PRUint32 sig = PEEK32((unsigned char *)buf + pos);
            if (sig == ZIP_EOCDR_HEADER_SIGNATURE) {
                
                pos += 10;
                PRUint32 entries = READ16(buf, &pos);
                
                pos += 4;
                mCDSOffset = READ32(buf, &pos);
                PRUint32 commentlen = READ16(buf, &pos);

                if (commentlen == 0)
                    mComment.Truncate();
                else if (pos + commentlen <= length)
                    mComment.Assign(buf + pos, commentlen);
                else {
                    if ((seek + pos + commentlen) > size) {
                        inputStream->Close();
                        return NS_ERROR_FILE_CORRUPTED;
                    }
                    nsAutoArrayPtr<char> field(new char[commentlen]);
                    NS_ENSURE_TRUE(field, NS_ERROR_OUT_OF_MEMORY);
                    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                        seek + pos);
                    if (NS_FAILED(rv)) {
                        inputStream->Close();
                        return rv;
                    }
                    rv = ZW_ReadData(inputStream, field.get(), length);
                    if (NS_FAILED(rv)) {
                        inputStream->Close();
                        return rv;
                    }
                    mComment.Assign(field.get(), commentlen);
                }

                rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                    mCDSOffset);
                if (NS_FAILED(rv)) {
                    inputStream->Close();
                    return rv;
                }

                for (PRUint32 entry = 0; entry < entries; entry++) {
                    nsZipHeader* header = new nsZipHeader();
                    if (!header) {
                        inputStream->Close();
                        mEntryHash.Clear();
                        mHeaders.Clear();
                        return NS_ERROR_OUT_OF_MEMORY;
                    }
                    rv = header->ReadCDSHeader(inputStream);
                    if (NS_FAILED(rv)) {
                        inputStream->Close();
                        mEntryHash.Clear();
                        mHeaders.Clear();
                        return rv;
                    }
                    if (!mEntryHash.Put(header->mName, mHeaders.Count()))
                        return NS_ERROR_OUT_OF_MEMORY;
                    if (!mHeaders.AppendObject(header))
                        return NS_ERROR_OUT_OF_MEMORY;
                }

                return inputStream->Close();
            }
        }

        if (seek == 0) {
            
            inputStream->Close();
            return NS_ERROR_FILE_CORRUPTED;
        }

        
        seek -= (1024 - ZIP_EOCDR_HEADER_SIZE);
    }
    
    NS_NOTREACHED("Loop should never complete");
    return NS_ERROR_UNEXPECTED;
}


NS_IMETHODIMP nsZipWriter::Open(nsIFile *aFile, PRInt32 aIoFlags)
{
    if (mStream)
        return NS_ERROR_ALREADY_INITIALIZED;

    NS_ENSURE_ARG_POINTER(aFile);

    
    if (aIoFlags & PR_RDONLY)
        return NS_ERROR_FAILURE;
    
    nsresult rv = aFile->Clone(getter_AddRefs(mFile));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool exists;
    rv = mFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!exists && !(aIoFlags & PR_CREATE_FILE))
        return NS_ERROR_FILE_NOT_FOUND;

    if (exists && !(aIoFlags & (PR_TRUNCATE | PR_WRONLY))) {
        rv = ReadFile(mFile);
        NS_ENSURE_SUCCESS(rv, rv);
        mCDSDirty = PR_FALSE;
    }
    else {
        mCDSOffset = 0;
        mCDSDirty = PR_TRUE;
        mComment.Truncate();
    }

    
    aIoFlags &= 0xef;

    nsCOMPtr<nsIOutputStream> stream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(stream), mFile, aIoFlags);
    if (NS_FAILED(rv)) {
        mHeaders.Clear();
        mEntryHash.Clear();
        return rv;
    }

    rv = NS_NewBufferedOutputStream(getter_AddRefs(mStream), stream, 0x800);
    if (NS_FAILED(rv)) {
        stream->Close();
        mHeaders.Clear();
        mEntryHash.Clear();
        return rv;
    }

    if (mCDSOffset > 0) {
        rv = SeekCDS();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}


NS_IMETHODIMP nsZipWriter::GetEntry(const nsACString & aZipEntry,
                                    nsIZipEntry **_retval)
{
    PRInt32 pos;
    if (mEntryHash.Get(aZipEntry, &pos))
        NS_ADDREF(*_retval = mHeaders[pos]);
    else
        *_retval = nsnull;

    return NS_OK;
}


NS_IMETHODIMP nsZipWriter::HasEntry(const nsACString & aZipEntry,
                                    PRBool *_retval)
{
    *_retval = mEntryHash.Get(aZipEntry, nsnull);

    return NS_OK;
}



NS_IMETHODIMP nsZipWriter::AddEntryDirectory(const nsACString & aZipEntry,
                                             PRTime aModTime, PRBool aQueue)
{
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    if (aQueue) {
        nsZipQueueItem item;
        item.mOperation = OPERATION_ADD;
        item.mZipEntry = aZipEntry;
        item.mModTime = aModTime;
        item.mPermissions = PERMISSIONS_DIR;
        if (!mQueue.AppendElement(item))
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;
    return InternalAddEntryDirectory(aZipEntry, aModTime, PERMISSIONS_DIR);
}



NS_IMETHODIMP nsZipWriter::AddEntryFile(const nsACString & aZipEntry,
                                        PRInt32 aCompression, nsIFile *aFile,
                                        PRBool aQueue)
{
    NS_ENSURE_ARG_POINTER(aFile);
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
    if (aQueue) {
        nsZipQueueItem item;
        item.mOperation = OPERATION_ADD;
        item.mZipEntry = aZipEntry;
        item.mCompression = aCompression;
        rv = aFile->Clone(getter_AddRefs(item.mFile));
        NS_ENSURE_SUCCESS(rv, rv);
        if (!mQueue.AppendElement(item))
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;

    PRBool exists;
    rv = aFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;

    PRBool isdir;
    rv = aFile->IsDirectory(&isdir);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 modtime;
    rv = aFile->GetLastModifiedTime(&modtime);
    NS_ENSURE_SUCCESS(rv, rv);
    modtime *= PR_USEC_PER_MSEC;

    PRUint32 permissions;
    rv = aFile->GetPermissions(&permissions);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isdir)
        return InternalAddEntryDirectory(aZipEntry, modtime, permissions);

    if (mEntryHash.Get(aZipEntry, nsnull))
        return NS_ERROR_FILE_ALREADY_EXISTS;

    nsCOMPtr<nsIInputStream> inputStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream),
                                    aFile);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddEntryStream(aZipEntry, modtime, aCompression, inputStream,
                        PR_FALSE, permissions);
    NS_ENSURE_SUCCESS(rv, rv);

    return inputStream->Close();
}




NS_IMETHODIMP nsZipWriter::AddEntryChannel(const nsACString & aZipEntry,
                                           PRTime aModTime,
                                           PRInt32 aCompression,
                                           nsIChannel *aChannel,
                                           PRBool aQueue)
{
    NS_ENSURE_ARG_POINTER(aChannel);
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    if (aQueue) {
        nsZipQueueItem item;
        item.mOperation = OPERATION_ADD;
        item.mZipEntry = aZipEntry;
        item.mModTime = aModTime;
        item.mCompression = aCompression;
        item.mPermissions = PERMISSIONS_FILE;
        item.mChannel = aChannel;
        if (!mQueue.AppendElement(item))
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;
    if (mEntryHash.Get(aZipEntry, nsnull))
        return NS_ERROR_FILE_ALREADY_EXISTS;

    nsCOMPtr<nsIInputStream> inputStream;
    nsresult rv = aChannel->Open(getter_AddRefs(inputStream));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddEntryStream(aZipEntry, aModTime, aCompression, inputStream,
                        PR_FALSE, PERMISSIONS_FILE);
    NS_ENSURE_SUCCESS(rv, rv);

    return inputStream->Close();
}




NS_IMETHODIMP nsZipWriter::AddEntryStream(const nsACString & aZipEntry,
                                          PRTime aModTime,
                                          PRInt32 aCompression,
                                          nsIInputStream *aStream,
                                          PRBool aQueue)
{
    return AddEntryStream(aZipEntry, aModTime, aCompression, aStream, aQueue,
                          PERMISSIONS_FILE);
}




nsresult nsZipWriter::AddEntryStream(const nsACString & aZipEntry,
                                     PRTime aModTime,
                                     PRInt32 aCompression,
                                     nsIInputStream *aStream,
                                     PRBool aQueue,
                                     PRUint32 aPermissions)
{
    NS_ENSURE_ARG_POINTER(aStream);
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    if (aQueue) {
        nsZipQueueItem item;
        item.mOperation = OPERATION_ADD;
        item.mZipEntry = aZipEntry;
        item.mModTime = aModTime;
        item.mCompression = aCompression;
        item.mPermissions = aPermissions;
        item.mStream = aStream;
        if (!mQueue.AppendElement(item))
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;
    if (mEntryHash.Get(aZipEntry, nsnull))
        return NS_ERROR_FILE_ALREADY_EXISTS;

    nsRefPtr<nsZipHeader> header = new nsZipHeader();
    NS_ENSURE_TRUE(header, NS_ERROR_OUT_OF_MEMORY);
    header->Init(aZipEntry, aModTime, ZIP_ATTRS(aPermissions, ZIP_ATTRS_FILE),
                 mCDSOffset);
    nsresult rv = header->WriteFileHeader(mStream);
    if (NS_FAILED(rv)) {
        SeekCDS();
        return rv;
    }

    nsRefPtr<nsZipDataStream> stream = new nsZipDataStream();
    if (!stream) {
        SeekCDS();
        return NS_ERROR_OUT_OF_MEMORY;
    }
    rv = stream->Init(this, mStream, header, aCompression);
    if (NS_FAILED(rv)) {
        SeekCDS();
        return rv;
    }

    rv = stream->ReadStream(aStream);
    if (NS_FAILED(rv))
        SeekCDS();
    return rv;
}


NS_IMETHODIMP nsZipWriter::RemoveEntry(const nsACString & aZipEntry,
                                       PRBool aQueue)
{
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;

    if (aQueue) {
        nsZipQueueItem item;
        item.mOperation = OPERATION_REMOVE;
        item.mZipEntry = aZipEntry;
        if (!mQueue.AppendElement(item))
            return NS_ERROR_OUT_OF_MEMORY;
        return NS_OK;
    }

    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;

    PRInt32 pos;
    if (mEntryHash.Get(aZipEntry, &pos)) {
        
        nsresult rv = mStream->Flush();
        NS_ENSURE_SUCCESS(rv, rv);
        if (pos < mHeaders.Count() - 1) {
            
            nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mStream);
            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                mHeaders[pos]->mOffset);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIInputStream> inputStream;
            rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream),
                                            mFile);
            NS_ENSURE_SUCCESS(rv, rv);
            seekable = do_QueryInterface(inputStream);
            rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                                mHeaders[pos + 1]->mOffset);
            if (NS_FAILED(rv)) {
                inputStream->Close();
                return rv;
            }

            PRUint32 count = mCDSOffset - mHeaders[pos + 1]->mOffset;
            PRUint32 read = 0;
            char buf[4096];
            while (count > 0) {
                if (count < sizeof(buf))
                    read = count;
                else
                    read = sizeof(buf);

                rv = inputStream->Read(buf, read, &read);
                if (NS_FAILED(rv)) {
                    inputStream->Close();
                    Cleanup();
                    return rv;
                }

                rv = ZW_WriteData(mStream, buf, read);
                if (NS_FAILED(rv)) {
                    inputStream->Close();
                    Cleanup();
                    return rv;
                }

                count -= read;
            }
            inputStream->Close();

            
            PRUint32 shift = (mHeaders[pos + 1]->mOffset -
                              mHeaders[pos]->mOffset);
            mCDSOffset -= shift;
            PRInt32 pos2 = pos + 1;
            while (pos2 < mHeaders.Count()) {
                if (!mEntryHash.Put(mHeaders[pos2]->mName, pos2-1)) {
                    Cleanup();
                    return NS_ERROR_OUT_OF_MEMORY;
                }
                mHeaders[pos2]->mOffset -= shift;
                pos2++;
            }
        }
        else {
            
            mCDSOffset = mHeaders[pos]->mOffset;
            rv = SeekCDS();
            NS_ENSURE_SUCCESS(rv, rv);
        }

        mEntryHash.Remove(mHeaders[pos]->mName);
        mHeaders.RemoveObjectAt(pos);
        mCDSDirty = PR_TRUE;

        return NS_OK;
    }

    return NS_ERROR_FILE_NOT_FOUND;
}



NS_IMETHODIMP nsZipWriter::ProcessQueue(nsIRequestObserver *aObserver,
                                        nsISupports *aContext)
{
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;
    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;

    mProcessObserver = aObserver;
    mProcessContext = aContext;
    mInQueue = PR_TRUE;

    if (mProcessObserver)
        mProcessObserver->OnStartRequest(nsnull, mProcessContext);

    BeginProcessingNextItem();

    return NS_OK;
}


NS_IMETHODIMP nsZipWriter::Close()
{
    if (!mStream)
        return NS_ERROR_NOT_INITIALIZED;
    if (mInQueue)
        return NS_ERROR_IN_PROGRESS;

    if (mCDSDirty) {
        PRUint32 size = 0;
        for (PRInt32 i = 0; i < mHeaders.Count(); i++) {
            nsresult rv = mHeaders[i]->WriteCDSHeader(mStream);
            if (NS_FAILED(rv)) {
                Cleanup();
                return rv;
            }
            size += mHeaders[i]->GetCDSHeaderLength();
        }

        char buf[ZIP_EOCDR_HEADER_SIZE];
        PRUint32 pos = 0;
        WRITE32(buf, &pos, ZIP_EOCDR_HEADER_SIGNATURE);
        WRITE16(buf, &pos, 0);
        WRITE16(buf, &pos, 0);
        WRITE16(buf, &pos, mHeaders.Count());
        WRITE16(buf, &pos, mHeaders.Count());
        WRITE32(buf, &pos, size);
        WRITE32(buf, &pos, mCDSOffset);
        WRITE16(buf, &pos, mComment.Length());

        nsresult rv = ZW_WriteData(mStream, buf, pos);
        if (NS_FAILED(rv)) {
            Cleanup();
            return rv;
        }

        rv = ZW_WriteData(mStream, mComment.get(), mComment.Length());
        if (NS_FAILED(rv)) {
            Cleanup();
            return rv;
        }

        nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mStream);
        rv = seekable->SetEOF();
        if (NS_FAILED(rv)) {
            Cleanup();
            return rv;
        }
    }

    nsresult rv = mStream->Close();
    mStream = nsnull;
    mHeaders.Clear();
    mEntryHash.Clear();
    mQueue.Clear();

    return rv;
}



NS_IMETHODIMP nsZipWriter::OnStartRequest(nsIRequest *aRequest,
                                          nsISupports *aContext)
{
    return NS_OK;
}



NS_IMETHODIMP nsZipWriter::OnStopRequest(nsIRequest *aRequest,
                                         nsISupports *aContext,
                                         nsresult aStatusCode)
{
    if (NS_FAILED(aStatusCode)) {
        FinishQueue(aStatusCode);
        Cleanup();
    }

    nsresult rv = mStream->Flush();
    if (NS_FAILED(rv)) {
        FinishQueue(rv);
        Cleanup();
        return rv;
    }
    rv = SeekCDS();
    if (NS_FAILED(rv)) {
        FinishQueue(rv);
        return rv;
    }

    BeginProcessingNextItem();

    return NS_OK;
}

nsresult nsZipWriter::InternalAddEntryDirectory(const nsACString & aZipEntry,
                                                PRTime aModTime,
                                                PRUint32 aPermissions)
{
    nsRefPtr<nsZipHeader> header = new nsZipHeader();
    NS_ENSURE_TRUE(header, NS_ERROR_OUT_OF_MEMORY);

    PRUint32 zipAttributes = ZIP_ATTRS(aPermissions, ZIP_ATTRS_DIRECTORY);

    if (aZipEntry.Last() != '/') {
        nsCString dirPath;
        dirPath.Assign(aZipEntry + NS_LITERAL_CSTRING("/"));
        header->Init(dirPath, aModTime, zipAttributes, mCDSOffset);
    }
    else
        header->Init(aZipEntry, aModTime, zipAttributes, mCDSOffset);

    if (mEntryHash.Get(header->mName, nsnull))
        return NS_ERROR_FILE_ALREADY_EXISTS;

    nsresult rv = header->WriteFileHeader(mStream);
    if (NS_FAILED(rv)) {
        Cleanup();
        return rv;
    }

    mCDSDirty = PR_TRUE;
    mCDSOffset += header->GetFileHeaderLength();
    if (!mEntryHash.Put(header->mName, mHeaders.Count())) {
        Cleanup();
        return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!mHeaders.AppendObject(header)) {
        Cleanup();
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}






nsresult nsZipWriter::SeekCDS()
{
    nsresult rv;
    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mStream, &rv);
    if (NS_FAILED(rv)) {
        Cleanup();
        return rv;
    }
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, mCDSOffset);
    if (NS_FAILED(rv))
        Cleanup();
    return rv;
}





void nsZipWriter::Cleanup()
{
    mHeaders.Clear();
    mEntryHash.Clear();
    if (mStream)
        mStream->Close();
    mStream = nsnull;
    mFile = nsnull;
}




nsresult nsZipWriter::EntryCompleteCallback(nsZipHeader* aHeader,
                                            nsresult aStatus)
{
    if (NS_SUCCEEDED(aStatus)) {
        if (!mEntryHash.Put(aHeader->mName, mHeaders.Count())) {
            SeekCDS();
            return NS_ERROR_OUT_OF_MEMORY;
        }
        if (!mHeaders.AppendObject(aHeader)) {
            mEntryHash.Remove(aHeader->mName);
            SeekCDS();
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mCDSDirty = PR_TRUE;
        mCDSOffset += aHeader->mCSize + aHeader->GetFileHeaderLength();

        if (mInQueue)
            BeginProcessingNextItem();

        return NS_OK;
    }

    nsresult rv = SeekCDS();
    if (mInQueue)
        FinishQueue(aStatus);
    return rv;
}

inline nsresult nsZipWriter::BeginProcessingAddition(nsZipQueueItem* aItem,
                                                     PRBool* complete)
{
    if (aItem->mFile) {
        PRBool exists;
        nsresult rv = aItem->mFile->Exists(&exists);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!exists) return NS_ERROR_FILE_NOT_FOUND;

        PRBool isdir;
        rv = aItem->mFile->IsDirectory(&isdir);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aItem->mFile->GetLastModifiedTime(&aItem->mModTime);
        NS_ENSURE_SUCCESS(rv, rv);
        aItem->mModTime *= PR_USEC_PER_MSEC;

        rv = aItem->mFile->GetPermissions(&aItem->mPermissions);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!isdir) {
            
            rv = NS_NewLocalFileInputStream(getter_AddRefs(aItem->mStream),
                                            aItem->mFile);
            NS_ENSURE_SUCCESS(rv, rv);
        }
        
    }

    PRUint32 zipAttributes = ZIP_ATTRS(aItem->mPermissions, ZIP_ATTRS_FILE);

    if (aItem->mStream) {
        nsRefPtr<nsZipHeader> header = new nsZipHeader();
        NS_ENSURE_TRUE(header, NS_ERROR_OUT_OF_MEMORY);

        header->Init(aItem->mZipEntry, aItem->mModTime, zipAttributes,
                     mCDSOffset);
        nsresult rv = header->WriteFileHeader(mStream);
        NS_ENSURE_SUCCESS(rv, rv);

        nsRefPtr<nsZipDataStream> stream = new nsZipDataStream();
        rv = stream->Init(this, mStream, header, aItem->mCompression);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIInputStreamPump> pump;
        rv = NS_NewInputStreamPump(getter_AddRefs(pump), aItem->mStream, -1,
                                   -1, 0, 0, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = pump->AsyncRead(stream, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);

        return NS_OK;
    }

    if (aItem->mChannel) {
        nsRefPtr<nsZipHeader> header = new nsZipHeader();
        NS_ENSURE_TRUE(header, NS_ERROR_OUT_OF_MEMORY);

        header->Init(aItem->mZipEntry, aItem->mModTime, zipAttributes,
                     mCDSOffset);

        nsRefPtr<nsZipDataStream> stream = new nsZipDataStream();
        NS_ENSURE_TRUE(stream, NS_ERROR_OUT_OF_MEMORY);
        nsresult rv = stream->Init(this, mStream, header, aItem->mCompression);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = aItem->mChannel->AsyncOpen(stream, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);

        return NS_OK;
    }

    
    *complete = PR_TRUE;
    return InternalAddEntryDirectory(aItem->mZipEntry, aItem->mModTime,
                                     aItem->mPermissions);
}

inline nsresult nsZipWriter::BeginProcessingRemoval(PRInt32 aPos)
{
    
    nsCOMPtr<nsIInputStream> inputStream;
    nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream),
                                             mFile);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIInputStreamPump> pump;
    rv = NS_NewInputStreamPump(getter_AddRefs(pump), inputStream, -1, -1, 0,
                               0, PR_TRUE);
    if (NS_FAILED(rv)) {
        inputStream->Close();
        return rv;
    }
    nsCOMPtr<nsIStreamListener> listener;
    rv = NS_NewSimpleStreamListener(getter_AddRefs(listener), mStream, this);
    if (NS_FAILED(rv)) {
        inputStream->Close();
        return rv;
    }

    nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mStream);
    rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET,
                        mHeaders[aPos]->mOffset);
    if (NS_FAILED(rv)) {
        inputStream->Close();
        return rv;
    }

    PRUint32 shift = (mHeaders[aPos + 1]->mOffset -
                      mHeaders[aPos]->mOffset);
    mCDSOffset -= shift;
    PRInt32 pos2 = aPos + 1;
    while (pos2 < mHeaders.Count()) {
        mEntryHash.Put(mHeaders[pos2]->mName, pos2 - 1);
        mHeaders[pos2]->mOffset -= shift;
        pos2++;
    }

    mEntryHash.Remove(mHeaders[aPos]->mName);
    mHeaders.RemoveObjectAt(aPos);
    mCDSDirty = PR_TRUE;

    rv = pump->AsyncRead(listener, nsnull);
    if (NS_FAILED(rv)) {
        inputStream->Close();
        Cleanup();
        return rv;
    }
    return NS_OK;
}




void nsZipWriter::BeginProcessingNextItem()
{
    while (!mQueue.IsEmpty()) {

        nsZipQueueItem next = mQueue[0];
        mQueue.RemoveElementAt(0);

        if (next.mOperation == OPERATION_REMOVE) {
            PRInt32 pos = -1;
            if (mEntryHash.Get(next.mZipEntry, &pos)) {
                if (pos < mHeaders.Count() - 1) {
                    nsresult rv = BeginProcessingRemoval(pos);
                    if (NS_FAILED(rv)) FinishQueue(rv);
                    return;
                }

                mCDSOffset = mHeaders[pos]->mOffset;
                nsresult rv = SeekCDS();
                if (NS_FAILED(rv)) {
                    FinishQueue(rv);
                    return;
                }
                mEntryHash.Remove(mHeaders[pos]->mName);
                mHeaders.RemoveObjectAt(pos);
            }
            else {
                FinishQueue(NS_ERROR_FILE_NOT_FOUND);
                return;
            }
        }
        else if (next.mOperation == OPERATION_ADD) {
            if (mEntryHash.Get(next.mZipEntry, nsnull)) {
                FinishQueue(NS_ERROR_FILE_ALREADY_EXISTS);
                return;
            }

            PRBool complete = PR_FALSE;
            nsresult rv = BeginProcessingAddition(&next, &complete);
            if (NS_FAILED(rv)) {
                SeekCDS();
                FinishQueue(rv);
                return;
            }
            if (!complete)
                return;
        }
    }

    FinishQueue(NS_OK);
}




void nsZipWriter::FinishQueue(nsresult aStatus)
{
    nsCOMPtr<nsIRequestObserver> observer = mProcessObserver;
    nsCOMPtr<nsISupports> context = mProcessContext;
    
    
    mProcessObserver = nsnull;
    mProcessContext = nsnull;
    mInQueue = PR_FALSE;

    if (observer)
        observer->OnStopRequest(nsnull, context, aStatus);
}
