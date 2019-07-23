








































#include "nsGopherChannel.h"
#include "nsGopherHandler.h"
#include "nsBaseContentStream.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsIStringBundle.h"
#include "nsITXTToHTMLConv.h"
#include "nsIPrompt.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsStreamUtils.h"
#include "nsMimeTypes.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsEscape.h"
#include "nsCRT.h"
#include "netCore.h"




#define GOPHER_MAX_WRITE_SEGMENT_COUNT 100



class nsGopherContentStream : public nsBaseContentStream
                            , public nsIInputStreamCallback
                            , public nsIOutputStreamCallback
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSIOUTPUTSTREAMCALLBACK

    
    NS_IMETHOD Available(PRUint32 *result);
    NS_IMETHOD ReadSegments(nsWriteSegmentFun writer, void *closure,
                            PRUint32 count, PRUint32 *result);
    NS_IMETHOD CloseWithStatus(nsresult status);

    nsGopherContentStream(nsGopherChannel *channel)
        : nsBaseContentStream(PR_TRUE)  
        , mChannel(channel) {
    }

    nsresult OpenSocket(nsIEventTarget *target);
    nsresult OnSocketWritable();
    nsresult ParseTypeAndSelector(char &type, nsCString &selector);
    nsresult PromptForQueryString(nsCString &result);
    void     UpdateContentType(char type);
    nsresult SendRequest();

protected:
    virtual void OnCallbackPending();

private:
    nsRefPtr<nsGopherChannel>      mChannel;
    nsCOMPtr<nsISocketTransport>   mSocket;
    nsCOMPtr<nsIAsyncOutputStream> mSocketOutput;
    nsCOMPtr<nsIAsyncInputStream>  mSocketInput;
};

NS_IMPL_ISUPPORTS_INHERITED2(nsGopherContentStream,
                             nsBaseContentStream,
                             nsIInputStreamCallback,
                             nsIOutputStreamCallback)

NS_IMETHODIMP
nsGopherContentStream::Available(PRUint32 *result)
{
    if (mSocketInput)
        return mSocketInput->Available(result);

    return nsBaseContentStream::Available(result);
}

NS_IMETHODIMP
nsGopherContentStream::ReadSegments(nsWriteSegmentFun writer, void *closure,
                                    PRUint32 count, PRUint32 *result)
{
    
    
    if (mSocketInput) {
        nsWriteSegmentThunk thunk = { this, writer, closure };
        return mSocketInput->ReadSegments(NS_WriteSegmentThunk, &thunk, count,
                                          result);
    }

    return nsBaseContentStream::ReadSegments(writer, closure, count, result);
}

NS_IMETHODIMP
nsGopherContentStream::CloseWithStatus(nsresult status)
{
    if (mSocket) {
        mSocket->Close(status);
        mSocket = nsnull;
        mSocketInput = nsnull;
        mSocketOutput = nsnull;
    }
    return nsBaseContentStream::CloseWithStatus(status);
}

NS_IMETHODIMP
nsGopherContentStream::OnInputStreamReady(nsIAsyncInputStream *stream)
{
    
    DispatchCallbackSync();
    return NS_OK;
}

NS_IMETHODIMP
nsGopherContentStream::OnOutputStreamReady(nsIAsyncOutputStream *stream)
{
    
    
    

    nsresult rv = OnSocketWritable();
    if (NS_FAILED(rv))
        CloseWithStatus(rv);

    return NS_OK;
}

void
nsGopherContentStream::OnCallbackPending()
{
    nsresult rv;

    
    if (!mSocket) {
        rv = OpenSocket(CallbackTarget());
    } else if (mSocketInput) {
        rv = mSocketInput->AsyncWait(this, 0, 0, CallbackTarget());
    }
 
    if (NS_FAILED(rv))
        CloseWithStatus(rv);
}

