




#include "mozilla/ArrayUtils.h"

#include "nspr.h"

#include "nsIFileStreams.h"       
#include <algorithm>

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
#include "nsIFileChannel.h"
#include "nsEscape.h"
#include "nsUnicharUtils.h"
#include "nsIStringEnumerator.h"
#include "nsCRT.h"
#include "nsSupportsArray.h"
#include "nsContentCID.h"
#include "nsStreamUtils.h"

#include "nsCExternalHandlerService.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXMLDocument.h"
#include "nsIDOMTreeWalker.h"
#include "nsIDOMNode.h"
#include "nsIDOMComment.h"
#include "nsIDOMMozNamedAttrMap.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNodeList.h"
#include "nsIWebProgressListener.h"
#include "nsIAuthPrompt.h"
#include "nsIPrompt.h"
#include "nsISHEntry.h"
#include "nsIWebPageDescriptor.h"
#include "nsIFormControl.h"
#include "nsContentUtils.h"

#include "nsIDOMNodeFilter.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLCollection.h"
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
#include "nsIDOMHTMLSourceElement.h"
#include "nsIDOMHTMLMediaElement.h"

#include "nsIImageLoadingContent.h"

#include "ftpCore.h"
#include "nsITransport.h"
#include "nsISocketTransport.h"
#include "nsIStringBundle.h"
#include "nsIProtocolHandler.h"

#include "nsWebBrowserPersist.h"

#include "nsIContent.h"
#include "nsIMIMEInfo.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "mozilla/dom/HTMLSharedElement.h"
#include "mozilla/dom/HTMLSharedObjectElement.h"

using namespace mozilla;
using namespace mozilla::dom;


#define BUFFERED_OUTPUT_SIZE (1024 * 32)


struct DocData
{
    nsCOMPtr<nsIURI> mBaseURI;
    nsCOMPtr<nsIDOMDocument> mDocument;
    nsCOMPtr<nsIURI> mFile;
    nsCOMPtr<nsIURI> mDataPath;
    bool mDataPathIsRelative;
    nsCString mRelativePathToData;
    nsCString mCharset;
};


struct URIData
{
    bool mNeedsPersisting;
    bool mSaved;
    bool mIsSubFrame;
    bool mDataPathIsRelative;
    bool mNeedsFixup;
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
    int64_t mSelfProgress;
    int64_t mSelfProgressMax;
    bool mCalcFileExt;

    OutputData(nsIURI *aFile, nsIURI *aOriginalLocation, bool aCalcFileExt) :
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
    int64_t mSelfProgress;
    int64_t mSelfProgressMax;

    explicit UploadData(nsIURI *aFile) :
        mFile(aFile),
        mSelfProgress(0),
        mSelfProgressMax(10000)
    {
    }
};

struct CleanupData
{
    nsCOMPtr<nsIFile> mFile;
    
    
    
    bool mIsDirectory;
};





const uint32_t kDefaultMaxFilenameLength = 64;


const uint32_t kDefaultPersistFlags =
    nsIWebBrowserPersist::PERSIST_FLAGS_NO_CONVERSION |
    nsIWebBrowserPersist::PERSIST_FLAGS_REPLACE_EXISTING_FILES;


const char *kWebBrowserPersistStringBundle =
    "chrome://global/locale/nsWebBrowserPersist.properties";

nsWebBrowserPersist::nsWebBrowserPersist() :
    mCurrentThingsToPersist(0),
    mFirstAndOnlyUse(true),
    mCancel(false),
    mJustStartedLoading(true),
    mCompleted(false),
    mStartSaving(false),
    mReplaceExisting(true),
    mSerializingOutput(false),
    mIsPrivate(false),
    mPersistFlags(kDefaultPersistFlags),
    mPersistResult(NS_OK),
    mTotalCurrentProgress(0),
    mTotalMaxProgress(0),
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

    *aIFace = nullptr;

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







NS_IMETHODIMP nsWebBrowserPersist::GetPersistFlags(uint32_t *aPersistFlags)
{
    NS_ENSURE_ARG_POINTER(aPersistFlags);
    *aPersistFlags = mPersistFlags;
    return NS_OK;
}
NS_IMETHODIMP nsWebBrowserPersist::SetPersistFlags(uint32_t aPersistFlags)
{
    mPersistFlags = aPersistFlags;
    mReplaceExisting = (mPersistFlags & PERSIST_FLAGS_REPLACE_EXISTING_FILES) ? true : false;
    mSerializingOutput = (mPersistFlags & PERSIST_FLAGS_SERIALIZE_OUTPUT) ? true : false;
    return NS_OK;
}


NS_IMETHODIMP nsWebBrowserPersist::GetCurrentState(uint32_t *aCurrentState)
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


NS_IMETHODIMP nsWebBrowserPersist::GetResult(nsresult *aResult)
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
    nsIURI *aURI, nsISupports *aCacheKey,
    nsIURI *aReferrer, uint32_t aReferrerPolicy,
    nsIInputStream *aPostData, const char *aExtraHeaders,
    nsISupports *aFile, nsILoadContext* aPrivacyContext)
{
    return SavePrivacyAwareURI(aURI, aCacheKey, aReferrer, aReferrerPolicy,
                               aPostData, aExtraHeaders, aFile,
                               aPrivacyContext && aPrivacyContext->UsePrivateBrowsing());
}

NS_IMETHODIMP nsWebBrowserPersist::SavePrivacyAwareURI(
    nsIURI *aURI, nsISupports *aCacheKey,
    nsIURI *aReferrer, uint32_t aReferrerPolicy,
    nsIInputStream *aPostData, const char *aExtraHeaders, 
    nsISupports *aFile, bool aIsPrivate)
{
    NS_ENSURE_TRUE(mFirstAndOnlyUse, NS_ERROR_FAILURE);
    mFirstAndOnlyUse = false; 

    nsCOMPtr<nsIURI> fileAsURI;
    nsresult rv;
    rv = GetValidURIFromObject(aFile, getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

    
    mPersistFlags |= PERSIST_FLAGS_FAIL_ON_BROKEN_LINKS;
    rv = SaveURIInternal(aURI, aCacheKey, aReferrer, aReferrerPolicy,
                         aPostData, aExtraHeaders, fileAsURI, false, aIsPrivate);
    return NS_FAILED(rv) ? rv : NS_OK;
}


NS_IMETHODIMP nsWebBrowserPersist::SaveChannel(
    nsIChannel *aChannel, nsISupports *aFile)
{
    NS_ENSURE_TRUE(mFirstAndOnlyUse, NS_ERROR_FAILURE);
    mFirstAndOnlyUse = false; 

    nsCOMPtr<nsIURI> fileAsURI;
    nsresult rv;
    rv = GetValidURIFromObject(aFile, getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);

    rv = aChannel->GetURI(getter_AddRefs(mURI));
    NS_ENSURE_SUCCESS(rv, rv);

    
    mPersistFlags |= PERSIST_FLAGS_FAIL_ON_BROKEN_LINKS;
    rv = SaveChannelInternal(aChannel, fileAsURI, false);
    return NS_FAILED(rv) ? rv : NS_OK;
}





NS_IMETHODIMP nsWebBrowserPersist::SaveDocument(
    nsIDOMDocument *aDocument, nsISupports *aFile, nsISupports *aDataPath,
    const char *aOutputContentType, uint32_t aEncodingFlags, uint32_t aWrapColumn)
{
    NS_ENSURE_TRUE(mFirstAndOnlyUse, NS_ERROR_FAILURE);
    mFirstAndOnlyUse = false; 

    nsCOMPtr<nsIURI> fileAsURI;
    nsCOMPtr<nsIURI> datapathAsURI;
    nsresult rv;

    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDocument);
    nsCOMPtr<nsILoadContext> privacyContext = doc ? doc->GetLoadContext() : nullptr;
    mIsPrivate = privacyContext && privacyContext->UsePrivateBrowsing();

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
        
        mProgressListener->OnStateChange(nullptr, nullptr,
                                         nsIWebProgressListener::STATE_START |
                                         nsIWebProgressListener::STATE_IS_NETWORK,
                                         NS_OK);
        mProgressListener->OnStateChange(nullptr, nullptr,
                                         nsIWebProgressListener::STATE_STOP |
                                         nsIWebProgressListener::STATE_IS_NETWORK,
                                         rv);
    }

    return rv;
}


