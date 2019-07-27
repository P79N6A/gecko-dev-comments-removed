




#include "nsMultiMixedConv.h"
#include "plstr.h"
#include "nsIHttpChannel.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsIStringStream.h"
#include "nsCRT.h"
#include "nsIHttpChannelInternal.h"
#include "nsURLHelper.h"
#include "nsIStreamConverterService.h"
#include <algorithm>






static uint32_t
LengthToToken(const char *cursor, const char *token)
{
    uint32_t len = token - cursor;
    
    if (len && *(token-1) == '\n') {
        --len;
        if (len && *(token-2) == '\r')
            --len;
    }
    return len;
}

nsPartChannel::nsPartChannel(nsIChannel *aMultipartChannel, uint32_t aPartID,
                             nsIStreamListener* aListener) :
  mMultipartChannel(aMultipartChannel),
  mListener(aListener),
  mStatus(NS_OK),
  mContentLength(UINT64_MAX),
  mIsByteRangeRequest(false),
  mByteRangeStart(0),
  mByteRangeEnd(0),
  mPartID(aPartID),
  mIsLastPart(false)
{
    mMultipartChannel = aMultipartChannel;

    
    mMultipartChannel->GetLoadFlags(&mLoadFlags);

    mMultipartChannel->GetLoadGroup(getter_AddRefs(mLoadGroup));
}

nsPartChannel::~nsPartChannel()
{
}

void nsPartChannel::InitializeByteRange(int64_t aStart, int64_t aEnd)
{
    mIsByteRangeRequest = true;
    
    mByteRangeStart = aStart;
    mByteRangeEnd   = aEnd;
}

nsresult nsPartChannel::SendOnStartRequest(nsISupports* aContext)
{
    return mListener->OnStartRequest(this, aContext);
}

nsresult nsPartChannel::SendOnDataAvailable(nsISupports* aContext,
                                            nsIInputStream* aStream,
                                            uint64_t aOffset, uint32_t aLen)
{
    return mListener->OnDataAvailable(this, aContext, aStream, aOffset, aLen);
}

nsresult nsPartChannel::SendOnStopRequest(nsISupports* aContext,
                                          nsresult aStatus)
{
    
    nsCOMPtr<nsIStreamListener> listener;
    listener.swap(mListener);
    return listener->OnStopRequest(this, aContext, aStatus);
}

void nsPartChannel::SetContentDisposition(const nsACString& aContentDispositionHeader)
{
    mContentDispositionHeader = aContentDispositionHeader;
    nsCOMPtr<nsIURI> uri;
    GetURI(getter_AddRefs(uri));
    NS_GetFilenameFromDisposition(mContentDispositionFilename,
                                  mContentDispositionHeader, uri);
    mContentDisposition = NS_GetContentDispositionFromHeader(mContentDispositionHeader, this);
}





NS_IMPL_ADDREF(nsPartChannel)
NS_IMPL_RELEASE(nsPartChannel)

NS_INTERFACE_MAP_BEGIN(nsPartChannel)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIChannel)
    NS_INTERFACE_MAP_ENTRY(nsIRequest)
    NS_INTERFACE_MAP_ENTRY(nsIChannel)
    NS_INTERFACE_MAP_ENTRY(nsIByteRangeRequest)
    NS_INTERFACE_MAP_ENTRY(nsIMultiPartChannel)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsPartChannel::GetName(nsACString &aResult)
{
    return mMultipartChannel->GetName(aResult);
}

NS_IMETHODIMP
nsPartChannel::IsPending(bool *aResult)
{
    
    
    
    return mMultipartChannel->IsPending(aResult);
}

NS_IMETHODIMP
nsPartChannel::GetStatus(nsresult *aResult)
{
    nsresult rv = NS_OK;

    if (NS_FAILED(mStatus)) {
        *aResult = mStatus;
    } else {
        rv = mMultipartChannel->GetStatus(aResult);
    }

    return rv;
}

