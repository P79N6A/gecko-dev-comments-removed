







































#include "nspr.h"

#include "nsIFileStreams.h"       

#ifdef XP_OS2
#include "nsILocalFileOS2.h"
#endif

#include "nsNetUtil.h"
#include "nsComponentManagerUtils.h"
#include "nsIComponentRegistrar.h"
#include "nsIStorageStream.h"
#include "nsISeekableStream.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIEncodedChannel.h"
#include "nsIUploadChannel.h"
#include "nsICachingChannel.h"
#include "nsEscape.h"
#include "nsUnicharUtils.h"
#include "nsIStringEnumerator.h"
#include "nsCRT.h"
#include "nsSupportsArray.h"
#include "nsInt64.h"
#include "nsContentCID.h"
#include "nsStreamUtils.h"

#include "nsCExternalHandlerService.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMDocumentTraversal.h"
#include "nsIDOMTreeWalker.h"
#include "nsIDOMNode.h"
#include "nsIDOMComment.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNSDocument.h"
#include "nsIWebProgressListener.h"
#include "nsIAuthPrompt.h"
#include "nsIPrompt.h"
#include "nsISHEntry.h"
#include "nsIWebPageDescriptor.h"
#include "nsIFormControl.h"
#include "nsIDOM3Node.h"

#include "nsIDOMNodeFilter.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsIDOMHTMLBaseElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMText.h"
#ifdef MOZ_SVG
#include "nsIDOMSVGImageElement.h"
#include "nsIDOMSVGScriptElement.h"
#endif 

#include "nsIImageLoadingContent.h"

#include "ftpCore.h"
#include "nsITransport.h"
#include "nsISocketTransport.h"
#include "nsIStringBundle.h"
#include "nsIProtocolHandler.h"

#include "nsWebBrowserPersist.h"


#define BUFFERED_OUTPUT_SIZE (1024 * 32)

#define NS_SUCCESS_DONT_FIXUP NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_GENERAL, 1)


struct DocData
{
    nsCOMPtr<nsIURI> mBaseURI;
    nsCOMPtr<nsIDOMDocument> mDocument;
    nsCOMPtr<nsIURI> mFile;
    nsCOMPtr<nsIURI> mDataPath;
    PRPackedBool mDataPathIsRelative;
    nsCString mRelativePathToData;
    nsCString mCharset;
};


struct URIData
{
    PRPackedBool mNeedsPersisting;
    PRPackedBool mSaved;
    PRPackedBool mIsSubFrame;
    PRPackedBool mDataPathIsRelative;
    PRPackedBool mNeedsFixup;
    nsString mFilename;
    nsString mSubFrameExt;
    nsCOMPtr<nsIURI> mFile;
    nsCOMPtr<nsIURI> mDataPath;
    nsCString mRelativePathToData;
    nsCString mCharset;
};


struct OutputData
{
    nsCOMPtr<nsIURI> mFile;
    nsCOMPtr<nsIURI> mOriginalLocation;
    nsCOMPtr<nsIOutputStream> mStream;
    nsInt64 mSelfProgress;
    nsInt64 mSelfProgressMax;
    PRPackedBool mCalcFileExt;

    OutputData(nsIURI *aFile, nsIURI *aOriginalLocation, PRBool aCalcFileExt) :
        mFile(aFile),
        mOriginalLocation(aOriginalLocation),
        mSelfProgress(0),
        mSelfProgressMax(10000),
        mCalcFileExt(aCalcFileExt)
    {
    }
    ~OutputData()
    {
        if (mStream)
        {
            mStream->Close();
        }
    }
};

struct UploadData
{
    nsCOMPtr<nsIURI> mFile;
    nsInt64 mSelfProgress;
    nsInt64 mSelfProgressMax;

    UploadData(nsIURI *aFile) :
        mFile(aFile),
        mSelfProgress(0),
        mSelfProgressMax(10000)
    {
    }
};

struct CleanupData
{
    nsCOMPtr<nsILocalFile> mFile;
    
    
    
    PRPackedBool mIsDirectory;
};






#ifdef XP_MAC
const PRUint32 kDefaultMaxFilenameLength = 31;
#else
const PRUint32 kDefaultMaxFilenameLength = 64;
#endif


const PRUint32 kDefaultPersistFlags = 
    nsIWebBrowserPersist::PERSIST_FLAGS_NO_CONVERSION |
    nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES;


const char *kWebBrowserPersistStringBundle =
    "chrome://global/locale/nsWebBrowserPersist.properties";

nsWebBrowserPersist::nsWebBrowserPersist() :
    mCurrentThingsToPersist(0),
    mFirstAndOnlyUse(PR_TRUE),
    mCancel(PR_FALSE),
    mJustStartedLoading(PR_TRUE),
    mCompleted(PR_FALSE),
    mStartSaving(PR_FALSE),
    mReplaceExisting(PR_TRUE),
    mSerializingOutput(PR_FALSE),
    mPersistFlags(kDefaultPersistFlags),
    mPersistResult(NS_OK),
    mWrapColumn(72),
    mEncodingFlags(0)
{
}

nsWebBrowserPersist::~nsWebBrowserPersist()
{
    Cleanup();
}





NS_IMPL_ADDREF(nsWebBrowserPersist)
NS_IMPL_RELEASE(nsWebBrowserPersist)

NS_INTERFACE_MAP_BEGIN(nsWebBrowserPersist)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserPersist)
    NS_INTERFACE_MAP_ENTRY(nsIWebBrowserPersist)
    NS_INTERFACE_MAP_ENTRY(nsICancelable)
    NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
    NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
    NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
    NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
NS_INTERFACE_MAP_END






NS_IMETHODIMP nsWebBrowserPersist::GetInterface(const nsIID & aIID, void **aIFace)
{
    NS_ENSURE_ARG_POINTER(aIFace);

    *aIFace = nsnull;

    nsresult rv = QueryInterface(aIID, aIFace);
    if (NS_SUCCEEDED(rv))
    {
        return rv;
    }
    
    if (mProgressListener && (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) 
                             || aIID.Equals(NS_GET_IID(nsIPrompt))))
    {
        mProgressListener->QueryInterface(aIID, aIFace);
        if (*aIFace)
            return NS_OK;
    }

    nsCOMPtr<nsIInterfaceRequestor> req = do_QueryInterface(mProgressListener);
    if (req)
    {
        return req->GetInterface(aIID, aIFace);
    }

    return NS_ERROR_NO_INTERFACE;
}