NS_IMETHODIMP nsWebBrowserPersist::Cancel(nsresult aReason)
{
    mCancel = true;
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
    return StartUpload(inputstream, aDestinationURI, aContentType);
}

nsresult
nsWebBrowserPersist::StartUpload(nsIInputStream *aInputStream,
    nsIURI *aDestinationURI, const nsACString &aContentType)
{
    nsCOMPtr<nsIChannel> destChannel;
    CreateChannelFromURI(aDestinationURI, getter_AddRefs(destChannel));
    nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(destChannel));
    NS_ENSURE_TRUE(uploadChannel, NS_ERROR_FAILURE);

    
    
    nsresult rv = uploadChannel->SetUploadStream(aInputStream, aContentType, -1);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    rv = destChannel->AsyncOpen(this, nullptr);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(destChannel);
    mUploadList.Put(keyPtr, new UploadData(aDestinationURI));

    return NS_OK;
}

nsresult
nsWebBrowserPersist::SaveGatheredURIs(nsIURI *aFileAsURI)
{
    nsresult rv = NS_OK;

    
    uint32_t urisToPersist = 0;
    if (mURIMap.Count() > 0)
    {
        mURIMap.EnumerateRead(EnumCountURIsToPersist, &urisToPersist);
    }

    if (urisToPersist > 0)
    {
        
        
        mURIMap.EnumerateRead(EnumPersistURIs, this);
    }

    
    
    if (mOutputMap.Count() == 0)
    {
        

        
        uint32_t addToStateFlags = 0;
        if (mProgressListener)
        {
            if (mJustStartedLoading)
            {
                addToStateFlags |= nsIWebProgressListener::STATE_IS_NETWORK;
            }
            mProgressListener->OnStateChange(nullptr, nullptr,
                nsIWebProgressListener::STATE_START | addToStateFlags, NS_OK);
        }

        rv = SaveDocuments();
        if (NS_FAILED(rv))
            EndDownload(rv);
        else if (aFileAsURI)
        {
            
            bool isFile = false;
            aFileAsURI->SchemeIs("file", &isFile);
            if (isFile)
                EndDownload(NS_OK);
        }

        
        if (mProgressListener)
        {
            mProgressListener->OnStateChange(nullptr, nullptr,
                nsIWebProgressListener::STATE_STOP | addToStateFlags, rv);
        }
    }

    return rv;
}