NS_IMETHODIMP
nsPartChannel::Cancel(nsresult aStatus)
{
    
    
    
    mStatus = aStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::Suspend(void)
{
    
    
    
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::Resume(void)
{
    
    
    
    return NS_OK;
}





NS_IMETHODIMP
nsPartChannel::GetOriginalURI(nsIURI * *aURI)
{
    return mMultipartChannel->GetOriginalURI(aURI);
}

NS_IMETHODIMP
nsPartChannel::SetOriginalURI(nsIURI *aURI)
{
    return mMultipartChannel->SetOriginalURI(aURI);
}

NS_IMETHODIMP
nsPartChannel::GetURI(nsIURI * *aURI)
{
    return mMultipartChannel->GetURI(aURI);
}

NS_IMETHODIMP
nsPartChannel::Open(nsIInputStream **result)
{
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPartChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPartChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
    *aLoadGroup = mLoadGroup;
    NS_IF_ADDREF(*aLoadGroup);

    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
    mLoadGroup = aLoadGroup;

    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetOwner(nsISupports* *aOwner)
{
    return mMultipartChannel->GetOwner(aOwner);
}

NS_IMETHODIMP
nsPartChannel::SetOwner(nsISupports* aOwner)
{
    return mMultipartChannel->SetOwner(aOwner);
}

NS_IMETHODIMP
nsPartChannel::GetLoadInfo(nsILoadInfo* *aLoadInfo)
{
    return mMultipartChannel->GetLoadInfo(aLoadInfo);
}

NS_IMETHODIMP
nsPartChannel::SetLoadInfo(nsILoadInfo* aLoadInfo)
{
    return mMultipartChannel->SetLoadInfo(aLoadInfo);
}

NS_IMETHODIMP
nsPartChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aCallbacks)
{
    return mMultipartChannel->GetNotificationCallbacks(aCallbacks);
}

NS_IMETHODIMP
nsPartChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aCallbacks)
{
    return mMultipartChannel->SetNotificationCallbacks(aCallbacks);
}

NS_IMETHODIMP 
nsPartChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    return mMultipartChannel->GetSecurityInfo(aSecurityInfo);
}