nsresult
nsGopherContentStream::OpenSocket(nsIEventTarget *target)
{
    

    if (mChannel->HasLoadFlag(nsIChannel::LOAD_NO_NETWORK_IO))
        return NS_ERROR_NEEDS_NETWORK;

    
    

    nsCAutoString host;
    nsresult rv = mChannel->URI()->GetAsciiHost(host);
    if (NS_FAILED(rv))
        return rv;
    if (host.IsEmpty())
        return NS_ERROR_MALFORMED_URI;

    
    
    PRInt32 port = GOPHER_PORT;

    
    nsCOMPtr<nsISocketTransportService> sts = 
             do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;
    rv = sts->CreateTransport(nsnull, 0, host, port, mChannel->ProxyInfo(),
                              getter_AddRefs(mSocket));
    if (NS_FAILED(rv))
        return rv;

    
    rv = mSocket->SetEventSink(mChannel, target);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIOutputStream> output;
    rv = mSocket->OpenOutputStream(0, 0, GOPHER_MAX_WRITE_SEGMENT_COUNT,
                                   getter_AddRefs(output));
    if (NS_FAILED(rv))
        return rv;
    mSocketOutput = do_QueryInterface(output);
    NS_ENSURE_STATE(mSocketOutput);

    return mSocketOutput->AsyncWait(this, 0, 0, target);
}

nsresult
nsGopherContentStream::OnSocketWritable()
{
    
    nsresult rv = SendRequest();
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIInputStream> input;
    rv = mSocket->OpenInputStream(0, 0, 0, getter_AddRefs(input));
    if (NS_FAILED(rv))
        return rv;
    mSocketInput = do_QueryInterface(input, &rv);

    NS_ASSERTION(CallbackTarget(), "where is my pending callback?");
    rv = mSocketInput->AsyncWait(this, 0, 0, CallbackTarget());

    return rv;
}

nsresult
nsGopherContentStream::ParseTypeAndSelector(char &type, nsCString &selector)
{
    nsCAutoString buffer;
    nsresult rv = mChannel->URI()->GetPath(buffer); 
    if (NS_FAILED(rv))
        return rv;

    
    if (buffer[0] == '\0' || (buffer[0] == '/' && buffer[1] == '\0')) {
        type = '1';
        selector.Truncate();
    } else {
        NS_ENSURE_STATE(buffer[1] != '\0');

        type = buffer[1]; 

        
        
        char *sel = buffer.BeginWriting() + 2;
        PRInt32 count = nsUnescapeCount(sel);
        selector.Assign(sel, count);

        
        if (selector.FindCharInSet("\t\n\r") != kNotFound ||
            selector.FindChar('\0') != kNotFound) {
            
            return NS_ERROR_MALFORMED_URI;
        }
    }

    return NS_OK;
}

nsresult
nsGopherContentStream::PromptForQueryString(nsCString &result)
{
    nsCOMPtr<nsIPrompt> prompter;
    mChannel->GetCallback(prompter);
    if (!prompter) {
        NS_ERROR("We need a prompter!");
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIStringBundle> bundle;
    nsCOMPtr<nsIStringBundleService> bundleSvc =
            do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (bundleSvc)
        bundleSvc->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(bundle));

    nsXPIDLString promptTitle, promptText;
    if (bundle) {
        bundle->GetStringFromName(NS_LITERAL_STRING("GopherPromptTitle").get(),
                                  getter_Copies(promptTitle));
        bundle->GetStringFromName(NS_LITERAL_STRING("GopherPromptText").get(),
                                  getter_Copies(promptText));
    }
    if (promptTitle.IsEmpty())
        promptTitle.AssignLiteral("Search");
    if (promptText.IsEmpty())
        promptText.AssignLiteral("Enter a search term:");

    nsXPIDLString value;
    PRBool res = PR_FALSE;
    prompter->Prompt(promptTitle.get(), promptText.get(),
                     getter_Copies(value), NULL, NULL, &res);
    if (!res || value.IsEmpty())
        return NS_ERROR_FAILURE;

    CopyUTF16toUTF8(value, result);  
    return NS_OK;
}