NS_IMETHODIMP nsWebBrowserPersist::GetPersistFlags(PRUint32 *aPersistFlags)
{
    NS_ENSURE_ARG_POINTER(aPersistFlags);
    *aPersistFlags = mPersistFlags;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserPersist::SetPersistFlags(PRUint32 aPersistFlags)
{
    mPersistFlags = aPersistFlags;
    mReplaceExisting = (mPersistFlags & PERSIST_FLAGS_REPLACE_EXISTING_FILES) ? PR_TRUE : PR_FALSE;
    mSerializingOutput = (mPersistFlags & PERSIST_FLAGS_SERIALIZE_OUTPUT) ? PR_TRUE : PR_FALSE;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserPersist::GetCurrentState(PRUint32 *aCurrentState)
{
    NS_ENSURE_ARG_POINTER(aCurrentState);
    if (mCompleted)
    {
        *aCurrentState = PERSIST_STATE_FINISHED;
    }
    else if (mFirstAndOnlyUse)
    {
        *aCurrentState = PERSIST_STATE_SAVING;
    }
    else
    {
        *aCurrentState = PERSIST_STATE_READY;
    }
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserPersist::GetResult(PRUint32 *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = mPersistResult;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserPersist::GetProgressListener(
    nsIWebProgressListener * *aProgressListener)
{
    NS_ENSURE_ARG_POINTER(aProgressListener);
    *aProgressListener = mProgressListener;
    NS_IF_ADDREF(*aProgressListener);
    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserPersist::SetProgressListener(
    nsIWebProgressListener * aProgressListener)
{
    mProgressListener = aProgressListener;
    mProgressListener2 = do_QueryInterface(aProgressListener);
    mEventSink = do_GetInterface(aProgressListener);
    return NS_OK;
}




NS_IMETHODIMP nsWebBrowserPersist::SaveURI(
    nsIURI *aURI, nsISupports *aCacheKey, nsIURI *aReferrer, nsIInputStream *aPostData, const char *aExtraHeaders, nsISupports *aFile)
{
    NS_ENSURE_TRUE(mFirstAndOnlyUse, NS_ERROR_FAILURE);
    mFirstAndOnlyUse = PR_FALSE; 

    nsCOMPtr<nsIURI> fileAsURI;
    nsresult rv;
    rv = GetValidURIFromObject(aFile, getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

    
    mPersistFlags |= PERSIST_FLAGS_FAIL_ON_BROKEN_LINKS;
    rv = SaveURIInternal(aURI, aCacheKey, aReferrer, aPostData, aExtraHeaders, fileAsURI, PR_FALSE);
    return NS_FAILED(rv) ? rv : NS_OK;
}


NS_IMETHODIMP nsWebBrowserPersist::SaveChannel(
    nsIChannel *aChannel, nsISupports *aFile)
{
    NS_ENSURE_TRUE(mFirstAndOnlyUse, NS_ERROR_FAILURE);
    mFirstAndOnlyUse = PR_FALSE; 

    nsCOMPtr<nsIURI> fileAsURI;
    nsresult rv;
    rv = GetValidURIFromObject(aFile, getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

    rv = aChannel->GetURI(getter_AddRefs(mURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    mPersistFlags |= PERSIST_FLAGS_FAIL_ON_BROKEN_LINKS;
    rv = SaveChannelInternal(aChannel, fileAsURI, PR_FALSE);
    return NS_FAILED(rv) ? rv : NS_OK;
}





NS_IMETHODIMP nsWebBrowserPersist::SaveDocument(
    nsIDOMDocument *aDocument, nsISupports *aFile, nsISupports *aDataPath,
    const char *aOutputContentType, PRUint32 aEncodingFlags, PRUint32 aWrapColumn)
{
    NS_ENSURE_TRUE(mFirstAndOnlyUse, NS_ERROR_FAILURE);
    mFirstAndOnlyUse = PR_FALSE; 

    nsCOMPtr<nsIURI> fileAsURI;
    nsCOMPtr<nsIURI> datapathAsURI;
    nsresult rv;

    rv = GetValidURIFromObject(aFile, getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);
    if (aDataPath)
    {
        rv = GetValidURIFromObject(aDataPath, getter_AddRefs(datapathAsURI));
        NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);
    }

    mWrapColumn = aWrapColumn;

    
    mEncodingFlags = 0;
    if (aEncodingFlags & ENCODE_FLAGS_SELECTION_ONLY)
        mEncodingFlags |= nsIDocumentEncoder::OutputSelectionOnly;
    if (aEncodingFlags & ENCODE_FLAGS_FORMATTED)
        mEncodingFlags |= nsIDocumentEncoder::OutputFormatted;
    if (aEncodingFlags & ENCODE_FLAGS_RAW)
        mEncodingFlags |= nsIDocumentEncoder::OutputRaw;
    if (aEncodingFlags & ENCODE_FLAGS_BODY_ONLY)
        mEncodingFlags |= nsIDocumentEncoder::OutputBodyOnly;
    if (aEncodingFlags & ENCODE_FLAGS_PREFORMATTED)
        mEncodingFlags |= nsIDocumentEncoder::OutputPreformatted;
    if (aEncodingFlags & ENCODE_FLAGS_WRAP)
        mEncodingFlags |= nsIDocumentEncoder::OutputWrap;
    if (aEncodingFlags & ENCODE_FLAGS_FORMAT_FLOWED)
        mEncodingFlags |= nsIDocumentEncoder::OutputFormatFlowed;
    if (aEncodingFlags & ENCODE_FLAGS_ABSOLUTE_LINKS)
        mEncodingFlags |= nsIDocumentEncoder::OutputAbsoluteLinks;
    if (aEncodingFlags & ENCODE_FLAGS_ENCODE_BASIC_ENTITIES)
        mEncodingFlags |= nsIDocumentEncoder::OutputEncodeBasicEntities;
    if (aEncodingFlags & ENCODE_FLAGS_ENCODE_LATIN1_ENTITIES)
        mEncodingFlags |= nsIDocumentEncoder::OutputEncodeLatin1Entities;
    if (aEncodingFlags & ENCODE_FLAGS_ENCODE_HTML_ENTITIES)
        mEncodingFlags |= nsIDocumentEncoder::OutputEncodeHTMLEntities;
    if (aEncodingFlags & ENCODE_FLAGS_ENCODE_W3C_ENTITIES)
        mEncodingFlags |= nsIDocumentEncoder::OutputEncodeW3CEntities;
    if (aEncodingFlags & ENCODE_FLAGS_CR_LINEBREAKS)
        mEncodingFlags |= nsIDocumentEncoder::OutputCRLineBreak;
    if (aEncodingFlags & ENCODE_FLAGS_LF_LINEBREAKS)
        mEncodingFlags |= nsIDocumentEncoder::OutputLFLineBreak;
    if (aEncodingFlags & ENCODE_FLAGS_NOSCRIPT_CONTENT)
        mEncodingFlags |= nsIDocumentEncoder::OutputNoScriptContent;
    if (aEncodingFlags & ENCODE_FLAGS_NOFRAMES_CONTENT)
        mEncodingFlags |= nsIDocumentEncoder::OutputNoFramesContent;
    
    if (aOutputContentType)
    {
        mContentType.AssignASCII(aOutputContentType);
    }

    rv = SaveDocumentInternal(aDocument, fileAsURI, datapathAsURI);

    

    if (NS_SUCCEEDED(rv) && datapathAsURI)
    {
        rv = SaveGatheredURIs(fileAsURI);
    }
    else if (mProgressListener)
    {
        
        mProgressListener->OnStateChange(nsnull, nsnull,
                                         nsIWebProgressListener::STATE_START |
                                         nsIWebProgressListener::STATE_IS_NETWORK,
                                         NS_OK);
        mProgressListener->OnStateChange(nsnull, nsnull,
                                         nsIWebProgressListener::STATE_STOP |
                                         nsIWebProgressListener::STATE_IS_NETWORK,
                                         rv);
    }

    return rv;
}


NS_IMETHODIMP nsWebBrowserPersist::Cancel(nsresult aReason)
{
    mCancel = PR_TRUE;
    EndDownload(aReason);
    return NS_OK;
}



NS_IMETHODIMP nsWebBrowserPersist::CancelSave()
{
    return Cancel(NS_BINDING_ABORTED);
}


nsresult
nsWebBrowserPersist::StartUpload(nsIStorageStream *storStream, 
    nsIURI *aDestinationURI, const nsACString &aContentType)
{
     
    nsCOMPtr<nsIInputStream> inputstream;
    nsresult rv = storStream->NewInputStream(0, getter_AddRefs(inputstream));
    NS_ENSURE_TRUE(inputstream, NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    nsCOMPtr<nsIChannel> destChannel;
    rv = CreateChannelFromURI(aDestinationURI, getter_AddRefs(destChannel));
    nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(destChannel));
    NS_ENSURE_TRUE(uploadChannel, NS_ERROR_FAILURE);

    
    
    rv = uploadChannel->SetUploadStream(inputstream, aContentType, -1);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    rv = destChannel->AsyncOpen(this, nsnull);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(destChannel);
    nsISupportsKey key(keyPtr);
    mUploadList.Put(&key, new UploadData(aDestinationURI));

    return NS_OK;
}

nsresult
nsWebBrowserPersist::SaveGatheredURIs(nsIURI *aFileAsURI)
{
    nsresult rv = NS_OK;

    
    PRUint32 urisToPersist = 0;
    if (mURIMap.Count() > 0)
    {
        mURIMap.Enumerate(EnumCountURIsToPersist, &urisToPersist);
    }

    if (urisToPersist > 0)
    {
        
        
        mURIMap.Enumerate(EnumPersistURIs, this);
    }

    
    
    if (mOutputMap.Count() == 0)
    {
        

        
        PRUint32 addToStateFlags = 0;
        if (mProgressListener)
        {
            if (mJustStartedLoading)
            {
                addToStateFlags |= nsIWebProgressListener::STATE_IS_NETWORK;
            }
            mProgressListener->OnStateChange(nsnull, nsnull,
                nsIWebProgressListener::STATE_START | addToStateFlags, NS_OK);
        }

        rv = SaveDocuments();
        if (NS_FAILED(rv))
            EndDownload(rv);
        else if (aFileAsURI)
        {
            
            PRBool isFile = PR_FALSE;
            aFileAsURI->SchemeIs("file", &isFile);
            if (isFile)
                EndDownload(NS_OK);
        }

        
        if (mProgressListener)
        {
            mProgressListener->OnStateChange(nsnull, nsnull,
                nsIWebProgressListener::STATE_STOP | addToStateFlags, rv);
        }
    }

    return rv;
}


PRBool
nsWebBrowserPersist::SerializeNextFile()
{
    if (!mSerializingOutput)
    {
        return PR_FALSE;
    }

    nsresult rv = SaveGatheredURIs(nsnull);
    if (NS_FAILED(rv))
    {
        return PR_FALSE;
    }

    return (mURIMap.Count() 
        || mUploadList.Count()
        || mDocList.Length()
        || mOutputMap.Count());
}






NS_IMETHODIMP nsWebBrowserPersist::OnStartRequest(
    nsIRequest* request, nsISupports *ctxt)
{
    if (mProgressListener)
    {
        PRUint32 stateFlags = nsIWebProgressListener::STATE_START |
                              nsIWebProgressListener::STATE_IS_REQUEST;
        if (mJustStartedLoading)
        {
            stateFlags |= nsIWebProgressListener::STATE_IS_NETWORK;
        }
        mProgressListener->OnStateChange(nsnull, request, stateFlags, NS_OK);
    }

    mJustStartedLoading = PR_FALSE;

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
    nsISupportsKey key(keyPtr);
    OutputData *data = (OutputData *) mOutputMap.Get(&key);

    
    
    
    
    if (!data)
    {
        UploadData *upData = (UploadData *) mUploadList.Get(&key);
        if (!upData)
        {
            
            nsresult rv = FixRedirectedChannelEntry(channel);
            NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

            
            
            data = (OutputData *) mOutputMap.Get(&key);
            if (!data)
            {
                return NS_ERROR_FAILURE;
            }
        }
    }

    if (data && data->mFile)
    {
        
        
        
        NS_ASSERTION(!((mPersistFlags & PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION) &&
                      (mPersistFlags & PERSIST_FLAGS_NO_CONVERSION)),
                     "Conflict in persist flags: both AUTODETECT and NO_CONVERSION set");
        if (mPersistFlags & PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION)
            SetApplyConversionIfNeeded(channel);

        if (data->mCalcFileExt && !(mPersistFlags & PERSIST_FLAGS_DONT_CHANGE_FILENAMES))
        {
            
            CalculateAndAppendFileExt(data->mFile, channel, data->mOriginalLocation);

            
            CalculateUniqueFilename(data->mFile);
        }

        
        PRBool isEqual = PR_FALSE;
        if (NS_SUCCEEDED(data->mFile->Equals(data->mOriginalLocation, &isEqual))
            && isEqual)
        {
            
            delete data;
            mOutputMap.Remove(&key);

            
            
            request->Cancel(NS_BINDING_ABORTED);
        }
    }

    return NS_OK;
}
 
NS_IMETHODIMP nsWebBrowserPersist::OnStopRequest(
    nsIRequest* request, nsISupports *ctxt, nsresult status)
{
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
    nsISupportsKey key(keyPtr);
    OutputData *data = (OutputData *) mOutputMap.Get(&key);
    if (data)
    {
        if (NS_SUCCEEDED(mPersistResult) && NS_FAILED(status))
            SendErrorStatusChange(PR_TRUE, status, request, data->mFile);

#if defined(XP_OS2)
        
        
        nsCOMPtr<nsIURI> uriSource = data->mOriginalLocation;
        nsCOMPtr<nsILocalFile> localFile;
        GetLocalFileFromURI(data->mFile, getter_AddRefs(localFile));
        delete data;
        mOutputMap.Remove(&key);
        if (localFile)
        {
            nsCOMPtr<nsILocalFileOS2> localFileOS2 = do_QueryInterface(localFile);
            if (localFileOS2)
            {
                nsCAutoString url;
                uriSource->GetSpec(url);
                localFileOS2->SetFileSource(url);
            }
        }
#else
        
        delete data;
        mOutputMap.Remove(&key);
#endif
    }
    else
    {
        
        UploadData *upData = (UploadData *) mUploadList.Get(&key);
        if (upData)
        {
            delete upData;
            mUploadList.Remove(&key);
        }
    }

    
    
    
    
    if (mOutputMap.Count() == 0 && !mCancel && !mStartSaving && !mSerializingOutput)
    {
        nsresult rv = SaveDocuments();
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    }

    PRBool completed = PR_FALSE;
    if (mOutputMap.Count() == 0 && mUploadList.Count() == 0 && !mCancel)
    {
        
        
        if (mDocList.Length() == 0
            || (!SerializeNextFile() && NS_SUCCEEDED(mPersistResult)))
        {
            completed = PR_TRUE;
        }
    }

    if (completed)
    {
        
        EndDownload(status);
    }

    if (mProgressListener)
    {
        PRUint32 stateFlags = nsIWebProgressListener::STATE_STOP |
                              nsIWebProgressListener::STATE_IS_REQUEST;
        if (completed)
        {
            stateFlags |= nsIWebProgressListener::STATE_IS_NETWORK;
        }
        mProgressListener->OnStateChange(nsnull, request, stateFlags, status);
    }
    if (completed)
    {
        mProgressListener = nsnull;
        mProgressListener2 = nsnull;
        mEventSink = nsnull;
    }

    return NS_OK;
}





NS_IMETHODIMP nsWebBrowserPersist::OnDataAvailable(
    nsIRequest* request, nsISupports *aContext, nsIInputStream *aIStream,
    PRUint32 aOffset, PRUint32 aLength)
{
    PRBool cancel = mCancel;
    if (!cancel)
    {
        nsresult rv = NS_OK;
        PRUint32 bytesRemaining = aLength;

        nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
        NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

        nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
        nsISupportsKey key(keyPtr);
        OutputData *data = (OutputData *) mOutputMap.Get(&key);
        if (!data) {
            
            PRUint32 n;
            return aIStream->ReadSegments(NS_DiscardSegment, nsnull, aLength, &n);
        }

        PRBool readError = PR_TRUE;

        
        if (!data->mStream)
        {
            rv = MakeOutputStream(data->mFile, getter_AddRefs(data->mStream));
            if (NS_FAILED(rv))
            {
                readError = PR_FALSE;
                cancel = PR_TRUE;
            }
        }

        
        char buffer[8192];
        PRUint32 bytesRead;
        while (!cancel && bytesRemaining)
        {
            readError = PR_TRUE;
            rv = aIStream->Read(buffer, PR_MIN(sizeof(buffer), bytesRemaining), &bytesRead);
            if (NS_SUCCEEDED(rv))
            {
                readError = PR_FALSE;
                
                
                
                
                
                const char *bufPtr = buffer; 
                while (NS_SUCCEEDED(rv) && bytesRead)
                {
                    PRUint32 bytesWritten = 0;
                    rv = data->mStream->Write(bufPtr, bytesRead, &bytesWritten);
                    if (NS_SUCCEEDED(rv))
                    {
                        bytesRead -= bytesWritten;
                        bufPtr += bytesWritten;
                        bytesRemaining -= bytesWritten;
                        
                        
                        if (!bytesWritten)
                        {
                            rv = NS_ERROR_FAILURE;
                            cancel = PR_TRUE;
                        }
                    }
                    else
                    {
                        
                        cancel = PR_TRUE;
                    }
                }
            }
            else
            {
                
                cancel = PR_TRUE;
            }
        }

        PRInt32 channelContentLength = -1;
        if (!cancel &&
            NS_SUCCEEDED(channel->GetContentLength(&channelContentLength)))
        {
            
            
            
            if ((-1 == channelContentLength) ||
                ((channelContentLength - (aOffset + aLength)) == 0))
            {
                NS_WARN_IF_FALSE(channelContentLength != -1,
                    "nsWebBrowserPersist::OnDataAvailable() no content length "
                    "header, pushing what we have");
                
                nsCAutoString contentType;
                channel->GetContentType(contentType);
                
                nsCOMPtr<nsIStorageStream> storStream(do_QueryInterface(data->mStream));
                if (storStream)
                {
                    data->mStream->Close();
                    data->mStream = nsnull; 
                    rv = StartUpload(storStream, data->mFile, contentType);
                    if (NS_FAILED(rv))
                    {
                        readError = PR_FALSE;
                        cancel = PR_TRUE;
                    }
                }
            }
        }

        
        if (cancel)
        {
            SendErrorStatusChange(readError, rv,
                readError ? request : nsnull, data->mFile);
        }
    }

    
    if (cancel)
    {
        EndDownload(NS_BINDING_ABORTED);
    }

    return NS_OK;
}








NS_IMETHODIMP nsWebBrowserPersist::OnProgress(
    nsIRequest *request, nsISupports *ctxt, PRUint64 aProgress,
    PRUint64 aProgressMax)
{
    if (!mProgressListener)
    {
        return NS_OK;
    }

    
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
    nsISupportsKey key(keyPtr);
    OutputData *data = (OutputData *) mOutputMap.Get(&key);
    if (data)
    {
        data->mSelfProgress = PRInt64(aProgress);
        data->mSelfProgressMax = PRInt64(aProgressMax);
    }
    else
    {
        UploadData *upData = (UploadData *) mUploadList.Get(&key);
        if (upData)
        {
            upData->mSelfProgress = PRInt64(aProgress);
            upData->mSelfProgressMax = PRInt64(aProgressMax);
        }
    }

    
    CalcTotalProgress();
    if (mProgressListener2)
    {
      mProgressListener2->OnProgressChange64(nsnull, request, aProgress,
            aProgressMax, mTotalCurrentProgress, mTotalMaxProgress);
    }
    else
    {
      
      mProgressListener->OnProgressChange(nsnull, request, PRUint64(aProgress),
              PRUint64(aProgressMax), mTotalCurrentProgress, mTotalMaxProgress);
    }

    
    
    if (mEventSink)
    {
        mEventSink->OnProgress(request, ctxt, aProgress, aProgressMax);
    }

    return NS_OK;
}



NS_IMETHODIMP nsWebBrowserPersist::OnStatus(
    nsIRequest *request, nsISupports *ctxt, nsresult status,
    const PRUnichar *statusArg)
{
    if (mProgressListener)
    {
        
        
        switch ( status )
        {
        case NS_NET_STATUS_RESOLVING_HOST:
        case NS_NET_STATUS_BEGIN_FTP_TRANSACTION:
        case NS_NET_STATUS_END_FTP_TRANSACTION:
        case NS_NET_STATUS_CONNECTING_TO:
        case NS_NET_STATUS_CONNECTED_TO:
        case NS_NET_STATUS_SENDING_TO:
        case NS_NET_STATUS_RECEIVING_FROM:
        case NS_NET_STATUS_WAITING_FOR:
        case nsITransport::STATUS_READING:
        case nsITransport::STATUS_WRITING:
            break;

        default:
            
            mProgressListener->OnStatusChange(nsnull, request, status, statusArg);
            break;
        }

    }

    
    
    if (mEventSink)
    {
        mEventSink->OnStatus(request, ctxt, status, statusArg);
    }

    return NS_OK;
}








nsresult nsWebBrowserPersist::SendErrorStatusChange( 
    PRBool aIsReadError, nsresult aResult, nsIRequest *aRequest, nsIURI *aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);

    if (!mProgressListener)
    {
        
        return NS_OK;
    }

    
    nsCOMPtr<nsILocalFile> file;
    GetLocalFileFromURI(aURI, getter_AddRefs(file));
    nsAutoString path;
    if (file)
    {
        file->GetPath(path);
    }
    else
    {
        nsCAutoString fileurl;
        aURI->GetSpec(fileurl);
        AppendUTF8toUTF16(fileurl, path);
    }
    
    nsAutoString msgId;
    switch(aResult)
    {
    case NS_ERROR_FILE_NAME_TOO_LONG:
        
        msgId.AssignLiteral("fileNameTooLongError");
        break;
    case NS_ERROR_FILE_ALREADY_EXISTS:
        
        msgId.AssignLiteral("fileAlreadyExistsError");
        break;
    case NS_ERROR_FILE_DISK_FULL:
    case NS_ERROR_FILE_NO_DEVICE_SPACE:
        
        msgId.AssignLiteral("diskFull");
        break;

    case NS_ERROR_FILE_READ_ONLY:
        
        msgId.AssignLiteral("readOnly");
        break;

    case NS_ERROR_FILE_ACCESS_DENIED:
        
        msgId.AssignLiteral("accessError");
        break;

    default:
        
        if (aIsReadError)
            msgId.AssignLiteral("readError");
        else
            msgId.AssignLiteral("writeError");
        break;
    }
    
    nsresult rv;
    nsCOMPtr<nsIStringBundleService> s = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && s, NS_ERROR_FAILURE);

    nsCOMPtr<nsIStringBundle> bundle;
    rv = s->CreateBundle(kWebBrowserPersistStringBundle, getter_AddRefs(bundle));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && bundle, NS_ERROR_FAILURE);
    
    nsXPIDLString msgText;
    const PRUnichar *strings[1];
    strings[0] = path.get();
    rv = bundle->FormatStringFromName(msgId.get(), strings, 1, getter_Copies(msgText));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    mProgressListener->OnStatusChange(nsnull, aRequest, aResult, msgText);

    return NS_OK;
}

nsresult nsWebBrowserPersist::GetValidURIFromObject(nsISupports *aObject, nsIURI **aURI) const
{
    NS_ENSURE_ARG_POINTER(aObject);
    NS_ENSURE_ARG_POINTER(aURI);
    
    nsCOMPtr<nsIFile> objAsFile = do_QueryInterface(aObject);
    if (objAsFile)
    {
        return NS_NewFileURI(aURI, objAsFile);
    }
    nsCOMPtr<nsIURI> objAsURI = do_QueryInterface(aObject);
    if (objAsURI)
    {
        *aURI = objAsURI;
        NS_ADDREF(*aURI);
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}

nsresult nsWebBrowserPersist::GetLocalFileFromURI(nsIURI *aURI, nsILocalFile **aLocalFile) const
{
    nsresult rv;

    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv))
        rv = CallQueryInterface(file, aLocalFile);

    return rv;
}

nsresult nsWebBrowserPersist::AppendPathToURI(nsIURI *aURI, const nsAString & aPath) const
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsCAutoString newPath;
    nsresult rv = aURI->GetPath(newPath);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    PRInt32 len = newPath.Length();
    if (len > 0 && newPath.CharAt(len - 1) != '/')
    {
        newPath.Append('/');
    }

    
    AppendUTF16toUTF8(aPath, newPath);
    aURI->SetPath(newPath);

    return NS_OK;
}

nsresult nsWebBrowserPersist::SaveURIInternal(
    nsIURI *aURI, nsISupports *aCacheKey, nsIURI *aReferrer,
    nsIInputStream *aPostData, const char *aExtraHeaders,
    nsIURI *aFile, PRBool aCalcFileExt)
{
    NS_ENSURE_ARG_POINTER(aURI);
    NS_ENSURE_ARG_POINTER(aFile);

    nsresult rv = NS_OK;
    
    mURI = aURI;

    nsLoadFlags loadFlags = nsIRequest::LOAD_NORMAL;
    if (mPersistFlags & PERSIST_FLAGS_BYPASS_CACHE)
    {
        loadFlags |= nsIRequest::LOAD_BYPASS_CACHE;
    }
    else if (mPersistFlags & PERSIST_FLAGS_FROM_CACHE)
    {
        loadFlags |= nsIRequest::LOAD_FROM_CACHE;
    }

    
    nsCOMPtr<nsISupports> cacheKey;
    if (aCacheKey)
    {
        
        
        nsCOMPtr<nsISHEntry> shEntry = do_QueryInterface(aCacheKey);
        if (!shEntry)
        {
            nsCOMPtr<nsIWebPageDescriptor> webPageDescriptor =
                do_QueryInterface(aCacheKey);
            if (webPageDescriptor)
            {
                nsCOMPtr<nsISupports> currentDescriptor;
                webPageDescriptor->GetCurrentDescriptor(getter_AddRefs(currentDescriptor));
                shEntry = do_QueryInterface(currentDescriptor);
            }
        }

        if (shEntry)
        {
            shEntry->GetCacheKey(getter_AddRefs(cacheKey));
        }
        else
        {
            
            cacheKey = aCacheKey;
        }
    }

    
    nsCOMPtr<nsIChannel> inputChannel;
    rv = NS_NewChannel(getter_AddRefs(inputChannel), aURI,
            nsnull, nsnull, static_cast<nsIInterfaceRequestor *>(this),
            loadFlags);
    
    if (NS_FAILED(rv) || inputChannel == nsnull)
    {
        EndDownload(NS_ERROR_FAILURE);
        return NS_ERROR_FAILURE;
    }
    
    
    if (mPersistFlags & PERSIST_FLAGS_NO_CONVERSION)
    {
        nsCOMPtr<nsIEncodedChannel> encodedChannel(do_QueryInterface(inputChannel));
        if (encodedChannel)
        {
            encodedChannel->SetApplyConversion(PR_FALSE);
        }
    }

    if (mPersistFlags & PERSIST_FLAGS_FORCE_ALLOW_COOKIES) 
    {
        nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal =
                do_QueryInterface(inputChannel);
        if (httpChannelInternal)
            httpChannelInternal->SetForceAllowThirdPartyCookie(PR_TRUE);
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(inputChannel));
    if (httpChannel)
    {
        
        if (aReferrer)
        {
            httpChannel->SetReferrer(aReferrer);
        }

        
        if (aPostData)
        {
            nsCOMPtr<nsISeekableStream> stream(do_QueryInterface(aPostData));
            if (stream)
            {
                
                stream->Seek(nsISeekableStream::NS_SEEK_SET, 0);
                nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
                NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");
                
                uploadChannel->SetUploadStream(aPostData, EmptyCString(), -1);
            }
        }

        
        nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(httpChannel));
        if (cacheChannel && cacheKey)
        {
            cacheChannel->SetCacheKey(cacheKey);
        }

        
        if (aExtraHeaders)
        {
            nsCAutoString oneHeader;
            nsCAutoString headerName;
            nsCAutoString headerValue;
            PRInt32 crlf = 0;
            PRInt32 colon = 0;
            const char *kWhitespace = "\b\t\r\n ";
            nsCAutoString extraHeaders(aExtraHeaders);
            while (PR_TRUE)
            {
                crlf = extraHeaders.Find("\r\n", PR_TRUE);
                if (crlf == -1)
                    break;
                extraHeaders.Mid(oneHeader, 0, crlf);
                extraHeaders.Cut(0, crlf + 2);
                colon = oneHeader.Find(":");
                if (colon == -1)
                    break; 
                oneHeader.Left(headerName, colon);
                colon++;
                oneHeader.Mid(headerValue, colon, oneHeader.Length() - colon);
                headerName.Trim(kWhitespace);
                headerValue.Trim(kWhitespace);
                
                rv = httpChannel->SetRequestHeader(headerName, headerValue, PR_TRUE);
                if (NS_FAILED(rv))
                {
                    EndDownload(NS_ERROR_FAILURE);
                    return NS_ERROR_FAILURE;
                }
            }
        }
    }
    return SaveChannelInternal(inputChannel, aFile, aCalcFileExt);
}

nsresult nsWebBrowserPersist::SaveChannelInternal(
    nsIChannel *aChannel, nsIURI *aFile, PRBool aCalcFileExt)
{
    NS_ENSURE_ARG_POINTER(aChannel);
    NS_ENSURE_ARG_POINTER(aFile);

    
    nsresult rv = aChannel->AsyncOpen(this, nsnull);
    if (rv == NS_ERROR_NO_CONTENT)
    {
        
        
        return NS_SUCCESS_DONT_FIXUP;
    }
    else if (NS_FAILED(rv))
    {
        
        if (mPersistFlags & PERSIST_FLAGS_FAIL_ON_BROKEN_LINKS)
        {
            SendErrorStatusChange(PR_TRUE, rv, aChannel, aFile);
            EndDownload(NS_ERROR_FAILURE);
            return NS_ERROR_FAILURE;
        }
        return NS_SUCCESS_DONT_FIXUP;
    }
    else
    {
        
        nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(aChannel);
        nsISupportsKey key(keyPtr);
        mOutputMap.Put(&key, new OutputData(aFile, mURI, aCalcFileExt));
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::GetExtensionForContentType(const PRUnichar *aContentType, PRUnichar **aExt)
{
    NS_ENSURE_ARG_POINTER(aContentType);
    NS_ENSURE_ARG_POINTER(aExt);

    *aExt = nsnull;

    nsresult rv;
    if (!mMIMEService)
    {
        mMIMEService = do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
        NS_ENSURE_TRUE(mMIMEService, NS_ERROR_FAILURE);
    }

    nsCOMPtr<nsIMIMEInfo> mimeInfo;
    nsCAutoString contentType;
    contentType.AssignWithConversion(aContentType);
    nsCAutoString ext;
    rv = mMIMEService->GetPrimaryExtension(contentType, EmptyCString(), ext);
    if (NS_SUCCEEDED(rv))
    {
        *aExt = UTF8ToNewUnicode(ext);
        NS_ENSURE_TRUE(*aExt, NS_ERROR_OUT_OF_MEMORY);
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}

nsresult
nsWebBrowserPersist::GetDocumentExtension(nsIDOMDocument *aDocument, PRUnichar **aExt)
{
    NS_ENSURE_ARG_POINTER(aDocument);
    NS_ENSURE_ARG_POINTER(aExt);

    nsXPIDLString contentType;
    nsresult rv = GetDocEncoderContentType(aDocument, nsnull, getter_Copies(contentType));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    return GetExtensionForContentType(contentType.get(), aExt);
}

nsresult
nsWebBrowserPersist::GetDocEncoderContentType(nsIDOMDocument *aDocument, const PRUnichar *aContentType, PRUnichar **aRealContentType)
{
    NS_ENSURE_ARG_POINTER(aDocument);
    NS_ENSURE_ARG_POINTER(aRealContentType);

    *aRealContentType = nsnull;

    nsAutoString defaultContentType(NS_LITERAL_STRING("text/html"));

    
    

    nsAutoString contentType;
    if (aContentType)
    {
        contentType.Assign(aContentType);
    }
    else
    {
        
        nsCOMPtr<nsIDOMNSDocument> nsDoc = do_QueryInterface(aDocument);
        if (nsDoc)
        {
            nsAutoString type;
            if (NS_SUCCEEDED(nsDoc->GetContentType(type)) && !type.IsEmpty())
            {
                contentType.Assign(type);
            }
        }
    }

    
    
    
    
    
    
    
    
    

    if (!contentType.IsEmpty() &&
        !contentType.Equals(defaultContentType, nsCaseInsensitiveStringComparator()))
    {
        
        nsCAutoString contractID(NS_DOC_ENCODER_CONTRACTID_BASE);
        AppendUTF16toUTF8(contentType, contractID);

        nsCOMPtr<nsIComponentRegistrar> registrar;
        NS_GetComponentRegistrar(getter_AddRefs(registrar));
        if (registrar)
        {
            PRBool result;
            nsresult rv = registrar->IsContractIDRegistered(contractID.get(), &result);
            if (NS_SUCCEEDED(rv) && result)
            {
                *aRealContentType = ToNewUnicode(contentType);
            }
        }
    }

    
    if (!*aRealContentType)
    {
        *aRealContentType = ToNewUnicode(defaultContentType);
    }
    
    NS_ENSURE_TRUE(*aRealContentType, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

nsresult nsWebBrowserPersist::SaveDocumentInternal(
    nsIDOMDocument *aDocument, nsIURI *aFile, nsIURI *aDataPath)
{
    NS_ENSURE_ARG_POINTER(aDocument);
    NS_ENSURE_ARG_POINTER(aFile);

    
    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv = GetLocalFileFromURI(aFile, getter_AddRefs(localFile));

    nsCOMPtr<nsILocalFile> localDataPath;
    if (NS_SUCCEEDED(rv) && aDataPath)
    {
        
        rv = GetLocalFileFromURI(aDataPath, getter_AddRefs(localDataPath));
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    }

    nsCOMPtr<nsIDOMNode> docAsNode = do_QueryInterface(aDocument);

    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDocument));
    mURI = doc->GetDocumentURI();

    nsCOMPtr<nsIURI> oldBaseURI = mCurrentBaseURI;
    nsCAutoString oldCharset(mCurrentCharset);

    
    mCurrentBaseURI = doc->GetBaseURI();
    mCurrentCharset = doc->GetDocumentCharacterSet();

    
    if (aDataPath)
    {
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsIURI> oldDataPath = mCurrentDataPath;
        PRBool oldDataPathIsRelative = mCurrentDataPathIsRelative;
        nsCString oldCurrentRelativePathToData = mCurrentRelativePathToData;
        PRUint32 oldThingsToPersist = mCurrentThingsToPersist;

        mCurrentDataPathIsRelative = PR_FALSE;
        mCurrentDataPath = aDataPath;
        mCurrentRelativePathToData = "";
        mCurrentThingsToPersist = 0;

        
        
        

        
        

        if (localDataPath && localFile)
        {
            nsCOMPtr<nsIFile> baseDir;
            localFile->GetParent(getter_AddRefs(baseDir));

            nsCAutoString relativePathToData;
            nsCOMPtr<nsIFile> dataDirParent;
            dataDirParent = localDataPath;
            while (dataDirParent)
            {
                PRBool sameDir = PR_FALSE;
                dataDirParent->Equals(baseDir, &sameDir);
                if (sameDir)
                {
                    mCurrentRelativePathToData = relativePathToData;
                    mCurrentDataPathIsRelative = PR_TRUE;
                    break;
                }

                nsAutoString dirName;
                dataDirParent->GetLeafName(dirName);

                nsCAutoString newRelativePathToData;
                newRelativePathToData = NS_ConvertUTF16toUTF8(dirName)
                                      + NS_LITERAL_CSTRING("/")
                                      + relativePathToData;
                relativePathToData = newRelativePathToData;

                nsCOMPtr<nsIFile> newDataDirParent;
                rv = dataDirParent->GetParent(getter_AddRefs(newDataDirParent));
                dataDirParent = newDataDirParent;
            }
        }
        else
        {
            
            nsCOMPtr<nsIURL> pathToBaseURL(do_QueryInterface(aFile));
            if (pathToBaseURL)
            {
                nsCAutoString relativePath;  
                if (NS_SUCCEEDED(pathToBaseURL->GetRelativeSpec(aDataPath, relativePath)))
                {
                    mCurrentDataPathIsRelative = PR_TRUE;
                    mCurrentRelativePathToData = relativePath;
                }
            }
        }

        
        
        

        DocData *docData = new DocData;
        docData->mBaseURI = mCurrentBaseURI;
        docData->mCharset = mCurrentCharset;
        docData->mDocument = aDocument;
        docData->mFile = aFile;
        docData->mRelativePathToData = mCurrentRelativePathToData;
        docData->mDataPath = mCurrentDataPath;
        docData->mDataPathIsRelative = mCurrentDataPathIsRelative;
        mDocList.AppendElement(docData);

        
        nsCOMPtr<nsIDOMDocumentTraversal> trav = do_QueryInterface(docData->mDocument, &rv);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
        nsCOMPtr<nsIDOMTreeWalker> walker;
        rv = trav->CreateTreeWalker(docAsNode, 
            nsIDOMNodeFilter::SHOW_ELEMENT |
                nsIDOMNodeFilter::SHOW_DOCUMENT |
                nsIDOMNodeFilter::SHOW_PROCESSING_INSTRUCTION,
            nsnull, PR_TRUE, getter_AddRefs(walker));
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

        nsCOMPtr<nsIDOMNode> currentNode;
        walker->GetCurrentNode(getter_AddRefs(currentNode));
        while (currentNode)
        {
            OnWalkDOMNode(currentNode);
            walker->NextNode(getter_AddRefs(currentNode));
        }

        
        if (mCurrentThingsToPersist > 0)
        {
            if (localDataPath)
            {
                PRBool exists = PR_FALSE;
                PRBool haveDir = PR_FALSE;

                localDataPath->Exists(&exists);
                if (exists)
                {
                    localDataPath->IsDirectory(&haveDir);
                }
                if (!haveDir)
                {
                    rv = localDataPath->Create(nsILocalFile::DIRECTORY_TYPE, 0755);
                    if (NS_SUCCEEDED(rv))
                        haveDir = PR_TRUE;
                    else
                        SendErrorStatusChange(PR_FALSE, rv, nsnull, aFile);
                }
                if (!haveDir)
                {
                    EndDownload(NS_ERROR_FAILURE);
                    mCurrentBaseURI = oldBaseURI;
                    mCurrentCharset = oldCharset;
                    return NS_ERROR_FAILURE;
                }
                if (mPersistFlags & PERSIST_FLAGS_CLEANUP_ON_FAILURE)
                {
                    
                    CleanupData *cleanupData = new CleanupData;
                    NS_ENSURE_TRUE(cleanupData, NS_ERROR_OUT_OF_MEMORY);
                    cleanupData->mFile = localDataPath;
                    cleanupData->mIsDirectory = PR_TRUE;
                    mCleanupList.AppendElement(cleanupData);
                }
#if defined(XP_OS2)
                
                nsCOMPtr<nsILocalFileOS2> localFileOS2 = do_QueryInterface(localDataPath);
                if (localFileOS2)
                {
                    nsCAutoString url;
                    mCurrentBaseURI->GetSpec(url);
                    localFileOS2->SetFileSource(url);
                }
#endif
            }
        }

        mCurrentThingsToPersist = oldThingsToPersist;
        mCurrentDataPath = oldDataPath;
        mCurrentDataPathIsRelative = oldDataPathIsRelative;
        mCurrentRelativePathToData = oldCurrentRelativePathToData;
    }
    else
    {
        
        SetDocumentBase(aDocument, mCurrentBaseURI);

        
        nsXPIDLString realContentType;
        GetDocEncoderContentType(aDocument,
            !mContentType.IsEmpty() ? mContentType.get() : nsnull,
            getter_Copies(realContentType));

        nsCAutoString contentType; contentType.AssignWithConversion(realContentType);
        nsCAutoString charType; 

        
        rv = SaveDocumentWithFixup(
            aDocument,
            nsnull,  
            aFile,
            mReplaceExisting,
            contentType,
            charType,
            mEncodingFlags);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    }

    mCurrentBaseURI = oldBaseURI;
    mCurrentCharset = oldCharset;

    return NS_OK;
}

nsresult nsWebBrowserPersist::SaveDocuments()
{
    nsresult rv = NS_OK;

    mStartSaving = PR_TRUE;

    
    

    PRUint32 i;
    for (i = 0; i < mDocList.Length(); i++)
    {
        DocData *docData = mDocList.ElementAt(i);
        if (!docData)
        {
            rv = NS_ERROR_FAILURE;
            break;
        }

        mCurrentBaseURI = docData->mBaseURI;
        mCurrentCharset = docData->mCharset;

        
        
        nsEncoderNodeFixup *nodeFixup;
        nodeFixup = new nsEncoderNodeFixup;
        if (nodeFixup)
            nodeFixup->mWebBrowserPersist = this;

        
        nsXPIDLString realContentType;
        GetDocEncoderContentType(docData->mDocument,
            !mContentType.IsEmpty() ? mContentType.get() : nsnull,
            getter_Copies(realContentType));

        nsCAutoString contentType; contentType.AssignWithConversion(realContentType.get());
        nsCAutoString charType; 

        
        rv = SaveDocumentWithFixup(
            docData->mDocument,
            nodeFixup,
            docData->mFile,
            mReplaceExisting,
            contentType,
            charType,
            mEncodingFlags);

        if (NS_FAILED(rv))
            break;

        
        if (mSerializingOutput)
            break;
    }

    
    for (i = 0; i < mDocList.Length(); i++)
    {
        DocData *docData = mDocList.ElementAt(i);
        delete docData;
        if (mSerializingOutput)
        {
            mDocList.RemoveElementAt(i);
            break;
        }
    }

    if (!mSerializingOutput)
    {
        mDocList.Clear();
    }

    return rv;
}

void nsWebBrowserPersist::Cleanup()
{
    mURIMap.Enumerate(EnumCleanupURIMap, this);
    mURIMap.Reset();
    mOutputMap.Enumerate(EnumCleanupOutputMap, this);
    mOutputMap.Reset();
    mUploadList.Enumerate(EnumCleanupUploadList, this);
    mUploadList.Reset();
    PRUint32 i;
    for (i = 0; i < mDocList.Length(); i++)
    {
        DocData *docData = mDocList.ElementAt(i);
        delete docData;
    }
    mDocList.Clear();
    for (i = 0; i < mCleanupList.Length(); i++)
    {
        CleanupData *cleanupData = mCleanupList.ElementAt(i);
        delete cleanupData;
    }
    mCleanupList.Clear();
    mFilenameList.Clear();
}

void nsWebBrowserPersist::CleanupLocalFiles()
{
    
    
    
    
    int pass;
    for (pass = 0; pass < 2; pass++)
    {
        PRUint32 i;
        for (i = 0; i < mCleanupList.Length(); i++)
        {
            CleanupData *cleanupData = mCleanupList.ElementAt(i);
            nsCOMPtr<nsILocalFile> file = cleanupData->mFile;

            
            
            PRBool exists = PR_FALSE;
            file->Exists(&exists);
            if (!exists)
                continue;

            
            
            PRBool isDirectory = PR_FALSE;
            file->IsDirectory(&isDirectory);
            if (isDirectory != cleanupData->mIsDirectory)
                continue; 

            if (pass == 0 && !isDirectory)
            {
                file->Remove(PR_FALSE);
            }
            else if (pass == 1 && isDirectory) 
            {
                
                
                
                
                
                
                

                PRBool isEmptyDirectory = PR_TRUE;
                nsCOMArray<nsISimpleEnumerator> dirStack;
                PRInt32 stackSize = 0;

                
                nsCOMPtr<nsISimpleEnumerator> pos;
                if (NS_SUCCEEDED(file->GetDirectoryEntries(getter_AddRefs(pos))))
                    dirStack.AppendObject(pos);

                while (isEmptyDirectory && (stackSize = dirStack.Count()))
                {
                    
                    nsCOMPtr<nsISimpleEnumerator> curPos;
                    curPos = dirStack[stackSize-1];
                    dirStack.RemoveObjectAt(stackSize - 1);
                    
                    
                    PRBool hasMoreElements = PR_FALSE;
                    curPos->HasMoreElements(&hasMoreElements);
                    if (!hasMoreElements)
                    {
                        continue;
                    }

                    
                    
                    nsCOMPtr<nsISupports> child;
                    curPos->GetNext(getter_AddRefs(child));
                    NS_ASSERTION(child, "No child element, but hasMoreElements says otherwise");
                    if (!child)
                        continue;
                    nsCOMPtr<nsILocalFile> childAsFile = do_QueryInterface(child);
                    NS_ASSERTION(childAsFile, "This should be a file but isn't");

                    PRBool childIsSymlink = PR_FALSE;
                    childAsFile->IsSymlink(&childIsSymlink);
                    PRBool childIsDir = PR_FALSE;
                    childAsFile->IsDirectory(&childIsDir);                           
                    if (!childIsDir || childIsSymlink)
                    {
                        
                        
                        isEmptyDirectory = PR_FALSE;
                        break;
                    }
                    
                    nsCOMPtr<nsISimpleEnumerator> childPos;
                    childAsFile->GetDirectoryEntries(getter_AddRefs(childPos));
                    dirStack.AppendObject(curPos);
                    if (childPos)
                        dirStack.AppendObject(childPos);

                }
                dirStack.Clear();

                
                if (isEmptyDirectory)
                {
                    file->Remove(PR_TRUE);
                }
            }
        }
    }
}

nsresult
nsWebBrowserPersist::CalculateUniqueFilename(nsIURI *aURI)
{
    nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
    NS_ENSURE_TRUE(url, NS_ERROR_FAILURE);

    PRBool nameHasChanged = PR_FALSE;
    nsresult rv;

    
    nsCAutoString filename;
    rv = url->GetFileName(filename);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    nsCAutoString directory;
    rv = url->GetDirectory(directory);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    
    
    
    
    
    PRInt32 lastDot = filename.RFind(".");
    nsCAutoString base;
    nsCAutoString ext;
    if (lastDot >= 0)
    {
        filename.Mid(base, 0, lastDot);
        filename.Mid(ext, lastDot, filename.Length() - lastDot); 
    }
    else
    {
        
        base = filename;
    }

    
    PRInt32 needToChop = filename.Length() - kDefaultMaxFilenameLength;
    if (needToChop > 0)
    {
        
        if (base.Length() > (PRUint32) needToChop)
        {
            base.Truncate(base.Length() - needToChop);
        }
        else
        {
            needToChop -= base.Length() - 1;
            base.Truncate(1);
            if (ext.Length() > (PRUint32) needToChop)
            {
                ext.Truncate(ext.Length() - needToChop);
            }
            else
            {
                ext.Truncate(0);
            }
            
            
            
        }

        filename.Assign(base);
        filename.Append(ext);
        nameHasChanged = PR_TRUE;
    }

    
    
    

    if (base.IsEmpty() || !mFilenameList.IsEmpty())
    {
        nsCAutoString tmpPath;
        nsCAutoString tmpBase;
        PRUint32 duplicateCounter = 1;
        while (1)
        {
            
            
            

            if (base.IsEmpty() || duplicateCounter > 1)
            {
                char * tmp = PR_smprintf("_%03d", duplicateCounter);
                NS_ENSURE_TRUE(tmp, NS_ERROR_OUT_OF_MEMORY);
                if (filename.Length() < kDefaultMaxFilenameLength - 4)
                {
                    tmpBase = base;
                }
                else
                {
                    base.Mid(tmpBase, 0, base.Length() - 4);
                }
                tmpBase.Append(tmp);
                PR_smprintf_free(tmp);
            }
            else
            {
                tmpBase = base;
            }
        
            tmpPath.Assign(directory);
            tmpPath.Append(tmpBase);
            tmpPath.Append(ext);

            
            if (!mFilenameList.Contains(tmpPath))
            {
                if (!base.Equals(tmpBase))
                {
                    filename.Assign(tmpBase);
                    filename.Append(ext);
                    nameHasChanged = PR_TRUE;
                }
                break;
            }
            duplicateCounter++;
        }
    }

    
    nsCAutoString newFilepath(directory);
    newFilepath.Append(filename);
    mFilenameList.AppendElement(newFilepath);

    
    if (nameHasChanged)
    {
        
        if (filename.Length() > kDefaultMaxFilenameLength)
        {
            NS_WARNING("Filename wasn't truncated less than the max file length - how can that be?");
            return NS_ERROR_FAILURE;
        }

        nsCOMPtr<nsILocalFile> localFile;
        GetLocalFileFromURI(aURI, getter_AddRefs(localFile));

        if (localFile)
        {
            nsAutoString filenameAsUnichar;
            filenameAsUnichar.AssignWithConversion(filename.get());
            localFile->SetLeafName(filenameAsUnichar);

            
            nsresult rv;
            nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
            NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
            fileURL->SetFile(localFile);  
        }
        else
        {
            url->SetFileName(filename);
        }
    }

    return NS_OK;
}


nsresult
nsWebBrowserPersist::MakeFilenameFromURI(nsIURI *aURI, nsString &aFilename)
{
    
    nsAutoString fileName;

    
    

    nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
    if (url)
    {
        nsCAutoString nameFromURL;
        url->GetFileName(nameFromURL);
        if (mPersistFlags & PERSIST_FLAGS_DONT_CHANGE_FILENAMES)
        {
            fileName.AssignWithConversion(NS_UnescapeURL(nameFromURL).get());
            goto end;
        }
        if (!nameFromURL.IsEmpty())
        {
            
            NS_UnescapeURL(nameFromURL);
            PRUint32 nameLength = 0;
            const char *p = nameFromURL.get();
            for (;*p && *p != ';' && *p != '?' && *p != '#' && *p != '.'
                 ;p++)
            {
                if (nsCRT::IsAsciiAlpha(*p) || nsCRT::IsAsciiDigit(*p)
                    || *p == '.' || *p == '-' ||  *p == '_' || (*p == ' '))
                {
                    fileName.Append(PRUnichar(*p));
                    if (++nameLength == kDefaultMaxFilenameLength)
                    {
                        
                        
                        
                        
                        
                        
                        
                        break;
                    }
                }
            }
        }
    }

    
    
    
    
    if (fileName.IsEmpty())
    {
        fileName.Append(PRUnichar('a')); 
    }
 
end:
    aFilename = fileName;
    return NS_OK;
}


nsresult
nsWebBrowserPersist::CalculateAndAppendFileExt(nsIURI *aURI, nsIChannel *aChannel, nsIURI *aOriginalURIWithExtension)
{
    nsresult rv;

    if (!mMIMEService)
    {
        mMIMEService = do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
        NS_ENSURE_TRUE(mMIMEService, NS_ERROR_FAILURE);
    }

    nsCAutoString contentType;

    
    aChannel->GetContentType(contentType);

    
    if (contentType.IsEmpty())
    {
        nsCOMPtr<nsIURI> uri;
        aChannel->GetOriginalURI(getter_AddRefs(uri));
        mMIMEService->GetTypeFromURI(uri, contentType);
    }

    
    if (!contentType.IsEmpty())
    {
        nsCOMPtr<nsIMIMEInfo> mimeInfo;
        mMIMEService->GetFromTypeAndExtension(
            contentType, EmptyCString(), getter_AddRefs(mimeInfo));

        nsCOMPtr<nsILocalFile> localFile;
        GetLocalFileFromURI(aURI, getter_AddRefs(localFile));

        if (mimeInfo)
        {
            nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
            NS_ENSURE_TRUE(url, NS_ERROR_FAILURE);

            nsCAutoString newFileName;
            url->GetFileName(newFileName);

            
            PRBool hasExtension = PR_FALSE;
            PRInt32 ext = newFileName.RFind(".");
            if (ext != -1)
            {
                mimeInfo->ExtensionExists(Substring(newFileName, ext + 1), &hasExtension);
            }

            
            nsCAutoString fileExt;
            if (!hasExtension)
            {
                
                nsCOMPtr<nsIURL> oldurl(do_QueryInterface(aOriginalURIWithExtension));
                NS_ENSURE_TRUE(oldurl, NS_ERROR_FAILURE);
                oldurl->GetFileExtension(fileExt);
                PRBool useOldExt = PR_FALSE;
                if (!fileExt.IsEmpty())
                {
                    mimeInfo->ExtensionExists(fileExt, &useOldExt);
                }

                
                if (!useOldExt)
                {
                    mimeInfo->GetPrimaryExtension(fileExt);
                } 

                if (!fileExt.IsEmpty())
                {
                    PRUint32 newLength = newFileName.Length() + fileExt.Length() + 1;
                    if (newLength > kDefaultMaxFilenameLength)
                    {
                        newFileName.Truncate(newFileName.Length() - (newLength - kDefaultMaxFilenameLength));
                    }
                    newFileName.Append(".");
                    newFileName.Append(fileExt);
                }

                if (localFile)
                {
                    localFile->SetLeafName(NS_ConvertUTF8toUTF16(newFileName));

                    
                    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
                    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
                    fileURL->SetFile(localFile);  
                }
                else
                {
                    url->SetFileName(newFileName);
                }
            }

        }
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::MakeOutputStream(
    nsIURI *aURI, nsIOutputStream **aOutputStream)
{
    nsresult rv;

    nsCOMPtr<nsILocalFile> localFile;
    GetLocalFileFromURI(aURI, getter_AddRefs(localFile));
    if (localFile)
        rv = MakeOutputStreamFromFile(localFile, aOutputStream);
    else
        rv = MakeOutputStreamFromURI(aURI, aOutputStream);

    return rv;
}

nsresult
nsWebBrowserPersist::MakeOutputStreamFromFile(
    nsILocalFile *aFile, nsIOutputStream **aOutputStream)
{
    nsresult rv = NS_OK;

    nsCOMPtr<nsIFileOutputStream> fileOutputStream =
        do_CreateInstance(NS_LOCALFILEOUTPUTSTREAM_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    PRInt32 ioFlags = -1;
    if (mPersistFlags & nsIWebBrowserPersist::PERSIST_FLAGS_APPEND_TO_FILE)
      ioFlags = PR_APPEND | PR_CREATE_FILE | PR_WRONLY; 
    rv = fileOutputStream->Init(aFile, ioFlags, -1, 0);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOutputStream = NS_BufferOutputStream(fileOutputStream,
                                           BUFFERED_OUTPUT_SIZE).get();

    if (mPersistFlags & PERSIST_FLAGS_CLEANUP_ON_FAILURE)
    {
        
        CleanupData *cleanupData = new CleanupData;
        if (!cleanupData) {
          NS_RELEASE(*aOutputStream);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        cleanupData->mFile = aFile;
        cleanupData->mIsDirectory = PR_FALSE;
        mCleanupList.AppendElement(cleanupData);
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::MakeOutputStreamFromURI(
    nsIURI *aURI, nsIOutputStream  **aOutputStream)
{
    PRUint32 segsize = 8192;
    PRUint32 maxsize = PRUint32(-1);
    nsCOMPtr<nsIStorageStream> storStream;
    nsresult rv = NS_NewStorageStream(segsize, maxsize, getter_AddRefs(storStream));
    NS_ENSURE_SUCCESS(rv, rv);
    
    NS_ENSURE_SUCCESS(CallQueryInterface(storStream, aOutputStream), NS_ERROR_FAILURE);
    return NS_OK;
}

void
nsWebBrowserPersist::EndDownload(nsresult aResult)
{
    
    if (NS_SUCCEEDED(mPersistResult) && NS_FAILED(aResult))
    {
        mPersistResult = aResult;
    }

    
    if (NS_FAILED(aResult) && (mPersistFlags & PERSIST_FLAGS_CLEANUP_ON_FAILURE))
    {
        CleanupLocalFiles();
    }

    
    mCompleted = PR_TRUE;
    Cleanup();
}


class nsMyISupportsKey : public nsISupportsKey
{
public:
    nsMyISupportsKey(nsISupports *key) : nsISupportsKey(key)
    {
    }

    nsresult GetISupports(nsISupports **ret)
    {
        *ret = mKey;
        NS_IF_ADDREF(mKey);
        return NS_OK;
    }
};

struct NS_STACK_CLASS FixRedirectData
{
    nsCOMPtr<nsIChannel> mNewChannel;
    nsCOMPtr<nsIURI> mOriginalURI;
    nsISupportsKey *mMatchingKey;
};

nsresult
nsWebBrowserPersist::FixRedirectedChannelEntry(nsIChannel *aNewChannel)
{
    NS_ENSURE_ARG_POINTER(aNewChannel);
    nsCOMPtr<nsIURI> originalURI;

    
    

    FixRedirectData data;
    data.mMatchingKey = nsnull;
    data.mNewChannel = aNewChannel;
    data.mNewChannel->GetOriginalURI(getter_AddRefs(data.mOriginalURI));
    mOutputMap.Enumerate(EnumFixRedirect, (void *) &data);

    
    

    if (data.mMatchingKey)
    {
        OutputData *outputData = (OutputData *) mOutputMap.Get(data.mMatchingKey);
        NS_ENSURE_TRUE(outputData, NS_ERROR_FAILURE);
        mOutputMap.Remove(data.mMatchingKey);

        
        if (!(mPersistFlags & PERSIST_FLAGS_IGNORE_REDIRECTED_DATA))
        {
            nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(aNewChannel);
            nsISupportsKey key(keyPtr);
            mOutputMap.Put(&key, outputData);
        }
    }

    return NS_OK;
}

PRBool
nsWebBrowserPersist::EnumFixRedirect(nsHashKey *aKey, void *aData, void* closure)
{
    FixRedirectData *data = (FixRedirectData *) closure;

    nsCOMPtr<nsISupports> keyPtr;
    ((nsMyISupportsKey *) aKey)->GetISupports(getter_AddRefs(keyPtr));

    nsCOMPtr<nsIChannel> thisChannel = do_QueryInterface(keyPtr);
    nsCOMPtr<nsIURI> thisURI;

    thisChannel->GetOriginalURI(getter_AddRefs(thisURI));

    
    PRBool matchingURI = PR_FALSE;
    thisURI->Equals(data->mOriginalURI, &matchingURI);
    if (matchingURI)
    {
        data->mMatchingKey = (nsISupportsKey *) aKey;
        return PR_FALSE; 
    }

    return PR_TRUE;
}

void
nsWebBrowserPersist::CalcTotalProgress()
{
    mTotalCurrentProgress = 0;
    mTotalMaxProgress = 0;

    if (mOutputMap.Count() > 0)
    {
        
        mOutputMap.Enumerate(EnumCalcProgress, this);
    }

    if (mUploadList.Count() > 0)
    {
        
        mUploadList.Enumerate(EnumCalcUploadProgress, this);
    }

    
    if (mTotalCurrentProgress == LL_ZERO && mTotalMaxProgress == LL_ZERO)
    {
        
        mTotalCurrentProgress = 10000;
        mTotalMaxProgress = 10000;
    }
}

PRBool
nsWebBrowserPersist::EnumCalcProgress(nsHashKey *aKey, void *aData, void* closure)
{
    nsWebBrowserPersist *pthis = (nsWebBrowserPersist *) closure;
    OutputData *data = (OutputData *) aData;

    
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(data->mFile);
    if (fileURL)
    {
        pthis->mTotalCurrentProgress += data->mSelfProgress;
        pthis->mTotalMaxProgress += data->mSelfProgressMax;
    }
    return PR_TRUE;
}

PRBool
nsWebBrowserPersist::EnumCalcUploadProgress(nsHashKey *aKey, void *aData, void* closure)
{
    if (aData && closure)
    {
        nsWebBrowserPersist *pthis = (nsWebBrowserPersist *) closure;
        UploadData *data = (UploadData *) aData;
        pthis->mTotalCurrentProgress += data->mSelfProgress;
        pthis->mTotalMaxProgress += data->mSelfProgressMax;
    }
    return PR_TRUE;
}

PRBool
nsWebBrowserPersist::EnumCountURIsToPersist(nsHashKey *aKey, void *aData, void* closure)
{
    URIData *data = (URIData *) aData;
    PRUint32 *count = (PRUint32 *) closure;
    if (data->mNeedsPersisting && !data->mSaved)
    {
        (*count)++;
    }
    return PR_TRUE;
}

PRBool
nsWebBrowserPersist::EnumPersistURIs(nsHashKey *aKey, void *aData, void* closure)
{
    URIData *data = (URIData *) aData;
    if (!data->mNeedsPersisting || data->mSaved)
    {
        return PR_TRUE;
    }

    nsWebBrowserPersist *pthis = (nsWebBrowserPersist *) closure;
    nsresult rv;

    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), 
                   nsDependentCString(((nsCStringKey *) aKey)->GetString(),
                                      ((nsCStringKey *) aKey)->GetStringLength()),
                   data->mCharset.get());
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    
    nsCOMPtr<nsIURI> fileAsURI;
    rv = data->mDataPath->Clone(getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    rv = pthis->AppendPathToURI(fileAsURI, data->mFilename);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    rv = pthis->SaveURIInternal(uri, nsnull, nsnull, nsnull, nsnull, fileAsURI, PR_TRUE);
    
    
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if (rv == NS_OK)
    {
        
        

        data->mFile = fileAsURI;
        data->mSaved = PR_TRUE;
    }
    else
    {
        data->mNeedsFixup = PR_FALSE;
    }

    if (pthis->mSerializingOutput)
        return PR_FALSE;

    return PR_TRUE;
}

PRBool
nsWebBrowserPersist::EnumCleanupOutputMap(nsHashKey *aKey, void *aData, void* closure)
{
    nsCOMPtr<nsISupports> keyPtr;
    ((nsMyISupportsKey *) aKey)->GetISupports(getter_AddRefs(keyPtr));
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(keyPtr);
    if (channel)
    {
        channel->Cancel(NS_BINDING_ABORTED);
    }
    OutputData *data = (OutputData *) aData;
    if (data)
    {
        delete data;
    }
    return PR_TRUE;
}


PRBool
nsWebBrowserPersist::EnumCleanupURIMap(nsHashKey *aKey, void *aData, void* closure)
{
    URIData *data = (URIData *) aData;
    if (data)
    {
        delete data; 
    }
    return PR_TRUE;
}


PRBool
nsWebBrowserPersist::EnumCleanupUploadList(nsHashKey *aKey, void *aData, void* closure)
{
    nsCOMPtr<nsISupports> keyPtr;
    ((nsMyISupportsKey *) aKey)->GetISupports(getter_AddRefs(keyPtr));
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(keyPtr);
    if (channel)
    {
        channel->Cancel(NS_BINDING_ABORTED);
    }
    UploadData *data = (UploadData *) aData;
    if (data)
    {
        delete data; 
    }
    return PR_TRUE;
}


PRBool
nsWebBrowserPersist::GetQuotedAttributeValue(
    const nsAString &aSource, const nsAString &aAttribute, nsAString &aValue)
{  
    
    aValue.Truncate();
    nsAString::const_iterator start, end;
    aSource.BeginReading(start);
    aSource.EndReading(end);
    nsAString::const_iterator iter(end);

    while (start != end) {
        if (FindInReadable(aAttribute, start, iter))
        {
            
            while (iter != end && nsCRT::IsAsciiSpace(*iter))
            {
                ++iter;
            }

            if (iter == end)
                break;
            
            
            if (*iter != '=')
            {
                start = iter;
                iter = end;
                continue;
            }
            
            ++iter;

            while (iter != end && nsCRT::IsAsciiSpace(*iter))
            {
                ++iter;
            }

            if (iter == end)
                break;

            PRUnichar q = *iter;
            if (q != '"' && q != '\'')
            {
                start = iter;
                iter = end;
                continue;
            }

            
            ++iter;
            start = iter;
            if (FindCharInReadable(q, iter, end))
            {
                aValue = Substring(start, iter);
                return PR_TRUE;
            }

            
            break;
         }
    }
    return PR_FALSE;
}

nsresult nsWebBrowserPersist::FixupXMLStyleSheetLink(nsIDOMProcessingInstruction *aPI, const nsAString &aHref)
{
    NS_ENSURE_ARG_POINTER(aPI);
    nsresult rv = NS_OK;

    nsAutoString data;
    rv = aPI->GetData(data);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    nsAutoString href;
    GetQuotedAttributeValue(data, NS_LITERAL_STRING("href"), href);

    
    if (!aHref.IsEmpty() && !href.IsEmpty())
    {
        nsAutoString alternate;
        nsAutoString charset;
        nsAutoString title;
        nsAutoString type;
        nsAutoString media;

        GetQuotedAttributeValue(data, NS_LITERAL_STRING("alternate"), alternate);
        GetQuotedAttributeValue(data, NS_LITERAL_STRING("charset"), charset);
        GetQuotedAttributeValue(data, NS_LITERAL_STRING("title"), title);
        GetQuotedAttributeValue(data, NS_LITERAL_STRING("type"), type);
        GetQuotedAttributeValue(data, NS_LITERAL_STRING("media"), media);

        NS_NAMED_LITERAL_STRING(kCloseAttr, "\" ");
        nsAutoString newData;
        newData += NS_LITERAL_STRING("href=\"") + aHref + kCloseAttr;
        if (!title.IsEmpty())
        {
            newData += NS_LITERAL_STRING("title=\"") + title + kCloseAttr;
        }
        if (!media.IsEmpty())
        {
            newData += NS_LITERAL_STRING("media=\"") + media + kCloseAttr;
        }
        if (!type.IsEmpty())
        {
            newData += NS_LITERAL_STRING("type=\"") + type + kCloseAttr;
        }
        if (!charset.IsEmpty())
        {
            newData += NS_LITERAL_STRING("charset=\"") + charset + kCloseAttr;
        }
        if (!alternate.IsEmpty())
        {
            newData += NS_LITERAL_STRING("alternate=\"") + alternate + kCloseAttr;
        }
        newData.Truncate(newData.Length() - 1);  
        aPI->SetData(newData);
    }

    return rv;
}

nsresult nsWebBrowserPersist::GetXMLStyleSheetLink(nsIDOMProcessingInstruction *aPI, nsAString &aHref)
{
    NS_ENSURE_ARG_POINTER(aPI);

    nsresult rv = NS_OK;
    nsAutoString data;
    rv = aPI->GetData(data);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    GetQuotedAttributeValue(data, NS_LITERAL_STRING("href"), aHref);

    return NS_OK;
}

nsresult nsWebBrowserPersist::OnWalkDOMNode(nsIDOMNode *aNode)
{
    
    nsCOMPtr<nsIDOMProcessingInstruction> nodeAsPI = do_QueryInterface(aNode);
    if (nodeAsPI)
    {
        nsAutoString target;
        nodeAsPI->GetTarget(target);
        if (target.EqualsLiteral("xml-stylesheet"))
        {
            nsAutoString href;
            GetXMLStyleSheetLink(nodeAsPI, href);
            if (!href.IsEmpty())
            {
                StoreURI(NS_ConvertUTF16toUTF8(href).get());
            }
        }
        return NS_OK;
    }

    
    nsCOMPtr<nsIDOMHTMLImageElement> nodeAsImage = do_QueryInterface(aNode);
    if (nodeAsImage)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }

#ifdef MOZ_SVG
    nsCOMPtr<nsIDOMSVGImageElement> nodeAsSVGImage = do_QueryInterface(aNode);
    if (nodeAsSVGImage)
    {
        StoreURIAttributeNS(aNode, "http://www.w3.org/1999/xlink", "href");
        return NS_OK;
    }
#endif 

    nsCOMPtr<nsIDOMHTMLBodyElement> nodeAsBody = do_QueryInterface(aNode);
    if (nodeAsBody)
    {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLTableElement> nodeAsTable = do_QueryInterface(aNode);
    if (nodeAsTable)
    {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLTableRowElement> nodeAsTableRow = do_QueryInterface(aNode);
    if (nodeAsTableRow)
    {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLTableCellElement> nodeAsTableCell = do_QueryInterface(aNode);
    if (nodeAsTableCell)
    {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLScriptElement> nodeAsScript = do_QueryInterface(aNode);
    if (nodeAsScript)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }

#ifdef MOZ_SVG
    nsCOMPtr<nsIDOMSVGScriptElement> nodeAsSVGScript = do_QueryInterface(aNode);
    if (nodeAsSVGScript)
    {
        StoreURIAttributeNS(aNode, "http://www.w3.org/1999/xlink", "href");
        return NS_OK;
    }
#endif 

    nsCOMPtr<nsIDOMHTMLEmbedElement> nodeAsEmbed = do_QueryInterface(aNode);
    if (nodeAsEmbed)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }
    
    nsCOMPtr<nsIDOMHTMLObjectElement> nodeAsObject = do_QueryInterface(aNode);
    if (nodeAsObject)
    {
        StoreURIAttribute(aNode, "data");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLAppletElement> nodeAsApplet = do_QueryInterface(aNode);
    if (nodeAsApplet)
    {
        
        
        nsCOMPtr<nsIURI> oldBase = mCurrentBaseURI;
        nsAutoString codebase;
        nodeAsApplet->GetCodeBase(codebase);
        if (!codebase.IsEmpty()) {
            nsCOMPtr<nsIURI> baseURI;
            NS_NewURI(getter_AddRefs(baseURI), codebase,
                      mCurrentCharset.get(), mCurrentBaseURI);
            if (baseURI) {
                mCurrentBaseURI = baseURI;
            }
        }

        URIData *archiveURIData = nsnull;
        StoreURIAttribute(aNode, "archive", PR_TRUE, &archiveURIData);
        
        
        if (!archiveURIData)
            StoreURIAttribute(aNode, "code");

        
        mCurrentBaseURI = oldBase;
        return NS_OK;
    }
    
    nsCOMPtr<nsIDOMHTMLLinkElement> nodeAsLink = do_QueryInterface(aNode);
    if (nodeAsLink)
    {
        
        nsAutoString linkRel;
        if (NS_SUCCEEDED(nodeAsLink->GetRel(linkRel)) && !linkRel.IsEmpty())
        {
            nsReadingIterator<PRUnichar> start;
            nsReadingIterator<PRUnichar> end;
            nsReadingIterator<PRUnichar> current;

            linkRel.BeginReading(start);
            linkRel.EndReading(end);

            
            for (current = start; current != end; ++current)
            {
                
                if (nsCRT::IsAsciiSpace(*current))
                    continue;

                
                nsReadingIterator<PRUnichar> startWord = current;
                do {
                    ++current;
                } while (current != end && !nsCRT::IsAsciiSpace(*current));

                
                if (Substring(startWord, current)
                        .LowerCaseEqualsLiteral("stylesheet"))
                {
                    StoreURIAttribute(aNode, "href");
                    return NS_OK;
                }
                if (current == end)
                    break;
            }
        }
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLFrameElement> nodeAsFrame = do_QueryInterface(aNode);
    if (nodeAsFrame)
    {
        URIData *data = nsnull;
        StoreURIAttribute(aNode, "src", PR_FALSE, &data);
        if (data)
        {
            data->mIsSubFrame = PR_TRUE;
            
            nsCOMPtr<nsIDOMDocument> content;
            nodeAsFrame->GetContentDocument(getter_AddRefs(content));
            if (content)
            {
                SaveSubframeContent(content, data);
            }
        }
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLIFrameElement> nodeAsIFrame = do_QueryInterface(aNode);
    if (nodeAsIFrame && !(mPersistFlags & PERSIST_FLAGS_IGNORE_IFRAMES))
    {
        URIData *data = nsnull;
        StoreURIAttribute(aNode, "src", PR_FALSE, &data);
        if (data)
        {
            data->mIsSubFrame = PR_TRUE;
            
            nsCOMPtr<nsIDOMDocument> content;
            nodeAsIFrame->GetContentDocument(getter_AddRefs(content));
            if (content)
            {
                SaveSubframeContent(content, data);
            }
        }
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLInputElement> nodeAsInput = do_QueryInterface(aNode);
    if (nodeAsInput)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::GetNodeToFixup(nsIDOMNode *aNodeIn, nsIDOMNode **aNodeOut)
{
    if (!(mPersistFlags & PERSIST_FLAGS_FIXUP_ORIGINAL_DOM))
    {
        nsresult rv = aNodeIn->CloneNode(PR_FALSE, aNodeOut);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
        NS_ADDREF(*aNodeOut = aNodeIn);
    }
    nsCOMPtr<nsIDOMHTMLElement> element(do_QueryInterface(*aNodeOut));
    if (element) {
        
        nsAutoString namespaceURI;
        element->GetNamespaceURI(namespaceURI);
        if (namespaceURI.IsEmpty()) {
            
            
            
            
            element->RemoveAttribute(NS_LITERAL_STRING("_base_href"));
        }
    }
    return NS_OK;
}

nsresult
nsWebBrowserPersist::CloneNodeWithFixedUpAttributes(
    nsIDOMNode *aNodeIn, PRBool *aSerializeCloneKids, nsIDOMNode **aNodeOut)
{
    nsresult rv;
    *aNodeOut = nsnull;
    *aSerializeCloneKids = PR_FALSE;

    
    nsCOMPtr<nsIDOMProcessingInstruction> nodeAsPI = do_QueryInterface(aNodeIn);
    if (nodeAsPI)
    {
        nsAutoString target;
        nodeAsPI->GetTarget(target);
        if (target.EqualsLiteral("xml-stylesheet"))
        {
            rv = GetNodeToFixup(aNodeIn, aNodeOut);
            if (NS_SUCCEEDED(rv) && *aNodeOut)
            {
                nsCOMPtr<nsIDOMProcessingInstruction> outNode = do_QueryInterface(*aNodeOut);
                nsAutoString href;
                GetXMLStyleSheetLink(nodeAsPI, href);
                if (!href.IsEmpty())
                {
                    FixupURI(href);
                    FixupXMLStyleSheetLink(outNode, href);
                }
            }
        }
    }

    

    if (!(mPersistFlags & PERSIST_FLAGS_NO_BASE_TAG_MODIFICATIONS))
    {
        nsCOMPtr<nsIDOMHTMLBaseElement> nodeAsBase = do_QueryInterface(aNodeIn);
        if (nodeAsBase)
        {
            nsCOMPtr<nsIDOMDocument> ownerDocument;
            nodeAsBase->GetOwnerDocument(getter_AddRefs(ownerDocument));
            if (ownerDocument)
            {
                nsAutoString href;
                nodeAsBase->GetHref(href); 
                nsCOMPtr<nsIDOMComment> comment;
                nsAutoString commentText; commentText.AssignLiteral(" base ");
                if (!href.IsEmpty())
                {
                    commentText += NS_LITERAL_STRING("href=\"") + href + NS_LITERAL_STRING("\" ");
                }
                rv = ownerDocument->CreateComment(commentText, getter_AddRefs(comment));
                if (comment)
                {
                    return CallQueryInterface(comment, aNodeOut);
                }
            }
        }
    }

    

    nsCOMPtr<nsIDOMHTMLAnchorElement> nodeAsAnchor = do_QueryInterface(aNodeIn);
    if (nodeAsAnchor)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupAnchor(*aNodeOut);
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLAreaElement> nodeAsArea = do_QueryInterface(aNodeIn);
    if (nodeAsArea)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupAnchor(*aNodeOut);
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLBodyElement> nodeAsBody = do_QueryInterface(aNodeIn);
    if (nodeAsBody)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLTableElement> nodeAsTable = do_QueryInterface(aNodeIn);
    if (nodeAsTable)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLTableRowElement> nodeAsTableRow = do_QueryInterface(aNodeIn);
    if (nodeAsTableRow)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLTableCellElement> nodeAsTableCell = do_QueryInterface(aNodeIn);
    if (nodeAsTableCell)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLImageElement> nodeAsImage = do_QueryInterface(aNodeIn);
    if (nodeAsImage)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            
            nsCOMPtr<nsIImageLoadingContent> imgCon =
                do_QueryInterface(*aNodeOut);
            if (imgCon)
                imgCon->SetLoadingEnabled(PR_FALSE);

            FixupAnchor(*aNodeOut);
            FixupNodeAttribute(*aNodeOut, "src");
        }
        return rv;
    }

#ifdef MOZ_SVG
    nsCOMPtr<nsIDOMSVGImageElement> nodeAsSVGImage = do_QueryInterface(aNodeIn);
    if (nodeAsSVGImage)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            
            nsCOMPtr<nsIImageLoadingContent> imgCon =
                do_QueryInterface(*aNodeOut);
            if (imgCon)
                imgCon->SetLoadingEnabled(PR_FALSE);

            
            FixupNodeAttributeNS(*aNodeOut, "http://www.w3.org/1999/xlink", "href");
        }
        return rv;
    }
#endif 

    nsCOMPtr<nsIDOMHTMLScriptElement> nodeAsScript = do_QueryInterface(aNodeIn);
    if (nodeAsScript)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "src");
        }
        return rv;
    }

#ifdef MOZ_SVG
    nsCOMPtr<nsIDOMSVGScriptElement> nodeAsSVGScript = do_QueryInterface(aNodeIn);
    if (nodeAsSVGScript)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttributeNS(*aNodeOut, "http://www.w3.org/1999/xlink", "href");
        }
        return rv;
    }
#endif 

    nsCOMPtr<nsIDOMHTMLEmbedElement> nodeAsEmbed = do_QueryInterface(aNodeIn);
    if (nodeAsEmbed)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "src");
        }
        return rv;
    }
    
    nsCOMPtr<nsIDOMHTMLObjectElement> nodeAsObject = do_QueryInterface(aNodeIn);
    if (nodeAsObject)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "data");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLAppletElement> nodeAsApplet = do_QueryInterface(aNodeIn);
    if (nodeAsApplet)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            nsCOMPtr<nsIDOMHTMLAppletElement> newApplet =
                do_QueryInterface(*aNodeOut);
            
            
            nsCOMPtr<nsIURI> oldBase = mCurrentBaseURI;
            nsAutoString codebase;
            nodeAsApplet->GetCodeBase(codebase);
            if (!codebase.IsEmpty()) {
                nsCOMPtr<nsIURI> baseURI;
                NS_NewURI(getter_AddRefs(baseURI), codebase,
                          mCurrentCharset.get(), mCurrentBaseURI);
                if (baseURI) {
                    mCurrentBaseURI = baseURI;
                }
            }
            
            
            newApplet->RemoveAttribute(NS_LITERAL_STRING("codebase"));
            FixupNodeAttribute(*aNodeOut, "code");
            FixupNodeAttribute(*aNodeOut, "archive");
            
            mCurrentBaseURI = oldBase;
        }
        return rv;
    }
    
    nsCOMPtr<nsIDOMHTMLLinkElement> nodeAsLink = do_QueryInterface(aNodeIn);
    if (nodeAsLink)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            
            rv = FixupNodeAttribute(*aNodeOut, "href");
            if (NS_FAILED(rv))
            {
                
                FixupAnchor(*aNodeOut);
            }
            
            
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLFrameElement> nodeAsFrame = do_QueryInterface(aNodeIn);
    if (nodeAsFrame)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "src");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLIFrameElement> nodeAsIFrame = do_QueryInterface(aNodeIn);
    if (nodeAsIFrame)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "src");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLInputElement> nodeAsInput = do_QueryInterface(aNodeIn);
    if (nodeAsInput)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            
            nsCOMPtr<nsIImageLoadingContent> imgCon =
                do_QueryInterface(*aNodeOut);
            if (imgCon)
                imgCon->SetLoadingEnabled(PR_FALSE);

            FixupNodeAttribute(*aNodeOut, "src");

            nsAutoString valueStr;
            NS_NAMED_LITERAL_STRING(valueAttr, "value");
            
            nsCOMPtr<nsIDOMHTMLInputElement> outElt = do_QueryInterface(*aNodeOut);
            nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(*aNodeOut);
            switch (formControl->GetType()) {
                case NS_FORM_INPUT_TEXT:
                    nodeAsInput->GetValue(valueStr);
                    
                    if (valueStr.IsEmpty())
                      outElt->RemoveAttribute(valueAttr);
                    else
                      outElt->SetAttribute(valueAttr, valueStr);
                    break;
                case NS_FORM_INPUT_CHECKBOX:
                case NS_FORM_INPUT_RADIO:
                    PRBool checked;
                    nodeAsInput->GetChecked(&checked);
                    outElt->SetDefaultChecked(checked);
                    break;
                default:
                    break;
            }
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLTextAreaElement> nodeAsTextArea = do_QueryInterface(aNodeIn);
    if (nodeAsTextArea)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            
            *aSerializeCloneKids = PR_TRUE;

            nsAutoString valueStr;
            nodeAsTextArea->GetValue(valueStr);
            
            nsCOMPtr<nsIDOM3Node> out = do_QueryInterface(*aNodeOut);
            out->SetTextContent(valueStr);
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLOptionElement> nodeAsOption = do_QueryInterface(aNodeIn);
    if (nodeAsOption)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {          
            nsCOMPtr<nsIDOMHTMLOptionElement> outElt = do_QueryInterface(*aNodeOut);
            PRBool selected;
            nodeAsOption->GetSelected(&selected);
            outElt->SetDefaultSelected(selected);
        }
        return rv;
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::StoreURI(
    const char *aURI, PRBool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri),
                            nsDependentCString(aURI),
                            mCurrentCharset.get(),
                            mCurrentBaseURI);
    NS_ENSURE_SUCCESS(rv, rv);

    return StoreURI(uri, aNeedsPersisting, aData);
}

