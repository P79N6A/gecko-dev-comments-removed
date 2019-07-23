




































#include "nsMultiMixedConv.h"
#include "nsMemory.h"
#include "nsInt64.h"
#include "plstr.h"
#include "nsIHttpChannel.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsIStringStream.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsIHttpChannelInternal.h"
#include "nsURLHelper.h"






static PRUint32
LengthToToken(const char *cursor, const char *token)
{
    PRUint32 len = token - cursor;
    
    if (len && *(token-1) == '\n') {
        --len;
        if (len && *(token-2) == '\r')
            --len;
    }
    return len;
}

nsPartChannel::nsPartChannel(nsIChannel *aMultipartChannel, PRUint32 aPartID) :
  mStatus(NS_OK),
  mContentLength(LL_MAXUINT),
  mIsByteRangeRequest(PR_FALSE),
  mByteRangeStart(0),
  mByteRangeEnd(0),
  mPartID(aPartID),
  mIsLastPart(PR_FALSE)
{
    mMultipartChannel = aMultipartChannel;

    
    mMultipartChannel->GetLoadFlags(&mLoadFlags);

    mMultipartChannel->GetLoadGroup(getter_AddRefs(mLoadGroup));
}

nsPartChannel::~nsPartChannel()
{
}

void nsPartChannel::InitializeByteRange(PRInt64 aStart, PRInt64 aEnd)
{
    mIsByteRangeRequest = PR_TRUE;
    
    mByteRangeStart = aStart;
    mByteRangeEnd   = aEnd;
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
nsPartChannel::IsPending(PRBool *aResult)
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
    PRBool dummy;
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
nsPartChannel::GetContentLength(PRInt32 *aContentLength)
{
    *aContentLength = mContentLength; 
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentLength(PRInt32 aContentLength)
{
    mContentLength = aContentLength;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetContentDisposition(nsACString &aContentDisposition)
{
    aContentDisposition = mContentDisposition;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::SetContentDisposition(const nsACString &aContentDisposition)
{
    mContentDisposition = aContentDisposition;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetPartID(PRUint32 *aPartID)
{
    *aPartID = mPartID;
    return NS_OK;
}

NS_IMETHODIMP
nsPartChannel::GetIsLastPart(PRBool *aIsLastPart)
{
    *aIsLastPart = mIsLastPart;
    return NS_OK;
}





NS_IMETHODIMP 
nsPartChannel::GetIsByteRangeRequest(PRBool *aIsByteRangeRequest)
{
    *aIsByteRangeRequest = mIsByteRangeRequest;

    return NS_OK;
}


NS_IMETHODIMP 
nsPartChannel::GetStartRange(PRInt64 *aStartRange)
{
    *aStartRange = mByteRangeStart;

    return NS_OK;
}

NS_IMETHODIMP 
nsPartChannel::GetEndRange(PRInt64 *aEndRange)
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



NS_IMPL_ISUPPORTS3(nsMultiMixedConv,
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

#define ERR_OUT { free(buffer); return rv; }


NS_IMETHODIMP
nsMultiMixedConv::OnDataAvailable(nsIRequest *request, nsISupports *context,
                                  nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count) {

    if (mToken.IsEmpty()) 
        return NS_ERROR_FAILURE;

    nsresult rv = NS_OK;
    char *buffer = nsnull;
    PRUint32 bufLen = 0, read = 0;

    NS_ASSERTION(request, "multimixed converter needs a request");

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request, &rv);
    if (NS_FAILED(rv)) return rv;

    
    {
        bufLen = count + mBufLen;
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
        
        
        
        
        mFirstOnData = PR_FALSE;
        NS_ASSERTION(!mBufLen, "this is our first time through, we can't have buffered data");
        const char * token = mToken.get();
           
        PushOverLine(cursor, bufLen);

        if (bufLen < mTokenLen+2) {
            
            
            
            mFirstOnData = PR_TRUE;
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

    char *token = nsnull;

    if (mProcessingHeaders) {
        
        
        
        PRBool done = PR_FALSE;
        rv = ParseHeaders(channel, cursor, bufLen, &done);
        if (NS_FAILED(rv)) ERR_OUT

        if (done) {
            mProcessingHeaders = PR_FALSE;
            rv = SendStart(channel);
            if (NS_FAILED(rv)) ERR_OUT
        }
    }

    PRInt32 tokenLinefeed = 1;
    while ( (token = FindToken(cursor, bufLen)) ) {

        if (*(token+mTokenLen+1) == '-') {
            
            rv = SendData(cursor, LengthToToken(cursor, token));
            free(buffer);
            if (NS_FAILED(rv)) return rv;
            return SendStop(NS_OK);
        }

        if (!mNewPart && token > cursor) {
            
            NS_ASSERTION(!mProcessingHeaders, "we should be pushing raw data");
            rv = SendData(cursor, LengthToToken(cursor, token));
            bufLen -= token - cursor;
            if (NS_FAILED(rv)) ERR_OUT
        }
        
        token += mTokenLen;
        bufLen -= mTokenLen;
        tokenLinefeed = PushOverLine(token, bufLen);

        if (mNewPart) {
            
            mNewPart = PR_FALSE;
            cursor = token;
            PRBool done = PR_FALSE; 
            rv = ParseHeaders(channel, cursor, bufLen, &done);
            if (NS_FAILED(rv)) ERR_OUT
            if (done) {
                rv = SendStart(channel);
                if (NS_FAILED(rv)) ERR_OUT
            }
            else {
                
                
                mProcessingHeaders = PR_TRUE;
                break;
            }
        }
        else {
            mNewPart = PR_TRUE;
            
            mContentType.Truncate();
            mContentLength = LL_MAXUINT;
            mContentDisposition.Truncate();
            mIsByteRangeRequest = PR_FALSE;
            mByteRangeStart = 0;
            mByteRangeEnd = 0;
            
            rv = SendStop(NS_OK);
            if (NS_FAILED(rv)) ERR_OUT
            
            
            token -= mTokenLen + tokenLinefeed;
            bufLen += mTokenLen + tokenLinefeed;
            cursor = token;
        }
    }

    
    
    

    
    PRUint32 bufAmt = 0;
    if (mProcessingHeaders)
        bufAmt = bufLen;
    else if (bufLen) {
        
        
        
        
        
        
        if (!mPartChannel || !(cursor[bufLen-1] == nsCRT::LF) )
            bufAmt = PR_MIN(mTokenLen - 1, bufLen);
    }

    if (bufAmt) {
        rv = BufferData(cursor + (bufLen - bufAmt), bufAmt);
        if (NS_FAILED(rv)) ERR_OUT
        bufLen -= bufAmt;
    }

    if (bufLen) {
        rv = SendData(cursor, bufLen);
        if (NS_FAILED(rv)) ERR_OUT
    }

    free(buffer);
    return rv;
}



NS_IMETHODIMP
nsMultiMixedConv::OnStartRequest(nsIRequest *request, nsISupports *ctxt) {
    
    NS_ASSERTION(mToken.IsEmpty(), "a second on start???");
    const char *bndry = nsnull;
    nsCAutoString delimiter;
    nsresult rv = NS_OK;
    mContext = ctxt;

    mFirstOnData = PR_TRUE;
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

    nsCAutoString boundaryString(bndry);
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
            mBuffer = nsnull;
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
    mNewPart            = PR_TRUE;
    mContentLength      = LL_MAXUINT;
    mBuffer             = nsnull;
    mBufLen             = 0;
    mProcessingHeaders  = PR_FALSE;
    mByteRangeStart     = 0;
    mByteRangeEnd       = 0;
    mTotalSent          = 0;
    mIsByteRangeRequest = PR_FALSE;
}

nsMultiMixedConv::~nsMultiMixedConv() {
    NS_ASSERTION(!mBuffer, "all buffered data should be gone");
    if (mBuffer) {
        free(mBuffer);
        mBuffer = nsnull;
    }
}

nsresult
nsMultiMixedConv::BufferData(char *aData, PRUint32 aLen) {
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

    if (mContentType.IsEmpty())
        mContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);

    
    
    NS_ASSERTION(!mPartChannel, "tisk tisk, shouldn't be overwriting a channel");

    nsPartChannel *newChannel;
    newChannel = new nsPartChannel(aChannel, mCurrentPartID++);
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

    rv = mPartChannel->SetContentDisposition(mContentDisposition);
    if (NS_FAILED(rv)) return rv;

    nsLoadFlags loadFlags = 0;
    mPartChannel->GetLoadFlags(&loadFlags);
    loadFlags |= nsIChannel::LOAD_REPLACE;
    mPartChannel->SetLoadFlags(loadFlags);

    nsCOMPtr<nsILoadGroup> loadGroup;
    (void)mPartChannel->GetLoadGroup(getter_AddRefs(loadGroup));

    
    if (loadGroup) {
        rv = loadGroup->AddRequest(mPartChannel, nsnull);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    return mFinalListener->OnStartRequest(mPartChannel, mContext);
}


nsresult
nsMultiMixedConv::SendStop(nsresult aStatus) {
    
    nsresult rv = NS_OK;
    if (mPartChannel) {
        rv = mFinalListener->OnStopRequest(mPartChannel, mContext, aStatus);
        
        

        
        nsCOMPtr<nsILoadGroup> loadGroup;
        (void) mPartChannel->GetLoadGroup(getter_AddRefs(loadGroup));
        if (loadGroup) 
            (void) loadGroup->RemoveRequest(mPartChannel, mContext, aStatus);
    }

    mPartChannel = 0;
    return rv;
}

nsresult
nsMultiMixedConv::SendData(char *aBuffer, PRUint32 aLen) {

    nsresult rv = NS_OK;
    
    if (!mPartChannel) return NS_ERROR_FAILURE; 

    if (mContentLength != LL_MAXUINT) {
        
        
        if ((PRUint64(aLen) + mTotalSent) > mContentLength)
            aLen = mContentLength - mTotalSent;

        if (aLen == 0)
            return NS_OK;
    }

    PRUint32 offset = mTotalSent;
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

    return mFinalListener->OnDataAvailable(mPartChannel, mContext, inStream, offset, aLen);
}

PRInt32
nsMultiMixedConv::PushOverLine(char *&aPtr, PRUint32 &aLen) {
    PRInt32 chars = 0;
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
                               PRUint32 &aLen, PRBool *_retval) {
    
    
    nsresult rv = NS_OK;
    char *cursor = aPtr, *newLine = nsnull;
    PRUint32 cursorLen = aLen;
    PRBool done = PR_FALSE;
    PRUint32 lineFeedIncrement = 1;
    
    mContentLength = LL_MAXUINT; 
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

            done = PR_TRUE;
            break;
        }

        char tmpChar = *newLine;
        *newLine = '\0'; 
        char *colon = (char *) strchr(cursor, ':');
        if (colon) {
            *colon = '\0';
            nsCAutoString headerStr(cursor);
            headerStr.CompressWhitespace();
            *colon = ':';

            nsCAutoString headerVal(colon + 1);
            headerVal.CompressWhitespace();

            
            if (headerStr.LowerCaseEqualsLiteral("content-type")) {
                mContentType = headerVal;
            } else if (headerStr.LowerCaseEqualsLiteral("content-length")) {
                mContentLength = atoi(headerVal.get()); 
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

                if (range[0] == '*'){
                    mByteRangeStart = mByteRangeEnd = 0;
                }
                else {
                    tmpPtr = (char *) strchr(range, '-');
                    if (!tmpPtr)
                        return NS_ERROR_FAILURE;
                    
                    tmpPtr[0] = '\0';
                    
                    mByteRangeStart = atoi(range); 
                    tmpPtr++;
                    mByteRangeEnd = atoi(tmpPtr);
                }

                mIsByteRangeRequest = PR_TRUE;
                if (mContentLength == LL_MAXUINT)
                    mContentLength = PRUint64(PRInt64(mByteRangeEnd - mByteRangeStart + nsInt64(1)));
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
nsMultiMixedConv::FindToken(char *aCursor, PRUint32 aLen) {
    
    const char *token = mToken.get();
    char *cur = aCursor;

    if (!(token && aCursor && *token)) {
        NS_WARNING("bad data");
        return nsnull;
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

    return nsnull;
}

nsresult
NS_NewMultiMixedConv(nsMultiMixedConv** aMultiMixedConv)
{
    NS_PRECONDITION(aMultiMixedConv != nsnull, "null ptr");
    if (! aMultiMixedConv)
        return NS_ERROR_NULL_POINTER;

    *aMultiMixedConv = new nsMultiMixedConv();
    if (! *aMultiMixedConv)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aMultiMixedConv);
    return NS_OK;
}