bool
nsWebBrowserPersist::SerializeNextFile()
{
    if (!mSerializingOutput)
    {
        return false;
    }

    nsresult rv = SaveGatheredURIs(nullptr);
    if (NS_FAILED(rv))
    {
        return false;
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
        uint32_t stateFlags = nsIWebProgressListener::STATE_START |
                              nsIWebProgressListener::STATE_IS_REQUEST;
        if (mJustStartedLoading)
        {
            stateFlags |= nsIWebProgressListener::STATE_IS_NETWORK;
        }
        mProgressListener->OnStateChange(nullptr, request, stateFlags, NS_OK);
    }

    mJustStartedLoading = false;

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
    OutputData *data = mOutputMap.Get(keyPtr);

    
    
    
    
    if (!data)
    {
        UploadData *upData = mUploadList.Get(keyPtr);
        if (!upData)
        {
            
            nsresult rv = FixRedirectedChannelEntry(channel);
            NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

            
            
            data = mOutputMap.Get(keyPtr);
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

        
        bool isEqual = false;
        if (NS_SUCCEEDED(data->mFile->Equals(data->mOriginalLocation, &isEqual))
            && isEqual)
        {
            
            mOutputMap.Remove(keyPtr);

            
            
            request->Cancel(NS_BINDING_ABORTED);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP nsWebBrowserPersist::OnStopRequest(
    nsIRequest* request, nsISupports *ctxt, nsresult status)
{
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
    OutputData *data = mOutputMap.Get(keyPtr);
    if (data)
    {
        if (NS_SUCCEEDED(mPersistResult) && NS_FAILED(status))
            SendErrorStatusChange(true, status, request, data->mFile);

        
        mOutputMap.Remove(keyPtr);
    }
    else
    {
        
        UploadData *upData = mUploadList.Get(keyPtr);
        if (upData)
        {
            mUploadList.Remove(keyPtr);
        }
    }

    
    
    
    
    if (mOutputMap.Count() == 0 && !mCancel && !mStartSaving && !mSerializingOutput)
    {
        nsresult rv = SaveDocuments();
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    }

    bool completed = false;
    if (mOutputMap.Count() == 0 && mUploadList.Count() == 0 && !mCancel)
    {
        
        
        if (mDocList.Length() == 0
            || (!SerializeNextFile() && NS_SUCCEEDED(mPersistResult)))
        {
            completed = true;
        }
    }

    if (completed)
    {
        
        EndDownload(status);
    }

    if (mProgressListener)
    {
        uint32_t stateFlags = nsIWebProgressListener::STATE_STOP |
                              nsIWebProgressListener::STATE_IS_REQUEST;
        if (completed)
        {
            stateFlags |= nsIWebProgressListener::STATE_IS_NETWORK;
        }
        mProgressListener->OnStateChange(nullptr, request, stateFlags, status);
    }
    if (completed)
    {
        mProgressListener = nullptr;
        mProgressListener2 = nullptr;
        mEventSink = nullptr;
    }

    return NS_OK;
}





NS_IMETHODIMP
nsWebBrowserPersist::OnDataAvailable(
    nsIRequest* request, nsISupports *aContext, nsIInputStream *aIStream,
    uint64_t aOffset, uint32_t aLength)
{
    bool cancel = mCancel;
    if (!cancel)
    {
        nsresult rv = NS_OK;
        uint32_t bytesRemaining = aLength;

        nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
        NS_ENSURE_TRUE(channel, NS_ERROR_FAILURE);

        nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
        OutputData *data = mOutputMap.Get(keyPtr);
        if (!data) {
            
            uint32_t n;
            return aIStream->ReadSegments(NS_DiscardSegment, nullptr, aLength, &n);
        }

        bool readError = true;

        
        if (!data->mStream)
        {
            rv = MakeOutputStream(data->mFile, getter_AddRefs(data->mStream));
            if (NS_FAILED(rv))
            {
                readError = false;
                cancel = true;
            }
        }

        
        char buffer[8192];
        uint32_t bytesRead;
        while (!cancel && bytesRemaining)
        {
            readError = true;
            rv = aIStream->Read(buffer,
                                std::min(uint32_t(sizeof(buffer)), bytesRemaining),
                                &bytesRead);
            if (NS_SUCCEEDED(rv))
            {
                readError = false;
                
                
                
                
                
                const char *bufPtr = buffer; 
                while (NS_SUCCEEDED(rv) && bytesRead)
                {
                    uint32_t bytesWritten = 0;
                    rv = data->mStream->Write(bufPtr, bytesRead, &bytesWritten);
                    if (NS_SUCCEEDED(rv))
                    {
                        bytesRead -= bytesWritten;
                        bufPtr += bytesWritten;
                        bytesRemaining -= bytesWritten;
                        
                        
                        if (!bytesWritten)
                        {
                            rv = NS_ERROR_FAILURE;
                            cancel = true;
                        }
                    }
                    else
                    {
                        
                        cancel = true;
                    }
                }
            }
            else
            {
                
                cancel = true;
            }
        }

        int64_t channelContentLength = -1;
        if (!cancel &&
            NS_SUCCEEDED(channel->GetContentLength(&channelContentLength)))
        {
            
            
            
            if ((-1 == channelContentLength) ||
                ((channelContentLength - (aOffset + aLength)) == 0))
            {
                NS_WARN_IF_FALSE(channelContentLength != -1,
                    "nsWebBrowserPersist::OnDataAvailable() no content length "
                    "header, pushing what we have");
                
                nsAutoCString contentType;
                channel->GetContentType(contentType);
                
                nsCOMPtr<nsIStorageStream> storStream(do_QueryInterface(data->mStream));
                if (storStream)
                {
                    data->mStream->Close();
                    data->mStream = nullptr; 
                    rv = StartUpload(storStream, data->mFile, contentType);
                    if (NS_FAILED(rv))
                    {
                        readError = false;
                        cancel = true;
                    }
                }
            }
        }

        
        if (cancel)
        {
            SendErrorStatusChange(readError, rv,
                readError ? request : nullptr, data->mFile);
        }
    }

    
    if (cancel)
    {
        EndDownload(NS_BINDING_ABORTED);
    }

    return NS_OK;
}








NS_IMETHODIMP nsWebBrowserPersist::OnProgress(
    nsIRequest *request, nsISupports *ctxt, int64_t aProgress,
    int64_t aProgressMax)
{
    if (!mProgressListener)
    {
        return NS_OK;
    }

    
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(request);
    OutputData *data = mOutputMap.Get(keyPtr);
    if (data)
    {
        data->mSelfProgress = aProgress;
        data->mSelfProgressMax = aProgressMax;
    }
    else
    {
        UploadData *upData = mUploadList.Get(keyPtr);
        if (upData)
        {
            upData->mSelfProgress = aProgress;
            upData->mSelfProgressMax = aProgressMax;
        }
    }

    
    CalcTotalProgress();
    if (mProgressListener2)
    {
      mProgressListener2->OnProgressChange64(nullptr, request, aProgress,
            aProgressMax, mTotalCurrentProgress, mTotalMaxProgress);
    }
    else
    {
      
      mProgressListener->OnProgressChange(nullptr, request, uint64_t(aProgress),
              uint64_t(aProgressMax), mTotalCurrentProgress, mTotalMaxProgress);
    }

    
    
    if (mEventSink)
    {
        mEventSink->OnProgress(request, ctxt, aProgress, aProgressMax);
    }

    return NS_OK;
}



NS_IMETHODIMP nsWebBrowserPersist::OnStatus(
    nsIRequest *request, nsISupports *ctxt, nsresult status,
    const char16_t *statusArg)
{
    if (mProgressListener)
    {
        
        
        switch ( status )
        {
        case NS_NET_STATUS_RESOLVING_HOST:
        case NS_NET_STATUS_RESOLVED_HOST:
        case NS_NET_STATUS_BEGIN_FTP_TRANSACTION:
        case NS_NET_STATUS_END_FTP_TRANSACTION:
        case NS_NET_STATUS_CONNECTING_TO:
        case NS_NET_STATUS_CONNECTED_TO:
        case NS_NET_STATUS_SENDING_TO:
        case NS_NET_STATUS_RECEIVING_FROM:
        case NS_NET_STATUS_WAITING_FOR:
        case NS_NET_STATUS_READING:
        case NS_NET_STATUS_WRITING:
            break;

        default:
            
            mProgressListener->OnStatusChange(nullptr, request, status, statusArg);
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
    bool aIsReadError, nsresult aResult, nsIRequest *aRequest, nsIURI *aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);

    if (!mProgressListener)
    {
        
        return NS_OK;
    }

    
    nsCOMPtr<nsIFile> file;
    GetLocalFileFromURI(aURI, getter_AddRefs(file));
    nsAutoString path;
    if (file)
    {
        file->GetPath(path);
    }
    else
    {
        nsAutoCString fileurl;
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
    const char16_t *strings[1];
    strings[0] = path.get();
    rv = bundle->FormatStringFromName(msgId.get(), strings, 1, getter_Copies(msgText));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    mProgressListener->OnStatusChange(nullptr, aRequest, aResult, msgText);

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

nsresult nsWebBrowserPersist::GetLocalFileFromURI(nsIURI *aURI, nsIFile **aLocalFile) const
{
    nsresult rv;

    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    if (NS_FAILED(rv)) {
        return rv;
    }

    file.forget(aLocalFile);
    return NS_OK;
}

nsresult nsWebBrowserPersist::AppendPathToURI(nsIURI *aURI, const nsAString & aPath) const
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsAutoCString newPath;
    nsresult rv = aURI->GetPath(newPath);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    int32_t len = newPath.Length();
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
    uint32_t aReferrerPolicy, nsIInputStream *aPostData,
    const char *aExtraHeaders, nsIURI *aFile,
    bool aCalcFileExt, bool aIsPrivate)
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
    rv = NS_NewChannel(getter_AddRefs(inputChannel),
                       aURI,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       nullptr,  
                       static_cast<nsIInterfaceRequestor*>(this),
                       loadFlags);

    nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel = do_QueryInterface(inputChannel);
    if (pbChannel)
    {
        pbChannel->SetPrivate(aIsPrivate);
    }

    if (NS_FAILED(rv) || inputChannel == nullptr)
    {
        EndDownload(NS_ERROR_FAILURE);
        return NS_ERROR_FAILURE;
    }

    
    if (mPersistFlags & PERSIST_FLAGS_NO_CONVERSION)
    {
        nsCOMPtr<nsIEncodedChannel> encodedChannel(do_QueryInterface(inputChannel));
        if (encodedChannel)
        {
            encodedChannel->SetApplyConversion(false);
        }
    }

    if (mPersistFlags & PERSIST_FLAGS_FORCE_ALLOW_COOKIES)
    {
        nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal =
                do_QueryInterface(inputChannel);
        if (httpChannelInternal)
            httpChannelInternal->SetThirdPartyFlags(nsIHttpChannelInternal::THIRD_PARTY_FORCE_ALLOW);
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(inputChannel));
    if (httpChannel)
    {
        
        if (aReferrer)
        {
            httpChannel->SetReferrerWithPolicy(aReferrer, aReferrerPolicy);
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
            nsAutoCString oneHeader;
            nsAutoCString headerName;
            nsAutoCString headerValue;
            int32_t crlf = 0;
            int32_t colon = 0;
            const char *kWhitespace = "\b\t\r\n ";
            nsAutoCString extraHeaders(aExtraHeaders);
            while (true)
            {
                crlf = extraHeaders.Find("\r\n", true);
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
                
                rv = httpChannel->SetRequestHeader(headerName, headerValue, true);
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
    nsIChannel *aChannel, nsIURI *aFile, bool aCalcFileExt)
{
    NS_ENSURE_ARG_POINTER(aChannel);
    NS_ENSURE_ARG_POINTER(aFile);

    
    
    
    
    
    nsCOMPtr<nsIFileChannel> fc(do_QueryInterface(aChannel));
    nsCOMPtr<nsIFileURL> fu(do_QueryInterface(aFile));
    if (fc && !fu) {
        nsCOMPtr<nsIInputStream> fileInputStream, bufferedInputStream;
        nsresult rv = aChannel->Open(getter_AddRefs(fileInputStream));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedInputStream),
                                       fileInputStream, BUFFERED_OUTPUT_SIZE);
        NS_ENSURE_SUCCESS(rv, rv);
        nsAutoCString contentType;
        aChannel->GetContentType(contentType);
        return StartUpload(bufferedInputStream, aFile, contentType);
    }

    
    nsresult rv = aChannel->AsyncOpen(this, nullptr);
    if (rv == NS_ERROR_NO_CONTENT)
    {
        
        
        return NS_SUCCESS_DONT_FIXUP;
    }

    if (NS_FAILED(rv))
    {
        
        if (mPersistFlags & PERSIST_FLAGS_FAIL_ON_BROKEN_LINKS)
        {
            SendErrorStatusChange(true, rv, aChannel, aFile);
            EndDownload(NS_ERROR_FAILURE);
            return NS_ERROR_FAILURE;
        }
        return NS_SUCCESS_DONT_FIXUP;
    }

    
    nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(aChannel);
    mOutputMap.Put(keyPtr, new OutputData(aFile, mURI, aCalcFileExt));

    return NS_OK;
}

nsresult
nsWebBrowserPersist::GetExtensionForContentType(const char16_t *aContentType, char16_t **aExt)
{
    NS_ENSURE_ARG_POINTER(aContentType);
    NS_ENSURE_ARG_POINTER(aExt);

    *aExt = nullptr;

    nsresult rv;
    if (!mMIMEService)
    {
        mMIMEService = do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
        NS_ENSURE_TRUE(mMIMEService, NS_ERROR_FAILURE);
    }

    nsCOMPtr<nsIMIMEInfo> mimeInfo;
    nsAutoCString contentType;
    contentType.AssignWithConversion(aContentType);
    nsAutoCString ext;
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
nsWebBrowserPersist::GetDocumentExtension(nsIDOMDocument *aDocument, char16_t **aExt)
{
    NS_ENSURE_ARG_POINTER(aDocument);
    NS_ENSURE_ARG_POINTER(aExt);

    nsXPIDLString contentType;
    nsresult rv = GetDocEncoderContentType(aDocument, nullptr, getter_Copies(contentType));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    return GetExtensionForContentType(contentType.get(), aExt);
}

nsresult
nsWebBrowserPersist::GetDocEncoderContentType(nsIDOMDocument *aDocument, const char16_t *aContentType, char16_t **aRealContentType)
{
    NS_ENSURE_ARG_POINTER(aDocument);
    NS_ENSURE_ARG_POINTER(aRealContentType);

    *aRealContentType = nullptr;

    nsAutoString defaultContentType(NS_LITERAL_STRING("text/html"));

    
    

    nsAutoString contentType;
    if (aContentType)
    {
        contentType.Assign(aContentType);
    }
    else
    {
        
        nsAutoString type;
        if (NS_SUCCEEDED(aDocument->GetContentType(type)) && !type.IsEmpty())
            contentType.Assign(type);
    }

    
    
    
    
    
    
    
    
    

    if (!contentType.IsEmpty() &&
        !contentType.Equals(defaultContentType, nsCaseInsensitiveStringComparator()))
    {
        
        nsAutoCString contractID(NS_DOC_ENCODER_CONTRACTID_BASE);
        AppendUTF16toUTF8(contentType, contractID);

        nsCOMPtr<nsIComponentRegistrar> registrar;
        NS_GetComponentRegistrar(getter_AddRefs(registrar));
        if (registrar)
        {
            bool result;
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

    
    nsCOMPtr<nsIFile> localFile;
    nsresult rv = GetLocalFileFromURI(aFile, getter_AddRefs(localFile));

    nsCOMPtr<nsIFile> localDataPath;
    if (NS_SUCCEEDED(rv) && aDataPath)
    {
        
        rv = GetLocalFileFromURI(aDataPath, getter_AddRefs(localDataPath));
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    }

    nsCOMPtr<nsIDOMNode> docAsNode = do_QueryInterface(aDocument);

    
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(aDocument));
    if (!doc) {
        return NS_ERROR_UNEXPECTED;
    }
    mURI = doc->GetDocumentURI();

    nsCOMPtr<nsIURI> oldBaseURI = mCurrentBaseURI;
    nsAutoCString oldCharset(mCurrentCharset);

    
    mCurrentBaseURI = doc->GetBaseURI();
    mCurrentCharset = doc->GetDocumentCharacterSet();

    
    if (aDataPath)
    {
        
        
        
        
        
        
        
        
        
        

        nsCOMPtr<nsIURI> oldDataPath = mCurrentDataPath;
        bool oldDataPathIsRelative = mCurrentDataPathIsRelative;
        nsCString oldCurrentRelativePathToData = mCurrentRelativePathToData;
        uint32_t oldThingsToPersist = mCurrentThingsToPersist;

        mCurrentDataPathIsRelative = false;
        mCurrentDataPath = aDataPath;
        mCurrentRelativePathToData = "";
        mCurrentThingsToPersist = 0;

        
        
        

        
        

        if (localDataPath && localFile)
        {
            nsCOMPtr<nsIFile> baseDir;
            localFile->GetParent(getter_AddRefs(baseDir));

            nsAutoCString relativePathToData;
            nsCOMPtr<nsIFile> dataDirParent;
            dataDirParent = localDataPath;
            while (dataDirParent)
            {
                bool sameDir = false;
                dataDirParent->Equals(baseDir, &sameDir);
                if (sameDir)
                {
                    mCurrentRelativePathToData = relativePathToData;
                    mCurrentDataPathIsRelative = true;
                    break;
                }

                nsAutoString dirName;
                dataDirParent->GetLeafName(dirName);

                nsAutoCString newRelativePathToData;
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
                nsAutoCString relativePath;  
                if (NS_SUCCEEDED(pathToBaseURL->GetRelativeSpec(aDataPath, relativePath)))
                {
                    mCurrentDataPathIsRelative = true;
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

        
        nsCOMPtr<nsIDOMTreeWalker> walker;
        rv = aDocument->CreateTreeWalker(docAsNode,
            nsIDOMNodeFilter::SHOW_ELEMENT |
                nsIDOMNodeFilter::SHOW_DOCUMENT |
                nsIDOMNodeFilter::SHOW_PROCESSING_INSTRUCTION,
            nullptr, 1, getter_AddRefs(walker));
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
                bool exists = false;
                bool haveDir = false;

                localDataPath->Exists(&exists);
                if (exists)
                {
                    localDataPath->IsDirectory(&haveDir);
                }
                if (!haveDir)
                {
                    rv = localDataPath->Create(nsIFile::DIRECTORY_TYPE, 0755);
                    if (NS_SUCCEEDED(rv))
                        haveDir = true;
                    else
                        SendErrorStatusChange(false, rv, nullptr, aFile);
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
                    cleanupData->mIsDirectory = true;
                    mCleanupList.AppendElement(cleanupData);
                }
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
            !mContentType.IsEmpty() ? mContentType.get() : nullptr,
            getter_Copies(realContentType));

        nsAutoCString contentType; contentType.AssignWithConversion(realContentType);
        nsAutoCString charType; 

        
        rv = SaveDocumentWithFixup(
            aDocument,
            nullptr,  
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

    mStartSaving = true;

    
    

    uint32_t i;
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
            !mContentType.IsEmpty() ? mContentType.get() : nullptr,
            getter_Copies(realContentType));

        nsAutoCString contentType; contentType.AssignWithConversion(realContentType.get());
        nsAutoCString charType; 

        
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
    mURIMap.Clear();
    mOutputMap.EnumerateRead(EnumCleanupOutputMap, this);
    mOutputMap.Clear();
    mUploadList.EnumerateRead(EnumCleanupUploadList, this);
    mUploadList.Clear();
    uint32_t i;
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
        uint32_t i;
        for (i = 0; i < mCleanupList.Length(); i++)
        {
            CleanupData *cleanupData = mCleanupList.ElementAt(i);
            nsCOMPtr<nsIFile> file = cleanupData->mFile;

            
            
            bool exists = false;
            file->Exists(&exists);
            if (!exists)
                continue;

            
            
            bool isDirectory = false;
            file->IsDirectory(&isDirectory);
            if (isDirectory != cleanupData->mIsDirectory)
                continue; 

            if (pass == 0 && !isDirectory)
            {
                file->Remove(false);
            }
            else if (pass == 1 && isDirectory) 
            {
                
                
                
                
                
                
                

                bool isEmptyDirectory = true;
                nsCOMArray<nsISimpleEnumerator> dirStack;
                int32_t stackSize = 0;

                
                nsCOMPtr<nsISimpleEnumerator> pos;
                if (NS_SUCCEEDED(file->GetDirectoryEntries(getter_AddRefs(pos))))
                    dirStack.AppendObject(pos);

                while (isEmptyDirectory && (stackSize = dirStack.Count()))
                {
                    
                    nsCOMPtr<nsISimpleEnumerator> curPos;
                    curPos = dirStack[stackSize-1];
                    dirStack.RemoveObjectAt(stackSize - 1);

                    
                    bool hasMoreElements = false;
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
                    nsCOMPtr<nsIFile> childAsFile = do_QueryInterface(child);
                    NS_ASSERTION(childAsFile, "This should be a file but isn't");

                    bool childIsSymlink = false;
                    childAsFile->IsSymlink(&childIsSymlink);
                    bool childIsDir = false;
                    childAsFile->IsDirectory(&childIsDir);
                    if (!childIsDir || childIsSymlink)
                    {
                        
                        
                        isEmptyDirectory = false;
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
                    file->Remove(true);
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

    bool nameHasChanged = false;
    nsresult rv;

    
    nsAutoCString filename;
    rv = url->GetFileName(filename);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    nsAutoCString directory;
    rv = url->GetDirectory(directory);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    
    
    
    
    
    int32_t lastDot = filename.RFind(".");
    nsAutoCString base;
    nsAutoCString ext;
    if (lastDot >= 0)
    {
        filename.Mid(base, 0, lastDot);
        filename.Mid(ext, lastDot, filename.Length() - lastDot); 
    }
    else
    {
        
        base = filename;
    }

    
    int32_t needToChop = filename.Length() - kDefaultMaxFilenameLength;
    if (needToChop > 0)
    {
        
        if (base.Length() > (uint32_t) needToChop)
        {
            base.Truncate(base.Length() - needToChop);
        }
        else
        {
            needToChop -= base.Length() - 1;
            base.Truncate(1);
            if (ext.Length() > (uint32_t) needToChop)
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
        nameHasChanged = true;
    }

    
    
    

    if (base.IsEmpty() || !mFilenameList.IsEmpty())
    {
        nsAutoCString tmpPath;
        nsAutoCString tmpBase;
        uint32_t duplicateCounter = 1;
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
                    nameHasChanged = true;
                }
                break;
            }
            duplicateCounter++;
        }
    }

    
    nsAutoCString newFilepath(directory);
    newFilepath.Append(filename);
    mFilenameList.AppendElement(newFilepath);

    
    if (nameHasChanged)
    {
        
        if (filename.Length() > kDefaultMaxFilenameLength)
        {
            NS_WARNING("Filename wasn't truncated less than the max file length - how can that be?");
            return NS_ERROR_FAILURE;
        }

        nsCOMPtr<nsIFile> localFile;
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
        nsAutoCString nameFromURL;
        url->GetFileName(nameFromURL);
        if (mPersistFlags & PERSIST_FLAGS_DONT_CHANGE_FILENAMES)
        {
            fileName.AssignWithConversion(NS_UnescapeURL(nameFromURL).get());
            aFilename = fileName;
            return NS_OK;
        }
        if (!nameFromURL.IsEmpty())
        {
            
            NS_UnescapeURL(nameFromURL);
            uint32_t nameLength = 0;
            const char *p = nameFromURL.get();
            for (;*p && *p != ';' && *p != '?' && *p != '#' && *p != '.'
                 ;p++)
            {
                if (nsCRT::IsAsciiAlpha(*p) || nsCRT::IsAsciiDigit(*p)
                    || *p == '.' || *p == '-' ||  *p == '_' || (*p == ' '))
                {
                    fileName.Append(char16_t(*p));
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
        fileName.Append(char16_t('a')); 
    }

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

    nsAutoCString contentType;

    
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

        nsCOMPtr<nsIFile> localFile;
        GetLocalFileFromURI(aURI, getter_AddRefs(localFile));

        if (mimeInfo)
        {
            nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
            NS_ENSURE_TRUE(url, NS_ERROR_FAILURE);

            nsAutoCString newFileName;
            url->GetFileName(newFileName);

            
            bool hasExtension = false;
            int32_t ext = newFileName.RFind(".");
            if (ext != -1)
            {
                mimeInfo->ExtensionExists(Substring(newFileName, ext + 1), &hasExtension);
            }

            
            nsAutoCString fileExt;
            if (!hasExtension)
            {
                
                nsCOMPtr<nsIURL> oldurl(do_QueryInterface(aOriginalURIWithExtension));
                NS_ENSURE_TRUE(oldurl, NS_ERROR_FAILURE);
                oldurl->GetFileExtension(fileExt);
                bool useOldExt = false;
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
                    uint32_t newLength = newFileName.Length() + fileExt.Length() + 1;
                    if (newLength > kDefaultMaxFilenameLength)
                    {
                        if (fileExt.Length() > kDefaultMaxFilenameLength/2)
                            fileExt.Truncate(kDefaultMaxFilenameLength/2);

                        uint32_t diff = kDefaultMaxFilenameLength - 1 -
                                        fileExt.Length();
                        if (newFileName.Length() > diff)
                            newFileName.Truncate(diff);
                    }
                    newFileName.Append('.');
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

    nsCOMPtr<nsIFile> localFile;
    GetLocalFileFromURI(aURI, getter_AddRefs(localFile));
    if (localFile)
        rv = MakeOutputStreamFromFile(localFile, aOutputStream);
    else
        rv = MakeOutputStreamFromURI(aURI, aOutputStream);

    return rv;
}

nsresult
nsWebBrowserPersist::MakeOutputStreamFromFile(
    nsIFile *aFile, nsIOutputStream **aOutputStream)
{
    nsresult rv = NS_OK;

    nsCOMPtr<nsIFileOutputStream> fileOutputStream =
        do_CreateInstance(NS_LOCALFILEOUTPUTSTREAM_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    int32_t ioFlags = -1;
    if (mPersistFlags & nsIWebBrowserPersist::PERSIST_FLAGS_APPEND_TO_FILE)
      ioFlags = PR_APPEND | PR_CREATE_FILE | PR_WRONLY;
    rv = fileOutputStream->Init(aFile, ioFlags, -1, 0);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOutputStream = NS_BufferOutputStream(fileOutputStream,
                                           BUFFERED_OUTPUT_SIZE).take();

    if (mPersistFlags & PERSIST_FLAGS_CLEANUP_ON_FAILURE)
    {
        
        CleanupData *cleanupData = new CleanupData;
        if (!cleanupData) {
          NS_RELEASE(*aOutputStream);
          return NS_ERROR_OUT_OF_MEMORY;
        }
        cleanupData->mFile = aFile;
        cleanupData->mIsDirectory = false;
        mCleanupList.AppendElement(cleanupData);
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::MakeOutputStreamFromURI(
    nsIURI *aURI, nsIOutputStream  **aOutputStream)
{
    uint32_t segsize = 8192;
    uint32_t maxsize = uint32_t(-1);
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

    
    mCompleted = true;
    Cleanup();
}

struct MOZ_STACK_CLASS FixRedirectData
{
    nsCOMPtr<nsIChannel> mNewChannel;
    nsCOMPtr<nsIURI> mOriginalURI;
    nsCOMPtr<nsISupports> mMatchingKey;
};

nsresult
nsWebBrowserPersist::FixRedirectedChannelEntry(nsIChannel *aNewChannel)
{
    NS_ENSURE_ARG_POINTER(aNewChannel);
    nsCOMPtr<nsIURI> originalURI;

    
    

    FixRedirectData data;
    data.mNewChannel = aNewChannel;
    data.mNewChannel->GetOriginalURI(getter_AddRefs(data.mOriginalURI));
    mOutputMap.EnumerateRead(EnumFixRedirect, &data);

    
    

    if (data.mMatchingKey)
    {
        nsAutoPtr<OutputData> outputData;
        mOutputMap.RemoveAndForget(data.mMatchingKey, outputData);
        NS_ENSURE_TRUE(outputData, NS_ERROR_FAILURE);

        
        if (!(mPersistFlags & PERSIST_FLAGS_IGNORE_REDIRECTED_DATA))
        {
            nsCOMPtr<nsISupports> keyPtr = do_QueryInterface(aNewChannel);
            mOutputMap.Put(keyPtr, outputData.forget());
        }
    }

    return NS_OK;
}

PLDHashOperator
nsWebBrowserPersist::EnumFixRedirect(nsISupports *aKey, OutputData *aData, void* aClosure)
{
    FixRedirectData *data = static_cast<FixRedirectData*>(aClosure);

    nsCOMPtr<nsIChannel> thisChannel = do_QueryInterface(aKey);
    nsCOMPtr<nsIURI> thisURI;

    thisChannel->GetOriginalURI(getter_AddRefs(thisURI));

    
    bool matchingURI = false;
    thisURI->Equals(data->mOriginalURI, &matchingURI);
    if (matchingURI)
    {
        data->mMatchingKey = aKey;
        return PL_DHASH_STOP;
    }

    return PL_DHASH_NEXT;
}

void
nsWebBrowserPersist::CalcTotalProgress()
{
    mTotalCurrentProgress = 0;
    mTotalMaxProgress = 0;

    if (mOutputMap.Count() > 0)
    {
        
        mOutputMap.EnumerateRead(EnumCalcProgress, this);
    }

    if (mUploadList.Count() > 0)
    {
        
        mUploadList.EnumerateRead(EnumCalcUploadProgress, this);
    }

    
    if (mTotalCurrentProgress == 0 && mTotalMaxProgress == 0)
    {
        
        mTotalCurrentProgress = 10000;
        mTotalMaxProgress = 10000;
    }
}

PLDHashOperator
nsWebBrowserPersist::EnumCalcProgress(nsISupports *aKey, OutputData *aData, void* aClosure)
{
    nsWebBrowserPersist *pthis = static_cast<nsWebBrowserPersist*>(aClosure);

    
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aData->mFile);
    if (fileURL)
    {
        pthis->mTotalCurrentProgress += aData->mSelfProgress;
        pthis->mTotalMaxProgress += aData->mSelfProgressMax;
    }
    return PL_DHASH_NEXT;
}

PLDHashOperator
nsWebBrowserPersist::EnumCalcUploadProgress(nsISupports *aKey, UploadData *aData, void* aClosure)
{
    if (aData && aClosure)
    {
        nsWebBrowserPersist *pthis = static_cast<nsWebBrowserPersist*>(aClosure);
        pthis->mTotalCurrentProgress += aData->mSelfProgress;
        pthis->mTotalMaxProgress += aData->mSelfProgressMax;
    }
    return PL_DHASH_NEXT;
}

PLDHashOperator
nsWebBrowserPersist::EnumCountURIsToPersist(const nsACString &aKey, URIData *aData, void* aClosure)
{
    uint32_t *count = static_cast<uint32_t*>(aClosure);
    if (aData->mNeedsPersisting && !aData->mSaved)
    {
        (*count)++;
    }
    return PL_DHASH_NEXT;
}

PLDHashOperator
nsWebBrowserPersist::EnumPersistURIs(const nsACString &aKey, URIData *aData, void* aClosure)
{
    if (!aData->mNeedsPersisting || aData->mSaved)
    {
        return PL_DHASH_NEXT;
    }

    nsWebBrowserPersist *pthis = static_cast<nsWebBrowserPersist*>(aClosure);
    nsresult rv;

    
    nsAutoCString key = nsAutoCString(aKey);
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri),
                   nsDependentCString(key.get(), key.Length()),
                   aData->mCharset.get());
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

    
    nsCOMPtr<nsIURI> fileAsURI;
    rv = aData->mDataPath->Clone(getter_AddRefs(fileAsURI));
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
    rv = pthis->AppendPathToURI(fileAsURI, aData->mFilename);
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

    
    rv = pthis->SaveURIInternal(uri, nullptr, nullptr, mozilla::net::RP_Default,
                                nullptr, nullptr, fileAsURI, true, pthis->mIsPrivate);
    
    
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

    if (rv == NS_OK)
    {
        
        

        aData->mFile = fileAsURI;
        aData->mSaved = true;
    }
    else
    {
        aData->mNeedsFixup = false;
    }

    if (pthis->mSerializingOutput)
        return PL_DHASH_STOP;

    return PL_DHASH_NEXT;
}

PLDHashOperator
nsWebBrowserPersist::EnumCleanupOutputMap(nsISupports *aKey, OutputData *aData, void* aClosure)
{
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aKey);
    if (channel)
    {
        channel->Cancel(NS_BINDING_ABORTED);
    }
    return PL_DHASH_NEXT;
}

PLDHashOperator
nsWebBrowserPersist::EnumCleanupUploadList(nsISupports *aKey, UploadData *aData, void* aClosure)
{
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aKey);
    if (channel)
    {
        channel->Cancel(NS_BINDING_ABORTED);
    }
    return PL_DHASH_NEXT;
}

nsresult nsWebBrowserPersist::FixupXMLStyleSheetLink(nsIDOMProcessingInstruction *aPI, const nsAString &aHref)
{
    NS_ENSURE_ARG_POINTER(aPI);
    nsresult rv = NS_OK;

    nsAutoString data;
    rv = aPI->GetData(data);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    nsAutoString href;
    nsContentUtils::GetPseudoAttributeValue(data,
                                            nsGkAtoms::href,
                                            href);

    
    if (!aHref.IsEmpty() && !href.IsEmpty())
    {
        nsAutoString alternate;
        nsAutoString charset;
        nsAutoString title;
        nsAutoString type;
        nsAutoString media;

        nsContentUtils::GetPseudoAttributeValue(data,
                                                nsGkAtoms::alternate,
                                                alternate);
        nsContentUtils::GetPseudoAttributeValue(data,
                                                nsGkAtoms::charset,
                                                charset);
        nsContentUtils::GetPseudoAttributeValue(data,
                                                nsGkAtoms::title,
                                                title);
        nsContentUtils::GetPseudoAttributeValue(data,
                                                nsGkAtoms::type,
                                                type);
        nsContentUtils::GetPseudoAttributeValue(data,
                                                nsGkAtoms::media,
                                                media);

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

    nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::href, aHref);

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

    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (!content)
    {
        return NS_OK;
    }

    
    nsCOMPtr<nsIDOMHTMLImageElement> nodeAsImage = do_QueryInterface(aNode);
    if (nodeAsImage)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }

    if (content->IsSVGElement(nsGkAtoms::img))
    {
        StoreURIAttributeNS(aNode, "http://www.w3.org/1999/xlink", "href");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLMediaElement> nodeAsMedia = do_QueryInterface(aNode);
    if (nodeAsMedia)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }
    nsCOMPtr<nsIDOMHTMLSourceElement> nodeAsSource = do_QueryInterface(aNode);
    if (nodeAsSource)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }

    if (content->IsHTMLElement(nsGkAtoms::body)) {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    if (content->IsHTMLElement(nsGkAtoms::table)) {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    if (content->IsHTMLElement(nsGkAtoms::tr)) {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    if (content->IsAnyOfHTMLElements(nsGkAtoms::td, nsGkAtoms::th)) {
        StoreURIAttribute(aNode, "background");
        return NS_OK;
    }

    nsCOMPtr<nsIDOMHTMLScriptElement> nodeAsScript = do_QueryInterface(aNode);
    if (nodeAsScript)
    {
        StoreURIAttribute(aNode, "src");
        return NS_OK;
    }

    if (content->IsSVGElement(nsGkAtoms::script))
    {
        StoreURIAttributeNS(aNode, "http://www.w3.org/1999/xlink", "href");
        return NS_OK;
    }

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

        URIData *archiveURIData = nullptr;
        StoreURIAttribute(aNode, "archive", true, &archiveURIData);
        
        
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
            nsReadingIterator<char16_t> start;
            nsReadingIterator<char16_t> end;
            nsReadingIterator<char16_t> current;

            linkRel.BeginReading(start);
            linkRel.EndReading(end);

            
            for (current = start; current != end; ++current)
            {
                
                if (nsCRT::IsAsciiSpace(*current))
                    continue;

                
                nsReadingIterator<char16_t> startWord = current;
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
        URIData *data = nullptr;
        StoreURIAttribute(aNode, "src", false, &data);
        if (data)
        {
            data->mIsSubFrame = true;
            
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
        URIData *data = nullptr;
        StoreURIAttribute(aNode, "src", false, &data);
        if (data)
        {
            data->mIsSubFrame = true;
            
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
        nsresult rv = aNodeIn->CloneNode(false, 1, aNodeOut);
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
    nsIDOMNode *aNodeIn, bool *aSerializeCloneKids, nsIDOMNode **aNodeOut)
{
    nsresult rv;
    *aNodeOut = nullptr;
    *aSerializeCloneKids = false;

    
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
            HTMLSharedElement* base = static_cast<HTMLSharedElement*>(nodeAsBase.get());
            base->GetOwnerDocument(getter_AddRefs(ownerDocument));
            if (ownerDocument)
            {
                nsAutoString href;
                base->GetHref(href); 
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

    nsCOMPtr<nsIContent> content = do_QueryInterface(aNodeIn);
    if (!content)
    {
        return NS_OK;
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

    if (content->IsHTMLElement(nsGkAtoms::body)) {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    if (content->IsHTMLElement(nsGkAtoms::table)) {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    if (content->IsHTMLElement(nsGkAtoms::tr)) {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "background");
        }
        return rv;
    }

    if (content->IsAnyOfHTMLElements(nsGkAtoms::td, nsGkAtoms::th)) {
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
                imgCon->SetLoadingEnabled(false);

            FixupAnchor(*aNodeOut);
            FixupNodeAttribute(*aNodeOut, "src");
        }
        return rv;
    }

    nsCOMPtr<nsIDOMHTMLMediaElement> nodeAsMedia = do_QueryInterface(aNodeIn);
    if (nodeAsMedia)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "src");
        }

        return rv;
    }

    nsCOMPtr<nsIDOMHTMLSourceElement> nodeAsSource = do_QueryInterface(aNodeIn);
    if (nodeAsSource)
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttribute(*aNodeOut, "src");
        }

        return rv;
    }

    if (content->IsSVGElement(nsGkAtoms::img))
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            
            nsCOMPtr<nsIImageLoadingContent> imgCon =
                do_QueryInterface(*aNodeOut);
            if (imgCon)
                imgCon->SetLoadingEnabled(false);

            
            FixupNodeAttributeNS(*aNodeOut, "http://www.w3.org/1999/xlink", "href");
        }
        return rv;
    }

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

    if (content->IsSVGElement(nsGkAtoms::script))
    {
        rv = GetNodeToFixup(aNodeIn, aNodeOut);
        if (NS_SUCCEEDED(rv) && *aNodeOut)
        {
            FixupNodeAttributeNS(*aNodeOut, "http://www.w3.org/1999/xlink", "href");
        }
        return rv;
    }

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
            
            
            static_cast<HTMLSharedObjectElement*>(newApplet.get())->
              RemoveAttribute(NS_LITERAL_STRING("codebase"));
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
                imgCon->SetLoadingEnabled(false);

            FixupNodeAttribute(*aNodeOut, "src");

            nsAutoString valueStr;
            NS_NAMED_LITERAL_STRING(valueAttr, "value");
            
            nsCOMPtr<nsIContent> content = do_QueryInterface(*aNodeOut);
            nsRefPtr<HTMLInputElement> outElt =
              HTMLInputElement::FromContentOrNull(content);
            nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(*aNodeOut);
            switch (formControl->GetType()) {
                case NS_FORM_INPUT_EMAIL:
                case NS_FORM_INPUT_SEARCH:
                case NS_FORM_INPUT_TEXT:
                case NS_FORM_INPUT_TEL:
                case NS_FORM_INPUT_URL:
                case NS_FORM_INPUT_NUMBER:
                case NS_FORM_INPUT_RANGE:
                case NS_FORM_INPUT_DATE:
                case NS_FORM_INPUT_TIME:
                case NS_FORM_INPUT_COLOR:
                    nodeAsInput->GetValue(valueStr);
                    
                    if (valueStr.IsEmpty())
                      outElt->RemoveAttribute(valueAttr);
                    else
                      outElt->SetAttribute(valueAttr, valueStr);
                    break;
                case NS_FORM_INPUT_CHECKBOX:
                case NS_FORM_INPUT_RADIO:
                    bool checked;
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
            
            *aSerializeCloneKids = true;

            nsAutoString valueStr;
            nodeAsTextArea->GetValue(valueStr);

            (*aNodeOut)->SetTextContent(valueStr);
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
            bool selected;
            nodeAsOption->GetSelected(&selected);
            outElt->SetDefaultSelected(selected);
        }
        return rv;
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::StoreURI(
    const char *aURI, bool aNeedsPersisting, URIData **aData)
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
    nsIURI *aURI, bool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aURI);
    if (aData)
    {
        *aData = nullptr;
    }

    
    
    bool doNotPersistURI;
    nsresult rv = NS_URIChainHasFlags(aURI,
                                      nsIProtocolHandler::URI_NON_PERSISTABLE,
                                      &doNotPersistURI);
    if (NS_FAILED(rv))
    {
        doNotPersistURI = false;
    }

    if (doNotPersistURI)
    {
        return NS_OK;
    }

    URIData *data = nullptr;
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
    bool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aNode);
    NS_ENSURE_ARG_POINTER(aNamespaceURI);
    NS_ENSURE_ARG_POINTER(aAttribute);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
    MOZ_ASSERT(element);

    
    

    nsCOMPtr<nsIDOMMozNamedAttrMap> attrMap;
    nsresult rv = element->GetAttributes(getter_AddRefs(attrMap));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    NS_ConvertASCIItoUTF16 namespaceURI(aNamespaceURI);
    NS_ConvertASCIItoUTF16 attribute(aAttribute);
    nsCOMPtr<nsIDOMAttr> attr;
    rv = attrMap->GetNamedItemNS(namespaceURI, attribute, getter_AddRefs(attr));
    if (attr)
    {
        nsAutoString oldValue;
        attr->GetValue(oldValue);
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
    nsAutoCString spec;
    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!mURIMap.Contains(spec))
    {
        return NS_ERROR_FAILURE;
    }
    URIData *data = mURIMap.Get(spec);
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

        nsAutoCString filename;
        url->GetFileName(filename);

        nsAutoCString rawPathURL(data->mRelativePathToData);
        rawPathURL.Append(filename);

        nsAutoCString buf;
        AppendUTF8toUTF16(NS_EscapeURL(rawPathURL, esc_FilePath, buf),
                          newValue);
    }
    else
    {
        nsAutoCString fileurl;
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

    
    

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
    MOZ_ASSERT(element);

    nsCOMPtr<nsIDOMMozNamedAttrMap> attrMap;
    nsresult rv = element->GetAttributes(getter_AddRefs(attrMap));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    NS_ConvertASCIItoUTF16 attribute(aAttribute);
    NS_ConvertASCIItoUTF16 namespaceURI(aNamespaceURI);
    nsCOMPtr<nsIDOMAttr> attr;
    rv = attrMap->GetNamedItemNS(namespaceURI, attribute, getter_AddRefs(attr));
    if (attr) {
        nsString uri;
        attr->GetValue(uri);
        rv = FixupURI(uri);
        if (NS_SUCCEEDED(rv))
        {
            attr->SetValue(uri);
        }
    }

    return rv;
}

nsresult
nsWebBrowserPersist::FixupAnchor(nsIDOMNode *aNode)
{
    NS_ENSURE_ARG_POINTER(aNode);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
    MOZ_ASSERT(element);

    nsCOMPtr<nsIDOMMozNamedAttrMap> attrMap;
    nsresult rv = element->GetAttributes(getter_AddRefs(attrMap));
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    if (mPersistFlags & PERSIST_FLAGS_DONT_FIXUP_LINKS)
    {
        return NS_OK;
    }

    
    nsString attribute(NS_LITERAL_STRING("href"));
    nsCOMPtr<nsIDOMAttr> attr;
    rv = attrMap->GetNamedItem(attribute, getter_AddRefs(attr));
    if (attr)
    {
        nsString oldValue;
        attr->GetValue(oldValue);
        NS_ConvertUTF16toUTF8 oldCValue(oldValue);

        
        if (oldCValue.IsEmpty() || oldCValue.CharAt(0) == '#')
        {
            return NS_OK;
        }

        
        bool isEqual = false;
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
            nsAutoCString uriSpec;
            newURI->GetSpec(uriSpec);
            attr->SetValue(NS_ConvertUTF8toUTF16(uriSpec));
        }
    }

    return NS_OK;
}

nsresult
nsWebBrowserPersist::StoreAndFixupStyleSheet(nsIStyleSheet *aStyleSheet)
{
    
    return NS_OK;
}

bool
nsWebBrowserPersist::DocumentEncoderExists(const char16_t *aContentType)
{
    
    nsAutoCString contractID(NS_DOC_ENCODER_CONTRACTID_BASE);
    AppendUTF16toUTF8(aContentType, contractID);

    nsCOMPtr<nsIComponentRegistrar> registrar;
    NS_GetComponentRegistrar(getter_AddRefs(registrar));
    if (registrar)
    {
        bool result;
        nsresult rv = registrar->IsContractIDRegistered(contractID.get(),
                                                        &result);
        if (NS_SUCCEEDED(rv) && result)
        {
            return true;
        }
    }
    return false;
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
        nsAutoCString extension;
        if (NS_SUCCEEDED(rv))
        {
            url->GetFileExtension(extension);
        }
        else
        {
            extension.AssignLiteral("htm");
        }
        aData->mSubFrameExt.Assign(char16_t('.'));
        AppendUTF8toUTF16(extension, aData->mSubFrameExt);
    }
    else
    {
        aData->mSubFrameExt.Assign(char16_t('.'));
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
    *aChannel = nullptr;

    rv = NS_NewChannel(aChannel,
                       aURI,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_ARG_POINTER(*aChannel);

    rv = (*aChannel)->SetNotificationCallbacks(static_cast<nsIInterfaceRequestor*>(this));
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
}

nsresult
nsWebBrowserPersist::SaveDocumentWithFixup(
    nsIDOMDocument *aDocument, nsIDocumentEncoderNodeFixup *aNodeFixup,
    nsIURI *aFile, bool aReplaceExisting, const nsACString &aFormatType,
    const nsCString &aSaveCharset, uint32_t aFlags)
{
    NS_ENSURE_ARG_POINTER(aFile);

    nsresult  rv = NS_OK;
    nsCOMPtr<nsIFile> localFile;
    GetLocalFileFromURI(aFile, getter_AddRefs(localFile));
    if (localFile)
    {
        
        
        bool fileExists = false;
        rv = localFile->Exists(&fileExists);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

        if (!aReplaceExisting && fileExists)
            return NS_ERROR_FAILURE;                
    }

    nsCOMPtr<nsIOutputStream> outputStream;
    rv = MakeOutputStream(aFile, getter_AddRefs(outputStream));
    if (NS_FAILED(rv))
    {
        SendErrorStatusChange(false, rv, nullptr, aFile);
        return NS_ERROR_FAILURE;
    }
    NS_ENSURE_TRUE(outputStream, NS_ERROR_FAILURE);

    
    nsAutoCString contractID(NS_DOC_ENCODER_CONTRACTID_BASE);
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

    nsAutoCString charsetStr(aSaveCharset);
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

    return rv;
}



nsresult
nsWebBrowserPersist::MakeAndStoreLocalFilenameInURIMap(
    nsIURI *aURI, bool aNeedsPersisting, URIData **aData)
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsAutoCString spec;
    nsresult rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    
    URIData *data;
    if (mURIMap.Contains(spec))
    {
        data = mURIMap.Get(spec);
        if (aNeedsPersisting)
        {
          data->mNeedsPersisting = true;
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
    data->mNeedsFixup = true;
    data->mFilename = filename;
    data->mSaved = false;
    data->mIsSubFrame = false;
    data->mDataPath = mCurrentDataPath;
    data->mDataPathIsRelative = mCurrentDataPathIsRelative;
    data->mRelativePathToData = mCurrentRelativePathToData;
    data->mCharset = mCurrentCharset;

    if (aNeedsPersisting)
        mCurrentThingsToPersist++;

    mURIMap.Put(spec, data);
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

static bool IsSpecialXHTMLTag(nsIDOMNode *aNode)
{
    nsAutoString tmp;
    aNode->GetNamespaceURI(tmp);
    if (!tmp.EqualsLiteral("http://www.w3.org/1999/xhtml"))
        return false;

    aNode->GetLocalName(tmp);
    for (uint32_t i = 0; i < ArrayLength(kSpecialXHTMLTags); i++) {
        if (tmp.EqualsASCII(kSpecialXHTMLTags[i]))
        {
            
            
            
            
            return true;
        }
    }

    return false;
}

static bool HasSpecialXHTMLTags(nsIDOMNode *aParent)
{
    if (IsSpecialXHTMLTag(aParent))
        return true;

    nsCOMPtr<nsIDOMNodeList> list;
    aParent->GetChildNodes(getter_AddRefs(list));
    if (list)
    {
        uint32_t count;
        list->GetLength(&count);
        uint32_t i;
        for (i = 0; i < count; i++) {
            nsCOMPtr<nsIDOMNode> node;
            list->Item(i, getter_AddRefs(node));
            if (!node)
                break;
            uint16_t nodeType;
            node->GetNodeType(&nodeType);
            if (nodeType == nsIDOMNode::ELEMENT_NODE) {
                return HasSpecialXHTMLTags(node);
            }
        }
    }

    return false;
}

static bool NeedXHTMLBaseTag(nsIDOMDocument *aDocument)
{
    nsCOMPtr<nsIDOMElement> docElement;
    aDocument->GetDocumentElement(getter_AddRefs(docElement));

    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(docElement));
    if (node)
    {
        return HasSpecialXHTMLTags(node);
    }

    return false;
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
    nsCOMPtr<nsIDOMHTMLCollection> baseList;
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
    nsAutoCString uriSpec;
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

    
    encChannel->SetApplyConversion(false);

    nsCOMPtr<nsIURI> thisURI;
    aChannel->GetURI(getter_AddRefs(thisURI));
    nsCOMPtr<nsIURL> sourceURL(do_QueryInterface(thisURI));
    if (!sourceURL)
        return;
    nsAutoCString extension;
    sourceURL->GetFileExtension(extension);

    nsCOMPtr<nsIUTF8StringEnumerator> encEnum;
    encChannel->GetContentEncodings(getter_AddRefs(encEnum));
    if (!encEnum)
        return;
    nsCOMPtr<nsIExternalHelperAppService> helperAppService =
        do_GetService(NS_EXTERNALHELPERAPPSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return;
    bool hasMore;
    rv = encEnum->HasMore(&hasMore);
    if (NS_SUCCEEDED(rv) && hasMore)
    {
        nsAutoCString encType;
        rv = encEnum->GetNext(encType);
        if (NS_SUCCEEDED(rv))
        {
            bool applyConversion = false;
            rv = helperAppService->ApplyDecodingForExtension(extension, encType,
                                                             &applyConversion);
            if (NS_SUCCEEDED(rv))
                encChannel->SetApplyConversion(applyConversion);
        }
    }
}




nsEncoderNodeFixup::nsEncoderNodeFixup() : mWebBrowserPersist(nullptr)
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
    nsIDOMNode *aNode, bool *aSerializeCloneKids, nsIDOMNode **aOutNode)
{
    NS_ENSURE_ARG_POINTER(aNode);
    NS_ENSURE_ARG_POINTER(aOutNode);
    NS_ENSURE_TRUE(mWebBrowserPersist, NS_ERROR_FAILURE);

    *aOutNode = nullptr;

    
    uint16_t type = 0;
    aNode->GetNodeType(&type);
    if (type == nsIDOMNode::ELEMENT_NODE ||
        type == nsIDOMNode::PROCESSING_INSTRUCTION_NODE)
    {
        return mWebBrowserPersist->CloneNodeWithFixedUpAttributes(aNode, aSerializeCloneKids, aOutNode);
    }

    return NS_OK;
}