nsresult
nsWebBrowserPersist::StoreURI(
    nsIURI *aURI, PRBool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aURI);
    if (aData)
    {
        *aData = nsnull;
    }

    
    
    PRBool doNotPersistURI;
    nsresult rv = NS_URIChainHasFlags(aURI,
                                      nsIProtocolHandler::URI_NON_PERSISTABLE,
                                      &doNotPersistURI);
    if (NS_FAILED(rv))
    {
        doNotPersistURI = PR_FALSE;
    }

    if (doNotPersistURI)
    {
        return NS_OK;
    }

    URIData *data = nsnull;
    MakeAndStoreLocalFilenameInURIMap(aURI, aNeedsPersisting, &data);
    if (aData)
    {
        *aData = data;
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::StoreURIAttributeNS(
    nsIDOMNode *aNode, const char *aNamespaceURI, const char *aAttribute,
    PRBool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aNode);
    NS_ENSURE_ARG_POINTER(aNamespaceURI);
    NS_ENSURE_ARG_POINTER(aAttribute);

    nsresult rv = NS_OK;

    
    

    nsCOMPtr<nsIDOMNamedNodeMap> attrMap;
    nsCOMPtr<nsIDOMNode> attrNode;
    rv = aNode->GetAttributes(getter_AddRefs(attrMap));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    NS_ConvertASCIItoUTF16 namespaceURI(aNamespaceURI);
    NS_ConvertASCIItoUTF16 attribute(aAttribute);
    rv = attrMap->GetNamedItemNS(namespaceURI, attribute, getter_AddRefs(attrNode));
    if (attrNode)
    {
        nsAutoString oldValue;
        attrNode->GetNodeValue(oldValue);
        if (!oldValue.IsEmpty())
        {
            NS_ConvertUTF16toUTF8 oldCValue(oldValue);
            return StoreURI(oldCValue.get(), aNeedsPersisting, aData);
        }
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::FixupURI(nsAString &aURI)
{
    
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI, 
                            mCurrentCharset.get(), mCurrentBaseURI);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCStringKey key(spec.get());
    if (!mURIMap.Exists(&key))
    {
        return NS_ERROR_FAILURE;
    }
    URIData *data = (URIData *) mURIMap.Get(&key);
    if (!data->mNeedsFixup)
    {
        return NS_OK;
    }
    nsCOMPtr<nsIURI> fileAsURI;
    if (data->mFile)
    {
        rv = data->mFile->Clone(getter_AddRefs(fileAsURI)); 
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else
    {
        rv = data->mDataPath->Clone(getter_AddRefs(fileAsURI));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = AppendPathToURI(fileAsURI, data->mFilename);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    nsAutoString newValue;

    
    fileAsURI->SetUserPass(EmptyCString());

    
    
    if (data->mDataPathIsRelative)
    {
        nsCOMPtr<nsIURL> url(do_QueryInterface(fileAsURI));
        if (!url)
          return NS_ERROR_FAILURE;
          
        nsCAutoString filename;
        url->GetFileName(filename);

        nsCAutoString rawPathURL(data->mRelativePathToData);
        rawPathURL.Append(filename);

        nsCAutoString buf;
        AppendUTF8toUTF16(NS_EscapeURL(rawPathURL, esc_FilePath, buf),
                          newValue);
    }
    else
    {
        nsCAutoString fileurl;
        fileAsURI->GetSpec(fileurl);
        AppendUTF8toUTF16(fileurl, newValue);
    }
    if (data->mIsSubFrame)
    {
        newValue.Append(data->mSubFrameExt);
    }

    aURI = newValue;
    return NS_OK;
}

nsresult
nsWebBrowserPersist::FixupNodeAttributeNS(nsIDOMNode *aNode,
                                        const char *aNamespaceURI,
                                        const char *aAttribute)
{
    NS_ENSURE_ARG_POINTER(aNode);
    NS_ENSURE_ARG_POINTER(aNamespaceURI);
    NS_ENSURE_ARG_POINTER(aAttribute);

    nsresult rv = NS_OK;

    
    

    nsCOMPtr<nsIDOMNamedNodeMap> attrMap;
    nsCOMPtr<nsIDOMNode> attrNode;
    rv = aNode->GetAttributes(getter_AddRefs(attrMap));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    NS_ConvertASCIItoUTF16 attribute(aAttribute);
    NS_ConvertASCIItoUTF16 namespaceURI(aNamespaceURI);
    rv = attrMap->GetNamedItemNS(namespaceURI, attribute, getter_AddRefs(attrNode));
    if (attrNode)
    {
        nsString uri;
        attrNode->GetNodeValue(uri);
        rv = FixupURI(uri);
        if (NS_SUCCEEDED(rv))
        {
            attrNode->SetNodeValue(uri);
        }
    }

    return rv;
}

nsresult
nsWebBrowserPersist::FixupAnchor(nsIDOMNode *aNode)
{
    NS_ENSURE_ARG_POINTER(aNode);

    nsCOMPtr<nsIDOMNamedNodeMap> attrMap;
    nsCOMPtr<nsIDOMNode> attrNode;
    nsresult rv = aNode->GetAttributes(getter_AddRefs(attrMap));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    if (mPersistFlags & PERSIST_FLAGS_DONT_FIXUP_LINKS)
    {
        return NS_OK;
    }

    
    nsString attribute(NS_LITERAL_STRING("href"));
    rv = attrMap->GetNamedItem(attribute, getter_AddRefs(attrNode));
    if (attrNode)
    {
        nsString oldValue;
        attrNode->GetNodeValue(oldValue);
        NS_ConvertUTF16toUTF8 oldCValue(oldValue);

        
        if (oldCValue.IsEmpty() || oldCValue.CharAt(0) == '#')
        {
            return NS_OK;
        }

        
        PRBool isEqual = PR_FALSE;
        if (NS_SUCCEEDED(mCurrentBaseURI->Equals(mTargetBaseURI, &isEqual))
            && isEqual)
        {
            return NS_OK;
        }

        nsCOMPtr<nsIURI> relativeURI;
        relativeURI = (mPersistFlags & PERSIST_FLAGS_FIXUP_LINKS_TO_DESTINATION)
                      ? mTargetBaseURI : mCurrentBaseURI;
        
        nsCOMPtr<nsIURI> newURI;
        rv = NS_NewURI(getter_AddRefs(newURI), oldCValue, 
                       mCurrentCharset.get(), relativeURI);
        if (NS_SUCCEEDED(rv) && newURI)
        {
            newURI->SetUserPass(EmptyCString());
            nsCAutoString uriSpec;
            newURI->GetSpec(uriSpec);
            attrNode->SetNodeValue(NS_ConvertUTF8toUTF16(uriSpec));
        }
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::StoreAndFixupStyleSheet(nsIStyleSheet *aStyleSheet)
{
    
    return NS_OK;
}

PRBool
nsWebBrowserPersist::DocumentEncoderExists(const PRUnichar *aContentType)
{
    
    nsCAutoString contractID(NS_DOC_ENCODER_CONTRACTID_BASE);
    AppendUTF16toUTF8(aContentType, contractID);

    nsCOMPtr<nsIComponentRegistrar> registrar;
    NS_GetComponentRegistrar(getter_AddRefs(registrar));
    if (registrar)
    {
        PRBool result;
        nsresult rv = registrar->IsContractIDRegistered(contractID.get(),
                                                        &result);
        if (NS_SUCCEEDED(rv) && result)
        {
            return PR_TRUE;
        }
    }
    return PR_FALSE;
}

nsresult
nsWebBrowserPersist::SaveSubframeContent(
    nsIDOMDocument *aFrameContent, URIData *aData)
{
    NS_ENSURE_ARG_POINTER(aData);

    
    nsCOMPtr<nsIDocument> frameDoc(do_QueryInterface(aFrameContent));
    NS_ENSURE_STATE(frameDoc);

    nsAutoString contentType;
    nsresult rv = frameDoc->GetContentType(contentType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLString ext;
    GetExtensionForContentType(contentType.get(), getter_Copies(ext));

    
    
    if (ext.IsEmpty())
    {
        nsCOMPtr<nsIURL> url(do_QueryInterface(frameDoc->GetDocumentURI(),
                                               &rv));
        nsCAutoString extension;
        if (NS_SUCCEEDED(rv))
        {
            url->GetFileExtension(extension);
        }
        else
        {
            extension.AssignLiteral("htm");
        }
        aData->mSubFrameExt.Assign(PRUnichar('.'));
        AppendUTF8toUTF16(extension, aData->mSubFrameExt);
    }
    else
    {
        aData->mSubFrameExt.Assign(PRUnichar('.'));
        aData->mSubFrameExt.Append(ext);
    }

    nsString filenameWithExt = aData->mFilename;
    filenameWithExt.Append(aData->mSubFrameExt);

    
    nsCOMPtr<nsIURI> frameURI;
    rv = mCurrentDataPath->Clone(getter_AddRefs(frameURI));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AppendPathToURI(frameURI, filenameWithExt);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIURI> frameDataURI;
    rv = mCurrentDataPath->Clone(getter_AddRefs(frameDataURI));
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoString newFrameDataPath(aData->mFilename);

    
    newFrameDataPath.AppendLiteral("_data");
    rv = AppendPathToURI(frameDataURI, newFrameDataPath);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = CalculateUniqueFilename(frameURI);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = CalculateUniqueFilename(frameDataURI);
    NS_ENSURE_SUCCESS(rv, rv);

    mCurrentThingsToPersist++;

    
    
    if (DocumentEncoderExists(contentType.get()))
    {
        rv = SaveDocumentInternal(aFrameContent, frameURI, frameDataURI);
    }
    else
    {
        rv = StoreURI(frameDoc->GetDocumentURI());
    }
    NS_ENSURE_SUCCESS(rv, rv);

    
    aData->mFile = frameURI;
    aData->mSubFrameExt.Truncate(); 

    return NS_OK;
}

nsresult
nsWebBrowserPersist::CreateChannelFromURI(nsIURI *aURI, nsIChannel **aChannel)
{
    nsresult rv = NS_OK;
    *aChannel = nsnull;

    nsCOMPtr<nsIIOService> ioserv;
    ioserv = do_GetIOService(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ioserv->NewChannelFromURI(aURI, aChannel);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG_POINTER(*aChannel);

    rv = (*aChannel)->SetNotificationCallbacks(static_cast<nsIInterfaceRequestor *>(this));
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
} 

nsresult
nsWebBrowserPersist::SaveDocumentWithFixup(
    nsIDOMDocument *aDocument, nsIDocumentEncoderNodeFixup *aNodeFixup,
    nsIURI *aFile, PRBool aReplaceExisting, const nsACString &aFormatType,
    const nsCString &aSaveCharset, PRUint32 aFlags)
{
    NS_ENSURE_ARG_POINTER(aFile);
    
    nsresult  rv = NS_OK;
    nsCOMPtr<nsILocalFile> localFile;
    GetLocalFileFromURI(aFile, getter_AddRefs(localFile));
    if (localFile)
    {
        
        
        PRBool fileExists = PR_FALSE;
        rv = localFile->Exists(&fileExists);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

        if (!aReplaceExisting && fileExists)
            return NS_ERROR_FAILURE;                
    }
    
    nsCOMPtr<nsIOutputStream> outputStream;
    rv = MakeOutputStream(aFile, getter_AddRefs(outputStream));
    if (NS_FAILED(rv))
    {
        SendErrorStatusChange(PR_FALSE, rv, nsnull, aFile);
        return NS_ERROR_FAILURE;
    }
    NS_ENSURE_TRUE(outputStream, NS_ERROR_FAILURE);

    
    nsCAutoString contractID(NS_DOC_ENCODER_CONTRACTID_BASE);
    contractID.Append(aFormatType);
    
    nsCOMPtr<nsIDocumentEncoder> encoder = do_CreateInstance(contractID.get(), &rv);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    NS_ConvertASCIItoUTF16 newContentType(aFormatType);
    rv = encoder->Init(aDocument, newContentType, aFlags);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    mTargetBaseURI = aFile;

    
    encoder->SetNodeFixup(aNodeFixup);

    if (mWrapColumn && (aFlags & ENCODE_FLAGS_WRAP))
        encoder->SetWrapColumn(mWrapColumn);

    nsCAutoString charsetStr(aSaveCharset);
    if (charsetStr.IsEmpty())
    {
        nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDocument);
        NS_ASSERTION(doc, "Need a document");
        charsetStr = doc->GetDocumentCharacterSet();
    }

    rv = encoder->SetCharset(charsetStr);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    rv = encoder->EncodeToStream(outputStream);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    
    if (!localFile)
    {
        nsCOMPtr<nsIStorageStream> storStream(do_QueryInterface(outputStream));
        if (storStream)
        {
            outputStream->Close();
            rv = StartUpload(storStream, aFile, aFormatType);
            NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
        }
    }
#if defined(XP_OS2)
    else
    {
        
        outputStream->Close();
        nsCOMPtr<nsILocalFileOS2> localFileOS2 = do_QueryInterface(localFile);
        if (localFileOS2)
        {
            nsCAutoString url;
            mCurrentBaseURI->GetSpec(url);
            localFileOS2->SetFileSource(url);
        }
    }
#endif

    return rv;
}



nsresult
nsWebBrowserPersist::MakeAndStoreLocalFilenameInURIMap(
    nsIURI *aURI, PRBool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsCAutoString spec;
    nsresult rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    nsCStringKey key(spec.get());
    URIData *data;
    if (mURIMap.Exists(&key))
    {
        data = (URIData *) mURIMap.Get(&key);
        if (aNeedsPersisting)
        {
          data->mNeedsPersisting = PR_TRUE;
        }
        if (aData)
        {
            *aData = data;
        }
        return NS_OK;
    }

    
    nsString filename;
    rv = MakeFilenameFromURI(aURI, filename);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    data = new URIData;
    NS_ENSURE_TRUE(data, NS_ERROR_OUT_OF_MEMORY);

    data->mNeedsPersisting = aNeedsPersisting;
    data->mNeedsFixup = PR_TRUE;
    data->mFilename = filename;
    data->mSaved = PR_FALSE;
    data->mIsSubFrame = PR_FALSE;
    data->mDataPath = mCurrentDataPath;
    data->mDataPathIsRelative = mCurrentDataPathIsRelative;
    data->mRelativePathToData = mCurrentRelativePathToData;
    data->mCharset = mCurrentCharset;

    if (aNeedsPersisting)
        mCurrentThingsToPersist++;

    mURIMap.Put(&key, data);
    if (aData)
    {
        *aData = data;
    }

    return NS_OK;
}



static const char kSpecialXHTMLTags[][11] = {
    "body",
    "head",
    "img",
    "script",
    "a",
    "area",
    "link",
    "input",
    "frame",
    "iframe",
    "object",
    "applet",
    "form",
    "blockquote",
    "q",
    "del",
    "ins"
};

static PRBool IsSpecialXHTMLTag(nsIDOMNode *aNode)
{
    nsAutoString tmp;
    aNode->GetNamespaceURI(tmp);
    if (!tmp.EqualsLiteral("http://www.w3.org/1999/xhtml"))
        return PR_FALSE;

    aNode->GetLocalName(tmp);
    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(kSpecialXHTMLTags); i++) {
        if (tmp.EqualsASCII(kSpecialXHTMLTags[i]))
        {
            
            
            
            
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

static PRBool HasSpecialXHTMLTags(nsIDOMNode *aParent)
{
    if (IsSpecialXHTMLTag(aParent))
        return PR_TRUE;

    nsCOMPtr<nsIDOMNodeList> list;
    aParent->GetChildNodes(getter_AddRefs(list));
    if (list)
    {
        PRUint32 count;
        list->GetLength(&count);
        PRUint32 i;
        for (i = 0; i < count; i++) {
            nsCOMPtr<nsIDOMNode> node;
            list->Item(i, getter_AddRefs(node));
            if (!node)
                break;
            PRUint16 nodeType;
            node->GetNodeType(&nodeType);
            if (nodeType == nsIDOMNode::ELEMENT_NODE) {
                return HasSpecialXHTMLTags(node);
            }
        }
    }

    return PR_FALSE;
}

static PRBool NeedXHTMLBaseTag(nsIDOMDocument *aDocument)
{
    nsCOMPtr<nsIDOMElement> docElement;
    aDocument->GetDocumentElement(getter_AddRefs(docElement));

    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(docElement));
    if (node)
    {
        return HasSpecialXHTMLTags(node);
    }

    return PR_FALSE;
}


nsresult
nsWebBrowserPersist::SetDocumentBase(
    nsIDOMDocument *aDocument, nsIURI *aBaseURI)
{
    if (mPersistFlags & PERSIST_FLAGS_NO_BASE_TAG_MODIFICATIONS)
    {
        return NS_OK;
    }

    NS_ENSURE_ARG_POINTER(aBaseURI);

    nsCOMPtr<nsIDOMXMLDocument> xmlDoc;
    nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryInterface(aDocument);
    if (!htmlDoc)
    {
        xmlDoc = do_QueryInterface(aDocument);
        if (!xmlDoc)
        {
            return NS_ERROR_FAILURE;
        }
    }

    NS_NAMED_LITERAL_STRING(kXHTMLNS, "http://www.w3.org/1999/xhtml");
    NS_NAMED_LITERAL_STRING(kHead, "head");

    
    nsCOMPtr<nsIDOMElement> headElement;
    nsCOMPtr<nsIDOMNodeList> headList;
    if (xmlDoc)
    {
        
        
        if (!NeedXHTMLBaseTag(aDocument))
            return NS_OK;

        aDocument->GetElementsByTagNameNS(
            kXHTMLNS,
            kHead, getter_AddRefs(headList));
    }
    else
    {
        aDocument->GetElementsByTagName(
            kHead, getter_AddRefs(headList));
    }
    if (headList)
    {
        nsCOMPtr<nsIDOMNode> headNode;
        headList->Item(0, getter_AddRefs(headNode));
        headElement = do_QueryInterface(headNode);
    }
    if (!headElement)
    {
        
        nsCOMPtr<nsIDOMNode> firstChildNode;
        nsCOMPtr<nsIDOMNode> newNode;
        if (xmlDoc)
        {
            aDocument->CreateElementNS(
                kXHTMLNS,
                kHead, getter_AddRefs(headElement));
        }
        else
        {
            aDocument->CreateElement(
                kHead, getter_AddRefs(headElement));
        }
        nsCOMPtr<nsIDOMElement> documentElement;
        aDocument->GetDocumentElement(getter_AddRefs(documentElement));
        if (documentElement)
        {
            documentElement->GetFirstChild(getter_AddRefs(firstChildNode));
            documentElement->InsertBefore(headElement, firstChildNode, getter_AddRefs(newNode));
        }
    }
    if (!headElement)
    {
        return NS_ERROR_FAILURE;
    }

    
    NS_NAMED_LITERAL_STRING(kBase, "base");
    nsCOMPtr<nsIDOMElement> baseElement;
    nsCOMPtr<nsIDOMNodeList> baseList;
    if (xmlDoc)
    {
        headElement->GetElementsByTagNameNS(
            kXHTMLNS,
            kBase, getter_AddRefs(baseList));
    }
    else
    {
        headElement->GetElementsByTagName(
            kBase, getter_AddRefs(baseList));
    }
    if (baseList)
    {
        nsCOMPtr<nsIDOMNode> baseNode;
        baseList->Item(0, getter_AddRefs(baseNode));
        baseElement = do_QueryInterface(baseNode);
    }

    
    if (!baseElement)
    {
      nsCOMPtr<nsIDOMNode> newNode;
      if (xmlDoc)
      {
          aDocument->CreateElementNS(
              kXHTMLNS,
              kBase, getter_AddRefs(baseElement));
      }
      else
      {
          aDocument->CreateElement(
              kBase, getter_AddRefs(baseElement));
      }
      headElement->AppendChild(baseElement, getter_AddRefs(newNode));
    }
    if (!baseElement)
    {
        return NS_ERROR_FAILURE;
    }
    nsCAutoString uriSpec;
    aBaseURI->GetSpec(uriSpec);
    NS_ConvertUTF8toUTF16 href(uriSpec);
    baseElement->SetAttribute(NS_LITERAL_STRING("href"), href);

    return NS_OK;
}


void nsWebBrowserPersist::SetApplyConversionIfNeeded(nsIChannel *aChannel)
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIEncodedChannel> encChannel = do_QueryInterface(aChannel, &rv);
    if (NS_FAILED(rv))
        return;

    
    encChannel->SetApplyConversion(PR_FALSE);

    nsCOMPtr<nsIURI> thisURI;
    aChannel->GetURI(getter_AddRefs(thisURI));
    nsCOMPtr<nsIURL> sourceURL(do_QueryInterface(thisURI));
    if (!sourceURL)
        return;
    nsCAutoString extension;
    sourceURL->GetFileExtension(extension);

    nsCOMPtr<nsIUTF8StringEnumerator> encEnum;
    encChannel->GetContentEncodings(getter_AddRefs(encEnum));
    if (!encEnum)
        return;
    nsCOMPtr<nsIExternalHelperAppService> helperAppService =
        do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return;
    PRBool hasMore;
    rv = encEnum->HasMore(&hasMore);
    if (NS_SUCCEEDED(rv) && hasMore)
    {
        nsCAutoString encType;
        rv = encEnum->GetNext(encType);
        if (NS_SUCCEEDED(rv))
        {
            PRBool applyConversion = PR_FALSE;
            rv = helperAppService->ApplyDecodingForExtension(extension, encType,
                                                             &applyConversion);
            if (NS_SUCCEEDED(rv))
                encChannel->SetApplyConversion(applyConversion);
        }
    }
}




nsEncoderNodeFixup::nsEncoderNodeFixup() : mWebBrowserPersist(nsnull)
{
}


nsEncoderNodeFixup::~nsEncoderNodeFixup()
{
}


NS_IMPL_ADDREF(nsEncoderNodeFixup)
NS_IMPL_RELEASE(nsEncoderNodeFixup)


NS_INTERFACE_MAP_BEGIN(nsEncoderNodeFixup)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocumentEncoderNodeFixup)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentEncoderNodeFixup)
NS_INTERFACE_MAP_END


NS_IMETHODIMP nsEncoderNodeFixup::FixupNode(
    nsIDOMNode *aNode, PRBool *aSerializeCloneKids, nsIDOMNode **aOutNode)
{
    NS_ENSURE_ARG_POINTER(aNode);
    NS_ENSURE_ARG_POINTER(aOutNode);
    NS_ENSURE_TRUE(mWebBrowserPersist, NS_ERROR_FAILURE);

    *aOutNode = nsnull;
    
    
    PRUint16 type = 0;
    aNode->GetNodeType(&type);
    if (type == nsIDOMNode::ELEMENT_NODE ||
        type == nsIDOMNode::PROCESSING_INSTRUCTION_NODE)
    {
        return mWebBrowserPersist->CloneNodeWithFixedUpAttributes(aNode, aSerializeCloneKids, aOutNode);
    }

    return NS_OK;
}