void
nsGopherContentStream::UpdateContentType(char type)
{
    const char *contentType = nsnull;

    switch(type) {
    case '0':
    case 'h':
    case '2': 
    case '3': 
    case 'i': 
        contentType = TEXT_HTML;
        break;
    case '1':
    case '7': 
        contentType = APPLICATION_HTTP_INDEX_FORMAT;
        break;
    case 'g':
    case 'I':
        contentType = IMAGE_GIF;
        break;
    case 'T': 
    case '8': 
        contentType = TEXT_PLAIN;
        break;
    case '5': 
    case '9': 
        contentType = APPLICATION_OCTET_STREAM;
        break;
    case '4': 
        contentType = APPLICATION_BINHEX;
        break;
    case '6':
        contentType = APPLICATION_UUENCODE;
        break;
    }

    if (contentType)
        mChannel->SetContentType(nsDependentCString(contentType));
}

nsresult
nsGopherContentStream::SendRequest()
{
    char type;
    nsCAutoString request;  

    nsresult rv = ParseTypeAndSelector(type, request);
    if (NS_FAILED(rv))
        return rv;

    
    if (type == '7') {
        
        
        
        
        
        
        
        
        PRInt32 pos = request.RFindChar('?');
        if (pos != kNotFound) {
            
            request.SetCharAt('\t', pos);
        } else {
            
            
            nsCAutoString search;
            rv = PromptForQueryString(search);
            if (NS_FAILED(rv))
                return rv;
    
            request.Append('\t');
            request.Append(search);

            
            
            nsCAutoString spec;
            rv = mChannel->URI()->GetAsciiSpec(spec);
            if (NS_FAILED(rv))
                return rv;

            spec.Append('?');
            spec.Append(search);
            rv = mChannel->URI()->SetSpec(spec);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    request.Append(CRLF);
    
    PRUint32 n;
    rv = mSocketOutput->Write(request.get(), request.Length(), &n);
    if (NS_FAILED(rv))
        return rv;
    NS_ENSURE_STATE(n == request.Length());

    
    if (type == '1' || type == '7') {
        rv = mChannel->PushStreamConverter("text/gopher-dir",
                                           APPLICATION_HTTP_INDEX_FORMAT);
        if (NS_FAILED(rv))
            return rv;
    } else if (type == '0') {
        nsCOMPtr<nsIStreamListener> converter;
        rv = mChannel->PushStreamConverter(TEXT_PLAIN, TEXT_HTML, PR_TRUE,
                                           getter_AddRefs(converter));
        if (NS_FAILED(rv))
            return rv;
        nsCOMPtr<nsITXTToHTMLConv> config = do_QueryInterface(converter);
        if (config) {
            nsCAutoString spec;
            mChannel->URI()->GetSpec(spec);
            config->SetTitle(NS_ConvertUTF8toUTF16(spec).get());
            config->PreFormatHTML(PR_TRUE);
        }
    }

    UpdateContentType(type);
    return NS_OK;
}



NS_IMPL_ISUPPORTS_INHERITED1(nsGopherChannel,
                             nsBaseChannel,
                             nsIProxiedChannel)

NS_IMETHODIMP
nsGopherChannel::GetProxyInfo(nsIProxyInfo** aProxyInfo)
{
    *aProxyInfo = ProxyInfo();
    NS_IF_ADDREF(*aProxyInfo);
    return NS_OK;
}

nsresult
nsGopherChannel::OpenContentStream(PRBool async, nsIInputStream **result)
{
    
    if (!async)
        return NS_ERROR_NOT_IMPLEMENTED;

    nsRefPtr<nsIInputStream> stream = new nsGopherContentStream(this);
    if (!stream)
        return NS_ERROR_OUT_OF_MEMORY;

    *result = nsnull;
    stream.swap(*result);
    return NS_OK;
}

PRBool
nsGopherChannel::GetStatusArg(nsresult status, nsString &statusArg)
{
    nsCAutoString host;
    URI()->GetHost(host);
    CopyUTF8toUTF16(host, statusArg);
    return PR_TRUE;
}