NS_IMETHODIMP
nsPartChannel::GetContentType(nsACString &aContentType)
{
    aContentType = mContentType;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentType(const nsACString &aContentType)
{
    bool dummy;
    net_ParseContentType(aContentType, mContentType, mContentCharset, &dummy);
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetContentCharset(nsACString &aContentCharset)
{
    aContentCharset = mContentCharset;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentCharset(const nsACString &aContentCharset)
{
    mContentCharset = aContentCharset;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetContentLength(int64_t *aContentLength)
{
    *aContentLength = mContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentLength(int64_t aContentLength)
{
    mContentLength = aContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetContentDisposition(uint32_t *aContentDisposition)
{
    if (mContentDispositionHeader.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;

    *aContentDisposition = mContentDisposition;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentDisposition(uint32_t aContentDisposition)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsPartChannel::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
    if (mContentDispositionFilename.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;

    aContentDispositionFilename = mContentDispositionFilename;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentDispositionFilename(const nsAString &aContentDispositionFilename)
{
    return NS_ERROR_NOT_AVAILABLE;
}


NS_IMETHODIMP
nsPartChannel::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
    if (mContentDispositionHeader.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;

    aContentDispositionHeader = mContentDispositionHeader;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetPartID(uint32_t *aPartID)
{
    *aPartID = mPartID;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetIsLastPart(bool *aIsLastPart)
{
    *aIsLastPart = mIsLastPart;
    return NS_OK;
}





NS_IMETHODIMP 
nsPartChannel::GetIsByteRangeRequest(bool *aIsByteRangeRequest)
{
    *aIsByteRangeRequest = mIsByteRangeRequest;

    return NS_OK;
}


NS_IMETHODIMP 
nsPartChannel::GetStartRange(int64_t *aStartRange)
{
    *aStartRange = mByteRangeStart;

    return NS_OK;
}

NS_IMETHODIMP 
nsPartChannel::GetEndRange(int64_t *aEndRange)
{
    *aEndRange = mByteRangeEnd;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetBaseChannel(nsIChannel ** aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    *aReturn = mMultipartChannel;
    NS_IF_ADDREF(*aReturn);
    return NS_OK;
}



NS_IMPL_ISUPPORTS(nsMultiMixedConv,
                  nsIStreamConverter,
                  nsIStreamListener,
                  nsIRequestObserver)





NS_IMETHODIMP
nsMultiMixedConv::Convert(nsIInputStream *aFromStream,
                          const char *aFromType,
                          const char *aToType,
                          nsISupports *aCtxt, nsIInputStream **_retval) {
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMultiMixedConv::AsyncConvertData(const char *aFromType, const char *aToType,
                                   nsIStreamListener *aListener, nsISupports *aCtxt) {
    NS_ASSERTION(aListener && aFromType && aToType, "null pointer passed into multi mixed converter");

    
    
    
    
    
    
    mFinalListener = aListener;
    return NS_OK;
}


class AutoFree
{
public:
  AutoFree() : mBuffer(nullptr) {}

  explicit AutoFree(char *buffer) : mBuffer(buffer) {}

  ~AutoFree() {
    free(mBuffer);
  }

  AutoFree& operator=(char *buffer) {
    mBuffer = buffer;
    return *this;
  }

  operator char*() const {
    return mBuffer;
  }
private:
  char *mBuffer;
};


NS_IMETHODIMP
nsMultiMixedConv::OnDataAvailable(nsIRequest *request, nsISupports *context,
                                  nsIInputStream *inStr, uint64_t sourceOffset,
                                  uint32_t count) {

    if (mToken.IsEmpty()) 
        return NS_ERROR_FAILURE;

    nsresult rv = NS_OK;
    AutoFree buffer(nullptr);
    uint32_t bufLen = 0, read = 0;

    NS_ASSERTION(request, "multimixed converter needs a request");

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request, &rv);
    if (NS_FAILED(rv)) return rv;

    
    {
        bufLen = count + mBufLen;
        NS_ENSURE_TRUE((bufLen >= count) && (bufLen >= mBufLen),
                       NS_ERROR_FAILURE);
        buffer = (char *) malloc(bufLen);
        if (!buffer)
            return NS_ERROR_OUT_OF_MEMORY;

        if (mBufLen) {
            
            memcpy(buffer, mBuffer, mBufLen);
            free(mBuffer);
            mBuffer = 0;
            mBufLen = 0;
        }
        
        rv = inStr->Read(buffer + (bufLen - count), count, &read);

        if (NS_FAILED(rv) || read == 0) return rv;
        NS_ASSERTION(read == count, "poor data size assumption");
    }

    char *cursor = buffer;

    if (mFirstOnData) {
        
        
        
        
        mFirstOnData = false;
        NS_ASSERTION(!mBufLen, "this is our first time through, we can't have buffered data");
        const char * token = mToken.get();
           
        PushOverLine(cursor, bufLen);

        if (bufLen < mTokenLen+2) {
            
            
            
            mFirstOnData = true;
        }
        else if (!PL_strnstr(cursor, token, mTokenLen+2)) {
            buffer = (char *) realloc(buffer, bufLen + mTokenLen + 1);
            if (!buffer)
                return NS_ERROR_OUT_OF_MEMORY;

            memmove(buffer + mTokenLen + 1, buffer, bufLen);
            memcpy(buffer, token, mTokenLen);
            buffer[mTokenLen] = '\n';

            bufLen += (mTokenLen + 1);

            
            cursor = buffer;
        }
    }

    char *token = nullptr;

    if (mProcessingHeaders) {
        
        
        
        bool done = false;
        rv = ParseHeaders(channel, cursor, bufLen, &done);
        if (NS_FAILED(rv)) return rv;

        if (done) {
            mProcessingHeaders = false;
            rv = SendStart(channel);
            if (NS_FAILED(rv)) return rv;
        }
    }

    int32_t tokenLinefeed = 1;
    while ( (token = FindToken(cursor, bufLen)) ) {

        if (((token + mTokenLen) < (cursor + bufLen)) &&
            (*(token + mTokenLen + 1) == '-')) {
            
            rv = SendData(cursor, LengthToToken(cursor, token));
            if (NS_FAILED(rv)) return rv;
            return SendStop(NS_OK);
        }

        if (!mNewPart && token > cursor) {
            
            NS_ASSERTION(!mProcessingHeaders, "we should be pushing raw data");
            rv = SendData(cursor, LengthToToken(cursor, token));
            bufLen -= token - cursor;
            if (NS_FAILED(rv)) return rv;
        }
        
        token += mTokenLen;
        bufLen -= mTokenLen;
        tokenLinefeed = PushOverLine(token, bufLen);

        if (mNewPart) {
            
            mNewPart = false;
            cursor = token;
            bool done = false; 
            rv = ParseHeaders(channel, cursor, bufLen, &done);
            if (NS_FAILED(rv)) return rv;
            if (done) {
                rv = SendStart(channel);
                if (NS_FAILED(rv)) return rv;
            }
            else {
                
                
                mProcessingHeaders = true;
                break;
            }
        }
        else {
            mNewPart = true;
            
            mContentType.Truncate();
            mContentLength = UINT64_MAX;
            mContentDisposition.Truncate();
            mIsByteRangeRequest = false;
            mByteRangeStart = 0;
            mByteRangeEnd = 0;
            
            rv = SendStop(NS_OK);
            if (NS_FAILED(rv)) return rv;
            
            
            token -= mTokenLen + tokenLinefeed;
            bufLen += mTokenLen + tokenLinefeed;
            cursor = token;
        }
    }

    
    
    

    
    uint32_t bufAmt = 0;
    if (mProcessingHeaders)
        bufAmt = bufLen;
    else if (bufLen) {
        
        
        
        
        
        
        if (!mPartChannel || !(cursor[bufLen-1] == nsCRT::LF) )
            bufAmt = std::min(mTokenLen - 1, bufLen);
    }

    if (bufAmt) {
        rv = BufferData(cursor + (bufLen - bufAmt), bufAmt);
        if (NS_FAILED(rv)) return rv;
        bufLen -= bufAmt;
    }

    if (bufLen) {
        rv = SendData(cursor, bufLen);
        if (NS_FAILED(rv)) return rv;
    }

    return rv;
}



NS_IMETHODIMP
nsMultiMixedConv::OnStartRequest(nsIRequest *request, nsISupports *ctxt) {
    
    NS_ASSERTION(mToken.IsEmpty(), "a second on start???");
    const char *bndry = nullptr;
    nsAutoCString delimiter;
    nsresult rv = NS_OK;
    mContext = ctxt;

    mFirstOnData = true;
    mTotalSent   = 0;

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request, &rv);
    if (NS_FAILED(rv)) return rv;
    
    
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(channel, &rv);
    if (NS_SUCCEEDED(rv)) {
        rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-type"), delimiter);
        if (NS_FAILED(rv)) return rv;
    } else {
        
        rv = channel->GetContentType(delimiter);
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    }

    bndry = strstr(delimiter.BeginWriting(), "boundary");
    if (!bndry) return NS_ERROR_FAILURE;

    bndry = strchr(bndry, '=');
    if (!bndry) return NS_ERROR_FAILURE;

    bndry++; 

    char *attrib = (char *) strchr(bndry, ';');
    if (attrib) *attrib = '\0';

    nsAutoCString boundaryString(bndry);
    if (attrib) *attrib = ';';

    boundaryString.Trim(" \"");

    mToken = boundaryString;
    mTokenLen = boundaryString.Length();
    
    if (mTokenLen == 0)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsMultiMixedConv::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                                nsresult aStatus) {

    if (mToken.IsEmpty())  
        return NS_ERROR_FAILURE;

    if (mPartChannel) {
        mPartChannel->SetIsLastPart();

        
        
        if (mBufLen > 0 && mBuffer) {
            (void) SendData(mBuffer, mBufLen);
            
            
            free(mBuffer);
            mBuffer = nullptr;
            mBufLen = 0;
        }
        (void) SendStop(aStatus);
    } else if (NS_FAILED(aStatus)) {
        
        
        
        
        
        
        
        
        
        (void) mFinalListener->OnStopRequest(request, ctxt, aStatus);
    }

    return NS_OK;
}



nsMultiMixedConv::nsMultiMixedConv() :
  mCurrentPartID(0)
{
    mTokenLen           = 0;
    mNewPart            = true;
    mContentLength      = UINT64_MAX;
    mBuffer             = nullptr;
    mBufLen             = 0;
    mProcessingHeaders  = false;
    mByteRangeStart     = 0;
    mByteRangeEnd       = 0;
    mTotalSent          = 0;
    mIsByteRangeRequest = false;
}

nsMultiMixedConv::~nsMultiMixedConv() {
    NS_ASSERTION(!mBuffer, "all buffered data should be gone");
    if (mBuffer) {
        free(mBuffer);
        mBuffer = nullptr;
    }
}

nsresult
nsMultiMixedConv::BufferData(char *aData, uint32_t aLen) {
    NS_ASSERTION(!mBuffer, "trying to over-write buffer");

    char *buffer = (char *) malloc(aLen);
    if (!buffer) return NS_ERROR_OUT_OF_MEMORY;

    memcpy(buffer, aData, aLen);
    mBuffer = buffer;
    mBufLen = aLen;
    return NS_OK;
}


nsresult
nsMultiMixedConv::SendStart(nsIChannel *aChannel) {
    nsresult rv = NS_OK;

    nsCOMPtr<nsIStreamListener> partListener(mFinalListener);
    if (mContentType.IsEmpty()) {
        mContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);
        nsCOMPtr<nsIStreamConverterService> serv =
            do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIStreamListener> converter;
            rv = serv->AsyncConvertData(UNKNOWN_CONTENT_TYPE,
                                        "*/*",
                                        mFinalListener,
                                        mContext,
                                        getter_AddRefs(converter));
            if (NS_SUCCEEDED(rv)) {
                partListener = converter;
            }
        }
    }

    
    
    NS_ASSERTION(!mPartChannel, "tisk tisk, shouldn't be overwriting a channel");

    nsPartChannel *newChannel;
    newChannel = new nsPartChannel(aChannel, mCurrentPartID++, partListener);
    if (!newChannel)
        return NS_ERROR_OUT_OF_MEMORY;

    if (mIsByteRangeRequest) {
        newChannel->InitializeByteRange(mByteRangeStart, mByteRangeEnd);
    }

    mTotalSent = 0;

    
    mPartChannel = newChannel;

    rv = mPartChannel->SetContentType(mContentType);
    if (NS_FAILED(rv)) return rv;

    rv = mPartChannel->SetContentLength(mContentLength);
    if (NS_FAILED(rv)) return rv;

    mPartChannel->SetContentDisposition(mContentDisposition);

    nsLoadFlags loadFlags = 0;
    mPartChannel->GetLoadFlags(&loadFlags);
    loadFlags |= nsIChannel::LOAD_REPLACE;
    mPartChannel->SetLoadFlags(loadFlags);

    nsCOMPtr<nsILoadGroup> loadGroup;
    (void)mPartChannel->GetLoadGroup(getter_AddRefs(loadGroup));

    
    if (loadGroup) {
        rv = loadGroup->AddRequest(mPartChannel, nullptr);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    return mPartChannel->SendOnStartRequest(mContext);
}


nsresult
nsMultiMixedConv::SendStop(nsresult aStatus) {
    
    nsresult rv = NS_OK;
    if (mPartChannel) {
        rv = mPartChannel->SendOnStopRequest(mContext, aStatus);
        
        

        
        nsCOMPtr<nsILoadGroup> loadGroup;
        (void) mPartChannel->GetLoadGroup(getter_AddRefs(loadGroup));
        if (loadGroup) 
            (void) loadGroup->RemoveRequest(mPartChannel, mContext, aStatus);
    }

    mPartChannel = 0;
    return rv;
}

nsresult
nsMultiMixedConv::SendData(char *aBuffer, uint32_t aLen) {

    nsresult rv = NS_OK;
    
    if (!mPartChannel) return NS_ERROR_FAILURE; 

    if (mContentLength != UINT64_MAX) {
        
        
        if ((uint64_t(aLen) + mTotalSent) > mContentLength)
            aLen = static_cast<uint32_t>(mContentLength - mTotalSent);

        if (aLen == 0)
            return NS_OK;
    }

    uint64_t offset = mTotalSent;
    mTotalSent += aLen;

    nsCOMPtr<nsIStringInputStream> ss(
            do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv));
    if (NS_FAILED(rv))
        return rv;

    rv = ss->ShareData(aBuffer, aLen);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIInputStream> inStream(do_QueryInterface(ss, &rv));
    if (NS_FAILED(rv)) return rv;

    return mPartChannel->SendOnDataAvailable(mContext, inStream, offset, aLen);
}

int32_t
nsMultiMixedConv::PushOverLine(char *&aPtr, uint32_t &aLen) {
    int32_t chars = 0;
    if ((aLen > 0) && (*aPtr == nsCRT::CR || *aPtr == nsCRT::LF)) {
        if ((aLen > 1) && (aPtr[1] == nsCRT::LF))
            chars++;
        chars++;
        aPtr += chars;
        aLen -= chars;
    }
    return chars;
}

nsresult
nsMultiMixedConv::ParseHeaders(nsIChannel *aChannel, char *&aPtr, 
                               uint32_t &aLen, bool *_retval) {
    
    
    nsresult rv = NS_OK;
    char *cursor = aPtr, *newLine = nullptr;
    uint32_t cursorLen = aLen;
    bool done = false;
    uint32_t lineFeedIncrement = 1;
    
    mContentLength = UINT64_MAX; 
    while (cursorLen && (newLine = (char *) memchr(cursor, nsCRT::LF, cursorLen))) {
        
        if ((newLine > cursor) && (newLine[-1] == nsCRT::CR) ) { 
            lineFeedIncrement = 2;
            newLine--;
        }
        else
            lineFeedIncrement = 1; 

        if (newLine == cursor) {
            
            NS_ASSERTION(cursorLen >= lineFeedIncrement, "oops!");

            cursor += lineFeedIncrement;
            cursorLen -= lineFeedIncrement;

            done = true;
            break;
        }

        char tmpChar = *newLine;
        *newLine = '\0'; 
        char *colon = (char *) strchr(cursor, ':');
        if (colon) {
            *colon = '\0';
            nsAutoCString headerStr(cursor);
            headerStr.CompressWhitespace();
            *colon = ':';

            nsAutoCString headerVal(colon + 1);
            headerVal.CompressWhitespace();

            
            if (headerStr.LowerCaseEqualsLiteral("content-type")) {
                mContentType = headerVal;
            } else if (headerStr.LowerCaseEqualsLiteral("content-length")) {
                mContentLength = nsCRT::atoll(headerVal.get());
            } else if (headerStr.LowerCaseEqualsLiteral("content-disposition")) {
                mContentDisposition = headerVal;
            } else if (headerStr.LowerCaseEqualsLiteral("set-cookie")) {
                nsCOMPtr<nsIHttpChannelInternal> httpInternal =
                    do_QueryInterface(aChannel);
                if (httpInternal) {
                    httpInternal->SetCookie(headerVal.get());
                }
            } else if (headerStr.LowerCaseEqualsLiteral("content-range") || 
                       headerStr.LowerCaseEqualsLiteral("range") ) {
                
                char* tmpPtr;

                tmpPtr = (char *) strchr(colon + 1, '/');
                if (tmpPtr) 
                    *tmpPtr = '\0';

                
                char *range = (char *) strchr(colon + 2, ' ');
                if (!range)
                    return NS_ERROR_FAILURE;

                do {
                    range++;
                } while (*range == ' ');

                if (range[0] == '*'){
                    mByteRangeStart = mByteRangeEnd = 0;
                }
                else {
                    tmpPtr = (char *) strchr(range, '-');
                    if (!tmpPtr)
                        return NS_ERROR_FAILURE;
                    
                    tmpPtr[0] = '\0';
                    
                    mByteRangeStart = nsCRT::atoll(range);
                    tmpPtr++;
                    mByteRangeEnd = nsCRT::atoll(tmpPtr);
                }

                mIsByteRangeRequest = true;
                if (mContentLength == UINT64_MAX)
                    mContentLength = uint64_t(mByteRangeEnd - mByteRangeStart + 1);
            }
        }
        *newLine = tmpChar;
        newLine += lineFeedIncrement;
        cursorLen -= (newLine - cursor);
        cursor = newLine;
    }

    aPtr = cursor;
    aLen = cursorLen;

    *_retval = done;
    return rv;
}

char *
nsMultiMixedConv::FindToken(char *aCursor, uint32_t aLen) {
    
    const char *token = mToken.get();
    char *cur = aCursor;

    if (!(token && aCursor && *token)) {
        NS_WARNING("bad data");
        return nullptr;
    }

    for (; aLen >= mTokenLen; aCursor++, aLen--) {
        if (!memcmp(aCursor, token, mTokenLen) ) {
            if ((aCursor - cur) >= 2) {
                
                if ((*(aCursor-1) == '-') && (*(aCursor-2) == '-')) {
                    aCursor -= 2;
                    aLen += 2;

                    
                    mToken.Assign(aCursor, mTokenLen + 2);
                    mTokenLen = mToken.Length();
                }
            }
            return aCursor;
        }
    }

    return nullptr;
}

nsresult
NS_NewMultiMixedConv(nsMultiMixedConv** aMultiMixedConv)
{
    NS_PRECONDITION(aMultiMixedConv != nullptr, "null ptr");
    if (! aMultiMixedConv)
        return NS_ERROR_NULL_POINTER;

    *aMultiMixedConv = new nsMultiMixedConv();
    if (! *aMultiMixedConv)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aMultiMixedConv);
    return NS_OK;
}

